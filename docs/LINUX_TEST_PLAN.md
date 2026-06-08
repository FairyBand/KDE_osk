# Linux Test Plan

Run these checks on a KDE Plasma Wayland machine.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/kde-osk-shell --help
```

Confirm CMake did not print:

```text
LayerShellQt was not found
```

If that warning appears, install the LayerShellQt development package and
reconfigure from a clean build directory. Without LayerShellQt, Plasma Wayland
will treat the keyboard as a normal window and may ignore its requested
top/bottom/floating placement.

WSL can be used for compile checks, but it is not a reliable runtime test
environment for the LayerShellQt Wayland window. Use a real Plasma Wayland
session for `--force-show` UI testing.

## Shell Smoke Test

```sh
./build/kde-osk-shell --force-show
```

Expected:

- The keyboard appears at the bottom of the screen.
- It does not steal focus from the active application.
- The `Input` and `Full` layout buttons switch layouts.
- The `Bottom`, `Top`, and `Float` window modes move the keyboard.
- In `Float` mode, dragging the small handle in the toolbar moves the keyboard.
- Pressing `Hide` hides the window.

## Basic Typing Test

Focus a text editor, then start the keyboard:

```sh
./build/kde-osk-shell --force-show
```

Expected:

- Pressing letter, number, space, enter, and backspace keys edits the focused
  text field.
- Holding a repeatable key starts repeating after a short delay.
- Full keyboard modifier keys such as `Ctrl`, `Alt`, `Meta`, and `Shift` affect
  subsequent key presses until toggled off. `Shift` is released after one
  normal key press.
- Tapping `Meta` opens the Plasma launcher if the desktop has the default Meta
  shortcut enabled.

If keys do not type, check `/dev/uinput`:

```sh
ls -l /dev/uinput
```

The process must have permission to open that device.

If `/dev/uinput` opens successfully but input still goes nowhere, confirm the
build found LayerShellQt. Without LayerShellQt, the keyboard may become the
focused Wayland toplevel and virtual key events can target the keyboard itself.

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
