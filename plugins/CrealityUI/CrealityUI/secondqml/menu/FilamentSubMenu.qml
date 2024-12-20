import QtQuick 2.0
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12

import "./"
import "../"
import "../../qml"

BasicMenuStyle {
    id: root
    title: qsTr("Change Filament")
    contentWidth : 260 * screenScaleFactor

    enabled: kernel_model_selector.selectedNum > 0

    signal extruderChanged(int index);
    // Column
    // {
    Repeater
    {
        id : idMenuItems
        model: kernel_parameter_manager.currentMachineObject.extrudersModel

        delegate: BasicMenuItemStyle {
            text: qsTr("Filament") + " " + (index + 1)
        
            function setTextColor(color){
                    let calcColor = ""
                    let bgColor = [color.r * 255, color.g * 255, color.b * 255]
                    let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255])
                    let blackContrast = Constants.getContrast(bgColor, [0, 0, 0])
                    if(whiteContrast > blackContrast)
                        calcColor = "#ffffff"
                    else
                        calcColor =  "#000000"

                    return calcColor
                }
            
            Rectangle {
                anchors.left: parent.left
                anchors.leftMargin: 12 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                width: 20 * screenScaleFactor
                height: 20 * screenScaleFactor

                color: model.extruderColor

                Text {
                    anchors.centerIn: parent
                    text: index + 1
                    color: setTextColor(model.extruderColor)
                    font.pointSize: Constants.labelFontPointSize_9
                }
            }

            onTriggered: {
                console.log("click filament " + index)
                extruderChanged(index)
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
