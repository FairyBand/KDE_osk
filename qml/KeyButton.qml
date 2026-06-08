import QtQuick
import QtQuick.Controls

Button {
    id: root

    checkable: false
    property string keyId: text
    property bool repeatable: true
    property int repeatDelay: 420
    property int repeatInterval: 34
    signal keyTriggered(string keyId)

    font.pixelSize: 20
    font.weight: Font.Medium

    onPressedChanged: {
        if (pressed) {
            keyTriggered(keyId)
            if (repeatable) {
                repeatDelayTimer.restart()
            }
        } else {
            repeatDelayTimer.stop()
            repeatTimer.stop()
        }
    }

    Timer {
        id: repeatDelayTimer
        interval: root.repeatDelay
        repeat: false
        onTriggered: {
            if (root.pressed && root.repeatable) {
                repeatTimer.start()
            }
        }
    }

    Timer {
        id: repeatTimer
        interval: root.repeatInterval
        repeat: true
        onTriggered: {
            if (root.pressed && root.repeatable) {
                root.keyTriggered(root.keyId)
            } else {
                stop()
            }
        }
    }

    background: Rectangle {
        radius: 8
        color: root.checked ? "#9ccaff" : root.down ? "#d8dee9" : root.hovered ? "#eef1f5" : "#f8fafd"
        border.color: "#c9d0d8"
        border.width: 1
    }

    contentItem: Text {
        text: root.text
        color: "#111318"
        font: root.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
