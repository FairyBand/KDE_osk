#include "KeyboardController.h"

#include "HardwareKeyboardMonitor.h"
#include "UInputKeyboard.h"

#include <QDBusConnection>
#include <QDebug>

KeyboardController::KeyboardController(HardwareKeyboardMonitor *hardwareKeyboardMonitor, UInputKeyboard *inputKeyboard, QObject *parent)
    : QObject(parent)
    , m_hardwareKeyboardMonitor(hardwareKeyboardMonitor)
    , m_inputKeyboard(inputKeyboard)
{
    connect(m_hardwareKeyboardMonitor,
            &HardwareKeyboardMonitor::hardwareKeyboardPresentChanged,
            this,
            [this]() { updateVisibilityFromPolicy(); });
    connect(m_inputKeyboard, &UInputKeyboard::availableChanged, this, &KeyboardController::inputBackendAvailableChanged);
    connect(m_inputKeyboard, &UInputKeyboard::lastErrorChanged, this, &KeyboardController::inputBackendErrorChanged);
}

bool KeyboardController::visible() const
{
    return m_visible;
}

bool KeyboardController::autoShowEnabled() const
{
    return m_autoShowEnabled;
}

bool KeyboardController::ignoreHardwareKeyboard() const
{
    return m_ignoreHardwareKeyboard;
}

bool KeyboardController::inputBackendAvailable() const
{
    return m_inputKeyboard->available();
}

QString KeyboardController::inputBackendError() const
{
    return m_inputKeyboard->lastError();
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
    if (shouldSuppressForHardwareKeyboard()) {
        return;
    }
    setVisible(true);
}

void KeyboardController::forceShowKeyboard()
{
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

void KeyboardController::setIgnoreHardwareKeyboard(bool ignore)
{
    if (m_ignoreHardwareKeyboard == ignore) {
        return;
    }

    m_ignoreHardwareKeyboard = ignore;
    emit ignoreHardwareKeyboardChanged(m_ignoreHardwareKeyboard);
    updateVisibilityFromPolicy();
}

void KeyboardController::keyPressed(const QString &keyId, bool shift, bool ctrl, bool alt, bool meta)
{
    emit commitTextRequested(keyId);
    if (!m_inputKeyboard->sendKey(keyId, shift, ctrl, alt, meta)) {
        qWarning() << "Failed to send key:" << keyId;
    }
}

void KeyboardController::updateVisibilityFromPolicy()
{
    if (!m_autoShowEnabled || shouldSuppressForHardwareKeyboard()) {
        setVisible(false);
        return;
    }

    setVisible(m_textFocusActive);
}

bool KeyboardController::shouldSuppressForHardwareKeyboard() const
{
    return !m_ignoreHardwareKeyboard && m_hardwareKeyboardMonitor->hardwareKeyboardPresent();
}

void KeyboardController::setVisible(bool visible)
{
    if (m_visible == visible) {
        return;
    }

    m_visible = visible;
    emit visibleChanged(m_visible);
}
