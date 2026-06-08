import QtQuick
import QtQuick.Controls

Button {
    id: root

    checkable: false
    property string keyId: text
    property bool repeatable: true
    property bool triggerOnPress: true
    property bool longPressable: false
    property int longPressDelay: 200
    property int repeatDelay: 420
    property int repeatInterval: 34
    signal keyTriggered(string keyId)
    signal longPressTriggered(string keyId)
    signal pressEnded(string keyId)

    font.pixelSize: 20
    font.weight: Font.Medium

    onPressedChanged: {
        if (pressed) {
            if (longPressable) {
                longPressTimer.restart()
            }
            if (triggerOnPress) {
                keyTriggered(keyId)
            }
            if (triggerOnPress && repeatable) {
                repeatDelayTimer.restart()
            }
        } else {
            longPressTimer.stop()
            repeatDelayTimer.stop()
            repeatTimer.stop()
            pressEnded(keyId)
        }
    }

    Timer {
        id: longPressTimer
        interval: root.longPressDelay
        repeat: false
        onTriggered: {
            if (root.pressed && root.longPressable) {
                root.longPressTriggered(root.keyId)
            }
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
