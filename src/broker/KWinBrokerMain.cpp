#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QProcessEnvironment>
#include <QStringList>

#ifdef Q_OS_UNIX
#include <cerrno>
#include <cstring>
#include <unistd.h>
#endif

namespace
{
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

    parser.process(app);

    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QString waylandSocket = environment.value(QStringLiteral("WAYLAND_SOCKET"));
    if (waylandSocket.isEmpty()) {
        qWarning() << "WAYLAND_SOCKET is not set. KWin normally sets it when starting a virtual keyboard backend.";
    } else {
        qInfo() << "KWin Wayland input-method socket detected:" << waylandSocket;
    }

    if (parser.isSet(checkOnlyOption)) {
        return waylandSocket.isEmpty() ? 1 : 0;
    }

    return execStockFcitx5(parser.value(fcitx5CommandOption), parser.values(fcitx5ArgumentOption));
}
