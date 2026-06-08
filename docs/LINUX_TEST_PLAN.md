# Linux Test Plan

Run these checks on a KDE Plasma Wayland machine.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

## Shell Smoke Test

```sh
./build/kde-osk-shell --force-show
```

Expected:

- The keyboard appears at the bottom of the screen.
- It does not steal focus from the active application.
- The `Input` and `Full` layout buttons switch layouts.
- The `Bottom`, `Top`, and `Float` window modes move the keyboard.
- Pressing `Hide` hides the window.

## Basic Typing Test

Focus a text editor, then start the keyboard:

```sh
./build/kde-osk-shell --force-show
```

Expected:

- Pressing letter, number, space, enter, and backspace keys edits the focused
  text field.
- Full keyboard modifier keys such as `Ctrl`, `Alt`, `Meta`, and `Shift` affect
  the next key press.

If keys do not type, check `/dev/uinput`:

```sh
ls -l /dev/uinput
```

The process must have permission to open that device.

For development, the included `data/70-kde-osk-uinput.rules` can be installed
or adapted to grant a trusted local group access.

## D-Bus Smoke Test

Start the shell without `--show`, then run:

```sh
busctl --user call org.kde.KdeOsk /Keyboard org.kde.KdeOsk.Keyboard setTextFocusActive b true
busctl --user call org.kde.KdeOsk /Keyboard org.kde.KdeOsk.Keyboard setTextFocusActive b false
```

Expected:

- The first command shows the keyboard when no external keyboard is detected.
- The second command hides it.

To bypass hardware keyboard suppression during development:

```sh
busctl --user call org.kde.KdeOsk /Keyboard org.kde.KdeOsk.Keyboard forceShowKeyboard
```

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

The temporary `uinput` backend sends virtual hardware key events. It is useful
for basic testing, but the planned fcitx5 bridge is still required for polished
text commit behavior, candidate integration, and reliable focus-driven display.
