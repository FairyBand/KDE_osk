# D-Bus API

Service:

```text
org.kde.KdeOsk
```

Object:

```text
/Keyboard
```

Interface:

```text
org.kde.KdeOsk.Keyboard
```

## Methods

`showKeyboard()`

Shows the keyboard unless an external hardware keyboard is currently connected.

`hideKeyboard()`

Hides the keyboard immediately.

`setTextFocusActive(bool active)`

Updates focus-driven visibility. Desktop, lock-screen, and greeter integrations
should call this when their controlled text field gains or loses focus.

## Properties

`visible: bool`

Current shell visibility.

`autoShowEnabled: bool`

User/session policy for automatic visibility. When this is disabled, focus
changes do not show the keyboard.

## Signals

`visibleChanged(bool visible)`

Emitted when the shell appears or disappears.

`autoShowEnabledChanged(bool enabled)`

Emitted when the automatic visibility policy changes.

`commitTextRequested(string text)`

Emitted when the user presses a key in the shell. The desktop fcitx5 bridge
should consume this signal and commit through the focused fcitx5 input context.
Lock-screen and SDDM integrations should avoid this user-session signal and
wire the QML key events directly inside their trusted process.
