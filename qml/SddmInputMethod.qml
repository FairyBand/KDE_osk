import QtQuick
import QtQuick.Window

Window {
    id: root

    property string keyboardMode: "typing"
    property string windowMode: "dockBottom"
    property int floatingWidth: Math.min(Screen.width - 48, 980)
    property int floatX: Math.round((Screen.width - floatingWidth) / 2)
    property int floatY: Math.round((Screen.height - keyboard.implicitHeight) / 2)
    property int dragStartX: 0
    property int dragStartY: 0
    property bool batchingFloatMove: false

    width: Screen.width
    height: windowMode === "dockBottom" ? keyboard.implicitHeight : Screen.height
    visible: sddmInputMethodController.visible
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus
    title: qsTr("KDE OSK SDDM Input Method")

    function maxFloatX() {
        return Math.max(0, Screen.width - floatingWidth)
    }

    function maxFloatY() {
        return Math.max(0, Screen.height - keyboard.implicitHeight)
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
        configureInputRegion()
    }

    function beginFloatingDrag() {
        if (windowMode !== "float") {
            windowMode = "float"
        }
        clampFloatPosition()
        dragStartX = floatX
        dragStartY = floatY
        configureInputRegion()
    }

    function moveFloatingBy(dx, dy) {
        if (windowMode !== "float") {
            return
        }
        setFloatingPosition(dragStartX + dx, dragStartY + dy)
    }

    function finishFloatingDrag() {
        clampFloatPosition()
        configureInputRegion()
    }

    function configureInputRegion() {
        sddmWindowAdapter.setInputRegion(root,
                                         keyboard.x,
                                         keyboard.y,
                                         keyboard.width,
                                         keyboard.height,
                                         root.visible)
    }

    Component.onCompleted: configureInputRegion()
    onVisibleChanged: configureInputRegion()
    onKeyboardModeChanged: {
        clampFloatPosition()
        configureInputRegion()
    }
    onWindowModeChanged: {
        clampFloatPosition()
        configureInputRegion()
    }
    onWidthChanged: configureInputRegion()
    onHeightChanged: {
        clampFloatPosition()
        configureInputRegion()
    }
    onFloatXChanged: {
        if (!batchingFloatMove) {
            configureInputRegion()
        }
    }
    onFloatYChanged: {
        if (!batchingFloatMove) {
            configureInputRegion()
        }
    }

    KeyboardView {
        id: keyboard

        width: root.windowMode === "float" ? root.floatingWidth : root.width
        height: implicitHeight
        x: root.windowMode === "float" ? root.floatX : 0
        y: root.windowMode === "dockTop"
            ? 0
            : root.windowMode === "float"
                ? root.floatY
                : Math.max(0, root.height - height)

        keyboardMode: root.keyboardMode
        windowMode: root.windowMode
        autoShowEnabled: sddmInputMethodController.autoShowEnabled
        hardwareKeyboardPresent: sddmInputMethodController.hardwareKeyboardPresent
        ignoreHardwareKeyboard: sddmInputMethodController.ignoreHardwareKeyboard
        inputBackendAvailable: sddmInputMethodController.inputBackendAvailable
        inputBackendError: sddmInputMethodController.inputBackendError
        capsLockActive: sddmInputMethodController.capsLockActive

        onXChanged: root.configureInputRegion()
        onYChanged: root.configureInputRegion()
        onWidthChanged: root.configureInputRegion()
        onHeightChanged: root.configureInputRegion()
        onKeyboardModeRequested: mode => root.keyboardMode = mode
        onWindowModeRequested: mode => {
            root.windowMode = mode
            root.clampFloatPosition()
            root.configureInputRegion()
        }
        onAutoShowToggled: enabled => sddmInputMethodController.autoShowEnabled = enabled
        onIgnoreHardwareKeyboardToggled: ignore => sddmInputMethodController.ignoreHardwareKeyboard = ignore
        onFloatingDragStarted: root.beginFloatingDrag()
        onFloatingDragMoved: (dx, dy) => root.moveFloatingBy(dx, dy)
        onFloatingDragFinished: root.finishFloatingDrag()
        onHideRequested: sddmInputMethodController.hideKeyboard()
        onKeyPressed: (keyId, shift, ctrl, alt, meta) =>
            sddmInputMethodController.keyPressed(keyId, shift, ctrl, alt, meta)
        onModifierHoldChanged: (keyId, active) =>
            sddmInputMethodController.setModifierActive(keyId, active)
    }
}
