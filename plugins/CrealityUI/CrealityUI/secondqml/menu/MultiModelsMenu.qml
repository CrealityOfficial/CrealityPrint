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
    highlightSeparator: true

    // 组合
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("assemble")
    }

    // 合并模型位置
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("reset model location")
    }

    // 克隆
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("clone")
    }

    // 删除
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("delete")
    }

    // 自动布局
    RightClickMenuItem {
        iconWidth: 14 * screenScaleFactor
        iconHeight: 13 * screenScaleFactor
        actionCmdItem: kernel_rclick_menu_data.getActionObject("layout")
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

    // 对象参数设置
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("model setting")
        highlightSeparator: true
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
