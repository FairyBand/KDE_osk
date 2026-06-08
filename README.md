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

Linux with Qt 6.4 or newer, libudev, KDE LayerShellQt, and the kernel uinput device is
required for the Wayland test path.

Typical dependencies:

```sh
# Arch Linux
sudo pacman -S cmake extra-cmake-modules qt6-base qt6-declarative qt6-quickcontrols2 qt6-wayland layer-shell-qt systemd

# Debian/Ubuntu family
sudo apt install cmake qt6-base-dev qt6-declarative-dev qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtquick-nativestyle qml6-module-qtqml-workerscript qt6-wayland libudev-dev liblayershellqtinterface-dev pkg-config
```

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/kde-osk-shell --force-show
```

`--force-show` is intended for development. It shows the keyboard immediately
and ignores external hardware keyboard suppression.

## Input Backend

The current development backend uses Linux `uinput` to create a virtual keyboard
device. This makes basic typing work before the fcitx5 bridge is implemented,
but the user running `kde-osk-shell` must be allowed to open `/dev/uinput`.

If the keyboard appears but does not type, check the terminal output and device
permissions:

```sh
ls -l /dev/uinput
```

For local development, a temporary test can be run with suitable permissions or
a distro-specific udev rule can grant a trusted input group access to
`/dev/uinput`.

This repository includes `data/70-kde-osk-uinput.rules` as a development helper.
After installing or copying it, reload udev rules and make sure your user is in
the chosen group:

```sh
sudo udevadm control --reload-rules
sudo udevadm trigger /dev/uinput
groups
```

The long-term desktop path is still the fcitx5 bridge, because it can commit
text through the active input context instead of relying on global virtual key
events.

On KDE Plasma Wayland, the shell uses LayerShellQt when available. Without it,
KWin treats the keyboard as a normal Wayland toplevel window: client-side `x/y`
placement can be ignored and the window may take focus.

LayerShellQt is required by default. For a fallback build that intentionally
keeps the old normal-window behavior:

```sh
cmake -S . -B build -DKDE_OSK_USE_LAYER_SHELL=OFF
```

Install:

```sh
cmake --install build
systemctl --user enable --now kde-osk-shell.service
```

## Current Status

This repository currently contains the project scaffold and the first Qt/QML
keyboard shell. It can show a typing keyboard or full keyboard, switch between
floating/top-docked/bottom-docked window modes, and send basic keys through
`uinput`. Desktop automatic show/hide, lock-screen integration, and SDDM
integration are intentionally documented as follow-up milestones because they
must be implemented through their respective trusted interfaces.
