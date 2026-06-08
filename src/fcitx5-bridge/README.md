# fcitx5 Bridge

This directory contains the desktop-session integration addon.

The addon must cooperate with fcitx5 instead of replacing it in KDE Plasma's
virtual keyboard slot. It should observe input context focus, drive
`kde-osk-shell` visibility through D-Bus, and keep fcitx5 as the active input
method framework.

Current scope:

- Watch `InputContextFocusIn`, `InputContextFocusOut`, and
  `InputContextCursorRectChanged`.
- Call `org.kde.KdeOsk.Keyboard.setTextFocusRect` with the focused cursor
  rectangle.
- Avoid registering as a Plasma virtual keyboard backend.

Key commit through fcitx5 is intentionally left for a later step so the bridge
does not double-submit while the shell still uses the temporary `uinput`
backend.
