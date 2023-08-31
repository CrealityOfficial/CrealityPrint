import QtQuick 2.10
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12

import "../qml"
import "../secondqml"

Rectangle {
    id: root
    width: 307 * screenScaleFactor
    height: 310 * screenScaleFactor
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

    property alias addEnabled : idAddButton.enabled

    property string imageUrl: ""
    property string printerName: ""

    signal addMachine(var machineName)

    Item {
        width: parent.width - 2 * parent.border.width
        height: 214 * screenScaleFactor

        anchors.top: parent.top
        anchors.topMargin: parent.border.width
        anchors.horizontalCenter: parent.horizontalCenter

        Canvas {
            width: 72 * screenScaleFactor
            height: 72 * screenScaleFactor
            visible: printerName.startsWith("Fast-")

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
                text: qsTr("Sonic")
                rotation: -45
            }
        }

        Image {
            anchors.fill: parent
            anchors.margins: 12 * screenScaleFactor

            cache: false
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            sourceSize: Qt.size(width, height)
            source: imageUrl
        }
    }
    Rectangle {
        width: parent.width - 2 * parent.border.width
        height: 96 * screenScaleFactor
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.border.width
        anchors.horizontalCenter: parent.horizontalCenter
        color: lower_background_color
        radius: 10
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 15 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter

            font.weight: Font.Medium
            font.family: Constants.mySystemFont.name
            font.pointSize: Constants.labelFontPointSize_10
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            lineHeightMode: Text.FixedHeight
            lineHeight: 24 * screenScaleFactor

            text: "<font color='#999999'>" + qsTr("Print Depth")  + "：" + "</font>" + "<font color='#333333'>" + machineDepth  + " mm" + "</font>" + "<br>" +
                  "<font color='#999999'>" + qsTr("Print Width")  + "：" + "</font>" + "<font color='#333333'>" + machineWidth  + " mm" + "</font>" + "<br>" +
                  "<font color='#999999'>" + qsTr("Print Height") + "：" + "</font>" + "<font color='#333333'>" + machineHeight + " mm" + "</font>"
        }

        Column {
            anchors.right: parent.right
            anchors.rightMargin: 25 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8 * screenScaleFactor

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.weight: Font.Bold
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_10
                color: text_weight_color
                text: printerName
            }

            Button {
                id: idAddButton
                width: 110 * screenScaleFactor
                height: 32 * screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter

				enabled: true

                background: Rectangle {
                    color: parent.hovered ? addPrinter_hovered_color : addPrinter_default_color
                    radius: height / 2
                }

                contentItem: Text
                {
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    color: parent.hovered ? addPrinter_text_hovered_color : addPrinter_text_default_color
                    text: qsTr("Add")
                }

                onClicked: addMachine(printerName)
            }
        }
    }
}
