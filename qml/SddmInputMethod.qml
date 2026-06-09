import QtQuick
import QtQuick.Window

Window {
    id: root

    width: Screen.width
    height: keyboard.implicitHeight
    visible: sddmInputMethodController.visible
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus
    title: qsTr("KDE OSK SDDM Input Method")

    KeyboardView {
        id: keyboard
        anchors.fill: parent
        keyboardMode: "typing"
        windowMode: "dockBottom"
        autoShowEnabled: true
        hardwareKeyboardPresent: sddmInputMethodController.hardwareKeyboardPresent
        ignoreHardwareKeyboard: false
        inputBackendAvailable: sddmInputMethodController.inputBackendAvailable
        inputBackendError: sddmInputMethodController.inputBackendError
        capsLockActive: sddmInputMethodController.capsLockActive
        showToolbar: false

        onHideRequested: sddmInputMethodController.hideKeyboard()
        onKeyPressed: (keyId, shift, ctrl, alt, meta) =>
            sddmInputMethodController.keyPressed(keyId, shift, ctrl, alt, meta)
        onModifierHoldChanged: (keyId, active) =>
            sddmInputMethodController.setModifierActive(keyId, active)
    }
}
