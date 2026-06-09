# SDDM Integration

The SDDM integration does not fork or patch the greeter theme. KDE Breeze
already has a virtual-keyboard button; on Wayland that button talks to the
greeter-only KWin virtual-keyboard path. KDE OSK integrates by providing the
input-method process that SDDM's greeter KWin starts.

The login screen runs before the user's desktop session, so it must not rely on
`org.kde.KdeOsk` from the user session bus. It must also not change the user's
Plasma virtual-keyboard backend; that remains fcitx5 in the logged-in desktop
session.

## Components

- `kde-osk-sddm-inputmethod`: greeter-only Wayland input-method/input-panel
  backend.
- `kde-osk-sddm-wayland.conf`: SDDM config sample that points only the greeter
  `kwin_wayland` process at the KDE OSK input method.
- `install-sddm-inputmethod.sh`: helper that installs the sample to
  `/etc/sddm.conf.d/kde-osk-wayland.conf`.

## Install

Configure with the system prefix:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
sudo install-sddm-inputmethod.sh /usr
```

The generated SDDM drop-in contains:

```ini
[General]
DisplayServer=wayland
InputMethod=

[Wayland]
CompositorCommand=kwin_wayland --no-global-shortcuts --no-lockscreen --inputmethod /usr/bin/kde-osk-sddm-inputmethod --locale1
```

This affects only SDDM's greeter KWin instance. After login, the user desktop
starts its own KWin instance and keeps the fcitx5 virtual-keyboard/input-method
configuration.

## Scope

- Do not modify `/usr/share/sddm/themes/breeze`.
- Do not install KDE OSK as a desktop-session virtual-keyboard choice.
- Do not write `~/.config/kwinrc`.
- Do not replace fcitx5 in the Plasma desktop session.
