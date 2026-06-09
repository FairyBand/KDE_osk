#pragma once

#include "WaylandInputMethod.h"

#include <QObject>
#include <QHash>
#include <QString>
#include <QTimer>

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
    SddmInputMethodController(HardwareKeyboardMonitor *hardwareKeyboardMonitor, int autoShowDelayMs, QObject *parent = nullptr);
    SddmInputMethodController(HardwareKeyboardMonitor *hardwareKeyboardMonitor,
                              int autoShowDelayMs,
                              bool requireContextUpdateForAutoShow,
                              QObject *parent = nullptr);
    SddmInputMethodController(HardwareKeyboardMonitor *hardwareKeyboardMonitor,
                              int autoShowDelayMs,
                              bool requireContextUpdateForAutoShow,
                              bool ignoreInitialContextUpdatesForAutoShow,
                              QObject *parent = nullptr);

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
    void handleContextActiveChanged(bool active);
    void handleContextUpdated();
    void handleContextInvoked();
    void finishInitialContextUpdateSettling();
    void updateState();
    bool shouldShow() const;
    void setVisible(bool visible);

    HardwareKeyboardMonitor *m_hardwareKeyboardMonitor = nullptr;
    WaylandInputMethod m_inputMethod;
    QTimer m_autoShowTimer;
    QTimer m_initialContextUpdateSettleTimer;
    bool m_visible = false;
    bool m_autoShowEnabled = true;
    bool m_ignoreHardwareKeyboard = false;
    bool m_userHidden = false;
    bool m_requireContextUpdateForAutoShow = false;
    bool m_ignoreInitialContextUpdatesForAutoShow = false;
    bool m_waitingForInitialContextUpdatesToSettle = false;
    bool m_contextReadyForAutoShow = false;
    bool m_capsLockActive = false;
    int m_autoShowDelayMs = 0;
    const QHash<QString, QString> m_shiftedLabels;
};
