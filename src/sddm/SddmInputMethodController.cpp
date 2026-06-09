#include "SddmInputMethodController.h"

#include "HardwareKeyboardMonitor.h"

namespace
{
constexpr uint XkbKeyBackspace = 0xff08;
constexpr uint XkbKeyTab = 0xff09;
constexpr uint XkbKeyReturn = 0xff0d;
constexpr uint XkbKeyEscape = 0xff1b;
constexpr uint XkbKeyHome = 0xff50;
constexpr uint XkbKeyLeft = 0xff51;
constexpr uint XkbKeyUp = 0xff52;
constexpr uint XkbKeyRight = 0xff53;
constexpr uint XkbKeyDown = 0xff54;
constexpr uint XkbKeyPageUp = 0xff55;
constexpr uint XkbKeyPageDown = 0xff56;
constexpr uint XkbKeyEnd = 0xff57;
constexpr uint XkbKeyDelete = 0xffff;
}

SddmInputMethodController::SddmInputMethodController(HardwareKeyboardMonitor *hardwareKeyboardMonitor, QObject *parent)
    : QObject(parent)
    , m_hardwareKeyboardMonitor(hardwareKeyboardMonitor)
    , m_shiftedLabels({
          {QStringLiteral("`"), QStringLiteral("~")},
          {QStringLiteral("1"), QStringLiteral("!")},
          {QStringLiteral("2"), QStringLiteral("@")},
          {QStringLiteral("3"), QStringLiteral("#")},
          {QStringLiteral("4"), QStringLiteral("$")},
          {QStringLiteral("5"), QStringLiteral("%")},
          {QStringLiteral("6"), QStringLiteral("^")},
          {QStringLiteral("7"), QStringLiteral("&")},
          {QStringLiteral("8"), QStringLiteral("*")},
          {QStringLiteral("9"), QStringLiteral("(")},
          {QStringLiteral("0"), QStringLiteral(")")},
          {QStringLiteral("-"), QStringLiteral("_")},
          {QStringLiteral("="), QStringLiteral("+")},
          {QStringLiteral("["), QStringLiteral("{")},
          {QStringLiteral("]"), QStringLiteral("}")},
          {QStringLiteral("\\"), QStringLiteral("|")},
          {QStringLiteral(";"), QStringLiteral(":")},
          {QStringLiteral("'"), QStringLiteral("\"")},
          {QStringLiteral(","), QStringLiteral("<")},
          {QStringLiteral("."), QStringLiteral(">")},
          {QStringLiteral("/"), QStringLiteral("?")},
      })
{
    connect(&m_inputMethod, &WaylandInputMethod::contextActiveChanged, this, &SddmInputMethodController::updateState);
    connect(&m_inputMethod, &WaylandInputMethod::protocolReadyChanged, this, &SddmInputMethodController::updateState);
    connect(m_hardwareKeyboardMonitor, &HardwareKeyboardMonitor::hardwareKeyboardPresentChanged, this, [this](bool present) {
        emit hardwareKeyboardPresentChanged(present);
        updateState();
    });
}

bool SddmInputMethodController::visible() const
{
    return m_visible;
}

bool SddmInputMethodController::inputBackendAvailable() const
{
    return m_inputMethod.protocolReady() && m_inputMethod.contextActive();
}

QString SddmInputMethodController::inputBackendError() const
{
    if (!m_inputMethod.protocolReady()) {
        return QStringLiteral("KWin input-method-v1 protocol is not available.");
    }
    if (!m_inputMethod.contextActive()) {
        return QStringLiteral("Waiting for an active greeter text field.");
    }
    return QString();
}

bool SddmInputMethodController::hardwareKeyboardPresent() const
{
    return m_hardwareKeyboardMonitor->hardwareKeyboardPresent();
}

bool SddmInputMethodController::capsLockActive() const
{
    return m_capsLockActive;
}

void SddmInputMethodController::hideKeyboard()
{
    if (!m_visible) {
        return;
    }
    m_visible = false;
    emit visibleChanged(m_visible);
}

