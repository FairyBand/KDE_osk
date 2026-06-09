#pragma once

#include "WaylandInputMethod.h"

#include <QObject>
#include <QHash>
#include <QString>

class HardwareKeyboardMonitor;

class SddmInputMethodController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(bool inputBackendAvailable READ inputBackendAvailable NOTIFY inputBackendAvailableChanged)
    Q_PROPERTY(QString inputBackendError READ inputBackendError NOTIFY inputBackendErrorChanged)
    Q_PROPERTY(bool hardwareKeyboardPresent READ hardwareKeyboardPresent NOTIFY hardwareKeyboardPresentChanged)
    Q_PROPERTY(bool autoShowEnabled READ autoShowEnabled WRITE setAutoShowEnabled NOTIFY autoShowEnabledChanged)
    Q_PROPERTY(bool ignoreHardwareKeyboard READ ignoreHardwareKeyboard WRITE setIgnoreHardwareKeyboard NOTIFY ignoreHardwareKeyboardChanged)
    Q_PROPERTY(bool capsLockActive READ capsLockActive NOTIFY capsLockActiveChanged)

public:
    explicit SddmInputMethodController(HardwareKeyboardMonitor *hardwareKeyboardMonitor, QObject *parent = nullptr);

    bool visible() const;
    bool inputBackendAvailable() const;
    QString inputBackendError() const;
    bool hardwareKeyboardPresent() const;
    bool autoShowEnabled() const;
    bool ignoreHardwareKeyboard() const;
    bool capsLockActive() const;

public slots:
    void hideKeyboard();
    void setAutoShowEnabled(bool enabled);
    void setIgnoreHardwareKeyboard(bool ignore);
    void keyPressed(const QString &keyId, bool shift, bool ctrl, bool alt, bool meta);
    void setModifierActive(const QString &keyId, bool active);

signals:
    void visibleChanged(bool visible);
    void inputBackendAvailableChanged(bool available);
    void inputBackendErrorChanged(const QString &error);
    void hardwareKeyboardPresentChanged(bool present);
    void autoShowEnabledChanged(bool enabled);
    void ignoreHardwareKeyboardChanged(bool ignore);
    void capsLockActiveChanged(bool active);

private:
    QString textForKey(const QString &keyId, bool shift) const;
    bool sendSpecialKey(const QString &keyId);
    void updateState();

    HardwareKeyboardMonitor *m_hardwareKeyboardMonitor = nullptr;
    WaylandInputMethod m_inputMethod;
    bool m_visible = false;
    bool m_autoShowEnabled = true;
    bool m_ignoreHardwareKeyboard = false;
    bool m_capsLockActive = false;
    const QHash<QString, QString> m_shiftedLabels;
};
