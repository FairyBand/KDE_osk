#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <QTimer>

struct udev;
struct udev_monitor;

class HardwareKeyboardMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hardwareKeyboardPresent READ hardwareKeyboardPresent NOTIFY hardwareKeyboardPresentChanged)

public:
    explicit HardwareKeyboardMonitor(QObject *parent = nullptr);
    ~HardwareKeyboardMonitor() override;

    bool hardwareKeyboardPresent() const;

signals:
    void hardwareKeyboardPresentChanged(bool present);

private slots:
    void refresh();
    void handleUdevEvent();

private:
    bool detectHardwareKeyboard() const;
    static bool isExternalKeyboardDevice(udev *udevContext, const char *syspath);
    void setHardwareKeyboardPresent(bool present);

    udev *m_udev = nullptr;
    udev_monitor *m_monitor = nullptr;
    QSocketNotifier *m_notifier = nullptr;
    QTimer m_refreshTimer;
    bool m_hardwareKeyboardPresent = false;
};
