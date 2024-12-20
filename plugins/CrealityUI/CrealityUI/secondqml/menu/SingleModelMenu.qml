import QtQuick 2.13
import QtQuick.Controls 2.13
import QtGraphicalEffects 1.13

import "./"
import "../"
import "../../qml"

RightClickMenu
{
    id: root
    // highlightSeparator: true
    contentWidth : 260 * screenScaleFactor


    // 铺满打印板
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("cover model")
    }

    // 克隆
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("clone")
    }

    // 居中
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("center")
    }

    // 修复
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("repair model")
    }

    // 删除
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("delete")
    }

    // 自动布局
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("layout")
    }

    // 拆分模型
    RightClickMenu {
        // command: { name: qsTr("Split") }
        title: qsTr("Split")
        contentWidth : 260 * screenScaleFactor

        RightClickMenuItem {
            actionCmdItem: kernel_rclick_menu_data.getActionObject("split to objects")
        }

        RightClickMenuItem {
            actionCmdItem: kernel_rclick_menu_data.getActionObject("split to parts")
        }

    }

    // 平移
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("move")
        highlightSeparator: true
    }

    // 旋转
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("rotate")
    }

    // 缩放
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("scale")
    }

    // 添加零件
    RightClickMenu {
        highlightSeparator: true
        command: kernel_rclick_menu_data.getSubMenuObject("add part")
        // title: command ? qsTr(command.name) : ""
         Component.onCompleted: function() {
            mymodel = command.subMenuActionmodel
        }
    } 

    // 添加负零件
    RightClickMenu {
        command: kernel_rclick_menu_data.getSubMenuObject("add negative part")
        title: command ? qsTr(command.name) : ""
        Component.onCompleted: function() {
            mymodel = command.subMenuActionmodel
        }
        
    } 

    // 添加修改器
    RightClickMenu {
        command: kernel_rclick_menu_data.getSubMenuObject("add modifier")
        title: command ? qsTr(command.name) : ""
        Component.onCompleted: function() {
            mymodel = command.subMenuActionmodel
        }
    } 

    // 添加支撑屏蔽器
    RightClickMenu {
        command: kernel_rclick_menu_data.getSubMenuObject("add support blocker")
        title: command ? qsTr(command.name) : ""
        Component.onCompleted: function() {
            mymodel = command.subMenuActionmodel
        }
    } 

    // 添加支撑添加器
    RightClickMenu {
        command: kernel_rclick_menu_data.getSubMenuObject("add support enforcer")
        title: command ? qsTr(command.name) : ""
        Component.onCompleted: function() {
            mymodel = command.subMenuActionmodel
        }
    } 

    // 换料冲刷选项
    FlushSubMenu {
        highlightSeparator: true
    }

    // 对象参数设置
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("model setting")
    }

    // // 设置对象耗材丝
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
