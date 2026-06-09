# KDE OSK

KDE OSK is a Wayland-first on-screen keyboard project for KDE Plasma tablets.
The goal is to provide a polished touch keyboard that appears automatically
when no external hardware keyboard is available, while keeping fcitx5 available
for normal input method work.

## Goals

- Work well on KDE Plasma Wayland while preserving the stock fcitx5 desktop
  input method experience.
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
- `kde-osk-kwin-broker`: long-term Plasma virtual-keyboard backend that appears
  in KDE settings, delegates normal desktop input to stock fcitx5, and provides
  the trusted KWin input-panel route for KDE OSK.
- `kde-osk-lockscreen`: planned Plasma lock-screen integration.
- `kde-osk-sddm-inputmethod`: SDDM greeter-only Wayland input-method backend
  that works through the greeter KWin instance without changing the SDDM theme
  or the user's desktop fcitx5 virtual-keyboard backend.

The first implementation milestone builds the shell and device monitor. The
long-term Plasma path is moving toward the KWin broker so KDE OSK can be
selected as the virtual-keyboard backend while stock fcitx5 continues to handle
normal desktop input.

中文需求说明见 [docs/PRODUCT_SPEC.zh-CN.md](docs/PRODUCT_SPEC.zh-CN.md)。
See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the design details.
See [docs/DBUS.md](docs/DBUS.md) for the first shell control API.

## Build

Linux with Qt 6.4 or newer, libudev, KDE LayerShellQt, and the kernel uinput
device is required for the desktop Wayland test path. Building the SDDM greeter
input-method backend additionally needs Qt Wayland client private development
files, Extra CMake Modules, and wayland-protocols.

Typical dependencies:

```sh
# Arch Linux
sudo pacman -S cmake extra-cmake-modules qt6-base qt6-declarative qt6-quickcontrols2 qt6-wayland wayland-protocols layer-shell-qt systemd

# Debian/Ubuntu family
sudo apt install cmake extra-cmake-modules qt6-base-dev qt6-declarative-dev qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtquick-nativestyle qml6-module-qtqml-workerscript qt6-wayland qt6-wayland-dev qt6-wayland-private-dev wayland-protocols libudev-dev liblayershellqtinterface-dev pkg-config
```

Some Debian/Ubuntu releases do not ship the Qt Wayland client private headers
needed to build `kde-osk-sddm-inputmethod`; in that case CMake leaves the SDDM
backend disabled while still building the desktop shell.
The greeter input-panel path uses a QtWayland private per-window shell
integration API that is available on current Arch Qt 6.11 packages. Older Qt
builds can compile the target for development checks, but the SDDM input-panel
window will report that the runtime integration is unavailable.

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

The release-grade Plasma path is moving toward `kde-osk-kwin-broker`: a KDE
virtual-keyboard backend entry that delegates normal desktop input to the
system's stock fcitx5 process while reserving a KWin input-panel path for KDE
OSK, lock-screen compatibility, and future secure-input behavior. See
[docs/BROKER.md](docs/BROKER.md).

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
`uinput`. It also supports long-press key repeat and sticky modifier keys.
Desktop automatic show/hide, lock-screen integration, and SDDM
integration must be implemented through their respective trusted interfaces.
The current tree includes the first SDDM Wayland input-method backend; it is
started only by SDDM's greeter KWin and does not replace fcitx5 in the user's
Plasma desktop session.
