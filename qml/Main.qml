import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root

    width: Screen.width
    height: keyboard.implicitHeight
    visible: keyboardController.visible
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
    color: "transparent"
    title: qsTr("KDE OSK")

    x: 0
    y: Screen.height - height

    KeyboardView {
        id: keyboard
        anchors.fill: parent
        autoShowEnabled: keyboardController.autoShowEnabled
        hardwareKeyboardPresent: hardwareKeyboardMonitor.hardwareKeyboardPresent

        onAutoShowToggled: enabled => keyboardController.autoShowEnabled = enabled
        onHideRequested: keyboardController.hideKeyboard()
        onKeyPressed: text => keyboardController.keyPressed(text)
    }
}
