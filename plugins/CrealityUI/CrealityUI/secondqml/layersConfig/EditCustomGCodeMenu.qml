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

    signal requestGCodeDialog()
    signal requestDeleteGCode()

    BasicMenuItemStyle {
        text:  qsTr("Edit Custom G-code") 
        actionShortcut: ""
        enabled: true
        separatorVisible: false
        hasIcon: false
        centerText: true

        onTriggered: {
            requestGCodeDialog()
        }
    }

    BasicMenuItemStyle {
        text:  qsTr("Delete Custom G-code") 
        actionShortcut: ""
        enabled: true
        separatorVisible: false
        hasIcon: false
        centerText: true

        onTriggered: {
            requestDeleteGCode()
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