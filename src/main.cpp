#include "HardwareKeyboardMonitor.h"
#include "KeyboardController.h"
#include "ShellWindowAdapter.h"
#include "ShellSettings.h"
#include "UInputKeyboard.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    ShellWindowAdapter::prepareEnvironment();

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kde-osk-shell"));
    app.setApplicationDisplayName(QStringLiteral("KDE OSK"));
    app.setOrganizationName(QStringLiteral("kde-osk"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("KDE Plasma on-screen keyboard shell"));
    parser.addHelpOption();

    QCommandLineOption forceVisibleOption(
        QStringLiteral("show"),
        QStringLiteral("Show the keyboard immediately on startup if policy allows it."));
    parser.addOption(forceVisibleOption);
    QCommandLineOption forceShowOption(
        QStringLiteral("force-show"),
        QStringLiteral("Show the keyboard immediately and ignore hardware keyboard suppression."));
    parser.addOption(forceShowOption);

    parser.process(app);

    HardwareKeyboardMonitor hardwareKeyboardMonitor;
    UInputKeyboard inputKeyboard;
    ShellWindowAdapter shellWindowAdapter;
    ShellSettings shellSettings;
    KeyboardController controller(&hardwareKeyboardMonitor, &inputKeyboard);
    if (parser.isSet(forceShowOption)) {
        controller.setIgnoreHardwareKeyboard(true);
    }
    if (!controller.registerDBus()) {
        qWarning("Failed to register D-Bus service. Another kde-osk-shell may already be running.");
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("keyboardController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("hardwareKeyboardMonitor"), &hardwareKeyboardMonitor);
    engine.rootContext()->setContextProperty(QStringLiteral("shellWindowAdapter"), &shellWindowAdapter);
    engine.rootContext()->setContextProperty(QStringLiteral("shellSettings"), &shellSettings);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(1); },
        Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/KdeOsk/qml/Main.qml")));

    if (parser.isSet(forceVisibleOption)) {
        controller.showKeyboard();
    }
    if (parser.isSet(forceShowOption)) {
        controller.forceShowKeyboard();
    }

    return app.exec();
}
