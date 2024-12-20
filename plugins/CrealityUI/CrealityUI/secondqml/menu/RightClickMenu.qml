import QtQuick 2.13
import QtQuick.Controls 2.13
import QtGraphicalEffects 1.13

import "./"
import "../"
import "../../qml"

Menu {
    id: root
    // topPadding: -4
    // implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
    //                         contentWidth + leftPadding + rightPadding)
    // implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
    //                          contentHeight + topPadding + bottomPadding)
    // contentWidth : 205 * screenScaleFactor
    // width: contentWidth

    property int cWidth: 205 * screenScaleFactor
    property var myItemObj
    property var separator
    property alias mymodel: idMenuItems.model

    property var command

    property bool highlightSeparator: false
    property bool hasIcon: true
    property bool centerText: false

    delegate: RightClickMenuItem
    {
        // width: root.cWidth  + 10 * screenScaleFactor
        actionCmdItem: subMenu.command
        enabled: subMenu.command ? subMenu.command.enalbe : true
        highlightSeparator: subMenu.highlightSeparator
        hasIcon: subMenu.hasIcon
        centerText: subMenu.centerText
    }

  
        Repeater
        {
            id : idMenuItems

            delegate: RightClickMenuItem
            {
                actionCmdItem: actionItem
            }
        }
 

    background: Rectangle {
        implicitWidth: root.cWidth + 10 * screenScaleFactor
        Rectangle {
            id: mainLayout
            anchors.fill: parent
            anchors.margins: 5
            color:"red" // Constants.menuStyleBgColor
            opacity: 1
        }

        DropShadow {
            anchors.fill: mainLayout
            horizontalOffset: 3
            verticalOffset: 3
            //radius: 8
            samples:9// 17
            source: mainLayout
            color: Constants.dropShadowColor
        }
    }
}

