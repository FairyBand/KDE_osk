# Linux Test Plan

Run these checks on a KDE Plasma Wayland machine.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

## Shell Smoke Test

```sh
./build/kde-osk-shell --show
```

Expected:

- The keyboard appears at the bottom of the screen.
- It does not steal focus from the active application.
- Pressing `Hide` hides the window.

## D-Bus Smoke Test

Start the shell without `--show`, then run:

```sh
busctl --user call org.kde.KdeOsk /Keyboard org.kde.KdeOsk.Keyboard setTextFocusActive b true
busctl --user call org.kde.KdeOsk /Keyboard org.kde.KdeOsk.Keyboard setTextFocusActive b false
```

Expected:

- The first command shows the keyboard when no external keyboard is detected.
- The second command hides it.

## Hardware Keyboard Policy

1. Disconnect external USB and Bluetooth keyboards.
2. Call `setTextFocusActive true`.
3. Connect a USB or Bluetooth keyboard.

Expected:

- The keyboard appears in step 2.
- It hides after the external keyboard appears.
- Further focus calls do not show it while the external keyboard remains
  connected.

## fcitx5 Safety

Before the fcitx5 bridge is implemented, verify that installing and running the
shell does not change Plasma's configured input method. fcitx5 should continue
to work exactly as before because the shell does not register itself as the
Plasma virtual keyboard backend.
