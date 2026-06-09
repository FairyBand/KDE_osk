import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Rectangle {
    id: root

    implicitHeight: keyboardMode === "full"
        ? Math.min(440, Math.max(330, Screen.height * 0.42))
        : Math.min(360, Math.max(260, Screen.height * 0.34))
    color: "#202124"

    property bool shift: false
    property bool ctrl: false
    property bool alt: false
    property bool meta: false
    property bool shiftHeld: false
    property bool ctrlHeld: false
    property bool altHeld: false
    property bool metaHeld: false
    property int modifierHoldThreshold: 200
    property bool autoShowEnabled: true
    property bool hardwareKeyboardPresent: false
    property bool ignoreHardwareKeyboard: false
    property bool inputBackendAvailable: false
    property string inputBackendError: ""
    property bool capsLockActive: false
    property string keyboardMode: "typing"
    property string symbolLayer: "letters"
    property string windowMode: "dockBottom"
    property bool showToolbar: true
    property bool showWindowModeControl: true
    property bool draggingToolbar: false

    signal autoShowToggled(bool enabled)
    signal ignoreHardwareKeyboardToggled(bool ignore)
    signal keyboardModeRequested(string mode)
    signal windowModeRequested(string mode)
    signal floatingDragStarted()
    signal floatingDragMoved(real dx, real dy)
    signal floatingDragFinished()
    signal hideRequested()
    signal keyPressed(string keyId, bool shift, bool ctrl, bool alt, bool meta)
    signal modifierHoldChanged(string keyId, bool active)

    readonly property var typingLetterRows: [
        ["q", "w", "e", "r", "t", "y", "u", "i", "o", "p"],
        ["a", "s", "d", "f", "g", "h", "j", "k", "l"],
        ["z", "x", "c", "v", "b", "n", "m"]
    ]
    readonly property var typingSymbolRows: [
        ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"],
        ["-", "/", ":", ";", "(", ")", "$", "&", "@", "\""],
        [".", ",", "?", "!", "'", "_", "+", "=", "\\"]
    ]
    readonly property var fullRows: [
        ["Esc", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"],
        ["`", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Backspace"],
        ["Tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "\\"],
        ["CapsLock", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "Enter"],
        ["Shift", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "Shift"],
        ["Ctrl", "Meta", "Alt", "Space", "Alt", "Menu", "Left", "Down", "Up", "Right"]
    ]
    readonly property var shiftedLabels: ({
        "`": "~",
        "1": "!",
        "2": "@",
        "3": "#",
        "4": "$",
        "5": "%",
        "6": "^",
        "7": "&",
        "8": "*",
        "9": "(",
        "0": ")",
        "-": "_",
        "=": "+",
        "[": "{",
        "]": "}",
        "\\": "|",
        ";": ":",
        "'": "\"",
        ",": "<",
        ".": ">",
        "/": "?"
    })

    function labelFor(keyId) {
        if (keyId === "Space") {
            return qsTr("Space")
        }
        if (keyId === "Backspace") {
            return qsTr("Bksp")
        }
        if (keyId.length === 1 && keyId >= "a" && keyId <= "z") {
            return root.effectiveShift() !== root.capsLockActive ? keyId.toUpperCase() : keyId
        }
        if (root.effectiveShift() && root.shiftedLabels[keyId] !== undefined) {
            return root.shiftedLabels[keyId]
        }
        return keyId
    }

    function outputKeyFor(keyId) {
        if (keyId === "Space") {
            return " "
        }
        return keyId
    }

    function keyWeight(keyId) {
        if (keyId === "Space") {
            return root.keyboardMode === "full" ? 6 : 8
        }
        if (["Backspace", "CapsLock", "Enter", "Shift"].indexOf(keyId) !== -1) {
            return 1.8
        }
        if (["Tab", "Ctrl", "Alt", "Meta", "Menu"].indexOf(keyId) !== -1) {
            return 1.35
        }
        if (keyId.length > 3) {
            return 1.2
        }
        return 1
    }

    function isModifier(keyId) {
        return ["Shift", "Ctrl", "Alt", "Meta"].indexOf(keyId) !== -1
    }

    function effectiveShift() {
        return root.shift || root.shiftHeld
    }

    function effectiveCtrl() {
        return root.ctrl || root.ctrlHeld
    }

    function effectiveAlt() {
        return root.alt || root.altHeld
    }

    function effectiveMeta() {
        return root.meta || root.metaHeld
    }

    function modifierActive(keyId) {
        if (keyId === "Shift") {
            return root.effectiveShift()
        }
        if (keyId === "Ctrl") {
            return root.effectiveCtrl()
        }
        if (keyId === "Alt") {
            return root.effectiveAlt()
        }
        if (keyId === "Meta") {
            return root.effectiveMeta()
        }
        return false
    }

    function modifierStickyActive(keyId) {
        if (keyId === "Shift") {
            return root.shift
        }
        if (keyId === "Ctrl") {
            return root.ctrl
        }
        if (keyId === "Alt") {
            return root.alt
        }
        if (keyId === "Meta") {
            return root.meta
        }
        return false
    }

    function modifierHeldActive(keyId) {
        if (keyId === "Shift") {
            return root.shiftHeld
        }
        if (keyId === "Ctrl") {
            return root.ctrlHeld
        }
        if (keyId === "Alt") {
            return root.altHeld
        }
        if (keyId === "Meta") {
            return root.metaHeld
        }
        return false
    }

    function setModifierSticky(keyId, active) {
        if (keyId === "Shift") {
            root.shift = active
            return
        }
        if (keyId === "Ctrl") {
            root.ctrl = active
            return
        }
        if (keyId === "Alt") {
            root.alt = active
            return
        }
        if (keyId === "Meta") {
            root.meta = active
        }
    }

    function setModifierHeld(keyId, active) {
        if (keyId === "Shift") {
            root.shiftHeld = active
            return
        }
        if (keyId === "Ctrl") {
            root.ctrlHeld = active
            return
        }
        if (keyId === "Alt") {
            root.altHeld = active
            return
        }
        if (keyId === "Meta") {
            root.metaHeld = active
        }
    }

    function modifierLabel(keyId) {
        if (keyId === "Shift" && root.modifierActive(keyId)) {
            return qsTr("SHIFT")
        }
        return keyId
    }

    function beginModifierHold(keyId) {
        if (!root.isModifier(keyId) || root.modifierHeldActive(keyId)) {
            return
        }
        if (root.modifierStickyActive(keyId)) {
            root.setModifierSticky(keyId, false)
        }
        root.setModifierHeld(keyId, true)
        root.modifierHoldChanged(keyId, true)
    }

    function finishModifierPress(keyId) {
        if (!root.isModifier(keyId)) {
            return
        }
        if (root.modifierHeldActive(keyId)) {
            root.setModifierHeld(keyId, false)
            root.modifierHoldChanged(keyId, false)
            return
        }
        root.setModifierSticky(keyId, !root.modifierStickyActive(keyId))
    }

    function sendKey(keyId) {
        root.keyPressed(outputKeyFor(keyId),
                        root.effectiveShift(),
                        root.effectiveCtrl(),
                        root.effectiveAlt(),
                        root.effectiveMeta())
        releaseStickyModifiers()
    }

    function releaseStickyModifiers() {
        if (root.shift) {
            root.shift = false
        }
        if (root.ctrl) {
            root.ctrl = false
        }
        if (root.alt) {
            root.alt = false
        }
        if (root.meta) {
            root.meta = false
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            id: toolbar
            visible: root.showToolbar
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 36 : 0
            spacing: 10

            DragHandler {
                id: toolbarDrag
                target: null
                enabled: root.windowMode === "float"
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchScreen | PointerDevice.Stylus
                margin: 0

                onActiveChanged: {
                    root.draggingToolbar = active
                    if (active) {
                        root.floatingDragStarted()
                    } else {
                        root.floatingDragFinished()
                    }
                }
                onCentroidChanged: {
                    if (active) {
                        const dx = centroid.position.x - centroid.pressPosition.x
                        const dy = centroid.position.y - centroid.pressPosition.y
                        root.floatingDragMoved(dx, dy)
                    }
                }
            }

            Label {
                text: root.inputBackendAvailable
                    ? (root.hardwareKeyboardPresent && !root.ignoreHardwareKeyboard
                        ? qsTr("Hardware keyboard connected")
                        : qsTr("KDE OSK"))
                    : qsTr("Input backend unavailable")
                color: "#f1f3f4"
                font.pixelSize: 14
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            ButtonGroup {
                id: keyboardModeGroup
            }

            RowLayout {
                spacing: 4

                ToolButton {
                    text: qsTr("Input")
                    checkable: true
                    checked: root.keyboardMode === "typing"
                    ButtonGroup.group: keyboardModeGroup
                    onClicked: root.keyboardModeRequested("typing")
                }

                ToolButton {
                    text: qsTr("Full")
                    checkable: true
                    checked: root.keyboardMode === "full"
                    ButtonGroup.group: keyboardModeGroup
                    onClicked: root.keyboardModeRequested("full")
                }
            }

            ComboBox {
                visible: root.showWindowModeControl
                Layout.preferredWidth: visible ? 126 : 0
                textRole: "label"
                valueRole: "value"
                model: [
                    { label: qsTr("Bottom"), value: "dockBottom" },
                    { label: qsTr("Top"), value: "dockTop" },
                    { label: qsTr("Float"), value: "float" }
                ]
                currentIndex: root.windowMode === "dockTop" ? 1 : root.windowMode === "float" ? 2 : 0
                onActivated: root.windowModeRequested(currentValue)
            }

            ToolButton {
                text: qsTr("Auto")
                checkable: true
                checked: root.autoShowEnabled
                onClicked: root.autoShowToggled(checked)
            }

            ToolButton {
                text: qsTr("Force")
                checkable: true
                checked: root.ignoreHardwareKeyboard
                onClicked: root.ignoreHardwareKeyboardToggled(checked)
            }

            Button {
                text: qsTr("Hide")
                onClicked: root.hideRequested()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !root.inputBackendAvailable && root.inputBackendError.length > 0
            text: root.inputBackendError
            color: "#ffb4ab"
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.keyboardMode === "typing"
            spacing: 7

            Repeater {
                model: root.symbolLayer === "letters" ? root.typingLetterRows : root.typingSymbolRows

                RowLayout {
                    required property int index
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 6

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredWidth: parent.index === 1 ? 0.5 : parent.index === 2 ? 1.1 : 0
                    }

                    Repeater {
                        model: parent.modelData

                        KeyButton {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            keyId: modelData
                            text: root.labelFor(modelData)
                            onKeyTriggered: keyId => root.sendKey(keyId)
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredWidth: parent.index === 1 ? 0.5 : parent.index === 2 ? 1.1 : 0
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 58
                spacing: 6

                KeyButton {
                    Layout.preferredWidth: 88
                    Layout.fillHeight: true
                    keyId: "Shift"
                    repeatable: false
                    triggerOnPress: false
                    longPressable: true
                    longPressDelay: root.modifierHoldThreshold
                    text: root.modifierLabel("Shift")
                    checked: root.modifierActive("Shift")
                    onLongPressTriggered: keyId => root.beginModifierHold(keyId)
                    onPressEnded: keyId => root.finishModifierPress(keyId)
                }

                KeyButton {
                    Layout.preferredWidth: 78
                    Layout.fillHeight: true
                    keyId: "Layer"
                    repeatable: false
                    text: root.symbolLayer === "letters" ? qsTr("123") : qsTr("ABC")
                    onKeyTriggered: root.symbolLayer = root.symbolLayer === "letters" ? "symbols" : "letters"
                }

                KeyButton {
                    Layout.preferredWidth: 80
                    Layout.fillHeight: true
                    keyId: "Tab"
                    text: qsTr("Tab")
                    onKeyTriggered: root.sendKey("Tab")
                }

                KeyButton {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    keyId: "Space"
                    text: qsTr("Space")
                    onKeyTriggered: root.sendKey("Space")
                }

                KeyButton {
                    Layout.preferredWidth: 96
                    Layout.fillHeight: true
                    keyId: "Enter"
                    text: qsTr("Enter")
                    onKeyTriggered: root.sendKey("Enter")
                }

                KeyButton {
                    Layout.preferredWidth: 88
                    Layout.fillHeight: true
                    keyId: "Backspace"
                    text: qsTr("Bksp")
                    onKeyTriggered: root.sendKey("Backspace")
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.keyboardMode === "full"
            spacing: 6

            Repeater {
                model: root.fullRows

                RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 5

                    Repeater {
                        model: parent.modelData

                        KeyButton {
                            readonly property bool modifierKey: root.isModifier(modelData)

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: root.keyWeight(modelData)
                            keyId: modelData
                            repeatable: !modifierKey
                            triggerOnPress: !modifierKey
                            longPressable: modifierKey
                            longPressDelay: root.modifierHoldThreshold
                            text: root.labelFor(modelData)
                            checked: root.modifierActive(modelData)
                                || (modelData === "CapsLock" && root.capsLockActive)
                            font.pixelSize: modelData.length > 6 ? 13 : 16
                            onKeyTriggered: keyId => root.sendKey(keyId)
                            onLongPressTriggered: keyId => root.beginModifierHold(keyId)
                            onPressEnded: keyId => root.finishModifierPress(keyId)
                        }
                    }
                }
            }
        }
    }
}
