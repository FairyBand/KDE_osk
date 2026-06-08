import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root

    property string windowMode: shellSettings.windowMode
    property int floatingWidth: Math.min(Screen.width - 48, 980)
    property int floatX: shellSettings.floatX >= 0 ? shellSettings.floatX : Math.round((Screen.width - floatingWidth) / 2)
    property int floatY: shellSettings.floatY >= 0 ? shellSettings.floatY : Math.round((Screen.height - keyboard.implicitHeight) / 2)
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

    function saveFloatingPosition() {
        shellSettings.floatX = floatX
        shellSettings.floatY = floatY
    }

    function saveCurrentPlacement() {
        shellSettings.windowMode = windowMode
        if (windowMode === "float") {
            saveFloatingPosition()
        }
    }

    function applyFocusPlacement() {
        if (!keyboardController.textFocusActive || shellSettings.windowMode === "float") {
            return
        }

        const focusCenterY = keyboardController.focusRectY + keyboardController.focusRectHeight / 2
        const nextMode = focusCenterY > Screen.height / 2 ? "dockTop" : "dockBottom"
        if (windowMode !== nextMode) {
            windowMode = nextMode
        } else {
            configureShellWindow()
        }
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
        shellSettings.windowMode = "float"
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
        saveFloatingPosition()
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
    onWindowModeChanged: {
        if (windowMode === "float") {
            saveFloatingPosition()
        }
        configureShellWindow()
    }
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

    Connections {
        target: keyboardController

        function onTextFocusActiveChanged(active) {
            if (active) {
                root.applyFocusPlacement()
            }
        }

        function onVisibleChanged(visible) {
            if (visible) {
                root.applyFocusPlacement()
            } else {
                root.saveCurrentPlacement()
            }
        }

        function onFocusRectChanged() {
            root.applyFocusPlacement()
        }
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
        capsLockActive: keyboardController.capsLockActive
        keyboardMode: shellSettings.keyboardMode
        windowMode: root.windowMode

        onAutoShowToggled: enabled => keyboardController.autoShowEnabled = enabled
        onIgnoreHardwareKeyboardToggled: ignore => keyboardController.ignoreHardwareKeyboard = ignore
        onKeyboardModeRequested: mode => shellSettings.keyboardMode = mode
        onWindowModeRequested: mode => {
            shellSettings.windowMode = mode
            root.windowMode = mode
            root.clampFloatPosition()
            if (mode === "float") {
                root.saveFloatingPosition()
            }
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
        onModifierHoldChanged: (keyId, active) =>
            keyboardController.setModifierActive(keyId, active)
    }
}
