# Plasma Lock-Screen Integration

This directory is reserved for the Plasma lock-screen integration.

The lock screen belongs to the already logged-in Plasma session. That session's
KWin virtual-keyboard backend must remain fcitx5, so the SDDM greeter
input-method approach is not suitable for the lock screen.

The regular desktop process should not type into the lock screen. The planned
integration should reuse the shared keyboard QML inside the lock-screen process
or a dedicated kscreenlocker integration and send text directly to the password
field within that trust boundary.

Hard requirements:

- Do not change the user's Plasma virtual-keyboard backend away from fcitx5.
- Do not rely on the user-session `org.kde.KdeOsk` D-Bus service to enter the
  lock-screen password.
- Do not treat the SDDM greeter input-method backend as a lock-screen backend.
