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

    onVisibleChanged: {
        if (visible) {
            var selectedModels = kernel_model_selector.selectedModelObjects
            if (selectedModels.length > 0) {
                let model = selectedModels[0]
                let group = model.getModelGroupObject()
                let modelCount = group.modelObjects(0).length
                let isSingleModelGroup = (modelCount <= 1)
                if (model.modelNType != 0) {
                    idModelTypeMenu.enabled = true
                    idDeleteItem.enabled = true
                } else {
                    idModelTypeMenu.enabled = !isSingleModelGroup
                    idDeleteItem.enabled = !isSingleModelGroup
                }
            } else {
                idModelTypeMenu.enabled = false
                idDeleteItem.enabled = false
            }
        }
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

    // 更改类型
    ModelTypeSubMenu {
        id: idModelTypeMenu
    }

    // 对象参数设置
    RightClickMenuItem {
        actionCmdItem: kernel_rclick_menu_data.getActionObject("model setting")
    }

    // 设置对象耗材丝
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
