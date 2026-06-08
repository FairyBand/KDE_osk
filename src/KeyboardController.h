#pragma once

#include <QObject>
#include <QString>

class HardwareKeyboardMonitor;
class UInputKeyboard;

class KeyboardController : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KdeOsk.Keyboard")
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(bool autoShowEnabled READ autoShowEnabled WRITE setAutoShowEnabled NOTIFY autoShowEnabledChanged)
    Q_PROPERTY(bool ignoreHardwareKeyboard READ ignoreHardwareKeyboard WRITE setIgnoreHardwareKeyboard NOTIFY ignoreHardwareKeyboardChanged)
    Q_PROPERTY(bool inputBackendAvailable READ inputBackendAvailable NOTIFY inputBackendAvailableChanged)
    Q_PROPERTY(QString inputBackendError READ inputBackendError NOTIFY inputBackendErrorChanged)

public:
    explicit KeyboardController(HardwareKeyboardMonitor *hardwareKeyboardMonitor, UInputKeyboard *inputKeyboard, QObject *parent = nullptr);

    bool visible() const;
    bool autoShowEnabled() const;
    bool ignoreHardwareKeyboard() const;
    bool inputBackendAvailable() const;
    QString inputBackendError() const;

    bool registerDBus();

public slots:
    Q_SCRIPTABLE void showKeyboard();
    Q_SCRIPTABLE void forceShowKeyboard();
    Q_SCRIPTABLE void hideKeyboard();
    Q_SCRIPTABLE void setTextFocusActive(bool active);
    void setAutoShowEnabled(bool enabled);
    void setIgnoreHardwareKeyboard(bool ignore);
    void keyPressed(const QString &keyId, bool shift = false, bool ctrl = false, bool alt = false, bool meta = false);

signals:
    void visibleChanged(bool visible);
    void autoShowEnabledChanged(bool enabled);
    void ignoreHardwareKeyboardChanged(bool ignore);
    void inputBackendAvailableChanged(bool available);
    void inputBackendErrorChanged(const QString &error);
    void commitTextRequested(const QString &text);

private:
    void updateVisibilityFromPolicy();
    bool shouldSuppressForHardwareKeyboard() const;
    void setVisible(bool visible);

    HardwareKeyboardMonitor *m_hardwareKeyboardMonitor = nullptr;
    UInputKeyboard *m_inputKeyboard = nullptr;
    bool m_visible = false;
    bool m_autoShowEnabled = true;
    bool m_ignoreHardwareKeyboard = false;
    bool m_textFocusActive = false;
};
