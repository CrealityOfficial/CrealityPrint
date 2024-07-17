import QtQuick 2.13
import QtQuick.Controls 2.13
import QtGraphicalEffects 1.13

import "./"
import "../"
import "../../qml"
import "../menu"

BasicMenuStyle
{
    id: root
    contentWidth : 163 * screenScaleFactor
    hasIcon: false
    centerText: true

    signal requestDeletePause()

    BasicMenuItemStyle {
        text:  qsTr("Delete Pause") //删除暂停打印
        actionShortcut: ""
        enabled: true
        separatorVisible: false
        hasIcon: false
        centerText: true

        onTriggered: {
            requestDeletePause()
        }
    }
    
    background: Rectangle {
        implicitWidth:maxImplicitWidth + 20
        color: Constants.itemBackgroundColor//"#061F3B"
        border.width: 1
        border.color: "black"
        radius: 5

        Rectangle
        {
            id: mainLayout
            anchors.fill: parent
            anchors.margins: 5
            color: Constants.themeColor
            opacity: 1
        }

        DropShadow {
            anchors.fill: mainLayout
            horizontalOffset: 5
            verticalOffset: 5
            radius: 8
            samples: 17
            source: mainLayout
            color: Constants.dropShadowColor // "#333333"
        }
    }
}