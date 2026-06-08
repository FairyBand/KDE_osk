# fcitx5 Bridge

This directory is reserved for the desktop-session integration addon.

The addon must cooperate with fcitx5 instead of replacing it in KDE Plasma's
virtual keyboard slot. It should observe input context focus, drive
`kde-osk-shell` visibility through D-Bus, and commit key presses through the
currently focused fcitx5 input context.

The next implementation step is to add a minimal fcitx5 addon target after
selecting the supported fcitx5 API baseline for packaging.
