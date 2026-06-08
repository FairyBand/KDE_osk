#pragma once

#include <QObject>
#include <QString>
#include <QVector>

class UInputKeyboard : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    struct KeyStroke {
        int code = -1;
        bool requiresShift = false;
    };

    explicit UInputKeyboard(QObject *parent = nullptr);
    ~UInputKeyboard() override;

    bool available() const;
    QString lastError() const;

    bool sendKey(const QString &keyId, bool shift, bool ctrl, bool alt, bool meta);

signals:
    void availableChanged(bool available);
    void lastErrorChanged(const QString &error);

private:
    bool initialize();
    void closeDevice();
    bool enableKey(int code);
    bool emitEvent(unsigned short type, unsigned short code, int value);
    bool sendCombo(const QVector<int> &modifiers, int code);
    void setAvailable(bool available);
    void setLastError(const QString &error);

    static bool lookupKey(const QString &keyId, KeyStroke *stroke);
    static QVector<int> modifierCodes(bool shift, bool ctrl, bool alt, bool meta);

    int m_fd = -1;
    bool m_available = false;
    QString m_lastError;
};
