#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

class ShellSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString windowMode READ windowMode WRITE setWindowMode NOTIFY windowModeChanged)
    Q_PROPERTY(QString keyboardMode READ keyboardMode WRITE setKeyboardMode NOTIFY keyboardModeChanged)
    Q_PROPERTY(int floatX READ floatX WRITE setFloatX NOTIFY floatXChanged)
    Q_PROPERTY(int floatY READ floatY WRITE setFloatY NOTIFY floatYChanged)

public:
    explicit ShellSettings(QObject *parent = nullptr);

    QString windowMode() const;
    QString keyboardMode() const;
    int floatX() const;
    int floatY() const;

public slots:
    void setWindowMode(const QString &mode);
    void setKeyboardMode(const QString &mode);
    void setFloatX(int x);
    void setFloatY(int y);

signals:
    void windowModeChanged(const QString &mode);
    void keyboardModeChanged(const QString &mode);
    void floatXChanged(int x);
    void floatYChanged(int y);

private:
    static bool isValidWindowMode(const QString &mode);
    static bool isValidKeyboardMode(const QString &mode);
    void syncValue(const QString &key, const QVariant &value);

    QSettings m_settings;
    QString m_windowMode;
    QString m_keyboardMode;
    int m_floatX = -1;
    int m_floatY = -1;
};
