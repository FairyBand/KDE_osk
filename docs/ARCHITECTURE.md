# Architecture

## Problem Statement

KDE Plasma already has a virtual keyboard slot in System Settings, but that slot
is also used by input methods such as fcitx5. Replacing fcitx5 there breaks a
normal Chinese/Japanese/Korean input workflow, while many standalone keyboard
applications are unable to work reliably on Wayland because they are ordinary
clients.

KDE OSK is designed around the Wayland security model instead of fighting it.
The keyboard shell is a normal visible surface, but text focus, text commit, and
privileged login/lock-screen behavior are delegated to integration points that
are already trusted by the compositor or greeter.

## Components

### `kde-osk-shell`

The shell owns the touch UI, layout state, and D-Bus API:

- `org.kde.KdeOsk /Keyboard showKeyboard()`
- `org.kde.KdeOsk /Keyboard forceShowKeyboard()`
- `org.kde.KdeOsk /Keyboard hideKeyboard()`
- `org.kde.KdeOsk /Keyboard setTextFocusActive(bool active)`

The shell also watches hardware keyboard state. When an external USB,
Bluetooth, or dock-style keyboard is present, automatic showing is suppressed.

The shell must not become the KDE virtual keyboard backend while fcitx5 is in
use. It is a companion UI controlled by fcitx5 and the session integrations.

During early development, the shell can send basic key events through Linux
`uinput`. This makes the keyboard immediately useful for typing tests, but it
is not a replacement for the planned fcitx5 text-commit bridge.

### Hardware Keyboard Detection

The first implementation uses libudev and treats USB, Bluetooth, and dock-like
keyboard devices as external hardware keyboards. This keeps convertible tablets
usable: the keyboard appears when the device is used as a tablet, and stops
appearing when a real keyboard is attached.

Future refinements:

- Add a configuration allowlist/blocklist for internal laptop keyboards.
- Detect tablet-mode switch devices where available.
- Expose device state on D-Bus for Plasma settings UI.

### Window Placement

The shell supports three development window modes:

- Bottom docked.
- Top docked.
- Floating.

Automatic avoidance of the active input field requires geometry from the text
focus integration. The first version exposes manual top/bottom/floating modes;
the fcitx5 bridge should later provide cursor rectangle data so the shell can
choose a non-overlapping edge automatically.

### Desktop Session Bridge

The desktop session bridge should be an fcitx5 addon, not a competing input
method. Its responsibilities are:

- Observe fcitx5 input context focus changes.
- Tell `kde-osk-shell` when a text field becomes active or inactive.
- Commit key presses through the active fcitx5 input context.
- Never replace fcitx5 in Plasma's virtual keyboard setting.

This keeps fcitx5 candidates, layout switching, and compose behavior under
fcitx5's control. Candidate-row embedding can be added later through a separate
fcitx5 UI protocol or addon API.

### Plasma Lock Screen

The lock screen runs in a privileged Plasma context. It should load the same
QML keyboard view or a thin wrapper around it and send text directly to the
lock-screen password field. The regular desktop D-Bus session may not be
available or appropriate here, so this must be a dedicated integration.

The shared `KeyboardView.qml` component is intentionally controller-agnostic:
it emits key and visibility-policy signals instead of directly calling the
desktop session service. That keeps it reusable in the lock-screen process.

### SDDM Greeter

The SDDM greeter should integrate the keyboard in the greeter theme or through
Qt Virtual Keyboard compatible input panels. SDDM already has greeter-level
configuration for an input method, but the project should avoid relying on a
global desktop-session process during login.

Recommended first target:

- Ship an SDDM theme helper that imports the keyboard QML component.
- Gate automatic display on the same external-keyboard detection policy.
- Keep password entry local to the greeter process.

The shared QML files are installed under `${datadir}/kde-osk/qml` for greeter
and lock-screen integrations that cannot import the executable-embedded module.

## Milestones

1. Build and install `kde-osk-shell`.
2. Provide a D-Bus API and hardware keyboard suppression.
3. Implement the fcitx5 bridge for desktop focus and text commit.
4. Package a Plasma lock-screen integration.
5. Package an SDDM greeter integration.
6. Add settings UI for layouts, device policy, height, opacity, and haptics.

## Non-Goals For Milestone 1

- No fcitx5 candidate-row embedding.
- No global key injection from an unprivileged Wayland client.
- No replacement of Plasma's fcitx5 virtual keyboard setting.

## References

See [REFERENCES.md](REFERENCES.md).
