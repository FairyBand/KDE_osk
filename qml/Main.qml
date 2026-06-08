import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root

    property string windowMode: "dockBottom"
    property int floatingWidth: Math.min(Screen.width - 48, 980)
    property int floatX: Math.round((Screen.width - floatingWidth) / 2)
    property int floatY: Math.round((Screen.height - height) / 2)
    property bool batchingFloatMove: false

    width: windowMode === "float" ? floatingWidth : Screen.width
    height: keyboard.implicitHeight
    visible: keyboardController.visible
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
    color: "transparent"
    title: qsTr("KDE OSK")

    x: windowMode === "float" ? floatX : 0
    y: {
        if (windowMode === "dockTop") {
            return 0
        }
        if (windowMode === "float") {
            return floatY
        }
        return Screen.height - height
    }

    function clampFloatPosition() {
        floatX = Math.max(0, Math.min(floatX, Screen.width - width))
        floatY = Math.max(0, Math.min(floatY, Screen.height - height))
    }

    function setFloatingPosition(nextX, nextY) {
        batchingFloatMove = true
        floatX = Math.max(0, Math.min(Math.round(nextX), Screen.width - width))
        floatY = Math.max(0, Math.min(Math.round(nextY), Screen.height - height))
        batchingFloatMove = false
        configureShellWindow()
    }

    function configureShellWindow() {
        shellWindowAdapter.configure(root, windowMode, floatX, floatY, width, height)
    }

    Component.onCompleted: configureShellWindow()
    onWindowModeChanged: configureShellWindow()
    onFloatXChanged: {
        if (!batchingFloatMove) {
            configureShellWindow()
        }
    }
    onFloatYChanged: {
        if (!batchingFloatMove) {
            configureShellWindow()
        }
    }
    onWidthChanged: {
        clampFloatPosition()
        configureShellWindow()
    }
    onHeightChanged: {
        clampFloatPosition()
        configureShellWindow()
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
        onWindowModeRequested: mode => {
            root.windowMode = mode
            root.clampFloatPosition()
            root.configureShellWindow()
        }
        onFloatingMoveRequested: (sceneX, sceneY, offsetX, offsetY) => {
            if (root.windowMode !== "float") {
                root.windowMode = "float"
            }
            root.setFloatingPosition(sceneX - offsetX, sceneY - offsetY)
        }
        onHideRequested: keyboardController.hideKeyboard()
        onKeyPressed: (keyId, shift, ctrl, alt, meta) =>
            keyboardController.keyPressed(keyId, shift, ctrl, alt, meta)
        onModifierChanged: (keyId, active) =>
            keyboardController.setModifierActive(keyId, active)
    }
}
