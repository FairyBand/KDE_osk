#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QVector>

class UInputKeyboard : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool capsLockActive READ capsLockActive NOTIFY capsLockActiveChanged)

public:
    struct KeyStroke {
        int code = -1;
        bool requiresShift = false;
    };

    explicit UInputKeyboard(QObject *parent = nullptr);
    ~UInputKeyboard() override;

    bool available() const;
    QString lastError() const;
    bool capsLockActive() const;

    bool sendKey(const QString &keyId, bool shift, bool ctrl, bool alt, bool meta);
    bool setModifierActive(const QString &keyId, bool active);

signals:
    void availableChanged(bool available);
    void lastErrorChanged(const QString &error);
    void capsLockActiveChanged(bool active);

private:
    bool initialize();
    void closeDevice();
    bool enableKey(int code);
    bool emitEvent(unsigned short type, unsigned short code, int value);
    bool sync();
    bool sendKeyPress(int code);
    bool sendCombo(const QVector<int> &modifiers, int code);
    void refreshCapsLockState();
    void setAvailable(bool available);
    void setLastError(const QString &error);
    void setCapsLockActive(bool active);

    static bool lookupKey(const QString &keyId, KeyStroke *stroke);
    static QVector<int> modifierCodes(bool shift, bool ctrl, bool alt, bool meta);

    int m_fd = -1;
    bool m_available = false;
    bool m_capsLockActive = false;
    QString m_lastError;
    QSet<int> m_activeModifiers;
    QTimer m_capsLockPollTimer;
};
