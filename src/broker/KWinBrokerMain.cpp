#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusServiceWatcher>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QProcessEnvironment>
#include <QStringList>
#include <QTimer>

#ifdef Q_OS_UNIX
#include <cerrno>
#include <cstring>
#include <unistd.h>
#endif

namespace
{
constexpr auto FcitxService = "org.fcitx.Fcitx5";
constexpr auto FcitxControllerPath = "/controller";
constexpr auto FcitxControllerInterface = "org.fcitx.Fcitx.Controller1";

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
        const int fdForDbus = dup(m_fd);
        if (fdForDbus < 0) {
            qCritical() << "Failed to duplicate WAYLAND_SOCKET:" << strerror(errno);
            QCoreApplication::exit(1);
            return;
        }

        QDBusUnixFileDescriptor descriptor(fdForDbus);
        QDBusInterface controller(owner,
                                  QString::fromLatin1(FcitxControllerPath),
                                  QString::fromLatin1(FcitxControllerInterface),
                                  m_bus);
        if (!controller.isValid()) {
            qCritical() << "Failed to create fcitx5 controller proxy:" << controller.lastError().message();
            QCoreApplication::exit(1);
            return;
        }

        qInfo() << "Delegating KWin Wayland socket to fcitx5 owner" << owner
                << "for display" << (m_displayName.isEmpty() ? QStringLiteral("<default>") : m_displayName);
        QDBusMessage reply = controller.call(QStringLiteral("ReopenWaylandConnectionSocket"),
                                             m_displayName,
                                             QVariant::fromValue(descriptor));
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qCritical() << "fcitx5 rejected Wayland socket delegation:" << reply.errorName() << reply.errorMessage();
            QCoreApplication::exit(1);
            return;
        }

        close(m_fd);
        m_fd = -1;
        m_connectedOwner = owner;
        m_timeout.stop();
        qInfo() << "fcitx5 Wayland socket delegation succeeded; broker will stay resident.";
#endif
    }

    int m_fd = -1;
    QString m_displayName;
    QDBusConnection m_bus;
    QDBusServiceWatcher m_watcher;
    QTimer m_timeout;
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
