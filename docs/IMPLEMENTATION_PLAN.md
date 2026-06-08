# Implementation Plan

## 1. Shell

Create a Qt 6 / QML shell that can be shown and hidden over the bottom of the
screen. It exposes a small D-Bus API for integrations and starts hidden by
default. The shell owns visual polish, touch targets, layout switching, and
future settings.

Initial done criteria:

- `kde-osk-shell --show` displays the keyboard.
- `kde-osk-shell --force-show` displays the keyboard while bypassing hardware
  keyboard suppression for development.
- D-Bus can call `showKeyboard`, `hideKeyboard`, and `setTextFocusActive`.
- External hardware keyboard presence hides the shell and blocks auto-show.
- A systemd user service can keep the shell available on the session bus.
- The keyboard QML can be reused by lock-screen and greeter integrations.
- Basic key input works through the temporary Linux `uinput` backend.

The `uinput` backend is a practical development bridge, not the final desktop
architecture. The fcitx5 bridge remains the preferred path for correct text
commit and focus handling.

Linux verification steps are tracked in [LINUX_TEST_PLAN.md](LINUX_TEST_PLAN.md).

## 2. fcitx5 Bridge

Implement `fcitx5-kde-osk-bridge` as an addon loaded by fcitx5.

Responsibilities:

- Subscribe to input context focus events.
- Call `setTextFocusActive(true)` when an editable field is focused.
- Call `setTextFocusActive(false)` when focus leaves.
- Receive shell key events and commit through the focused input context.

The exact commit channel should be finalized after the addon scaffold is in
place. The preferred direction is to keep a D-Bus boundary between the shell and
the addon so the QML UI remains independent from fcitx5 internals.

## 3. Lock Screen

Reuse the keyboard QML component inside the Plasma lock-screen process. This is
the correct place to edit the password field because the lock screen owns that
field and runs with the right trust boundary.

Done criteria:

- No external keyboard: password field focus opens the OSK.
- External keyboard: password field focus does not open the OSK.
- Unlock behavior is unchanged with fcitx5 installed.

## 4. SDDM

Build a greeter-side integration. The SDDM greeter cannot depend on the user's
desktop session D-Bus service, so the login keyboard should be loaded by the
greeter theme or a greeter plugin.

Done criteria:

- No external keyboard: password field focus opens the OSK on the login screen.
- External keyboard: password field focus does not open the OSK.
- The login password is delivered directly to the greeter's password input.

## 5. Packaging

Package as distro-friendly components:

- `kde-osk-shell`
- `fcitx5-kde-osk-bridge`
- `plasma-kde-osk-lockscreen`
- `sddm-kde-osk`

Each component should be optional except for the shared QML/assets package.
