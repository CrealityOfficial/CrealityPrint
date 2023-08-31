import QtQuick 2.10
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.5

import ".."
import "../qml"

Rectangle {
    id: root
    width: 470 * screenScaleFactor
    height: 285 * screenScaleFactor
    radius: 10 * screenScaleFactor
    opacity: enabled ? 1.0 : 0.7

    color: "#FFFFFF"
    border.color: mouseArea.containsMouse ? "#00A3FF" : "#DDDDE1"
    border.width: mouseArea.containsMouse ? 4 : (Constants.currentTheme ? 1 : 0)

    property var equipmentID: ""
    property var printerName: ""
    property var equipmentName: ""

    property var printerIp: ""
    property var printerImage: ""
    property var tFCardStatus: ""
    property var printerStatus: ""
    property var equipmentStatus: ""
    property var printerStatusShow: ""

    //property alias progressValue: progressBar.value
    //property alias progressVisible: progressBar.visible

    property string defaultImage: Constants.currentTheme ? "qrc:/UI/photo/userInfo_imageDefault_light.png" : "qrc:/UI/photo/userInfo_imageDefault_dark.png"

    signal itemClicked()

    MouseArea {
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent
        cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: itemClicked()
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10 * screenScaleFactor
        spacing: 20 * screenScaleFactor

        Rectangle {
            color: "#E0E0E0"
            Layout.preferredWidth: 200 * screenScaleFactor
            Layout.preferredHeight: 265 * screenScaleFactor

            Image {
                width: isValid ? parent.width : 87 * screenScaleFactor
                height: isValid ? parent.height : 80 * screenScaleFactor

                anchors.centerIn: parent
                property bool isValid: (printerImage != "")

                cache: false
                mipmap: true
                smooth: true
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                sourceSize: Qt.size(width, height)
                source: isValid ? printerImage : defaultImage
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Text {
                anchors.top: parent.top
                anchors.left: parent.left
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                font.weight: Font.Medium
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_9
                lineHeightMode: Text.FixedHeight
                lineHeight: 32 * screenScaleFactor

                text: "<font color='#999999'>" + qsTr("Device Name")    + "：" + "</font>" + "<font color='#333333'>" + equipmentName           + "</font>" + "<br>" +
                      "<font color='#999999'>" + qsTr("Device ID")      + "：" + "</font>" + "<font color='#333333'>" + equipmentID             + "</font>" + "<br>" +
                      "<font color='#999999'>" + qsTr("Status")         + "：" + "</font>" + "<font color='#333333'>" + qsTr(equipmentStatus)   + "</font>" + "<br>" +
                      "<font color='#999999'>" + qsTr("TF Card Status") + "：" + "</font>" + "<font color='#333333'>" + qsTr(tFCardStatus)      + "</font>" + "<br>" +
                      "<font color='#999999'>" + qsTr("Printer Status") + "：" + "</font>" + "<font color='#333333'>" + qsTr(printerStatusShow) + "</font>" + "<br>" +
                      "<font color='#999999'>" + qsTr("IP")             + "：" + "</font>" + "<font color='#333333'>" + printerIp               + "</font>"
            }
        }
    }
}
