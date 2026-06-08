#include "HardwareKeyboardMonitor.h"

#include <QDebug>

#include <chrono>
#include <cstring>

#include <libudev.h>

namespace
{
bool propertyEquals(udev_device *device, const char *name, const char *expected)
{
    const char *value = udev_device_get_property_value(device, name);
    return value != nullptr && std::strcmp(value, expected) == 0;
}

bool propertyContains(udev_device *device, const char *name, const char *needle)
{
    const char *value = udev_device_get_property_value(device, name);
    return value != nullptr && std::strstr(value, needle) != nullptr;
}

bool hasParentSubsystem(udev_device *device, const char *subsystem)
{
    udev_device *parent = udev_device_get_parent(device);
    while (parent != nullptr) {
        const char *candidate = udev_device_get_subsystem(parent);
        if (candidate != nullptr && std::strcmp(candidate, subsystem) == 0) {
            return true;
        }
        parent = udev_device_get_parent(parent);
    }
    return false;
}

bool hasParentDriver(udev_device *device, const char *driver)
{
    udev_device *parent = udev_device_get_parent(device);
    while (parent != nullptr) {
        const char *candidate = udev_device_get_driver(parent);
        if (candidate != nullptr && std::strcmp(candidate, driver) == 0) {
            return true;
        }
        parent = udev_device_get_parent(parent);
    }
    return false;
}
}

HardwareKeyboardMonitor::HardwareKeyboardMonitor(QObject *parent)
    : QObject(parent)
    , m_udev(udev_new())
{
    if (m_udev == nullptr) {
        qWarning() << "Failed to initialize udev; assuming no hardware keyboard.";
        return;
    }

    m_monitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (m_monitor != nullptr) {
        udev_monitor_filter_add_match_subsystem_devtype(m_monitor, "input", nullptr);
        if (udev_monitor_enable_receiving(m_monitor) == 0) {
            m_notifier = new QSocketNotifier(udev_monitor_get_fd(m_monitor), QSocketNotifier::Read, this);
            connect(m_notifier, &QSocketNotifier::activated, this, &HardwareKeyboardMonitor::handleUdevEvent);
        } else {
            qWarning() << "Failed to enable udev monitor.";
        }
    }

    m_refreshTimer.setInterval(std::chrono::seconds(5));
    connect(&m_refreshTimer, &QTimer::timeout, this, &HardwareKeyboardMonitor::refresh);
    m_refreshTimer.start();

    refresh();
}

HardwareKeyboardMonitor::~HardwareKeyboardMonitor()
{
    if (m_monitor != nullptr) {
        udev_monitor_unref(m_monitor);
    }
    if (m_udev != nullptr) {
        udev_unref(m_udev);
    }
}

bool HardwareKeyboardMonitor::hardwareKeyboardPresent() const
{
    return m_hardwareKeyboardPresent;
}

void HardwareKeyboardMonitor::refresh()
{
    setHardwareKeyboardPresent(detectHardwareKeyboard());
}

void HardwareKeyboardMonitor::handleUdevEvent()
{
    if (m_monitor == nullptr) {
        return;
    }

    udev_device *device = udev_monitor_receive_device(m_monitor);
    if (device != nullptr) {
        udev_device_unref(device);
    }
    refresh();
}

bool HardwareKeyboardMonitor::detectHardwareKeyboard() const
{
    if (m_udev == nullptr) {
        return false;
    }

    udev_enumerate *enumerate = udev_enumerate_new(m_udev);
    if (enumerate == nullptr) {
        return false;
    }

    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);

    bool found = false;
    udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry *entry = nullptr;
    udev_list_entry_foreach(entry, devices) {
        const char *syspath = udev_list_entry_get_name(entry);
        if (isExternalKeyboardDevice(m_udev, syspath)) {
            found = true;
            break;
        }
    }

    udev_enumerate_unref(enumerate);
    return found;
}

bool HardwareKeyboardMonitor::isExternalKeyboardDevice(udev *udevContext, const char *syspath)
{
    udev_device *device = udev_device_new_from_syspath(udevContext, syspath);
    if (device == nullptr) {
        return false;
    }

    const bool isKeyboard = propertyEquals(device, "ID_INPUT_KEYBOARD", "1");
    const bool isVirtual = propertyEquals(device, "ID_BUS", "virtual")
        || propertyContains(device, "NAME", "KDE OSK virtual keyboard")
        || propertyEquals(device, "ID_INPUT_JOYSTICK", "1")
        || hasParentSubsystem(device, "virtual")
        || hasParentDriver(device, "uinput");
    const bool isUsb = hasParentSubsystem(device, "usb");
    const bool isBluetooth = hasParentSubsystem(device, "bluetooth");
    const bool isDockLike = propertyEquals(device, "ID_BUS", "usb")
        || propertyEquals(device, "ID_BUS", "bluetooth");

    udev_device_unref(device);

    return isKeyboard && !isVirtual && (isUsb || isBluetooth || isDockLike);
}

void HardwareKeyboardMonitor::setHardwareKeyboardPresent(bool present)
{
    if (m_hardwareKeyboardPresent == present) {
        return;
    }

    m_hardwareKeyboardPresent = present;
    emit hardwareKeyboardPresentChanged(m_hardwareKeyboardPresent);
}
