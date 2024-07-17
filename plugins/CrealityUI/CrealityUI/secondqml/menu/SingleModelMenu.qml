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

    // 铺满打印板
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("cover model")
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
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("split model")
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

    // 换料冲刷选项
    FlushSubMenu {
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
