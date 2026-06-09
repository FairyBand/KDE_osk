#include "SddmInputMethodController.h"

#include "HardwareKeyboardMonitor.h"

#include <QStringView>

namespace
{
constexpr uint XkbKeyBackspace = 0xff08;
constexpr uint XkbKeyTab = 0xff09;
constexpr uint XkbKeyReturn = 0xff0d;
constexpr uint XkbKeyEscape = 0xff1b;
constexpr uint XkbKeyHome = 0xff50;
constexpr uint XkbKeyInsert = 0xff63;
constexpr uint XkbKeyMenu = 0xff67;
constexpr uint XkbKeyLeft = 0xff51;
constexpr uint XkbKeyUp = 0xff52;
constexpr uint XkbKeyRight = 0xff53;
constexpr uint XkbKeyDown = 0xff54;
constexpr uint XkbKeyPageUp = 0xff55;
constexpr uint XkbKeyPageDown = 0xff56;
constexpr uint XkbKeyEnd = 0xff57;
constexpr uint XkbKeyF1 = 0xffbe;
constexpr uint XkbKeyDelete = 0xffff;
constexpr int InitialContextUpdateSettleMs = 250;
}

SddmInputMethodController::SddmInputMethodController(HardwareKeyboardMonitor *hardwareKeyboardMonitor, QObject *parent)
    : SddmInputMethodController(hardwareKeyboardMonitor, 0, false, parent)
{
}

SddmInputMethodController::SddmInputMethodController(HardwareKeyboardMonitor *hardwareKeyboardMonitor, int autoShowDelayMs, QObject *parent)
    : SddmInputMethodController(hardwareKeyboardMonitor, autoShowDelayMs, false, parent)
{
}

SddmInputMethodController::SddmInputMethodController(HardwareKeyboardMonitor *hardwareKeyboardMonitor,
                                                     int autoShowDelayMs,
                                                     bool requireContextUpdateForAutoShow,
                                                     QObject *parent)
    : SddmInputMethodController(hardwareKeyboardMonitor, autoShowDelayMs, requireContextUpdateForAutoShow, false, parent)
{
}

SddmInputMethodController::SddmInputMethodController(HardwareKeyboardMonitor *hardwareKeyboardMonitor,
                                                     int autoShowDelayMs,
                                                     bool requireContextUpdateForAutoShow,
                                                     bool ignoreInitialContextUpdatesForAutoShow,
                                                     QObject *parent)
    : QObject(parent)
    , m_hardwareKeyboardMonitor(hardwareKeyboardMonitor)
    , m_requireContextUpdateForAutoShow(requireContextUpdateForAutoShow)
    , m_ignoreInitialContextUpdatesForAutoShow(ignoreInitialContextUpdatesForAutoShow)
    , m_autoShowDelayMs(autoShowDelayMs)
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
    m_autoShowTimer.setSingleShot(true);
    connect(&m_autoShowTimer, &QTimer::timeout, this, [this]() {
        if (shouldShow()) {
            setVisible(true);
        }
    });
    m_initialContextUpdateSettleTimer.setSingleShot(true);
    connect(&m_initialContextUpdateSettleTimer, &QTimer::timeout, this, &SddmInputMethodController::finishInitialContextUpdateSettling);

    connect(&m_inputMethod, &WaylandInputMethod::contextActiveChanged, this, &SddmInputMethodController::handleContextActiveChanged);
    connect(&m_inputMethod, &WaylandInputMethod::contextUpdated, this, &SddmInputMethodController::handleContextUpdated);
    connect(&m_inputMethod, &WaylandInputMethod::contextInvoked, this, &SddmInputMethodController::handleContextInvoked);
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

bool SddmInputMethodController::autoShowEnabled() const
{
    return m_autoShowEnabled;
}

bool SddmInputMethodController::ignoreHardwareKeyboard() const
{
    return m_ignoreHardwareKeyboard;
}

bool SddmInputMethodController::capsLockActive() const
{
    return m_capsLockActive;
}

void SddmInputMethodController::hideKeyboard()
{
    m_userHidden = true;
    m_autoShowTimer.stop();
    setVisible(false);
}

void SddmInputMethodController::setAutoShowEnabled(bool enabled)
{
    if (m_autoShowEnabled == enabled) {
        return;
    }

    m_autoShowEnabled = enabled;
    emit autoShowEnabledChanged(m_autoShowEnabled);
    updateState();
}

void SddmInputMethodController::setIgnoreHardwareKeyboard(bool ignore)
{
    if (m_ignoreHardwareKeyboard == ignore) {
        return;
    }

    m_ignoreHardwareKeyboard = ignore;
    emit ignoreHardwareKeyboardChanged(m_ignoreHardwareKeyboard);
    updateState();
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
        sendSpecialKey(keyId);
        return;
    }

    const QString text = textForKey(keyId, shift);
    if (!text.isEmpty()) {
        m_inputMethod.commitText(text);
        return;
    }

    sendSpecialKey(keyId);
}

