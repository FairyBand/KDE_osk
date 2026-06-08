#include "ShellSettings.h"

#include <QVariant>

namespace
{
constexpr auto WindowModeKey = "ui/windowMode";
constexpr auto KeyboardModeKey = "ui/keyboardMode";
constexpr auto FloatXKey = "ui/floatX";
constexpr auto FloatYKey = "ui/floatY";

QString settingKey(const char *key)
{
    return QString::fromLatin1(key);
}
}

ShellSettings::ShellSettings(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("kde-osk"), QStringLiteral("kde-osk-shell"))
    , m_windowMode(m_settings.value(settingKey(WindowModeKey), QStringLiteral("dockBottom")).toString())
    , m_keyboardMode(m_settings.value(settingKey(KeyboardModeKey), QStringLiteral("typing")).toString())
    , m_floatX(m_settings.value(settingKey(FloatXKey), -1).toInt())
    , m_floatY(m_settings.value(settingKey(FloatYKey), -1).toInt())
{
    if (!isValidWindowMode(m_windowMode)) {
        m_windowMode = QStringLiteral("dockBottom");
    }
    if (!isValidKeyboardMode(m_keyboardMode)) {
        m_keyboardMode = QStringLiteral("typing");
    }
}

QString ShellSettings::windowMode() const
{
    return m_windowMode;
}

QString ShellSettings::keyboardMode() const
{
    return m_keyboardMode;
}

int ShellSettings::floatX() const
{
    return m_floatX;
}

int ShellSettings::floatY() const
{
    return m_floatY;
}

void ShellSettings::setWindowMode(const QString &mode)
{
    if (!isValidWindowMode(mode) || m_windowMode == mode) {
        return;
    }

    m_windowMode = mode;
    syncValue(settingKey(WindowModeKey), m_windowMode);
    emit windowModeChanged(m_windowMode);
}

void ShellSettings::setKeyboardMode(const QString &mode)
{
    if (!isValidKeyboardMode(mode) || m_keyboardMode == mode) {
        return;
    }

    m_keyboardMode = mode;
    syncValue(settingKey(KeyboardModeKey), m_keyboardMode);
    emit keyboardModeChanged(m_keyboardMode);
}

void ShellSettings::setFloatX(int x)
{
    if (m_floatX == x) {
        return;
    }

    m_floatX = x;
    syncValue(settingKey(FloatXKey), m_floatX);
    emit floatXChanged(m_floatX);
}

void ShellSettings::setFloatY(int y)
{
    if (m_floatY == y) {
        return;
    }

    m_floatY = y;
    syncValue(settingKey(FloatYKey), m_floatY);
    emit floatYChanged(m_floatY);
}

bool ShellSettings::isValidWindowMode(const QString &mode)
{
    return mode == QStringLiteral("dockBottom")
        || mode == QStringLiteral("dockTop")
        || mode == QStringLiteral("float");
}

bool ShellSettings::isValidKeyboardMode(const QString &mode)
{
    return mode == QStringLiteral("typing")
        || mode == QStringLiteral("full");
}

void ShellSettings::syncValue(const QString &key, const QVariant &value)
{
    m_settings.setValue(key, value);
    m_settings.sync();
}
