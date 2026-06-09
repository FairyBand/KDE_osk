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

On Plasma Wayland the shell uses LayerShellQt when available. Layer-shell
placement is required because regular Wayland toplevel clients cannot position
themselves globally and may take focus when touched.

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

### KWin Virtual-Keyboard Broker

The release-grade Plasma path is a broker selected in KDE's virtual-keyboard
settings. The broker owns the KWin startup entry while preserving stock fcitx5
for normal desktop input. This changes the settings entry the user selects, but
does not require rebuilding or patching fcitx5.

Initial broker behavior is conservative: validate the KWin input-method startup
environment, start or wait for stock `fcitx5`, and delegate the KWin Wayland
input-method socket to fcitx5 through fcitx5's public D-Bus controller API. The
broker stays resident so KWin can supervise it. Later milestones can show KDE
OSK as a KWin input-panel surface when touch keyboard UI is needed.

This broker path is preferred over Plasma lock-screen QML integration for
long-term distribution use because the lock screen can use the existing KWin
virtual-keyboard path instead of a theme-specific QML hook.

### Plasma Lock Screen

The lock screen runs inside the already logged-in Plasma session. The long-term
path is to use the broker-selected KWin virtual-keyboard backend so the lock
screen can show KDE OSK through the trusted input-panel route while normal
desktop input remains delegated to stock fcitx5.

Direct kscreenlocker or lock-screen QML integration remains a fallback for
environments where the broker cannot be used. The regular desktop D-Bus service
is not an appropriate text-entry path for the lock screen.

The shared `KeyboardView.qml` component is intentionally controller-agnostic:
it emits key and visibility-policy signals instead of directly calling the
desktop session service. That keeps it reusable in the lock-screen process.

### SDDM Greeter

The SDDM greeter runs before any user Plasma session exists. On KDE's Wayland
greeter path, SDDM starts a greeter-only `kwin_wayland` instance through its
`[Wayland] CompositorCommand`. That compositor can have its own
`--inputmethod` command, separate from the user's desktop KWin configuration.

The stable SDDM route is to keep the Breeze greeter theme untouched and provide
a greeter-only Wayland input-method backend:

- `kde-osk-sddm-inputmethod` is started only by SDDM's greeter KWin.
- SDDM's compositor command points to it with `--inputmethod`.
- The Breeze virtual-keyboard button continues to use KDE's existing
  `KWinVirtualKeyboard` path.
- No user `kwinrc`, Plasma virtual-keyboard setting, or fcitx5 desktop setup is
  changed.
- External USB/Bluetooth keyboard detection still suppresses the OSK.

The project intentionally does not install a KDE virtual-keyboard desktop file
for the greeter backend. It is not a desktop-session alternative to fcitx5; it
is an SDDM greeter component.

## Milestones

1. Build and install `kde-osk-shell`.
2. Provide a D-Bus API and hardware keyboard suppression.
3. Add `kde-osk-kwin-broker` and verify that it delegates to stock fcitx5.
4. Implement the fcitx5 bridge for desktop focus and text commit where the
   non-broker shell path is still used.
5. Add the broker resident mode and KWin input-panel KDE OSK surface.
6. Package the SDDM greeter input-method backend and SDDM configuration helper.
7. Add settings UI for layouts, device policy, height, opacity, and haptics.

## Non-Goals For Milestone 1

- No fcitx5 candidate-row embedding.
- No global key injection from an unprivileged Wayland client.
- No custom fcitx5 package or fcitx5 patching.
- No SDDM theme fork for the primary greeter integration.

## References

See [REFERENCES.md](REFERENCES.md).
