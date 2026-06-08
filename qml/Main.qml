import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root

    property string windowMode: "dockBottom"
    property int floatingWidth: Math.min(Screen.width - 48, 980)

    width: windowMode === "float" ? floatingWidth : Screen.width
    height: keyboard.implicitHeight
    visible: keyboardController.visible
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
    color: "transparent"
    title: qsTr("KDE OSK")

    x: windowMode === "float" ? Math.round((Screen.width - width) / 2) : 0
    y: {
        if (windowMode === "dockTop") {
            return 0
        }
        if (windowMode === "float") {
            return Math.round(Screen.height * 0.55)
        }
        return Screen.height - height
    }

    KeyboardView {
        id: keyboard
        anchors.fill: parent
        autoShowEnabled: keyboardController.autoShowEnabled
        hardwareKeyboardPresent: hardwareKeyboardMonitor.hardwareKeyboardPresent
        ignoreHardwareKeyboard: keyboardController.ignoreHardwareKeyboard
        inputBackendAvailable: keyboardController.inputBackendAvailable
        inputBackendError: keyboardController.inputBackendError
        windowMode: root.windowMode

        onAutoShowToggled: enabled => keyboardController.autoShowEnabled = enabled
        onIgnoreHardwareKeyboardToggled: ignore => keyboardController.ignoreHardwareKeyboard = ignore
        onWindowModeRequested: mode => root.windowMode = mode
        onHideRequested: keyboardController.hideKeyboard()
        onKeyPressed: (keyId, shift, ctrl, alt, meta) =>
            keyboardController.keyPressed(keyId, shift, ctrl, alt, meta)
    }
}
