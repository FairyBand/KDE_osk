#include "HardwareKeyboardMonitor.h"
#include "InputPanelIntegration.h"
#include "SddmInputMethodController.h"

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QWindow>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kde-osk-input-panel"));
    app.setApplicationDisplayName(QStringLiteral("KDE OSK Input Panel"));
    app.setOrganizationName(QStringLiteral("kde-osk"));

    HardwareKeyboardMonitor hardwareKeyboardMonitor;
    SddmInputMethodController controller(&hardwareKeyboardMonitor, 700, true);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("inputPanelController"), &controller);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(1); },
        Qt::QueuedConnection);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [](QObject *object, const QUrl &) {
        auto *window = qobject_cast<QWindow *>(object);
        if (window == nullptr) {
            qCritical() << "KDE OSK input panel root object is not a window.";
            QCoreApplication::exit(1);
            return;
        }
        if (!initializeInputPanelSurface(window, InputPanelRole::Keyboard)) {
            QCoreApplication::exit(1);
        }
    });

    engine.load(QUrl(QStringLiteral("qrc:/KdeOsk/InputPanel/qml/InputPanel.qml")));

    return app.exec();
}
