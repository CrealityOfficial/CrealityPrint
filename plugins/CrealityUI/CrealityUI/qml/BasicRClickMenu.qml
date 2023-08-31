import QtQuick 2.13
import QtQuick.Controls 2.13
import QtGraphicalEffects 1.13

import "./"
import "../secondqml"

BasicMenuStyle
{
    id: root
    property int maxImplicitWidth: 200
    property var menuItems
    property var nozzleObj
    property var uploadObj

    Repeater {
        model: menuItems
        delegate: BasicMenuItem {
            actionCmdItem: modelData
            actionName: modelData.name
            actionShortcut: modelData.shortcut
            enabled: modelData.enabled
            icon.source: modelData.icon
            separatorVisible: modelData.bSeparator
            actionSource: modelData.source
        }
    }

    BasicRClickSubMenu {
        myItemObj: nozzleObj
        title: nozzleObj.name
        mymodel: nozzleObj.subMenuActionmodel
        enabled: nozzleObj.enabled
        separator: nozzleObj.Separator
    }

    BasicRClickSubMenu {
        id: upload_sub_menu
        myItemObj: uploadObj
        title: uploadObj.name
        mymodel: uploadObj.subMenuActionmodel
        enabled: uploadObj.enabled
        separator: uploadObj.Separator
    }

    background: Rectangle {
        implicitWidth:maxImplicitWidth + 20
        color: Constants.itemBackgroundColor//"#061F3B"
        border.width: 1
        border.color: "black"

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

    Component.onCompleted:{
        maxImplicitWidth = Math.max(implicitContentWidth, maxImplicitWidth)
        if (!kernel_global_const.cxcloudEnabled) {
            removeMenu(upload_sub_menu)
        }
    }
}
