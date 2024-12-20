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

    // 编辑文字
    RightClickMenuItem {
        id: idReliefItem
        actionCmdItem: kernel_rclick_menu_data.getActionObject("relief")
    }

    // 删除
    RightClickMenuItem {
        id: idDeleteItem
        actionCmdItem: kernel_rclick_menu_data.getActionObject("delete")
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

    ReliefTypeSubMenu {
        id: idModelTypeMenu
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
