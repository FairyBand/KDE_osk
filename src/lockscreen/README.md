# Plasma Lock-Screen Integration

This directory is reserved for the Plasma lock-screen integration.

The lock screen belongs to the already logged-in Plasma session. The long-term
release path is the KWin broker: KDE OSK is selected as the Plasma
virtual-keyboard backend, delegates normal desktop input to stock fcitx5, and
uses a trusted KWin input-panel surface for the OSK.

The regular desktop process should not type into the lock screen. If the broker
path is unavailable, the fallback integration should reuse the shared keyboard
QML inside the lock-screen process or a dedicated kscreenlocker integration and
send text directly to the password field within that trust boundary.

Hard requirements:

- Do not require a patched or custom fcitx5 package.
- Do not rely on the user-session `org.kde.KdeOsk` D-Bus service to enter the
  lock-screen password.
- Do not treat the SDDM greeter input-method backend as a lock-screen backend.
