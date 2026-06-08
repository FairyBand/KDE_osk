#pragma once

#include <QObject>
#include <QRect>
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
    Q_PROPERTY(bool capsLockActive READ capsLockActive NOTIFY capsLockActiveChanged)
    Q_PROPERTY(bool textFocusActive READ textFocusActive NOTIFY textFocusActiveChanged)
    Q_PROPERTY(int focusRectX READ focusRectX NOTIFY focusRectChanged)
    Q_PROPERTY(int focusRectY READ focusRectY NOTIFY focusRectChanged)
    Q_PROPERTY(int focusRectWidth READ focusRectWidth NOTIFY focusRectChanged)
    Q_PROPERTY(int focusRectHeight READ focusRectHeight NOTIFY focusRectChanged)

public:
    explicit KeyboardController(HardwareKeyboardMonitor *hardwareKeyboardMonitor, UInputKeyboard *inputKeyboard, QObject *parent = nullptr);

    bool visible() const;
    bool autoShowEnabled() const;
    bool ignoreHardwareKeyboard() const;
    bool inputBackendAvailable() const;
    QString inputBackendError() const;
    bool capsLockActive() const;
    bool textFocusActive() const;
    int focusRectX() const;
    int focusRectY() const;
    int focusRectWidth() const;
    int focusRectHeight() const;

    bool registerDBus();

public slots:
    Q_SCRIPTABLE void showKeyboard();
    Q_SCRIPTABLE void forceShowKeyboard();
    Q_SCRIPTABLE void hideKeyboard();
    Q_SCRIPTABLE void setTextFocusActive(bool active);
    Q_SCRIPTABLE void setTextFocusRect(bool active, int x, int y, int width, int height);
    void setAutoShowEnabled(bool enabled);
    void setIgnoreHardwareKeyboard(bool ignore);
    void keyPressed(const QString &keyId, bool shift = false, bool ctrl = false, bool alt = false, bool meta = false);
    void setModifierActive(const QString &keyId, bool active);

signals:
    void visibleChanged(bool visible);
    void autoShowEnabledChanged(bool enabled);
    void ignoreHardwareKeyboardChanged(bool ignore);
    void inputBackendAvailableChanged(bool available);
    void inputBackendErrorChanged(const QString &error);
    void capsLockActiveChanged(bool active);
    void textFocusActiveChanged(bool active);
    void focusRectChanged();
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
    QRect m_focusRect;
};
