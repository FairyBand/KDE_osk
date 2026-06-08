# SDDM Integration

This directory is reserved for the SDDM greeter integration.

The login screen runs before the user's desktop session, so it should not rely
on `org.kde.KdeOsk` from the user session bus. The first practical target is an
SDDM theme helper or greeter-side plugin that imports the shared keyboard QML
and gates automatic visibility on hardware keyboard detection.
