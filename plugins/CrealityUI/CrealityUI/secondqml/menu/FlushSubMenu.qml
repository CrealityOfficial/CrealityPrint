import QtQuick 2.0
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12

import "./"
import "../"
import "../../qml"

RightClickMenu {
    id: root
    title: qsTr("Flush Options")
    contentWidth : 260 * screenScaleFactor

    property var settingModel: []
    property var model

    onVisibleChanged: {
        settingModel = []
        var models = kernel_model_selector.selectedModelGroupObjects
        if (models.length <= 0)
            return
        
        model = models[0]

        var infill = { name: qsTr("Flush into objects' infill"), enabled: model.flushIntoInfill}
        var object = { name: qsTr("Flush into this object"), enabled: model.flushIntoObjects}
        var support = { name: qsTr("Flush into objects' support"), enabled: model.flushIntoSupport}

        settingModel = [infill, object, support]
    }

    Repeater
    {
        id : idMenuItems
        model: settingModel

        delegate: BasicMenuItemStyle {
            text: modelData.name

            Rectangle {
                visible: modelData.enabled
                anchors.left: parent.left
                anchors.leftMargin: 12 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                width: 20 * screenScaleFactor
                height: 20 * screenScaleFactor
                color: "#17CC5F"

                Image {
                    anchors.centerIn: parent
                    source: "qrc:/UI/images/check3.png"
                }
            }

            onTriggered: {
                if (index === 0)
                    root.model.flushIntoInfill = !modelData.enabled
                else if (index === 1)
                    root.model.flushIntoObjects = !modelData.enabled
                else if (index === 2)
                    root.model.flushIntoSupport = !modelData.enabled
            }
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
