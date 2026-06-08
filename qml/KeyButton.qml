import QtQuick
import QtQuick.Controls

Button {
    id: root

    font.pixelSize: 20
    font.weight: Font.Medium

    background: Rectangle {
        radius: 8
        color: root.down ? "#d8dee9" : root.hovered ? "#eef1f5" : "#f8fafd"
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
