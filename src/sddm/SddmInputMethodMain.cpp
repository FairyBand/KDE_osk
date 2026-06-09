#include "HardwareKeyboardMonitor.h"
#include "InputPanelIntegration.h"
#include "SddmInputMethodController.h"
#include "ShellWindowAdapter.h"

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kde-osk-sddm-inputmethod"));
    app.setApplicationDisplayName(QStringLiteral("KDE OSK SDDM Input Method"));
    app.setOrganizationName(QStringLiteral("kde-osk"));

    HardwareKeyboardMonitor hardwareKeyboardMonitor;
    SddmInputMethodController controller(&hardwareKeyboardMonitor);
    ShellWindowAdapter windowAdapter;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("sddmInputMethodController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("sddmWindowAdapter"), &windowAdapter);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(1); },
        Qt::QueuedConnection);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [](QObject *object, const QUrl &) {
        auto *window = qobject_cast<QWindow *>(object);
        if (window == nullptr) {
            qCritical() << "KDE OSK SDDM input method root object is not a window.";
            QCoreApplication::exit(1);
            return;
        }
        if (!initializeInputPanelSurface(window, InputPanelRole::Keyboard)) {
            QCoreApplication::exit(1);
        }
    });

    engine.load(QUrl(QStringLiteral("qrc:/KdeOsk/SddmInputMethod/qml/SddmInputMethod.qml")));

    return app.exec();
}
