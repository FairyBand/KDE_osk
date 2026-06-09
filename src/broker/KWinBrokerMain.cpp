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
#include <QProcessEnvironment>
#include <QSocketNotifier>
#include <QStringList>
#include <QTimer>

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
            readAvailable(m_upstreamFd, m_upstreamToDownstream, m_downstreamWriter, m_upstreamReader);
        });
        connect(m_downstreamReader, &QSocketNotifier::activated, this, [this]() {
            readAvailable(m_downstreamFd, m_downstreamToUpstream, m_upstreamWriter, m_downstreamReader);
        });
        connect(m_upstreamWriter, &QSocketNotifier::activated, this, [this]() {
            writeAvailable(m_upstreamFd, m_downstreamToUpstream, m_upstreamWriter, m_downstreamReader);
        });
        connect(m_downstreamWriter, &QSocketNotifier::activated, this, [this]() {
            writeAvailable(m_downstreamFd, m_upstreamToDownstream, m_downstreamWriter, m_upstreamReader);
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

    void readAvailable(int fd, QByteArray &buffer, QSocketNotifier *writer, QSocketNotifier *reader)
    {
#ifndef Q_OS_UNIX
        Q_UNUSED(fd)
        Q_UNUSED(buffer)
        Q_UNUSED(writer)
        Q_UNUSED(reader)
#else
        reader->setEnabled(false);

        char chunk[8192];
        while (buffer.size() < MaxProxyBufferBytes) {
            const ssize_t bytesRead = read(fd, chunk, sizeof(chunk));
            if (bytesRead > 0) {
                buffer.append(chunk, bytesRead);
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

    void writeAvailable(int fd, QByteArray &buffer, QSocketNotifier *writer, QSocketNotifier *readerToReenable)
    {
#ifndef Q_OS_UNIX
        Q_UNUSED(fd)
        Q_UNUSED(buffer)
        Q_UNUSED(writer)
        Q_UNUSED(readerToReenable)
#else
        writer->setEnabled(false);

        while (!buffer.isEmpty()) {
            const ssize_t bytesWritten = write(fd, buffer.constData(), static_cast<size_t>(buffer.size()));
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

    void closeProxy(const QString &reason)
    {
        qWarning() << reason;
        QCoreApplication::quit();
    }

    int m_upstreamFd = -1;
    int m_downstreamFd = -1;
    QByteArray m_upstreamToDownstream;
    QByteArray m_downstreamToUpstream;
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
