import QtQuick 2.10
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12

import "../qml"
import "../secondqml"

Rectangle {
    id: root
    width: 280 * screenScaleFactor
    height: 345 * screenScaleFactor
    radius: 10 * screenScaleFactor
    border.width: (itemChecked || Constants.currentTheme) ? 2 : 0
    border.color: itemChecked ? bSelection_border : background_border
    color: Constants.currentTheme ? upper_background_light : upper_background_dark
//    gradient: Gradient {
//        GradientStop { position: 0.0; color: Constants.currentTheme ? upper_background_light : upper_background_dark }
//        GradientStop { position: 1.0; color: lower_background_color }
//    }

    property var machineDepth: 220
    property var machineWidth: 220
    property var machineHeight: 220
    property var nozzleSize: 0.40

    property bool itemChecked: false
    property color text_light_color: "#999999"
    property color text_weight_color: "#333333"
    property color background_border: "#DDDDE1"
    property color bSelection_border: "#1E9BE2"
    property color upper_background_dark: "#DDDDE1"
    property color upper_background_light: "#F2F2F6"
    property color lower_background_color: "#FFFFFF"
    property color addPrinter_default_color: "#E2F5FF"
    property color addPrinter_hovered_color: "#1E9BE2"
    property color addPrinter_text_default_color: "#1E9BE2"
    property color addPrinter_text_hovered_color: "#FFFFFF"

  //  property alias addEnabled : true  //idAddButton.enabled

    signal reset()
    property string imageUrl: ""
    property string printerName: ""
    property string printerCodeName: ""
    property bool is_sonic: printerName.startsWith("Fast-")
    property bool is_nebula: printerName.startsWith("Nebula-")

    signal addMachine(var machineName)

    signal addPrinter(var machineName, var nozzle, var initCheck, var changeCheck)

    Item {
        width: parent.width - 2 * parent.border.width
        height: 220 * screenScaleFactor

        anchors.top: parent.top
        anchors.topMargin: parent.border.width
        anchors.horizontalCenter: parent.horizontalCenter

        Canvas {
            width: 72 * screenScaleFactor
            height: 72 * screenScaleFactor
            visible: is_sonic || is_nebula

            anchors.top: parent.top
            anchors.left: parent.left

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                //Draw path
                ctx.beginPath()
                ctx.moveTo(30 * screenScaleFactor, 0)
                ctx.lineTo(width, 0)
                ctx.lineTo(0, height)
                ctx.lineTo(0, 30 * screenScaleFactor)
                ctx.closePath()
                ctx.fillStyle = "#FBB143"; ctx.fill()
            }

            Text {
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -10 * screenScaleFactor
                anchors.horizontalCenterOffset: -10 * screenScaleFactor

                font.weight: Font.Medium
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_12
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "black"
                text: is_sonic ? qsTr("Sonic") : qsTr("Nebula")
                rotation: -45
            }
        }

        Image {
            anchors.horizontalCenter: parent.horizontalCenter

            // anchors.fill: parent
            width:250 * screenScaleFactor
            height:250 * screenScaleFactor
            anchors.margins: 12 * screenScaleFactor

            cache: false
            asynchronous: true
            // fillMode: Image.PreserveAspectFit
            sourceSize: Qt.size(width, height)
            source: imageUrl
        }
    }

    CusRoundedBg{
        width: parent.width - 2 * parent.border.width
        height: 125 * screenScaleFactor
        borderWidth:0
        leftBottom:true
        rightBottom:true
        color: "#ffffff"
        radius: 10* screenScaleFactor
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.border.width
        Column {
            anchors.fill: parent
            spacing: 15* screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 15 * screenScaleFactor
            Text {
                topPadding: 25* screenScaleFactor
                font.weight: Font.Medium
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_12
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                lineHeightMode: Text.FixedHeight
                color: text_weight_color
                text: printerName
            }
            Text {
                topPadding: 5* screenScaleFactor
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_9
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                lineHeightMode: Text.FixedHeight
                color: "#666666"
                text: `${machineDepth}*${machineWidth}*${machineHeight} mm`
            }

            Grid{
                id:checkContainer
                topPadding: 5* screenScaleFactor
                columns: 2
                columnSpacing: 50
                rowSpacing: 10
                Repeater {
                    id: modelItem
                    model: kernel_parameter_manager.extruderSupportDiameters(printerCodeName, 0)
                    delegate: CusCheckBox {
                        property bool initCheck: kernel_parameter_manager.machineNozzleSelectList(printerName).includes(modelData)
                        property bool isInit:true
                        Component.onCompleted:isInit = false
                        checked: initCheck
                        text: modelData + " mm"
                        textColor: text_weight_color
                        font.pointSize: Constants.labelFontPointSize_10
                        onCheckedChanged:{
                            if(isInit) return
                            if(checked){
                                 kernel_parameter_manager.addMachine(printerCodeName, modelData);
                            }else{
                                var machineObject = kernel_parameter_manager.getMachineObjectByName(printerCodeName, modelData);
                                kernel_parameter_manager.removeMachine(machineObject);
                            }
                        }
                        // onCheckedChanged: addPrinter(printerCodeName, modelData, initCheck, checked)                        
                        // Connections{
                        //     target:kernel_parameter_manager
                        //     function onMachineListChange(){
                        //         initCheck = kernel_parameter_manager.machineNozzleSelectList(printerName).includes(modelData)

                        //     }

                        // }

                        // Connections{
                        //     target: root
                        //     function onReset(){
                        //         checked =initCheck
                        //     }
                        // }
                    }

                }

            }

        }

}
}