void SddmInputMethodController::setModifierActive(const QString &keyId, bool active)
{
    Q_UNUSED(keyId)
    Q_UNUSED(active)
}

void SddmInputMethodController::handleContextActiveChanged(bool active)
{
    m_userHidden = false;
    m_contextReadyForAutoShow = active && !m_requireContextUpdateForAutoShow;
    m_waitingForInitialContextUpdatesToSettle = active
        && m_requireContextUpdateForAutoShow
        && m_ignoreInitialContextUpdatesForAutoShow;
    if (m_waitingForInitialContextUpdatesToSettle) {
        m_initialContextUpdateSettleTimer.start(InitialContextUpdateSettleMs);
    } else {
        m_initialContextUpdateSettleTimer.stop();
    }
    if (!active) {
        m_autoShowTimer.stop();
    }
    updateState();
}

void SddmInputMethodController::handleContextUpdated()
{
    if (!m_inputMethod.contextActive()) {
        return;
    }

    if (m_waitingForInitialContextUpdatesToSettle) {
        m_initialContextUpdateSettleTimer.start(InitialContextUpdateSettleMs);
        return;
    }

    if (m_userHidden) {
        m_userHidden = false;
    }
    m_contextReadyForAutoShow = true;
    updateState();
}

void SddmInputMethodController::handleContextInvoked()
{
    if (!m_inputMethod.contextActive()) {
        return;
    }

    finishInitialContextUpdateSettling();
    if (m_userHidden) {
        m_userHidden = false;
    }
    m_contextReadyForAutoShow = true;
    updateState();
}

void SddmInputMethodController::finishInitialContextUpdateSettling()
{
    m_initialContextUpdateSettleTimer.stop();
    m_waitingForInitialContextUpdatesToSettle = false;
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

bool SddmInputMethodController::sendSpecialKey(const QString &keyId)
{
    if (keyId == QStringLiteral("Left")) {
        m_inputMethod.sendKeysym(XkbKeyLeft);
        return true;
    } else if (keyId == QStringLiteral("Right")) {
        m_inputMethod.sendKeysym(XkbKeyRight);
        return true;
    } else if (keyId == QStringLiteral("Up")) {
        m_inputMethod.sendKeysym(XkbKeyUp);
        return true;
    } else if (keyId == QStringLiteral("Down")) {
        m_inputMethod.sendKeysym(XkbKeyDown);
        return true;
    } else if (keyId == QStringLiteral("Home")) {
        m_inputMethod.sendKeysym(XkbKeyHome);
        return true;
    } else if (keyId == QStringLiteral("End")) {
        m_inputMethod.sendKeysym(XkbKeyEnd);
        return true;
    } else if (keyId == QStringLiteral("Insert") || keyId == QStringLiteral("Ins")) {
        m_inputMethod.sendKeysym(XkbKeyInsert);
        return true;
    } else if (keyId == QStringLiteral("Menu")) {
        m_inputMethod.sendKeysym(XkbKeyMenu);
        return true;
    } else if (keyId == QStringLiteral("PageUp") || keyId == QStringLiteral("PgUp")) {
        m_inputMethod.sendKeysym(XkbKeyPageUp);
        return true;
    } else if (keyId == QStringLiteral("PageDown") || keyId == QStringLiteral("PgDn")) {
        m_inputMethod.sendKeysym(XkbKeyPageDown);
        return true;
    } else if (keyId == QStringLiteral("Delete") || keyId == QStringLiteral("Del")) {
        m_inputMethod.sendKeysym(XkbKeyDelete);
        return true;
    } else if (keyId == QStringLiteral("Backspace")) {
        m_inputMethod.sendKeysym(XkbKeyBackspace);
        return true;
    } else if (keyId.startsWith(QLatin1Char('F'))) {
        bool ok = false;
        const int functionKey = QStringView(keyId).mid(1).toInt(&ok);
        if (ok && functionKey >= 1 && functionKey <= 12) {
            m_inputMethod.sendKeysym(XkbKeyF1 + static_cast<uint>(functionKey - 1));
            return true;
        }
    }

    return false;
}

void SddmInputMethodController::updateState()
{
    const bool nextVisible = shouldShow();
    if (!nextVisible) {
        m_autoShowTimer.stop();
        setVisible(false);
    } else if (!m_visible && !m_autoShowTimer.isActive()) {
        if (m_autoShowDelayMs > 0) {
            m_autoShowTimer.start(m_autoShowDelayMs);
        } else {
            setVisible(true);
        }
    }
    emit inputBackendAvailableChanged(inputBackendAvailable());
    emit inputBackendErrorChanged(inputBackendError());
}

bool SddmInputMethodController::shouldShow() const
{
    return m_autoShowEnabled
        && m_inputMethod.contextActive()
        && m_contextReadyForAutoShow
        && !m_userHidden
        && (m_ignoreHardwareKeyboard || !m_hardwareKeyboardMonitor->hardwareKeyboardPresent());
}

void SddmInputMethodController::setVisible(bool visible)
{
    if (m_visible == visible) {
        return;
    }

    m_visible = visible;
    emit visibleChanged(m_visible);
}
