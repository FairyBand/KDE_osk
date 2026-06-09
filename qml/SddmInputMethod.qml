import QtQuick
import QtQuick.Window

Window {
    id: root

    property string keyboardMode: "typing"

    width: Screen.width
    height: keyboard.implicitHeight
    visible: sddmInputMethodController.visible
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus
    title: qsTr("KDE OSK SDDM Input Method")

    KeyboardView {
        id: keyboard
        anchors.fill: parent
        keyboardMode: root.keyboardMode
        windowMode: "dockBottom"
        showWindowModeControl: false
        autoShowEnabled: sddmInputMethodController.autoShowEnabled
        hardwareKeyboardPresent: sddmInputMethodController.hardwareKeyboardPresent
        ignoreHardwareKeyboard: sddmInputMethodController.ignoreHardwareKeyboard
        inputBackendAvailable: sddmInputMethodController.inputBackendAvailable
        inputBackendError: sddmInputMethodController.inputBackendError
        capsLockActive: sddmInputMethodController.capsLockActive

        onKeyboardModeRequested: mode => root.keyboardMode = mode
        onAutoShowToggled: enabled => sddmInputMethodController.autoShowEnabled = enabled
        onIgnoreHardwareKeyboardToggled: ignore => sddmInputMethodController.ignoreHardwareKeyboard = ignore
        onHideRequested: sddmInputMethodController.hideKeyboard()
        onKeyPressed: (keyId, shift, ctrl, alt, meta) =>
            sddmInputMethodController.keyPressed(keyId, shift, ctrl, alt, meta)
        onModifierHoldChanged: (keyId, active) =>
            sddmInputMethodController.setModifierActive(keyId, active)
    }
}
