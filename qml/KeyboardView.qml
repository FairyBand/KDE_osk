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
    property bool autoShowEnabled: true
    property bool hardwareKeyboardPresent: false
    property bool ignoreHardwareKeyboard: false
    property bool inputBackendAvailable: false
    property string inputBackendError: ""
    property string keyboardMode: "typing"
    property string symbolLayer: "letters"
    property string windowMode: "dockBottom"
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
    signal modifierChanged(string keyId, bool active)

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

    function labelFor(keyId) {
        if (keyId === "Space") {
            return qsTr("Space")
        }
        if (keyId === "Backspace") {
            return qsTr("Bksp")
        }
        if (keyId.length === 1 && keyId >= "a" && keyId <= "z") {
            return root.shift ? keyId.toUpperCase() : keyId
        }
        return keyId
    }

    function outputKeyFor(keyId) {
        if (keyId === "Space") {
            return " "
        }
        if (keyId.length === 1 && keyId >= "a" && keyId <= "z") {
            return root.shift ? keyId.toUpperCase() : keyId
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

    function toggleModifier(keyId) {
        if (keyId === "Shift") {
            root.shift = !root.shift
            root.modifierChanged("Shift", root.shift)
            return true
        }
        if (keyId === "Ctrl") {
            root.ctrl = !root.ctrl
            root.modifierChanged("Ctrl", root.ctrl)
            return true
        }
        if (keyId === "Alt") {
            root.alt = !root.alt
            root.modifierChanged("Alt", root.alt)
            return true
        }
        if (keyId === "Meta") {
            root.meta = !root.meta
            root.modifierChanged("Meta", root.meta)
            return true
        }
        return false
    }

    function sendKey(keyId) {
        if (toggleModifier(keyId)) {
            return
        }
        root.keyPressed(outputKeyFor(keyId), false, false, false, false)
        releaseActiveModifiers()
    }

    function releaseActiveModifiers() {
        if (root.shift) {
            root.shift = false
            root.modifierChanged("Shift", false)
        }
        if (root.ctrl) {
            root.ctrl = false
            root.modifierChanged("Ctrl", false)
        }
        if (root.alt) {
            root.alt = false
            root.modifierChanged("Alt", false)
        }
        if (root.meta) {
            root.meta = false
            root.modifierChanged("Meta", false)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            id: toolbar
            Layout.fillWidth: true
            Layout.preferredHeight: 36
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
                Layout.preferredWidth: 126
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
                    text: root.shift ? qsTr("SHIFT") : qsTr("Shift")
                    checked: root.shift
                    onKeyTriggered: root.sendKey("Shift")
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
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: root.keyWeight(modelData)
                            keyId: modelData
                            repeatable: ["Shift", "Ctrl", "Alt", "Meta"].indexOf(modelData) === -1
                            text: root.labelFor(modelData)
                            checked: (modelData === "Shift" && root.shift)
                                || (modelData === "Ctrl" && root.ctrl)
                                || (modelData === "Alt" && root.alt)
                                || (modelData === "Meta" && root.meta)
                            font.pixelSize: modelData.length > 6 ? 13 : 16
                            onKeyTriggered: keyId => root.sendKey(keyId)
                        }
                    }
                }
            }
        }
    }
}
