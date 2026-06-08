#include "HardwareKeyboardMonitor.h"
#include "KeyboardController.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kde-osk-shell"));
    app.setApplicationDisplayName(QStringLiteral("KDE OSK"));
    app.setOrganizationName(QStringLiteral("kde-osk"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("KDE Plasma on-screen keyboard shell"));
    parser.addHelpOption();

    QCommandLineOption forceVisibleOption(
        QStringLiteral("show"),
        QStringLiteral("Show the keyboard immediately on startup."));
    parser.addOption(forceVisibleOption);

    parser.process(app);

    HardwareKeyboardMonitor hardwareKeyboardMonitor;
    KeyboardController controller(&hardwareKeyboardMonitor);
    if (!controller.registerDBus()) {
        qWarning("Failed to register D-Bus service. Another kde-osk-shell may already be running.");
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("keyboardController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("hardwareKeyboardMonitor"), &hardwareKeyboardMonitor);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(1); },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("KdeOsk"), QStringLiteral("Main"));

    if (parser.isSet(forceVisibleOption)) {
        controller.showKeyboard();
    }

    return app.exec();
}
