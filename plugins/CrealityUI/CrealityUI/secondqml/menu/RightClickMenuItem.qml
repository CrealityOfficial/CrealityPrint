import QtQuick 2.13
import QtQuick.Controls 2.13
import QtGraphicalEffects 1.13

import "./"
import "../"
import "../../qml"

BasicMenuItemStyle
{
    id : root

    property var actionIconSource
    property var  actionCmdItem
    property int iconHeight: 14 * screenScaleFactor
    property int iconWidth: 14 * screenScaleFactor

    text: actionCmdItem ? actionCmdItem.name : ""
    actionShortcut: actionCmdItem ? actionCmdItem.shortcut : ""
    enabled: actionCmdItem ? actionCmdItem.enabled : false
    separatorVisible: actionCmdItem ? actionCmdItem.bSeparator : false

    signal loadSouce(string title,var commd)

    Image {
        anchors.left: parent.left
        anchors.leftMargin: 12 * screenScaleFactor
        anchors.verticalCenter: parent.verticalCenter
        width: iconWidth
        height: iconHeight
        source: parent.hovered ? actionCmdItem.icon : actionCmdItem.source
    }

    onTriggered: {
        if(actionCmdItem){
            Qt.callLater(actionCmdItem.execute)
        }
    }
    
}
