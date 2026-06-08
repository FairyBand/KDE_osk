# Plasma Lock-Screen Integration

This directory is reserved for the lock-screen integration.

The regular desktop process should not type into the lock screen. Instead, the
lock screen should load or wrap the same keyboard QML component inside its own
trusted process and send text directly to the lock-screen password field.
