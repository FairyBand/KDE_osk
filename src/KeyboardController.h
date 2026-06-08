#pragma once

#include <QObject>
#include <QString>

class HardwareKeyboardMonitor;

class KeyboardController : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KdeOsk.Keyboard")
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(bool autoShowEnabled READ autoShowEnabled WRITE setAutoShowEnabled NOTIFY autoShowEnabledChanged)

public:
    explicit KeyboardController(HardwareKeyboardMonitor *hardwareKeyboardMonitor, QObject *parent = nullptr);

    bool visible() const;
    bool autoShowEnabled() const;

    bool registerDBus();

public slots:
    Q_SCRIPTABLE void showKeyboard();
    Q_SCRIPTABLE void hideKeyboard();
    Q_SCRIPTABLE void setTextFocusActive(bool active);
    void setAutoShowEnabled(bool enabled);
    void keyPressed(const QString &text);

signals:
    void visibleChanged(bool visible);
    void autoShowEnabledChanged(bool enabled);
    void commitTextRequested(const QString &text);

private:
    void updateVisibilityFromPolicy();
    void setVisible(bool visible);

    HardwareKeyboardMonitor *m_hardwareKeyboardMonitor = nullptr;
    bool m_visible = false;
    bool m_autoShowEnabled = true;
    bool m_textFocusActive = false;
};
