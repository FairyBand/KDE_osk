#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QByteArray>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QHash>
#include <QProcessEnvironment>
#include <QSet>
#include <QSocketNotifier>
#include <QStringList>
#include <QTimer>
#include <QVector>

#include <algorithm>
#include <cstdint>
#include <utility>

#ifdef Q_OS_UNIX
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace
{
constexpr auto FcitxService = "org.fcitx.Fcitx5";
constexpr auto FcitxControllerPath = "/controller";
constexpr auto FcitxControllerInterface = "org.fcitx.Fcitx.Controller1";
constexpr qsizetype MaxProxyBufferBytes = 1024 * 1024;
constexpr uint32_t WlDisplayObjectId = 1;
constexpr uint16_t WlDisplayGetRegistryOpcode = 1;
constexpr uint16_t WlRegistryGlobalOpcode = 0;
constexpr uint16_t WlRegistryGlobalRemoveOpcode = 1;
constexpr uint16_t WlRegistryBindOpcode = 0;

int execStockFcitx5(const QString &command, const QStringList &arguments)
{
#ifndef Q_OS_UNIX
    qCritical() << "kde-osk-kwin-broker is only supported on Unix-like systems.";
    Q_UNUSED(command)
    Q_UNUSED(arguments)
    return 127;
#else
    QVector<QByteArray> storage;
    storage.reserve(arguments.size() + 1);
    storage.push_back(command.toLocal8Bit());
    for (const QString &argument : arguments) {
        storage.push_back(argument.toLocal8Bit());
    }

    QVector<char *> argv;
    argv.reserve(storage.size() + 1);
    for (QByteArray &entry : storage) {
        argv.push_back(entry.data());
    }
    argv.push_back(nullptr);

    qInfo() << "Delegating KWin input-method startup to stock fcitx5:" << command << arguments;
    execvp(argv.first(), argv.data());
    qCritical() << "Failed to exec stock fcitx5:" << command << strerror(errno);
    return 127;
#endif
}

int waylandSocketFd()
{
#ifndef Q_OS_UNIX
    return -1;
#else
    const QByteArray socketValue = qgetenv("WAYLAND_SOCKET");
    if (socketValue.isEmpty()) {
        return -1;
    }

    bool ok = false;
    const int fd = QString::fromLocal8Bit(socketValue).toInt(&ok);
    return ok ? fd : -1;
#endif
}

class WaylandProtocolInspector
{
public:
    enum class Direction {
        UpstreamToDownstream,
        DownstreamToUpstream,
    };

    enum class RegistryView {
        FcitxInputMethodBranch,
        KdeOskPanelBranch,
    };

    struct RegistryGlobal {
        uint32_t name = 0;
        QString interfaceName;
        uint32_t version = 0;
    };

    void inspect(Direction direction, const char *data, qsizetype size)
    {
        QByteArray &buffer = direction == Direction::UpstreamToDownstream ? m_upstreamBuffer : m_downstreamBuffer;
        buffer.append(data, size);
        parseBufferedMessages(direction, buffer);
    }

    void filterUpstreamToDownstream(const char *data, qsizetype size, QByteArray &output)
    {
        m_upstreamBuffer.append(data, size);

        while (m_upstreamBuffer.size() >= 8) {
            const char *message = m_upstreamBuffer.constData();
            const uint32_t objectId = readUint32(message);
            const uint32_t sizeAndOpcode = readUint32(message + 4);
            const uint16_t opcode = static_cast<uint16_t>(sizeAndOpcode & 0xffff);
            const uint16_t messageSize = static_cast<uint16_t>(sizeAndOpcode >> 16);

            if (messageSize < 8) {
                qWarning() << "Invalid Wayland message size from broker proxy:" << messageSize;
                m_upstreamBuffer.clear();
                return;
            }
            if (m_upstreamBuffer.size() < messageSize) {
                return;
            }

            const char *payload = message + 8;
            const qsizetype payloadSize = messageSize - 8;
            parseUpstreamMessage(objectId, opcode, payload, payloadSize);
            if (!shouldReserveForPanelBranch(objectId, opcode, payload, payloadSize)) {
                output.append(message, messageSize);
            }

            m_upstreamBuffer.remove(0, messageSize);
        }
    }

    QVector<RegistryGlobal> projectedGlobals(RegistryView view) const
    {
        QVector<RegistryGlobal> globals;
        for (const uint32_t name : m_globalOrder) {
            const QString interfaceName = m_globalInterfaces.value(name);
            if (interfaceName.isEmpty() || !shouldExposeInView(view, interfaceName, name)) {
                continue;
            }

            globals.push_back({
                name,
                interfaceName,
                m_globalVersions.value(name),
            });
        }
        return globals;
    }

private:
    static uint32_t readUint32(const char *data)
    {
        uint32_t value = 0;
        memcpy(&value, data, sizeof(value));
        return value;
    }

    static bool readWaylandString(const char *payload, qsizetype payloadSize, qsizetype offset, QString &text, qsizetype &nextOffset)
    {
        if (offset < 0 || offset + 4 > payloadSize) {
            return false;
        }

        const uint32_t length = readUint32(payload + offset);
        const qsizetype dataOffset = offset + 4;
        if (dataOffset + length > payloadSize) {
            return false;
        }

        const qsizetype textLength = length > 0 ? static_cast<qsizetype>(length - 1) : 0;
        text = QString::fromUtf8(payload + dataOffset, textLength);
        nextOffset = dataOffset + ((static_cast<qsizetype>(length) + 3) & ~qsizetype(3));
        return nextOffset <= payloadSize;
    }

    static bool isPrivilegedInputInterface(const QString &interfaceName)
    {
        return interfaceName == QStringLiteral("zwp_input_method_v1")
            || interfaceName == QStringLiteral("zwp_input_panel_v1");
    }

    bool shouldExposeInView(RegistryView view, const QString &interfaceName, uint32_t name) const
    {
        switch (view) {
        case RegistryView::FcitxInputMethodBranch:
            return !m_reservedPanelGlobalNames.contains(name)
                && interfaceName != QStringLiteral("zwp_input_panel_v1");
        case RegistryView::KdeOskPanelBranch:
            return interfaceName != QStringLiteral("zwp_input_method_v1");
        }
        return false;
    }

    void parseBufferedMessages(Direction direction, QByteArray &buffer)
    {
        while (buffer.size() >= 8) {
            const char *message = buffer.constData();
            const uint32_t objectId = readUint32(message);
            const uint32_t sizeAndOpcode = readUint32(message + 4);
            const uint16_t opcode = static_cast<uint16_t>(sizeAndOpcode & 0xffff);
            const uint16_t messageSize = static_cast<uint16_t>(sizeAndOpcode >> 16);

            if (messageSize < 8) {
                qWarning() << "Invalid Wayland message size from broker proxy:" << messageSize;
                buffer.clear();
                return;
            }
            if (buffer.size() < messageSize) {
                return;
            }

            const char *payload = message + 8;
            const qsizetype payloadSize = messageSize - 8;
            if (direction == Direction::UpstreamToDownstream) {
                parseUpstreamMessage(objectId, opcode, payload, payloadSize);
            } else {
                parseDownstreamMessage(objectId, opcode, payload, payloadSize);
            }

            buffer.remove(0, messageSize);
        }
    }

    void parseUpstreamMessage(uint32_t objectId, uint16_t opcode, const char *payload, qsizetype payloadSize)
    {
        if (!m_registryObjectIds.contains(objectId)) {
            return;
        }

        if (opcode == WlRegistryGlobalOpcode) {
            parseRegistryGlobal(payload, payloadSize);
        } else if (opcode == WlRegistryGlobalRemoveOpcode && payloadSize >= 4) {
            const uint32_t name = readUint32(payload);
            const QString interfaceName = m_globalInterfaces.take(name);
            m_globalVersions.remove(name);
            m_globalOrder.removeAll(name);
            if (isPrivilegedInputInterface(interfaceName)) {
                qInfo() << "KWin removed privileged Wayland global" << interfaceName << "name" << name;
            }
        }
    }

    void parseDownstreamMessage(uint32_t objectId, uint16_t opcode, const char *payload, qsizetype payloadSize)
    {
        if (objectId == WlDisplayObjectId && opcode == WlDisplayGetRegistryOpcode && payloadSize >= 4) {
            const uint32_t registryId = readUint32(payload);
            m_registryObjectIds.insert(registryId);
            return;
        }

        if (m_registryObjectIds.contains(objectId) && opcode == WlRegistryBindOpcode) {
            parseRegistryBind(payload, payloadSize);
        }
    }

    void parseRegistryGlobal(const char *payload, qsizetype payloadSize)
    {
        uint32_t name = 0;
        QString interfaceName;
        uint32_t version = 0;
        if (!parseRegistryGlobalPayload(payload, payloadSize, name, interfaceName, version)) {
            return;
        }

        m_globalInterfaces.insert(name, interfaceName);
        m_globalVersions.insert(name, version);
        if (!m_globalOrder.contains(name)) {
            m_globalOrder.push_back(name);
        }

        if (isPrivilegedInputInterface(interfaceName)) {
            qInfo() << "KWin advertised privileged Wayland global" << interfaceName
                    << "name" << name << "version" << version;
        }
    }

    static bool parseRegistryGlobalPayload(const char *payload, qsizetype payloadSize, uint32_t &name, QString &interfaceName, uint32_t &version)
    {
        if (payloadSize < 8) {
            return false;
        }

        name = readUint32(payload);
        qsizetype offset = 0;
        if (!readWaylandString(payload, payloadSize, 4, interfaceName, offset) || offset + 4 > payloadSize) {
            return false;
        }

        version = readUint32(payload + offset);
        return true;
    }

    bool shouldReserveForPanelBranch(uint32_t objectId, uint16_t opcode, const char *payload, qsizetype payloadSize)
    {
        if (!m_registryObjectIds.contains(objectId)) {
            return false;
        }

        if (opcode == WlRegistryGlobalOpcode) {
            uint32_t name = 0;
            QString interfaceName;
            uint32_t version = 0;
            if (!parseRegistryGlobalPayload(payload, payloadSize, name, interfaceName, version)) {
                return false;
            }
            if (interfaceName != QStringLiteral("zwp_input_panel_v1")) {
                return false;
            }

            m_reservedPanelGlobalNames.insert(name);
            qInfo() << "Reserving Wayland input-panel global for KDE OSK branch; hiding from fcitx5"
                    << "name" << name << "version" << version;
            return true;
        }

        if (opcode == WlRegistryGlobalRemoveOpcode && payloadSize >= 4) {
            const uint32_t name = readUint32(payload);
            if (!m_reservedPanelGlobalNames.remove(name)) {
                return false;
            }

            qInfo() << "Hiding removal of reserved Wayland input-panel global from fcitx5"
                    << "name" << name;
            return true;
        }

        return false;
    }

    void parseRegistryBind(const char *payload, qsizetype payloadSize)
    {
        if (payloadSize < 16) {
            return;
        }

        const uint32_t name = readUint32(payload);
        QString requestedInterface;
        qsizetype offset = 0;
        if (!readWaylandString(payload, payloadSize, 4, requestedInterface, offset) || offset + 8 > payloadSize) {
            return;
        }

        const uint32_t version = readUint32(payload + offset);
        const uint32_t newObjectId = readUint32(payload + offset + 4);
        m_objectInterfaces.insert(newObjectId, requestedInterface);

        if (isPrivilegedInputInterface(requestedInterface)) {
            qInfo() << "Downstream client bound privileged Wayland global" << requestedInterface
                    << "name" << name
                    << "version" << version
                    << "object" << newObjectId;
        }
    }

    QByteArray m_upstreamBuffer;
    QByteArray m_downstreamBuffer;
    QSet<uint32_t> m_registryObjectIds;
    QHash<uint32_t, QString> m_globalInterfaces;
    QHash<uint32_t, uint32_t> m_globalVersions;
    QHash<uint32_t, QString> m_objectInterfaces;
    QSet<uint32_t> m_reservedPanelGlobalNames;
    QVector<uint32_t> m_globalOrder;
};

void appendUint32(QByteArray &buffer, uint32_t value)
{
    buffer.append(reinterpret_cast<const char *>(&value), sizeof(value));
}

QByteArray makeWaylandString(const QString &text)
{
    QByteArray encoded = text.toUtf8();
    encoded.append('\0');

    QByteArray payload;
    appendUint32(payload, static_cast<uint32_t>(encoded.size()));
    payload.append(encoded);
    while (payload.size() % 4 != 0) {
        payload.append('\0');
    }
    return payload;
}

QByteArray makeWaylandMessage(uint32_t objectId, uint16_t opcode, const QByteArray &payload)
{
    const uint16_t messageSize = static_cast<uint16_t>(8 + payload.size());
    const uint32_t sizeAndOpcode = (static_cast<uint32_t>(messageSize) << 16) | opcode;

    QByteArray message;
    appendUint32(message, objectId);
    appendUint32(message, sizeAndOpcode);
    message.append(payload);
    return message;
}

QByteArray makeRegistryGlobalMessage(uint32_t registryId, uint32_t name, const QString &interfaceName, uint32_t version)
{
    QByteArray payload;
    appendUint32(payload, name);
    payload.append(makeWaylandString(interfaceName));
    appendUint32(payload, version);
    return makeWaylandMessage(registryId, WlRegistryGlobalOpcode, payload);
}

class PanelBranchEndpoint
{
public:
    explicit PanelBranchEndpoint(QVector<WaylandProtocolInspector::RegistryGlobal> globals)
        : m_globals(std::move(globals))
    {
    }

    void receive(const char *data, qsizetype size, QByteArray &output)
    {
        m_buffer.append(data, size);

        while (m_buffer.size() >= 8) {
            const char *message = m_buffer.constData();
            const uint32_t objectId = readUint32(message);
            const uint32_t sizeAndOpcode = readUint32(message + 4);
            const uint16_t opcode = static_cast<uint16_t>(sizeAndOpcode & 0xffff);
            const uint16_t messageSize = static_cast<uint16_t>(sizeAndOpcode >> 16);

            if (messageSize < 8) {
                qWarning() << "Invalid Wayland message size from panel branch:" << messageSize;
                m_buffer.clear();
                return;
            }
            if (m_buffer.size() < messageSize) {
                return;
            }

            const char *payload = message + 8;
            const qsizetype payloadSize = messageSize - 8;
            handleMessage(objectId, opcode, payload, payloadSize, output);
            m_buffer.remove(0, messageSize);
        }
    }

private:
    static uint32_t readUint32(const char *data)
    {
        uint32_t value = 0;
        memcpy(&value, data, sizeof(value));
        return value;
    }

    void handleMessage(uint32_t objectId, uint16_t opcode, const char *payload, qsizetype payloadSize, QByteArray &output)
    {
        if (objectId == WlDisplayObjectId && opcode == WlDisplayGetRegistryOpcode && payloadSize >= 4) {
            const uint32_t registryId = readUint32(payload);
            m_registryObjectIds.insert(registryId);
            sendRegistryProjection(registryId, output);
            return;
        }

        if (m_registryObjectIds.contains(objectId) && opcode == WlRegistryBindOpcode) {
            qInfo() << "KDE OSK panel branch registry bind received; object mapping is not enabled yet.";
        }
    }

    void sendRegistryProjection(uint32_t registryId, QByteArray &output)
    {
        for (const WaylandProtocolInspector::RegistryGlobal &global : std::as_const(m_globals)) {
            output.append(makeRegistryGlobalMessage(registryId, global.name, global.interfaceName, global.version));
        }
    }

    QVector<WaylandProtocolInspector::RegistryGlobal> m_globals;
    QByteArray m_buffer;
    QSet<uint32_t> m_registryObjectIds;
};

int runProtocolFilterSelfTest()
{
    constexpr uint32_t RegistryId = 2;
    constexpr uint32_t CompositorGlobalName = 3;
    constexpr uint32_t InputMethodGlobalName = 10;
    constexpr uint32_t InputPanelGlobalName = 11;

    WaylandProtocolInspector inspector;

    QByteArray getRegistryPayload;
    appendUint32(getRegistryPayload, RegistryId);
    const QByteArray getRegistry = makeWaylandMessage(WlDisplayObjectId, WlDisplayGetRegistryOpcode, getRegistryPayload);
    inspector.inspect(WaylandProtocolInspector::Direction::DownstreamToUpstream, getRegistry.constData(), getRegistry.size());

    const QByteArray compositorGlobal = makeRegistryGlobalMessage(RegistryId,
                                                                  CompositorGlobalName,
                                                                  QStringLiteral("wl_compositor"),
                                                                  6);
    const QByteArray inputMethodGlobal = makeRegistryGlobalMessage(RegistryId,
                                                                   InputMethodGlobalName,
                                                                   QStringLiteral("zwp_input_method_v1"),
                                                                   1);
    const QByteArray inputPanelGlobal = makeRegistryGlobalMessage(RegistryId,
                                                                  InputPanelGlobalName,
                                                                  QStringLiteral("zwp_input_panel_v1"),
                                                                  1);

    QByteArray filteredOutput;
    const QByteArray globals = compositorGlobal + inputMethodGlobal + inputPanelGlobal;
    inspector.filterUpstreamToDownstream(globals.constData(), globals.size(), filteredOutput);
    if (filteredOutput != compositorGlobal + inputMethodGlobal) {
        qCritical() << "Wayland protocol filter self-test failed: input-panel global was not reserved correctly.";
        return 1;
    }

    const QVector<WaylandProtocolInspector::RegistryGlobal> fcitxGlobals =
        inspector.projectedGlobals(WaylandProtocolInspector::RegistryView::FcitxInputMethodBranch);
    const QVector<WaylandProtocolInspector::RegistryGlobal> panelGlobals =
        inspector.projectedGlobals(WaylandProtocolInspector::RegistryView::KdeOskPanelBranch);
    const auto containsInterface = [](const QVector<WaylandProtocolInspector::RegistryGlobal> &globals, const QString &interfaceName) {
        return std::any_of(globals.cbegin(), globals.cend(), [&interfaceName](const WaylandProtocolInspector::RegistryGlobal &global) {
            return global.interfaceName == interfaceName;
        });
    };

    if (!containsInterface(fcitxGlobals, QStringLiteral("zwp_input_method_v1"))
        || containsInterface(fcitxGlobals, QStringLiteral("zwp_input_panel_v1"))) {
        qCritical() << "Wayland protocol filter self-test failed: fcitx registry projection is incorrect.";
        return 1;
    }
    if (!containsInterface(panelGlobals, QStringLiteral("zwp_input_panel_v1"))
        || containsInterface(panelGlobals, QStringLiteral("zwp_input_method_v1"))
        || !containsInterface(panelGlobals, QStringLiteral("wl_compositor"))) {
        qCritical() << "Wayland protocol filter self-test failed: KDE OSK panel registry projection is incorrect.";
        return 1;
    }

    constexpr uint32_t PanelRegistryId = 7;
    PanelBranchEndpoint panelBranch(panelGlobals);
    QByteArray panelGetRegistryPayload;
    appendUint32(panelGetRegistryPayload, PanelRegistryId);
    const QByteArray panelGetRegistry = makeWaylandMessage(WlDisplayObjectId, WlDisplayGetRegistryOpcode, panelGetRegistryPayload);
    QByteArray panelRegistryOutput;
    panelBranch.receive(panelGetRegistry.constData(), panelGetRegistry.size(), panelRegistryOutput);

    const QByteArray expectedPanelRegistry = makeRegistryGlobalMessage(PanelRegistryId,
                                                                       CompositorGlobalName,
                                                                       QStringLiteral("wl_compositor"),
                                                                       6)
        + makeRegistryGlobalMessage(PanelRegistryId,
                                    InputPanelGlobalName,
                                    QStringLiteral("zwp_input_panel_v1"),
                                    1);
    if (panelRegistryOutput != expectedPanelRegistry) {
        qCritical() << "Wayland protocol filter self-test failed: panel branch registry handshake is incorrect.";
        return 1;
    }

    QByteArray removePayload;
    appendUint32(removePayload, InputPanelGlobalName);
    const QByteArray panelRemove = makeWaylandMessage(RegistryId, WlRegistryGlobalRemoveOpcode, removePayload);
    filteredOutput.clear();
    inspector.filterUpstreamToDownstream(panelRemove.constData(), panelRemove.size(), filteredOutput);
    if (!filteredOutput.isEmpty()) {
        qCritical() << "Wayland protocol filter self-test failed: reserved input-panel removal leaked to fcitx5.";
        return 1;
    }

    qInfo() << "Wayland protocol filter self-test passed.";
    return 0;
}

class WaylandSocketProxy final : public QObject
{
public:
    WaylandSocketProxy(int upstreamFd, int downstreamFd, QObject *parent = nullptr)
        : QObject(parent)
        , m_upstreamFd(upstreamFd)
        , m_downstreamFd(downstreamFd)
    {
    }

    ~WaylandSocketProxy() override
    {
        closeFds(m_upstreamToDownstreamFds);
        closeFds(m_downstreamToUpstreamFds);
        closeFd(m_upstreamFd);
        closeFd(m_downstreamFd);
    }

    bool start()
    {
#ifndef Q_OS_UNIX
        qCritical() << "Wayland socket proxy is only supported on Unix-like systems.";
        return false;
#else
        if (!setNonBlocking(m_upstreamFd) || !setNonBlocking(m_downstreamFd)) {
            return false;
        }

        m_upstreamReader = new QSocketNotifier(m_upstreamFd, QSocketNotifier::Read, this);
        m_downstreamReader = new QSocketNotifier(m_downstreamFd, QSocketNotifier::Read, this);
        m_upstreamWriter = new QSocketNotifier(m_upstreamFd, QSocketNotifier::Write, this);
        m_downstreamWriter = new QSocketNotifier(m_downstreamFd, QSocketNotifier::Write, this);

        m_upstreamWriter->setEnabled(false);
        m_downstreamWriter->setEnabled(false);

        connect(m_upstreamReader, &QSocketNotifier::activated, this, [this]() {
            readAvailable(m_upstreamFd,
                          WaylandProtocolInspector::Direction::UpstreamToDownstream,
                          m_upstreamToDownstream,
                          m_upstreamToDownstreamFds,
                          m_downstreamWriter,
                          m_upstreamReader);
        });
        connect(m_downstreamReader, &QSocketNotifier::activated, this, [this]() {
            readAvailable(m_downstreamFd,
                          WaylandProtocolInspector::Direction::DownstreamToUpstream,
                          m_downstreamToUpstream,
                          m_downstreamToUpstreamFds,
                          m_upstreamWriter,
                          m_downstreamReader);
        });
        connect(m_upstreamWriter, &QSocketNotifier::activated, this, [this]() {
            writeAvailable(m_upstreamFd, m_downstreamToUpstream, m_downstreamToUpstreamFds, m_upstreamWriter, m_downstreamReader);
        });
        connect(m_downstreamWriter, &QSocketNotifier::activated, this, [this]() {
            writeAvailable(m_downstreamFd, m_upstreamToDownstream, m_upstreamToDownstreamFds, m_downstreamWriter, m_upstreamReader);
        });

        qInfo() << "Started transparent KWin Wayland socket proxy for fcitx5 delegation.";
        return true;
#endif
    }

private:
    static void closeFd(int &fd)
    {
#ifdef Q_OS_UNIX
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
#else
        Q_UNUSED(fd)
#endif
    }

    static void closeFds(QVector<int> &fds)
    {
#ifdef Q_OS_UNIX
        for (int fd : std::as_const(fds)) {
            if (fd >= 0) {
                close(fd);
            }
        }
#endif
        fds.clear();
    }

    static bool setNonBlocking(int fd)
    {
#ifndef Q_OS_UNIX
        Q_UNUSED(fd)
        return false;
#else
        const int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0) {
            qCritical() << "Failed to read socket flags:" << strerror(errno);
            return false;
        }
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            qCritical() << "Failed to set socket non-blocking:" << strerror(errno);
            return false;
        }
        return true;
#endif
    }

    void readAvailable(int fd,
                       WaylandProtocolInspector::Direction direction,
                       QByteArray &buffer,
                       QVector<int> &fds,
                       QSocketNotifier *writer,
                       QSocketNotifier *reader)
    {
#ifndef Q_OS_UNIX
        Q_UNUSED(fd)
        Q_UNUSED(direction)
        Q_UNUSED(buffer)
        Q_UNUSED(fds)
        Q_UNUSED(writer)
        Q_UNUSED(reader)
#else
        reader->setEnabled(false);

        char chunk[8192];
        while (buffer.size() < MaxProxyBufferBytes) {
            char control[CMSG_SPACE(sizeof(int) * 32)];
            struct iovec iov;
            iov.iov_base = chunk;
            iov.iov_len = sizeof(chunk);

            struct msghdr message;
            memset(&message, 0, sizeof(message));
            message.msg_iov = &iov;
            message.msg_iovlen = 1;
            message.msg_control = control;
            message.msg_controllen = sizeof(control);

            const ssize_t bytesRead = recvmsg(fd, &message, MSG_CMSG_CLOEXEC);
            if (bytesRead > 0) {
                if (direction == WaylandProtocolInspector::Direction::UpstreamToDownstream) {
                    m_protocolInspector.filterUpstreamToDownstream(chunk, bytesRead, buffer);
                } else {
                    buffer.append(chunk, bytesRead);
                    m_protocolInspector.inspect(direction, chunk, bytesRead);
                }
                appendReceivedFds(message, fds);
                continue;
            }
            if (bytesRead == 0) {
                closeProxy(QStringLiteral("Wayland peer closed the socket."));
                return;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            closeProxy(QStringLiteral("Failed to read Wayland proxy socket: %1").arg(QString::fromLocal8Bit(strerror(errno))));
            return;
        }

        writer->setEnabled(!buffer.isEmpty());
        reader->setEnabled(buffer.size() < MaxProxyBufferBytes);
#endif
    }

    static void appendReceivedFds(const struct msghdr &message, QVector<int> &fds)
    {
#ifdef Q_OS_UNIX
        for (struct cmsghdr *controlMessage = CMSG_FIRSTHDR(const_cast<struct msghdr *>(&message));
             controlMessage != nullptr;
             controlMessage = CMSG_NXTHDR(const_cast<struct msghdr *>(&message), controlMessage)) {
            if (controlMessage->cmsg_level != SOL_SOCKET || controlMessage->cmsg_type != SCM_RIGHTS) {
                continue;
            }

            const auto byteCount = static_cast<int>(controlMessage->cmsg_len - CMSG_LEN(0));
            const int fdCount = byteCount / static_cast<int>(sizeof(int));
            const int *receivedFds = reinterpret_cast<const int *>(CMSG_DATA(controlMessage));
            for (int index = 0; index < fdCount; ++index) {
                fds.push_back(receivedFds[index]);
            }
        }
#else
        Q_UNUSED(message)
        Q_UNUSED(fds)
#endif
    }

    void writeAvailable(int fd, QByteArray &buffer, QVector<int> &fds, QSocketNotifier *writer, QSocketNotifier *readerToReenable)
    {
#ifndef Q_OS_UNIX
        Q_UNUSED(fd)
        Q_UNUSED(buffer)
        Q_UNUSED(fds)
        Q_UNUSED(writer)
        Q_UNUSED(readerToReenable)
#else
        writer->setEnabled(false);

        while (!buffer.isEmpty()) {
            const ssize_t bytesWritten = writeProxyMessage(fd, buffer, fds);
            if (bytesWritten > 0) {
                buffer.remove(0, bytesWritten);
                continue;
            }
            if (bytesWritten == 0) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            closeProxy(QStringLiteral("Failed to write Wayland proxy socket: %1").arg(QString::fromLocal8Bit(strerror(errno))));
            return;
        }

        writer->setEnabled(!buffer.isEmpty());
        readerToReenable->setEnabled(buffer.size() < MaxProxyBufferBytes);
#endif
    }

    static ssize_t writeProxyMessage(int fd, const QByteArray &buffer, QVector<int> &fds)
    {
#ifndef Q_OS_UNIX
        Q_UNUSED(fd)
        Q_UNUSED(buffer)
        Q_UNUSED(fds)
        return -1;
#else
        if (fds.isEmpty()) {
            return write(fd, buffer.constData(), static_cast<size_t>(buffer.size()));
        }

        constexpr int MaxFdsPerMessage = 32;
        const int fdCount = static_cast<int>(std::min<qsizetype>(fds.size(), MaxFdsPerMessage));
        char control[CMSG_SPACE(sizeof(int) * MaxFdsPerMessage)];
        memset(control, 0, sizeof(control));

        struct iovec iov;
        iov.iov_base = const_cast<char *>(buffer.constData());
        iov.iov_len = static_cast<size_t>(buffer.size());

        struct msghdr message;
        memset(&message, 0, sizeof(message));
        message.msg_iov = &iov;
        message.msg_iovlen = 1;
        message.msg_control = control;
        message.msg_controllen = CMSG_SPACE(sizeof(int) * fdCount);

        struct cmsghdr *controlMessage = CMSG_FIRSTHDR(&message);
        controlMessage->cmsg_level = SOL_SOCKET;
        controlMessage->cmsg_type = SCM_RIGHTS;
        controlMessage->cmsg_len = CMSG_LEN(sizeof(int) * fdCount);
        memcpy(CMSG_DATA(controlMessage), fds.constData(), sizeof(int) * fdCount);

        const ssize_t bytesWritten = sendmsg(fd, &message, MSG_NOSIGNAL);
        if (bytesWritten > 0) {
            for (int index = 0; index < fdCount; ++index) {
                close(fds[index]);
            }
            fds.erase(fds.begin(), fds.begin() + fdCount);
        }
        return bytesWritten;
#endif
    }

    void closeProxy(const QString &reason)
    {
        qWarning() << reason;
        QCoreApplication::quit();
    }

    int m_upstreamFd = -1;
    int m_downstreamFd = -1;
    QByteArray m_upstreamToDownstream;
    QByteArray m_downstreamToUpstream;
    QVector<int> m_upstreamToDownstreamFds;
    QVector<int> m_downstreamToUpstreamFds;
    WaylandProtocolInspector m_protocolInspector;
    QSocketNotifier *m_upstreamReader = nullptr;
    QSocketNotifier *m_downstreamReader = nullptr;
    QSocketNotifier *m_upstreamWriter = nullptr;
    QSocketNotifier *m_downstreamWriter = nullptr;
};

