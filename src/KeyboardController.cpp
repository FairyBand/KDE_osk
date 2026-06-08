#include "KeyboardController.h"

#include "HardwareKeyboardMonitor.h"

#include <QDBusConnection>
#include <QDebug>

KeyboardController::KeyboardController(HardwareKeyboardMonitor *hardwareKeyboardMonitor, QObject *parent)
    : QObject(parent)
    , m_hardwareKeyboardMonitor(hardwareKeyboardMonitor)
{
    connect(m_hardwareKeyboardMonitor,
            &HardwareKeyboardMonitor::hardwareKeyboardPresentChanged,
            this,
            [this]() { updateVisibilityFromPolicy(); });
}

bool KeyboardController::visible() const
{
    return m_visible;
}

bool KeyboardController::autoShowEnabled() const
{
    return m_autoShowEnabled;
}

bool KeyboardController::registerDBus()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    const bool serviceOk = bus.registerService(QStringLiteral("org.kde.KdeOsk"));
    const bool objectOk = bus.registerObject(QStringLiteral("/Keyboard"),
                                             this,
                                             QDBusConnection::ExportScriptableSlots
                                                 | QDBusConnection::ExportAllSignals
                                                 | QDBusConnection::ExportAllProperties);
    return serviceOk && objectOk;
}

void KeyboardController::showKeyboard()
{
    if (m_hardwareKeyboardMonitor->hardwareKeyboardPresent()) {
        return;
    }
    setVisible(true);
}

void KeyboardController::hideKeyboard()
{
    setVisible(false);
}

void KeyboardController::setTextFocusActive(bool active)
{
    if (m_textFocusActive == active) {
        return;
    }

    m_textFocusActive = active;
    updateVisibilityFromPolicy();
}

void KeyboardController::setAutoShowEnabled(bool enabled)
{
    if (m_autoShowEnabled == enabled) {
        return;
    }

    m_autoShowEnabled = enabled;
    emit autoShowEnabledChanged(m_autoShowEnabled);
    updateVisibilityFromPolicy();
}

void KeyboardController::keyPressed(const QString &text)
{
    emit commitTextRequested(text);
    qInfo() << "Key pressed:" << text;
}

void KeyboardController::updateVisibilityFromPolicy()
{
    if (!m_autoShowEnabled || m_hardwareKeyboardMonitor->hardwareKeyboardPresent()) {
        setVisible(false);
        return;
    }

    setVisible(m_textFocusActive);
}

void KeyboardController::setVisible(bool visible)
{
    if (m_visible == visible) {
        return;
    }

    m_visible = visible;
    emit visibleChanged(m_visible);
}
