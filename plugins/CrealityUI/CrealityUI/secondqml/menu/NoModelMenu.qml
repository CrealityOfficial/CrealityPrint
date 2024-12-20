import QtQuick 2.13
import QtQuick.Controls 2.13
import QtGraphicalEffects 1.13

import "./"
import "../"
import "../../qml"

BasicMenuStyle
{
    id: root
    contentWidth : 260 * screenScaleFactor

    // 全选
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("select all")
    }

    // 删除所有模型
    RightClickMenuItem {
        iconWidth: 13 * screenScaleFactor
        iconHeight: 14 * screenScaleFactor
        actionCmdItem: kernel_rclick_menu_data.getActionObject("clear all")
    }

    // 自动朝向
    RightClickMenuItem {
        iconWidth: 15 * screenScaleFactor
        iconHeight: 19 * screenScaleFactor
        actionCmdItem: kernel_rclick_menu_data.getActionObject("pick bottom")
    }

    // 自动布局
    RightClickMenuItem {
        iconWidth: 14 * screenScaleFactor
        iconHeight: 12 * screenScaleFactor
        actionCmdItem: kernel_rclick_menu_data.getActionObject("layout")
    }

    // 删除盘
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("delete plate")
    }

    // 锁定
    RightClickMenuItem {
        iconWidth: 12 * screenScaleFactor
        iconHeight: 14 * screenScaleFactor
        actionCmdItem: kernel_rclick_menu_data.getActionObject("lock plate")
    }

    // 导入模型
    RightClickMenuItem {
        property int index: 3
        actionCmdItem: kernel_rclick_menu_data.getActionObject("import model")
        highlightSeparator: true
    }

    // 历史模型
    BasicSubMenu {
        separator: false
        property var command: kernel_rclick_menu_data.getSubMenuObject("recent files")
        title: command ? qsTr(command.name) : ""
        mymodel: command.subMenuActionmodel
    } 

    // 测试模型    
    BasicDoubleSubMenu {
        separator: false
        property var command: kernel_rclick_menu_data.getSubMenuObject("test models")
        title: command ? qsTr(command.name) : ""
        mymodel: actionCommands.getOpt("Standard Model").subMenuActionmodel
        mysecondmodel: command.subMenuActionmodel
    }   
    
    // 设置对象耗材丝
    FilamentSubMenu {
        onExtruderChanged: {
            kernel_kernel.changeMaterial(index)
        }
    }

    background: Rectangle {
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
