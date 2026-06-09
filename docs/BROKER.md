# KDE OSK KWin Broker

This document tracks the long-term KDE Plasma integration path.

## Goal

`kde-osk-kwin-broker` is intended to be the virtual-keyboard entry selected in
KDE System Settings. It should preserve the stock fcitx5 desktop input
experience while adding a KDE OSK input-panel path for touch keyboard behavior,
the lock screen, and future secure-input policy.

The broker must not require a custom fcitx5 package. It delegates to the
system-provided `fcitx5` executable and should use public fcitx5 and KWin
interfaces only.

## High-Level Model

```text
KWin virtual keyboard setting
        |
        v
kde-osk-kwin-broker
        |
        +-- stock fcitx5 for normal desktop input
        |
        +-- KDE OSK input-panel for touch keyboard UI
```

The first broker milestone is intentionally conservative: when KWin starts the
broker, the broker validates that it was launched in a Wayland input-method
environment and then `exec`s the stock `fcitx5` process. This proves the KDE
System Settings integration path without changing normal input behavior.

Later milestones can keep the broker resident and hand KWin's Wayland
input-method socket to fcitx5 through fcitx5's Wayland module socket APIs, while
also launching the KDE OSK input panel when appropriate.

## Responsibilities

- Install a virtual-keyboard desktop entry for KDE Plasma.
- Start stock fcitx5 without rebuilding or patching fcitx5.
- Keep fcitx5 responsible for normal desktop CJK input, candidates, groups, and
  user configuration.
- Provide a KWin input-panel based KDE OSK surface instead of a normal desktop
  window for lock-screen-capable keyboard UI.
- Reuse the existing hardware keyboard policy.
- Enter secure-input mode when the active input context reports password or
  hidden-text purpose.

## Non-Goals

- Do not patch the user's fcitx5 package.
- Do not use `uinput` as the release-grade desktop or lock-screen path.
- Do not type lock-screen passwords through the user-session
  `org.kde.KdeOsk` D-Bus service.
- Do not depend on Plasma lock-screen QML internals for the release-grade path.

## Milestones

1. Add the broker executable and KDE virtual-keyboard desktop entry.
2. Verify that selecting "KDE OSK Broker" in Plasma starts stock fcitx5 and
   keeps desktop input behavior unchanged.
3. Refactor the existing SDDM input-method/input-panel code into reusable broker
   building blocks.
4. Add resident broker mode that delegates the KWin Wayland input-method socket
   to stock fcitx5 through public fcitx5 mechanisms.
5. Add the KDE OSK input-panel process and connect it to the broker's visibility
   and hardware-keyboard policy.
6. Add secure-input mode based on input-method content type.
7. Keep the existing SDDM greeter backend separate, because SDDM runs before the
   user's fcitx5 session exists.
