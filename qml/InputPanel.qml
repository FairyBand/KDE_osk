import QtQuick
import QtQuick.Window

Window {
    id: root

    property string keyboardMode: "typing"

    width: Screen.width
    height: keyboard.implicitHeight
    visible: inputPanelController.visible
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus
    title: qsTr("KDE OSK Input Panel")

    KeyboardView {
        id: keyboard
        anchors.fill: parent
        keyboardMode: root.keyboardMode
        windowMode: "dockBottom"
        showWindowModeControl: false
        autoShowEnabled: inputPanelController.autoShowEnabled
        hardwareKeyboardPresent: inputPanelController.hardwareKeyboardPresent
        ignoreHardwareKeyboard: inputPanelController.ignoreHardwareKeyboard
        inputBackendAvailable: inputPanelController.inputBackendAvailable
        inputBackendError: inputPanelController.inputBackendError
        capsLockActive: inputPanelController.capsLockActive

        onKeyboardModeRequested: mode => root.keyboardMode = mode
        onAutoShowToggled: enabled => inputPanelController.autoShowEnabled = enabled
        onIgnoreHardwareKeyboardToggled: ignore => inputPanelController.ignoreHardwareKeyboard = ignore
        onHideRequested: inputPanelController.hideKeyboard()
        onKeyPressed: (keyId, shift, ctrl, alt, meta) =>
            inputPanelController.keyPressed(keyId, shift, ctrl, alt, meta)
        onModifierHoldChanged: (keyId, active) =>
            inputPanelController.setModifierActive(keyId, active)
    }
}
