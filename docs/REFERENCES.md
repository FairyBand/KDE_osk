# References

These references guide the project constraints and integration choices:

- [KDE Plasma Keyboard](https://github.com/KDE/plasma-keyboard): KDE's current
  Qt Virtual Keyboard based project uses Wayland input-method protocol
  integration rather than arbitrary key injection.
- [KDE Virtual Keyboard settings handbook](https://docs.kde.org/stable_kf6/en/kwin/kcontrol/kwinvirtualkeyboard/kwinvirtualkeyboard.pdf):
  Plasma exposes a virtual keyboard setting through KWin.
- [Qt Virtual Keyboard overview](https://doc.qt.io/qt-6/qtvirtualkeyboard-index.html):
  Qt's reference virtual keyboard framework and QML types.
- [Fcitx basic concepts](https://fcitx-im.org/wiki/Basic_concept):
  fcitx5 is addon-based, and focused input contexts represent the active client.
- [Wayland text-input protocol](https://wayland.app/protocols/xx-text-input-v3):
  text-input focus is compositor-mediated and tied to editable surfaces.
- [Wayland input-method protocol](https://wayland.app/protocols/xx-input-method-v2):
  input methods communicate with the compositor through a dedicated protocol.
- [SDDM configuration manual](https://man.archlinux.org/man/sddm.conf.5.en):
  SDDM has greeter-level input method configuration and separate Wayland
  compositor startup behavior.
