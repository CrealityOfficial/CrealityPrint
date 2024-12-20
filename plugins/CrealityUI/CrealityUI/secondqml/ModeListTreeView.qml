import QtQuick 2.9
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQml.Models 2.2
import QtQuick.Controls 2.12

TreeView {
    id:controll
    //anchors.fill: parent
    property alias vpolicy: verticalBar.policy
    ScrollBar {
        id: verticalBar
        hoverEnabled: true
        active: hovered || pressed
        orientation: Qt.Vertical
        size: controll.height / controll.flickableItem.contentHeight
        visible: true//controll.__verticalScrollBar.visible
        width: visible ? 10 : 0
        height: controll.height
        anchors.top: controll.top
        anchors.right: controll.right
        policy: ScrollBar.AlwaysOn
        onPositionChanged: {
            controll.flickableItem.contentY = position * (controll.flickableItem.contentHeight)
        }
    }

    ScrollBar {
        id: horizonBar
        hoverEnabled: true
        active: hovered || pressed
        orientation: Qt.Horizontal
        size: controll.width / controll.flickableItem.contentWidth
        visible: controll.__horizontalScrollBar.visible
        width: controll.width - verticalBar.width
        height: visible ? 10 : 0
        anchors.bottom: controll.bottom
        anchors.left: controll.left
        policy: ScrollBar.AlwaysOn
        onPositionChanged: {
            controll.flickableItem.contentX = position * (controll.flickableItem.contentWidth)
        }
    }

    Connections{
        target: controll.flickableItem
        onContentXChanged: {
            horizonBar.position = controll.flickableItem.contentX /
            controll.flickableItem.contentWidth
        }
    }

    Connections {
        target: controll.flickableItem
        onContentYChanged: {
            verticalBar.position = controll.flickableItem.contentY /
                controll.flickableItem.contentHeight
        }
    }
}