class FcitxSocketDelegate final : public QObject
{
public:
    FcitxSocketDelegate(int fd, QString displayName, int timeoutMs, QObject *parent = nullptr)
        : QObject(parent)
        , m_fd(fd)
        , m_displayName(std::move(displayName))
        , m_bus(QDBusConnection::sessionBus())
        , m_watcher(FcitxService,
                    m_bus,
                    QDBusServiceWatcher::WatchForOwnerChange,
                    this)
    {
        m_timeout.setSingleShot(true);
        m_timeout.setInterval(timeoutMs);
        connect(&m_timeout, &QTimer::timeout, this, [this]() {
            qCritical() << "Timed out waiting for fcitx5 Wayland socket delegation.";
            QCoreApplication::exit(1);
        });

        connect(&m_watcher,
                &QDBusServiceWatcher::serviceOwnerChanged,
                this,
                [this](const QString &, const QString &oldOwner, const QString &newOwner) {
                    handleOwnerChanged(oldOwner, newOwner);
                });
    }

    void start()
    {
        if (!m_bus.isConnected()) {
            qCritical() << "Failed to connect to the session bus.";
            QCoreApplication::exit(1);
            return;
        }

        const QString owner = m_bus.interface()->serviceOwner(FcitxService);
        if (owner.isEmpty()) {
            startFcitx5Service();
        } else {
            scheduleConnect(owner);
        }

        m_timeout.start();
    }

private:
    void startFcitx5Service()
    {
        qInfo() << "Requesting DBus activation for stock fcitx5.";
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"),
                                                              QStringLiteral("/"),
                                                              QStringLiteral("org.freedesktop.DBus"),
                                                              QStringLiteral("StartServiceByName"));
        message << QString::fromLatin1(FcitxService) << uint(0);
        m_bus.asyncCall(message);
    }

    void handleOwnerChanged(const QString &oldOwner, const QString &newOwner)
    {
        if (!m_connectedOwner.isEmpty() && oldOwner == m_connectedOwner) {
            qWarning() << "Connected fcitx5 instance disappeared; exiting so KWin can restart the broker.";
            QCoreApplication::quit();
            return;
        }

        if (m_connectedOwner.isEmpty() && !newOwner.isEmpty()) {
            scheduleConnect(newOwner);
        }
    }

    void scheduleConnect(const QString &owner)
    {
        if (m_connectScheduled || !m_connectedOwner.isEmpty()) {
            return;
        }

        m_connectScheduled = true;
        QTimer::singleShot(1000, this, [this, owner]() {
            connectToFcitx5(owner);
        });
    }

    void connectToFcitx5(const QString &owner)
    {
        if (m_fd < 0) {
            qCritical() << "WAYLAND_SOCKET is required for KWin input-method delegation.";
            QCoreApplication::exit(1);
            return;
        }

#ifndef Q_OS_UNIX
        qCritical() << "Wayland socket delegation is only supported on Unix-like systems.";
        Q_UNUSED(owner)
        QCoreApplication::exit(1);
#else
        int proxySockets[2] = {-1, -1};
        if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, proxySockets) < 0) {
            qCritical() << "Failed to create fcitx5 Wayland proxy socketpair:" << strerror(errno);
            QCoreApplication::exit(1);
            return;
        }

        auto *proxy = new WaylandSocketProxy(m_fd, proxySockets[0], this);
        if (!proxy->start()) {
            delete proxy;
            close(proxySockets[1]);
            QCoreApplication::exit(1);
            return;
        }
        m_fd = -1;

        QDBusUnixFileDescriptor descriptor;
        descriptor.giveFileDescriptor(proxySockets[1]);
        proxySockets[1] = -1;
        QDBusInterface controller(owner,
                                  QString::fromLatin1(FcitxControllerPath),
                                  QString::fromLatin1(FcitxControllerInterface),
                                  m_bus);
        if (!controller.isValid()) {
            qCritical() << "Failed to create fcitx5 controller proxy:" << controller.lastError().message();
            delete proxy;
            QCoreApplication::exit(1);
            return;
        }

        qInfo() << "Delegating KWin Wayland socket to fcitx5 owner" << owner
                << "through broker proxy for display" << (m_displayName.isEmpty() ? QStringLiteral("<default>") : m_displayName);
        QDBusMessage message = QDBusMessage::createMethodCall(owner,
                                                              QString::fromLatin1(FcitxControllerPath),
                                                              QString::fromLatin1(FcitxControllerInterface),
                                                              QStringLiteral("ReopenWaylandConnectionSocket"));
        message << m_displayName << QVariant::fromValue(descriptor);

        auto *watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(message), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, owner, proxy, watcher]() {
            QDBusPendingReply<> reply = *watcher;
            watcher->deleteLater();
            if (reply.isError()) {
                qCritical() << "fcitx5 rejected Wayland socket delegation:" << reply.error().name() << reply.error().message();
                delete proxy;
                QCoreApplication::exit(1);
                return;
            }

            m_proxy = proxy;
            m_connectedOwner = owner;
            m_timeout.stop();
            qInfo() << "fcitx5 Wayland socket delegation succeeded; broker proxy will stay resident.";
        });
