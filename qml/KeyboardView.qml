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

    signal autoShowToggled(bool enabled)
    signal ignoreHardwareKeyboardToggled(bool ignore)
    signal windowModeRequested(string mode)
    signal floatingMoveRequested(real dx, real dy)
    signal hideRequested()
    signal keyPressed(string keyId, bool shift, bool ctrl, bool alt, bool meta)

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
            return qsTr("Back")
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
            return true
        }
        if (keyId === "Ctrl") {
            root.ctrl = !root.ctrl
            return true
        }
        if (keyId === "Alt") {
            root.alt = !root.alt
            return true
        }
        if (keyId === "Meta") {
            root.keyPressed("Meta", false, false, false, false)
            return true
        }
        return false
    }

    function sendKey(keyId) {
        if (toggleModifier(keyId)) {
            return
        }
        root.keyPressed(outputKeyFor(keyId), root.shift, root.ctrl, root.alt, root.meta)
        if (root.shift || root.ctrl || root.alt || root.meta) {
            root.shift = false
            root.ctrl = false
            root.alt = false
            root.meta = false
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            spacing: 10

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

            Item {
                Layout.preferredWidth: 38
                Layout.fillHeight: true
                visible: root.windowMode === "float"

                Rectangle {
                    anchors.centerIn: parent
                    width: 26
                    height: 18
                    radius: 4
                    color: dragHandle.pressed ? "#9ccaff" : "#3a3f46"
                    border.color: "#6b7280"
                }

                MouseArea {
                    id: dragHandle
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.SizeAllCursor
                }

                DragHandler {
                    id: floatDragHandler
                    target: null
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchScreen | PointerDevice.Stylus
                    property real previousX: 0
                    property real previousY: 0

                    onActiveChanged: {
                        previousX = 0
                        previousY = 0
                    }
                    onActiveTranslationChanged: {
                        root.floatingMoveRequested(activeTranslation.x - previousX,
                                                   activeTranslation.y - previousY)
                        previousX = activeTranslation.x
                        previousY = activeTranslation.y
                    }
                }
            }

            RowLayout {
                spacing: 4

                ToolButton {
                    text: qsTr("Input")
                    checkable: true
                    checked: root.keyboardMode === "typing"
                    ButtonGroup.group: keyboardModeGroup
                    onClicked: root.keyboardMode = "typing"
                }

                ToolButton {
                    text: qsTr("Full")
                    checkable: true
                    checked: root.keyboardMode === "full"
                    ButtonGroup.group: keyboardModeGroup
                    onClicked: root.keyboardMode = "full"
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
                            text: root.labelFor(modelData)
                            onClicked: root.sendKey(modelData)
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
                    text: root.shift ? qsTr("SHIFT") : qsTr("Shift")
                    checked: root.shift
                    onClicked: root.shift = !root.shift
                }

                KeyButton {
                    Layout.preferredWidth: 78
                    Layout.fillHeight: true
                    text: root.symbolLayer === "letters" ? qsTr("123") : qsTr("ABC")
                    onClicked: root.symbolLayer = root.symbolLayer === "letters" ? "symbols" : "letters"
                }

                KeyButton {
                    Layout.preferredWidth: 80
                    Layout.fillHeight: true
                    text: qsTr("Tab")
                    onClicked: root.sendKey("Tab")
                }

                KeyButton {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: qsTr("Space")
                    onClicked: root.sendKey("Space")
                }

                KeyButton {
                    Layout.preferredWidth: 96
                    Layout.fillHeight: true
                    text: qsTr("Enter")
                    onClicked: root.sendKey("Enter")
                }

                KeyButton {
                    Layout.preferredWidth: 88
                    Layout.fillHeight: true
                    text: qsTr("Back")
                    onClicked: root.sendKey("Backspace")
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
                            text: root.labelFor(modelData)
                            checked: (modelData === "Shift" && root.shift)
                                || (modelData === "Ctrl" && root.ctrl)
                                || (modelData === "Alt" && root.alt)
                                || (modelData === "Meta" && root.meta)
                            font.pixelSize: modelData.length > 6 ? 13 : 16
                            onClicked: root.sendKey(modelData)
                        }
                    }
                }
            }
        }
    }
}
