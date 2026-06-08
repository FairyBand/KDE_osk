# KDE OSK

KDE OSK is a Wayland-first on-screen keyboard project for KDE Plasma tablets.
The goal is to provide a polished touch keyboard that appears automatically
when no external hardware keyboard is available, while keeping fcitx5 available
for normal input method work.

## Goals

- Work well on KDE Plasma Wayland without taking over fcitx5's virtual keyboard
  setting.
- Show automatically when a text field gains focus and hide when focus leaves.
- Suppress automatic display while an external hardware keyboard is connected.
- Provide separate integration paths for the Plasma desktop, lock screen, and
  SDDM greeter.
- Keep candidate-window integration out of scope for the first milestone, while
  preserving fcitx5 compatibility.

## Architecture

Wayland clients cannot globally observe text focus or inject arbitrary key
events. For that reason, this project is split into trusted integration points:

- `kde-osk-shell`: Qt/QML keyboard surface and D-Bus control API.
- `kde-osk-device-monitor`: libudev-backed hardware keyboard presence detection.
- `fcitx5-kde-osk-bridge`: planned fcitx5 addon that observes fcitx5 input
  context focus and asks the shell to show or hide without replacing fcitx5.
- `kde-osk-lockscreen`: planned Plasma lock-screen integration.
- `kde-osk-sddm`: planned SDDM greeter integration.

The first implementation milestone builds the shell and device monitor, then
adds the fcitx5 bridge so desktop-session focus can drive visibility while
fcitx5 remains the active input method.

中文需求说明见 [docs/PRODUCT_SPEC.zh-CN.md](docs/PRODUCT_SPEC.zh-CN.md)。
See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the design details.
See [docs/DBUS.md](docs/DBUS.md) for the first shell control API.

## Build

Linux with Qt 6 and libudev is required.

Typical dependencies:

```sh
# Arch Linux
sudo pacman -S cmake extra-cmake-modules qt6-base qt6-declarative qt6-quickcontrols2 qt6-wayland systemd

# Debian/Ubuntu family
sudo apt install cmake qt6-base-dev qt6-declarative-dev qml6-module-qtquick-controls qt6-wayland libudev-dev pkg-config
```

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/kde-osk-shell
```

Install:

```sh
cmake --install build
systemctl --user enable --now kde-osk-shell.service
```

## Current Status

This repository currently contains the project scaffold and the first Qt/QML
keyboard shell. Desktop automatic show/hide, lock-screen integration, and SDDM
integration are intentionally documented as follow-up milestones because they
must be implemented through their respective trusted interfaces.
