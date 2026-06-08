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

`forceShowKeyboard()`

Shows the keyboard immediately. This is intended for development and manual
testing because it bypasses hardware-keyboard suppression.

`hideKeyboard()`

Hides the keyboard immediately.

`setTextFocusActive(bool active)`

Updates focus-driven visibility. Desktop, lock-screen, and greeter integrations
should call this when their controlled text field gains or loses focus.

`setTextFocusRect(bool active, int x, int y, int width, int height)`

Updates focus-driven visibility and the focused text-field rectangle in screen
coordinates. When the remembered shell mode is `float`, the shell keeps the
remembered floating position. When the remembered mode is `dockTop` or
`dockBottom`, the shell automatically chooses the opposite screen edge so the
focused text field stays visible.

## Properties

`visible: bool`

Current shell visibility.

`textFocusActive: bool`

Whether an integration currently reports editable text focus.

`focusRectX`, `focusRectY`, `focusRectWidth`, `focusRectHeight: int`

The last focused text-field rectangle in screen coordinates.

`autoShowEnabled: bool`

User/session policy for automatic visibility. When this is disabled, focus
changes do not show the keyboard.

`ignoreHardwareKeyboard: bool`

Development policy switch. When enabled, automatic and manual display are not
suppressed by external hardware keyboard detection.

`inputBackendAvailable: bool`

Whether the shell successfully opened the current input backend.

`inputBackendError: string`

The most recent input-backend initialization or write error.

## Signals

`visibleChanged(bool visible)`

Emitted when the shell appears or disappears.

`autoShowEnabledChanged(bool enabled)`

Emitted when the automatic visibility policy changes.

`textFocusActiveChanged(bool active)`

Emitted when focus-driven visibility state changes.

`focusRectChanged()`

Emitted when the focused text-field rectangle changes.

`commitTextRequested(string text)`

Emitted when the user presses a key in the shell. The desktop fcitx5 bridge
should consume this signal and commit through the focused fcitx5 input context.
Lock-screen and SDDM integrations should avoid this user-session signal and
wire the QML key events directly inside their trusted process.
