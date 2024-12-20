import QtQuick 2.0
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12

import "./"
import "../"
import "../../qml"

BasicMenuStyle {
    id: root
    title: qsTr("Change Type")
    contentWidth : 260 * screenScaleFactor

    property var model: ["Part", "Negative Part", "Modifier"]
    // enabled: kernel_model_selector.selectedModelObjects > 0

    property var currentType: -1

    onVisibleChanged: {
        if (visible) {
            var selectedModels = kernel_model_selector.selectedModelObjects
            if (selectedModels.length > 0) {
                currentType = selectedModels[0].modelNType
            } else {
                currentType = -1
            }
        }
    }

    signal extruderChanged(int index);

    BasicMenuItemStyle {
        property int type: 0
        text: qsTr(root.model[type])
        
        Rectangle {
            visible: root.currentType == parent.type
            anchors.left: parent.left
            anchors.leftMargin: 12 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            width: 14 * screenScaleFactor
            height: 14 * screenScaleFactor
            radius: 7 * screenScaleFactor
            border.color: "#333333"
            border.width: 1
            color: "transparent"
            
            Rectangle {
                anchors.centerIn: parent
                width: 8 * screenScaleFactor
                height: 8 * screenScaleFactor
                radius: 4 * screenScaleFactor
                color: Constants.themeGreenColor
            }
        }

        onTriggered: {
            var action = kernel_rclick_menu_data.getActionObject("change type")
            action.setModelType(type);
            action.execute();
        }
    }

    BasicMenuItemStyle {
        property int type: 1
        text: qsTr(root.model[type])
        
        Rectangle {
            visible: root.currentType == parent.type
            anchors.left: parent.left
            anchors.leftMargin: 12 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            width: 14 * screenScaleFactor
            height: 14 * screenScaleFactor
            radius: 7 * screenScaleFactor
            border.color: "#333333"
            border.width: 1
            color: "transparent"
            
            Rectangle {
                anchors.centerIn: parent
                width: 8 * screenScaleFactor
                height: 8 * screenScaleFactor
                radius: 4 * screenScaleFactor
                color: Constants.themeGreenColor
            }
        }

        onTriggered: {
            var action = kernel_rclick_menu_data.getActionObject("change type")
            action.setModelType(type);
            action.execute();
        }
    }

    BasicMenuItemStyle {
        property int type: 2
        text: qsTr(root.model[type])
        
        Rectangle {
            visible: root.currentType == parent.type
            anchors.left: parent.left
            anchors.leftMargin: 12 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            width: 14 * screenScaleFactor
            height: 14 * screenScaleFactor
            radius: 7 * screenScaleFactor
            border.color: "#333333"
            border.width: 1
            color: "transparent"
            
            Rectangle {
                anchors.centerIn: parent
                width: 8 * screenScaleFactor
                height: 8 * screenScaleFactor
                radius: 4 * screenScaleFactor
                color: Constants.themeGreenColor
            }
        }

        onTriggered: {
            var action = kernel_rclick_menu_data.getActionObject("change type")
            action.setModelType(type);
            action.execute();
        }
    }

    background: Rectangle {
        implicitWidth:maxImplicitWidth + 20
        color: "white"/*Constants.itemBackgroundColor*///"#061F3B"
        border.width: 1
        border.color: "transparent "
        Rectangle
        {
            id: mainLayout2
            anchors.fill: parent
            anchors.margins: 5
            color: Constants.themeColor
            opacity: 1
        }
	
        DropShadow {
            anchors.fill: mainLayout2
            horizontalOffset: 5
            verticalOffset: 5
            radius: 8
            samples: 17
            source: mainLayout
            color: Constants.dropShadowColor // "#333333"
        }
    }

}