void SddmInputMethodController::keyPressed(const QString &keyId, bool shift, bool ctrl, bool alt, bool meta)
{
    if (!inputBackendAvailable()) {
        return;
    }

    if (keyId == QStringLiteral("CapsLock")) {
        m_capsLockActive = !m_capsLockActive;
        emit capsLockActiveChanged(m_capsLockActive);
        return;
    }

    if (keyId == QStringLiteral("Backspace")) {
        m_inputMethod.deleteSurroundingText(1, 0);
        return;
    }
    if (keyId == QStringLiteral("Delete") || keyId == QStringLiteral("Del")) {
        m_inputMethod.deleteSurroundingText(0, 1);
        return;
    }
    if (keyId == QStringLiteral("Enter") || keyId == QStringLiteral("\n")) {
        m_inputMethod.sendKeysym(XkbKeyReturn);
        return;
    }
    if (keyId == QStringLiteral("Tab") || keyId == QStringLiteral("\t")) {
        m_inputMethod.sendKeysym(XkbKeyTab);
        return;
    }
    if (keyId == QStringLiteral("Esc") || keyId == QStringLiteral("Escape")) {
        m_inputMethod.sendKeysym(XkbKeyEscape);
        return;
    }

    if (ctrl || alt || meta) {
        sendNavigationKey(keyId);
        return;
    }

    const QString text = textForKey(keyId, shift);
    if (!text.isEmpty()) {
        m_inputMethod.commitText(text);
        return;
    }

    sendNavigationKey(keyId);
}

void SddmInputMethodController::setModifierActive(const QString &keyId, bool active)
{
    Q_UNUSED(keyId)
    Q_UNUSED(active)
}

QString SddmInputMethodController::textForKey(const QString &keyId, bool shift) const
{
    if (keyId == QStringLiteral("Space")) {
        return QStringLiteral(" ");
    }
    if (keyId.size() == 1 && keyId.at(0) >= QLatin1Char('a') && keyId.at(0) <= QLatin1Char('z')) {
        return shift != m_capsLockActive ? keyId.toUpper() : keyId;
    }
    if (shift && m_shiftedLabels.contains(keyId)) {
        return m_shiftedLabels.value(keyId);
    }
    return keyId.size() == 1 ? keyId : QString();
}

void SddmInputMethodController::sendNavigationKey(const QString &keyId)
{
    if (keyId == QStringLiteral("Left")) {
        m_inputMethod.sendKeysym(XkbKeyLeft);
    } else if (keyId == QStringLiteral("Right")) {
        m_inputMethod.sendKeysym(XkbKeyRight);
    } else if (keyId == QStringLiteral("Up")) {
        m_inputMethod.sendKeysym(XkbKeyUp);
    } else if (keyId == QStringLiteral("Down")) {
        m_inputMethod.sendKeysym(XkbKeyDown);
    } else if (keyId == QStringLiteral("Home")) {
        m_inputMethod.sendKeysym(XkbKeyHome);
    } else if (keyId == QStringLiteral("End")) {
        m_inputMethod.sendKeysym(XkbKeyEnd);
    } else if (keyId == QStringLiteral("PageUp") || keyId == QStringLiteral("PgUp")) {
        m_inputMethod.sendKeysym(XkbKeyPageUp);
    } else if (keyId == QStringLiteral("PageDown") || keyId == QStringLiteral("PgDn")) {
        m_inputMethod.sendKeysym(XkbKeyPageDown);
    } else if (keyId == QStringLiteral("Delete") || keyId == QStringLiteral("Del")) {
        m_inputMethod.sendKeysym(XkbKeyDelete);
    } else if (keyId == QStringLiteral("Backspace")) {
        m_inputMethod.sendKeysym(XkbKeyBackspace);
    }
}

void SddmInputMethodController::updateState()
{
    const bool nextVisible = m_inputMethod.contextActive() && !m_hardwareKeyboardMonitor->hardwareKeyboardPresent();
    if (m_visible != nextVisible) {
        m_visible = nextVisible;
        emit visibleChanged(m_visible);
    }
    emit inputBackendAvailableChanged(inputBackendAvailable());
    emit inputBackendErrorChanged(inputBackendError());
}