#endif
    }

    int m_fd = -1;
    QString m_displayName;
    QDBusConnection m_bus;
    QDBusServiceWatcher m_watcher;
    QTimer m_timeout;
    WaylandSocketProxy *m_proxy = nullptr;
    QString m_connectedOwner;
    bool m_connectScheduled = false;
};
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kde-osk-kwin-broker"));
    app.setOrganizationName(QStringLiteral("kde-osk"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("KDE OSK KWin virtual keyboard broker"));
    parser.addHelpOption();

    QCommandLineOption fcitx5CommandOption(
        QStringLiteral("fcitx5-command"),
        QStringLiteral("Command used for the stock fcitx5 delegation target."),
        QStringLiteral("command"),
        QStringLiteral("fcitx5"));
    parser.addOption(fcitx5CommandOption);

    QCommandLineOption fcitx5ArgumentOption(
        QStringLiteral("fcitx5-arg"),
        QStringLiteral("Extra argument passed to the stock fcitx5 process. Can be repeated."),
        QStringLiteral("argument"));
    parser.addOption(fcitx5ArgumentOption);

    QCommandLineOption checkOnlyOption(
        QStringLiteral("check-only"),
        QStringLiteral("Validate the broker startup environment and exit without launching fcitx5."));
    parser.addOption(checkOnlyOption);

    QCommandLineOption selfTestProtocolFilterOption(
        QStringLiteral("self-test-protocol-filter"),
        QStringLiteral("Run the broker Wayland protocol filter self-test and exit."));
    parser.addOption(selfTestProtocolFilterOption);

    QCommandLineOption execFcitx5Option(
        QStringLiteral("exec-fcitx5"),
        QStringLiteral("Compatibility mode: replace the broker process with stock fcitx5."));
    parser.addOption(execFcitx5Option);

    QCommandLineOption timeoutOption(
        QStringLiteral("timeout-ms"),
        QStringLiteral("Resident mode timeout while waiting for fcitx5 delegation."),
        QStringLiteral("milliseconds"),
        QStringLiteral("10000"));
    parser.addOption(timeoutOption);

    parser.process(app);

    if (parser.isSet(selfTestProtocolFilterOption)) {
        return runProtocolFilterSelfTest();
    }

    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QString waylandSocket = environment.value(QStringLiteral("WAYLAND_SOCKET"));
    const int socketFd = waylandSocketFd();
    if (waylandSocket.isEmpty()) {
        qWarning() << "WAYLAND_SOCKET is not set. KWin normally sets it when starting a virtual keyboard backend.";
    } else if (socketFd < 0) {
        qWarning() << "WAYLAND_SOCKET is not a valid file descriptor number:" << waylandSocket;
    } else {
        qInfo() << "KWin Wayland input-method socket detected:" << waylandSocket;
    }

    if (parser.isSet(checkOnlyOption)) {
        return socketFd < 0 ? 1 : 0;
    }

    if (parser.isSet(execFcitx5Option)) {
        return execStockFcitx5(parser.value(fcitx5CommandOption), parser.values(fcitx5ArgumentOption));
    }

    if (socketFd < 0) {
        qCritical() << "Resident mode requires WAYLAND_SOCKET from KWin.";
        return 1;
    }

    bool timeoutOk = false;
    const int timeoutMs = parser.value(timeoutOption).toInt(&timeoutOk);
    FcitxSocketDelegate delegate(socketFd,
                                 environment.value(QStringLiteral("WAYLAND_DISPLAY")),
                                 timeoutOk ? timeoutMs : 10000,
                                 &app);
    delegate.start();

    return app.exec();
}
