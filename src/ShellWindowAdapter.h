#pragma once

#include <QObject>
#include <QString>

class ShellWindowAdapter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    explicit ShellWindowAdapter(QObject *parent = nullptr);

    static void prepareEnvironment();

    bool available() const;

    Q_INVOKABLE void configure(QObject *windowObject,
                               const QString &mode,
                               int floatingX,
                               int floatingY,
                               int width,
                               int height);
    Q_INVOKABLE void setInputRegion(QObject *windowObject,
                                    int x,
                                    int y,
                                    int width,
                                    int height,
                                    bool enabled);

private:
    bool m_available = false;
    bool m_warned = false;
};
