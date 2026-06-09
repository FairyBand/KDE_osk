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
environment, starts or waits for stock `fcitx5`, and delegates a broker-owned
Wayland proxy socket to fcitx5 through fcitx5's public D-Bus controller API.
At this stage the proxy forwards bytes and Wayland file descriptors while
inspecting `wl_registry` globals and binds. The fcitx5 branch still receives the
normal input-method path, but `zwp_input_panel_v1` is filtered out of fcitx5's
registry view and reserved for the future KDE OSK panel branch.

The old `exec` delegation path is kept as an explicit compatibility mode with
`--exec-fcitx5`.

For real-machine panel testing, `kde-osk-input-panel` is provided as a direct
KWin input-method/input-panel backend. It reuses the same Wayland input-method
implementation as the SDDM backend and shows the shared KDE OSK keyboard QML.
This target is a test and fallback backend; it does not delegate normal desktop
input to fcitx5 while selected.

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
2. Verify that selecting "KDE OSK Broker" in Plasma starts stock fcitx5,
   delegates KWin's socket through `ReopenWaylandConnectionSocket`, and keeps
   desktop input behavior unchanged.
3. Refactor the existing SDDM input-method/input-panel code into reusable
   building blocks.
4. Add `kde-osk-input-panel` for direct real-machine KWin input-panel testing.
5. Put the resident broker in the Wayland socket data path with a transparent
   proxy that preserves stock fcitx5 behavior, including Wayland fd passing.
6. Filter the input-panel global out of the fcitx5 branch and reserve it for
   KDE OSK.
7. Add the KDE OSK input-panel branch and proxy its surface traffic through the
   broker.
8. Connect a KDE OSK input-panel process to the resident broker's visibility and
   hardware-keyboard policy.
9. Add secure-input mode based on input-method content type.
10. Keep the existing SDDM greeter backend separate, because SDDM runs before the
   user's fcitx5 session exists.
