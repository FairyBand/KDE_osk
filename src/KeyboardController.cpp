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

bool KeyboardController::textFocusActive() const
{
    return m_textFocusActive;
}

int KeyboardController::focusRectX() const
{
    return m_focusRect.x();
}

int KeyboardController::focusRectY() const
{
    return m_focusRect.y();
}

int KeyboardController::focusRectWidth() const
{
    return m_focusRect.width();
}

int KeyboardController::focusRectHeight() const
{
    return m_focusRect.height();
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
    emit textFocusActiveChanged(m_textFocusActive);
    updateVisibilityFromPolicy();
}

void KeyboardController::setTextFocusRect(bool active, int x, int y, int width, int height)
{
    const QRect rect(x, y, qMax(0, width), qMax(0, height));
    const bool focusChanged = m_textFocusActive != active;
    const bool rectChanged = m_focusRect != rect;

    if (!focusChanged && !rectChanged) {
        return;
    }

    m_textFocusActive = active;
    if (rectChanged) {
        m_focusRect = rect;
        emit focusRectChanged();
    }
    if (focusChanged) {
        emit textFocusActiveChanged(m_textFocusActive);
    }
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

void KeyboardController::setModifierActive(const QString &keyId, bool active)
{
    if (!m_inputKeyboard->setModifierActive(keyId, active)) {
        qWarning() << "Failed to set modifier:" << keyId << active;
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
