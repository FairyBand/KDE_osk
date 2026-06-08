import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Rectangle {
    id: root

    implicitHeight: Math.min(360, Math.max(250, Screen.height * 0.32))
    color: "#202124"

    property bool shift: false
    property bool autoShowEnabled: true
    property bool hardwareKeyboardPresent: false

    signal autoShowToggled(bool enabled)
    signal hideRequested()
    signal keyPressed(string text)

    readonly property var rows: [
        ["q", "w", "e", "r", "t", "y", "u", "i", "o", "p"],
        ["a", "s", "d", "f", "g", "h", "j", "k", "l"],
        ["z", "x", "c", "v", "b", "n", "m"]
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            spacing: 10

            Label {
                text: root.hardwareKeyboardPresent
                    ? qsTr("Hardware keyboard connected")
                    : qsTr("Touch keyboard")
                color: "#f1f3f4"
                font.pixelSize: 14
                Layout.fillWidth: true
            }

            Switch {
                id: autoShowSwitch
                checked: root.autoShowEnabled
                text: qsTr("Auto")
                onToggled: root.autoShowToggled(checked)
            }

            Button {
                text: qsTr("Hide")
                onClicked: root.hideRequested()
            }
        }

        Repeater {
            model: root.rows

            RowLayout {
                required property int index
                required property var modelData

                Layout.fillWidth: true
                Layout.preferredHeight: 54
                spacing: 6

                Item {
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.index === 1 ? 0.5 : parent.index === 2 ? 1.1 : 0
                }

                Repeater {
                    model: parent.modelData

                    KeyButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 54
                        text: root.shift ? modelData.toUpperCase() : modelData
                        onClicked: root.keyPressed(text)
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
                Layout.preferredHeight: 58
                text: root.shift ? qsTr("Shift") : qsTr("shift")
                onClicked: root.shift = !root.shift
            }

            KeyButton {
                Layout.preferredWidth: 78
                Layout.preferredHeight: 58
                text: qsTr("123")
            }

            KeyButton {
                Layout.fillWidth: true
                Layout.preferredHeight: 58
                text: qsTr("Space")
                onClicked: root.keyPressed(" ")
            }

            KeyButton {
                Layout.preferredWidth: 96
                Layout.preferredHeight: 58
                text: qsTr("Enter")
                onClicked: root.keyPressed("\n")
            }

            KeyButton {
                Layout.preferredWidth: 88
                Layout.preferredHeight: 58
                text: qsTr("Back")
                onClicked: root.keyPressed("\b")
            }
        }
    }
}
