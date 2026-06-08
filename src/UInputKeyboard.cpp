#include "UInputKeyboard.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QHash>
#include <QList>
#include <QPair>
#include <QSet>

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <utility>

namespace
{
constexpr const char *DevicePath = "/dev/uinput";
constexpr int CapsLockPollIntervalMs = 500;

QString systemError(const QString &prefix)
{
    return QStringLiteral("%1: %2").arg(prefix, QString::fromLocal8Bit(std::strerror(errno)));
}

bool readCapsLockLedState(bool *active)
{
    QDir ledDir(QStringLiteral("/sys/class/leds"));
    const QFileInfoList entries = ledDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    bool sawCapsLockLed = false;
    bool anyActive = false;

    for (const QFileInfo &entry : entries) {
        if (!entry.fileName().contains(QStringLiteral("capslock"), Qt::CaseInsensitive)) {
            continue;
        }

        QFile brightness(entry.absoluteFilePath() + QStringLiteral("/brightness"));
        if (!brightness.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        sawCapsLockLed = true;
        bool ok = false;
        const int value = QString::fromLatin1(brightness.readAll()).trimmed().toInt(&ok);
        if (ok && value > 0) {
            anyActive = true;
        }
    }

    if (!sawCapsLockLed) {
        return false;
    }

    *active = anyActive;
    return true;
}

QHash<QString, UInputKeyboard::KeyStroke> createKeyMap()
{
    QHash<QString, UInputKeyboard::KeyStroke> map;

    const QList<QPair<QString, int>> letters = {
        {QStringLiteral("a"), KEY_A},
        {QStringLiteral("b"), KEY_B},
        {QStringLiteral("c"), KEY_C},
        {QStringLiteral("d"), KEY_D},
        {QStringLiteral("e"), KEY_E},
        {QStringLiteral("f"), KEY_F},
        {QStringLiteral("g"), KEY_G},
        {QStringLiteral("h"), KEY_H},
        {QStringLiteral("i"), KEY_I},
        {QStringLiteral("j"), KEY_J},
        {QStringLiteral("k"), KEY_K},
        {QStringLiteral("l"), KEY_L},
        {QStringLiteral("m"), KEY_M},
        {QStringLiteral("n"), KEY_N},
        {QStringLiteral("o"), KEY_O},
        {QStringLiteral("p"), KEY_P},
        {QStringLiteral("q"), KEY_Q},
        {QStringLiteral("r"), KEY_R},
        {QStringLiteral("s"), KEY_S},
        {QStringLiteral("t"), KEY_T},
        {QStringLiteral("u"), KEY_U},
        {QStringLiteral("v"), KEY_V},
        {QStringLiteral("w"), KEY_W},
        {QStringLiteral("x"), KEY_X},
        {QStringLiteral("y"), KEY_Y},
        {QStringLiteral("z"), KEY_Z},
    };
    for (const auto &letter : letters) {
        map.insert(letter.first, {letter.second, false});
        map.insert(letter.first.toUpper(), {letter.second, true});
    }

    const QList<QPair<QString, int>> digits = {
        {QStringLiteral("1"), KEY_1},
        {QStringLiteral("2"), KEY_2},
        {QStringLiteral("3"), KEY_3},
        {QStringLiteral("4"), KEY_4},
        {QStringLiteral("5"), KEY_5},
        {QStringLiteral("6"), KEY_6},
        {QStringLiteral("7"), KEY_7},
        {QStringLiteral("8"), KEY_8},
        {QStringLiteral("9"), KEY_9},
        {QStringLiteral("0"), KEY_0},
    };
    for (const auto &digit : digits) {
        map.insert(digit.first, {digit.second, false});
    }

    const QList<QPair<QString, int>> shiftedDigits = {
        {QStringLiteral("!"), KEY_1},
        {QStringLiteral("@"), KEY_2},
        {QStringLiteral("#"), KEY_3},
        {QStringLiteral("$"), KEY_4},
        {QStringLiteral("%"), KEY_5},
        {QStringLiteral("^"), KEY_6},
        {QStringLiteral("&"), KEY_7},
        {QStringLiteral("*"), KEY_8},
        {QStringLiteral("("), KEY_9},
        {QStringLiteral(")"), KEY_0},
    };
    for (const auto &digit : shiftedDigits) {
        map.insert(digit.first, {digit.second, true});
    }

    map.insert(QStringLiteral(" "), {KEY_SPACE, false});
    map.insert(QStringLiteral("Space"), {KEY_SPACE, false});
    map.insert(QStringLiteral("\n"), {KEY_ENTER, false});
    map.insert(QStringLiteral("Enter"), {KEY_ENTER, false});
    map.insert(QStringLiteral("\b"), {KEY_BACKSPACE, false});
    map.insert(QStringLiteral("Backspace"), {KEY_BACKSPACE, false});
    map.insert(QStringLiteral("\t"), {KEY_TAB, false});
    map.insert(QStringLiteral("Tab"), {KEY_TAB, false});

    map.insert(QStringLiteral("-"), {KEY_MINUS, false});
    map.insert(QStringLiteral("_"), {KEY_MINUS, true});
    map.insert(QStringLiteral("="), {KEY_EQUAL, false});
    map.insert(QStringLiteral("+"), {KEY_EQUAL, true});
    map.insert(QStringLiteral("["), {KEY_LEFTBRACE, false});
    map.insert(QStringLiteral("{"), {KEY_LEFTBRACE, true});
    map.insert(QStringLiteral("]"), {KEY_RIGHTBRACE, false});
    map.insert(QStringLiteral("}"), {KEY_RIGHTBRACE, true});
    map.insert(QStringLiteral("\\"), {KEY_BACKSLASH, false});
    map.insert(QStringLiteral("|"), {KEY_BACKSLASH, true});
    map.insert(QStringLiteral(";"), {KEY_SEMICOLON, false});
    map.insert(QStringLiteral(":"), {KEY_SEMICOLON, true});
    map.insert(QStringLiteral("'"), {KEY_APOSTROPHE, false});
    map.insert(QStringLiteral("\""), {KEY_APOSTROPHE, true});
    map.insert(QStringLiteral("`"), {KEY_GRAVE, false});
    map.insert(QStringLiteral("~"), {KEY_GRAVE, true});
    map.insert(QStringLiteral(","), {KEY_COMMA, false});
    map.insert(QStringLiteral("<"), {KEY_COMMA, true});
    map.insert(QStringLiteral("."), {KEY_DOT, false});
    map.insert(QStringLiteral(">"), {KEY_DOT, true});
    map.insert(QStringLiteral("/"), {KEY_SLASH, false});
    map.insert(QStringLiteral("?"), {KEY_SLASH, true});

    map.insert(QStringLiteral("Esc"), {KEY_ESC, false});
    map.insert(QStringLiteral("Escape"), {KEY_ESC, false});
    map.insert(QStringLiteral("Delete"), {KEY_DELETE, false});
    map.insert(QStringLiteral("Del"), {KEY_DELETE, false});
    map.insert(QStringLiteral("Insert"), {KEY_INSERT, false});
    map.insert(QStringLiteral("Ins"), {KEY_INSERT, false});
    map.insert(QStringLiteral("Home"), {KEY_HOME, false});
    map.insert(QStringLiteral("End"), {KEY_END, false});
    map.insert(QStringLiteral("PageUp"), {KEY_PAGEUP, false});
    map.insert(QStringLiteral("PgUp"), {KEY_PAGEUP, false});
    map.insert(QStringLiteral("PageDown"), {KEY_PAGEDOWN, false});
    map.insert(QStringLiteral("PgDn"), {KEY_PAGEDOWN, false});
    map.insert(QStringLiteral("Up"), {KEY_UP, false});
    map.insert(QStringLiteral("Down"), {KEY_DOWN, false});
    map.insert(QStringLiteral("Left"), {KEY_LEFT, false});
    map.insert(QStringLiteral("Right"), {KEY_RIGHT, false});
    map.insert(QStringLiteral("CapsLock"), {KEY_CAPSLOCK, false});
    map.insert(QStringLiteral("Menu"), {KEY_COMPOSE, false});
    map.insert(QStringLiteral("PrintScreen"), {KEY_SYSRQ, false});
    map.insert(QStringLiteral("Pause"), {KEY_PAUSE, false});
    map.insert(QStringLiteral("Shift"), {KEY_LEFTSHIFT, false});
    map.insert(QStringLiteral("Ctrl"), {KEY_LEFTCTRL, false});
    map.insert(QStringLiteral("Alt"), {KEY_LEFTALT, false});
    map.insert(QStringLiteral("Meta"), {KEY_LEFTMETA, false});

    for (int i = 1; i <= 12; ++i) {
        map.insert(QStringLiteral("F%1").arg(i), {KEY_F1 + i - 1, false});
    }

    return map;
}
}

UInputKeyboard::UInputKeyboard(QObject *parent)
    : QObject(parent)
{
    initialize();
    connect(&m_capsLockPollTimer, &QTimer::timeout, this, &UInputKeyboard::refreshCapsLockState);
    m_capsLockPollTimer.setInterval(CapsLockPollIntervalMs);
    m_capsLockPollTimer.start();
    refreshCapsLockState();
}

UInputKeyboard::~UInputKeyboard()
{
    closeDevice();
}

bool UInputKeyboard::available() const
{
    return m_available;
}

QString UInputKeyboard::lastError() const
{
    return m_lastError;
}

bool UInputKeyboard::capsLockActive() const
{
    return m_capsLockActive;
}

bool UInputKeyboard::sendKey(const QString &keyId, bool shift, bool ctrl, bool alt, bool meta)
{
    if (!m_available) {
        qWarning() << "Cannot send key; uinput backend is unavailable:" << m_lastError;
        return false;
    }

    KeyStroke stroke;
    if (!lookupKey(keyId, &stroke)) {
        qWarning() << "Unsupported key:" << keyId;
        return false;
    }

    const bool sent = sendCombo(modifierCodes(shift || stroke.requiresShift, ctrl, alt, meta), stroke.code);
    if (sent && keyId == QStringLiteral("CapsLock")) {
        QTimer::singleShot(60, this, &UInputKeyboard::refreshCapsLockState);
    }
    return sent;
}

bool UInputKeyboard::setModifierActive(const QString &keyId, bool active)
{
    if (!m_available) {
        qWarning() << "Cannot set modifier; uinput backend is unavailable:" << m_lastError;
        return false;
    }

    KeyStroke stroke;
    if (!lookupKey(keyId, &stroke)) {
        qWarning() << "Unsupported modifier:" << keyId;
        return false;
    }

    const bool alreadyActive = m_activeModifiers.contains(stroke.code);
    if (alreadyActive == active) {
        return true;
    }

    if (!emitEvent(EV_KEY, stroke.code, active ? 1 : 0) || !sync()) {
        return false;
    }

    if (active) {
        m_activeModifiers.insert(stroke.code);
    } else {
        m_activeModifiers.remove(stroke.code);
    }
    return true;
}

bool UInputKeyboard::initialize()
{
    m_fd = ::open(DevicePath, O_RDWR | O_NONBLOCK);
    if (m_fd < 0) {
        setLastError(systemError(QStringLiteral("Cannot open /dev/uinput")));
        qWarning() << m_lastError;
        return false;
    }

    if (::ioctl(m_fd, UI_SET_EVBIT, EV_KEY) < 0 || ::ioctl(m_fd, UI_SET_EVBIT, EV_SYN) < 0) {
        setLastError(systemError(QStringLiteral("Cannot configure uinput event types")));
        qWarning() << m_lastError;
        closeDevice();
        return false;
    }

    if (::ioctl(m_fd, UI_SET_EVBIT, EV_LED) < 0 || ::ioctl(m_fd, UI_SET_LEDBIT, LED_CAPSL) < 0) {
        qWarning() << systemError(QStringLiteral("Cannot configure CapsLock LED feedback"));
    }

    const QList<int> keys = {
        KEY_ESC,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,
        KEY_0,
        KEY_MINUS,
        KEY_EQUAL,
        KEY_BACKSPACE,
        KEY_TAB,
        KEY_Q,
        KEY_W,
        KEY_E,
        KEY_R,
        KEY_T,
        KEY_Y,
        KEY_U,
        KEY_I,
        KEY_O,
        KEY_P,
        KEY_LEFTBRACE,
        KEY_RIGHTBRACE,
        KEY_ENTER,
        KEY_LEFTCTRL,
        KEY_A,
        KEY_S,
        KEY_D,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_SEMICOLON,
        KEY_APOSTROPHE,
        KEY_GRAVE,
        KEY_LEFTSHIFT,
        KEY_BACKSLASH,
        KEY_Z,
        KEY_X,
        KEY_C,
        KEY_V,
        KEY_B,
        KEY_N,
        KEY_M,
        KEY_COMMA,
        KEY_DOT,
        KEY_SLASH,
        KEY_RIGHTSHIFT,
        KEY_LEFTALT,
        KEY_SPACE,
        KEY_CAPSLOCK,
        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,
        KEY_SYSRQ,
        KEY_PAUSE,
        KEY_INSERT,
        KEY_DELETE,
        KEY_HOME,
        KEY_END,
        KEY_PAGEUP,
        KEY_PAGEDOWN,
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_LEFTMETA,
        KEY_RIGHTMETA,
        KEY_COMPOSE,
    };

    for (int key : keys) {
        if (!enableKey(key)) {
            closeDevice();
            return false;
        }
    }

    uinput_user_dev device {};
    std::strncpy(device.name, "KDE OSK virtual keyboard", UINPUT_MAX_NAME_SIZE - 1);
    device.id.bustype = BUS_USB;
    device.id.vendor = 0x4b44;
    device.id.product = 0x4f53;
    device.id.version = 1;

    if (::write(m_fd, &device, sizeof(device)) != sizeof(device)) {
        setLastError(systemError(QStringLiteral("Cannot write uinput device description")));
        qWarning() << m_lastError;
        closeDevice();
        return false;
    }

    if (::ioctl(m_fd, UI_DEV_CREATE) < 0) {
        setLastError(systemError(QStringLiteral("Cannot create uinput device")));
        qWarning() << m_lastError;
        closeDevice();
        return false;
    }

    setAvailable(true);
    setLastError(QString());
    qInfo() << "uinput backend ready:" << DevicePath;
    return true;
}

void UInputKeyboard::closeDevice()
{
    if (m_fd < 0) {
        return;
    }

    for (int modifier : std::as_const(m_activeModifiers)) {
        emitEvent(EV_KEY, modifier, 0);
    }
    if (!m_activeModifiers.isEmpty()) {
        sync();
        m_activeModifiers.clear();
    }

    if (m_available) {
        ::ioctl(m_fd, UI_DEV_DESTROY);
    }
    ::close(m_fd);
    m_fd = -1;
    setAvailable(false);
}

bool UInputKeyboard::enableKey(int code)
{
    if (::ioctl(m_fd, UI_SET_KEYBIT, code) < 0) {
        setLastError(systemError(QStringLiteral("Cannot enable uinput key %1").arg(code)));
        qWarning() << m_lastError;
        return false;
    }
    return true;
}

bool UInputKeyboard::emitEvent(unsigned short type, unsigned short code, int value)
{
    input_event event {};
    event.type = type;
    event.code = code;
    event.value = value;

    if (::write(m_fd, &event, sizeof(event)) != sizeof(event)) {
        setLastError(systemError(QStringLiteral("Cannot write uinput event")));
        qWarning() << m_lastError;
        return false;
    }
    return true;
}

bool UInputKeyboard::sync()
{
    return emitEvent(EV_SYN, SYN_REPORT, 0);
}

bool UInputKeyboard::sendKeyPress(int code)
{
    if (!emitEvent(EV_KEY, code, 1) || !sync()) {
        return false;
    }

    if (!emitEvent(EV_KEY, code, 0) || !sync()) {
        return false;
    }

    return true;
}

bool UInputKeyboard::sendCombo(const QVector<int> &modifiers, int code)
{
    QVector<int> temporaryModifiers;
    const auto releaseTemporaryModifiers = [this, &temporaryModifiers]() {
        bool released = true;
        for (auto it = temporaryModifiers.crbegin(); it != temporaryModifiers.crend(); ++it) {
            released = emitEvent(EV_KEY, *it, 0) && sync() && released;
        }
        temporaryModifiers.clear();
        return released;
    };

    for (int modifier : modifiers) {
        if (m_activeModifiers.contains(modifier)) {
            continue;
        }
        if (!emitEvent(EV_KEY, modifier, 1) || !sync()) {
            return false;
        }
        temporaryModifiers.append(modifier);
    }

    if (!sendKeyPress(code)) {
        releaseTemporaryModifiers();
        return false;
    }

    return releaseTemporaryModifiers();
}

void UInputKeyboard::refreshCapsLockState()
{
    bool active = false;
    if (!readCapsLockLedState(&active)) {
        active = false;
    }
    setCapsLockActive(active);
}

void UInputKeyboard::setAvailable(bool available)
{
    if (m_available == available) {
        return;
    }
    m_available = available;
    emit availableChanged(m_available);
}

void UInputKeyboard::setLastError(const QString &error)
{
    if (m_lastError == error) {
        return;
    }
    m_lastError = error;
    emit lastErrorChanged(m_lastError);
}

void UInputKeyboard::setCapsLockActive(bool active)
{
    if (m_capsLockActive == active) {
        return;
    }
    m_capsLockActive = active;
    emit capsLockActiveChanged(m_capsLockActive);
}

bool UInputKeyboard::lookupKey(const QString &keyId, KeyStroke *stroke)
{
    static const QHash<QString, KeyStroke> keyMap = createKeyMap();
    const auto it = keyMap.constFind(keyId);
    if (it == keyMap.constEnd()) {
        return false;
    }

    *stroke = it.value();
    return true;
}

QVector<int> UInputKeyboard::modifierCodes(bool shift, bool ctrl, bool alt, bool meta)
{
    QVector<int> modifiers;
    if (ctrl) {
        modifiers.append(KEY_LEFTCTRL);
    }
    if (alt) {
        modifiers.append(KEY_LEFTALT);
    }
    if (meta) {
        modifiers.append(KEY_LEFTMETA);
    }
    if (shift) {
        modifiers.append(KEY_LEFTSHIFT);
    }
    return modifiers;
}
