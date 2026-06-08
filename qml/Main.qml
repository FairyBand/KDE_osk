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
    property bool floatDragActive: false
    property bool batchingFloatMove: false

    readonly property bool layerShellActive: shellWindowAdapter.available
    readonly property bool floatOverlayActive: layerShellActive && windowMode === "float"

    width: floatOverlayActive ? Screen.width : windowMode === "float" ? floatingWidth : Screen.width
    height: floatOverlayActive ? Screen.height : keyboard.implicitHeight
    visible: keyboardController.visible
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
    color: "transparent"
    title: qsTr("KDE OSK")

    x: layerShellActive ? 0 : windowMode === "float" ? floatX : 0
    y: {
        if (layerShellActive) {
            return 0
        }
        if (windowMode === "dockTop") {
            return 0
        }
        if (windowMode === "float") {
            return floatY
        }
        return Screen.height - height
    }

    function floatingContentHeight() {
        return keyboard.implicitHeight
    }

    function maxFloatX() {
        return Math.max(0, Screen.width - floatingWidth)
    }

    function maxFloatY() {
        return Math.max(0, Screen.height - floatingContentHeight())
    }

    function clampFloatPosition() {
        floatX = Math.max(0, Math.min(floatX, maxFloatX()))
        floatY = Math.max(0, Math.min(floatY, maxFloatY()))
    }

    function setFloatingPosition(nextX, nextY) {
        batchingFloatMove = true
        floatX = Math.max(0, Math.min(Math.round(nextX), maxFloatX()))
        floatY = Math.max(0, Math.min(Math.round(nextY), maxFloatY()))
        batchingFloatMove = false
        configureShellWindow()
    }

    function beginFloatingDrag() {
        if (windowMode !== "float") {
            windowMode = "float"
        }
        clampFloatPosition()
        floatDragActive = true
        configureShellWindow()
    }

    function moveFloatingBy(dx, dy) {
        if (windowMode !== "float") {
            return
        }
        setFloatingPosition(floatX + dx, floatY + dy)
    }

    function finishFloatingDrag() {
        floatDragActive = false
        clampFloatPosition()
        configureShellWindow()
    }

    function configureShellWindow() {
        const shellMode = floatOverlayActive ? "floatOverlay" : windowMode
        shellWindowAdapter.configure(root, shellMode, floatX, floatY, width, height)
        shellWindowAdapter.setInputRegion(root,
                                          keyboard.x,
                                          keyboard.y,
                                          keyboard.width,
                                          keyboard.height,
                                          floatOverlayActive)
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
        width: root.windowMode === "float" ? root.floatingWidth : root.width
        height: implicitHeight
        x: root.floatOverlayActive ? root.floatX : 0
        y: root.floatOverlayActive ? root.floatY : 0
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
        onFloatingDragStarted: root.beginFloatingDrag()
        onFloatingDragMoved: (dx, dy) => {
            root.moveFloatingBy(dx, dy)
        }
        onFloatingDragFinished: root.finishFloatingDrag()
        onHideRequested: keyboardController.hideKeyboard()
        onKeyPressed: (keyId, shift, ctrl, alt, meta) =>
            keyboardController.keyPressed(keyId, shift, ctrl, alt, meta)
        onModifierChanged: (keyId, active) =>
            keyboardController.setModifierActive(keyId, active)
    }
}
