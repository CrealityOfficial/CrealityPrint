import QtQuick 2.0
import QtQuick.Controls 2.5

import "../qml"
import "../secondqml"

Rectangle {
    id: btnGroupBox
    width: 300 * screenScaleFactor
    height: contentColumn.height + 2 * contentColumn.spacing
    visible: kernel_kernel.currentPhase == 1

    signal usbButtonClicked()
    signal wifiButtonClicked()
    signal uploadButtonClicked()
    signal exportButtonClicked()
    property bool showExtraButtons: true

    color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
    border.color: Constants.currentTheme ? "#CBCBCC" : "#262626"
    border.width: 1
    radius: 5

    Column {
        id: contentColumn
        anchors.left: parent.left
        anchors.leftMargin: 10 * screenScaleFactor
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10 * screenScaleFactor

        Rectangle {
            width: 234 * screenScaleFactor
            height: 36 * screenScaleFactor
            visible: true

            color: "#1E9BE2"
            radius: 5

            Row {
                height: parent.height
                anchors.centerIn: parent
                spacing: 10 * screenScaleFactor

                Image {
                    width: 22 * screenScaleFactor
                    height: 16 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:/UI/photo/rightDrawer/wifiPrint_dark.svg"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("LAN Printing")
                    color: "#FFFFFF"
                }
            }

            MouseArea {
                id: wifiBtnArea
                hoverEnabled: true
                anchors.fill: parent
                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: wifiButtonClicked()
            }
        }

        Rectangle {
            width: 234 * screenScaleFactor
            height: 36 * screenScaleFactor
            visible: btnGroupBox.showExtraButtons

            color: exportBtnArea.containsMouse ? Constants.currentTheme ? "#F2F2F5" : "#6E6E73" : Constants.currentTheme ? "#FFFFFF" : "#59595D"
            border.color: Constants.currentTheme ? "#CECECF" : "transparent"
            border.width: Constants.currentTheme ? 1 : 0
            radius: 5

            Row {
                height: parent.height
                anchors.centerIn: parent
                spacing: 10 * screenScaleFactor

                Image {
                    width: 20 * screenScaleFactor
                    height: 16 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    source: Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/exportLocal_light.svg" : "qrc:/UI/photo/rightDrawer/exportLocal_dark.svg"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("Export to Local")
                    color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                }
            }

            MouseArea {
                id: exportBtnArea
                hoverEnabled: true
                anchors.fill: parent
                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: exportButtonClicked()
            }
        }

        Rectangle {
            width: 234 * screenScaleFactor
            height: 36 * screenScaleFactor
            visible: btnGroupBox.showExtraButtons && kernel_global_const.cxcloudEnabled

            color: uploadBtnArea.containsMouse ? Constants.currentTheme ? "#F2F2F5" : "#6E6E73" : Constants.currentTheme ? "#FFFFFF" : "#59595D"
            border.color: Constants.currentTheme ? "#CECECF" : "transparent"
            border.width: Constants.currentTheme ? 1 : 0
            radius: 5

            Row {
                height: parent.height
                anchors.centerIn: parent
                spacing: 10 * screenScaleFactor

                Image {
                    width: 22 * screenScaleFactor
                    height: 22 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:/UI/photo/rightDrawer/uploadCrealityCloud.svg"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("Upload to Crealitycloud")
                    color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                }
            }

            MouseArea {
                id: uploadBtnArea
                hoverEnabled: true
                anchors.fill: parent
                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: uploadButtonClicked()
            }
        }

        Rectangle {
            width: 234 * screenScaleFactor
            height: 36 * screenScaleFactor
            opacity: enabled ? 1.0 : 0.7
            visible: false//btnGroupBox.showExtraButtons
            enabled: plugin_usb_printer.printerOutputDevices.length >= 1 && plugin_usb_printer.printerOutputDevices[0].activePrinter !== null

            color: usbBtnArea.containsMouse ? Constants.currentTheme ? "#F2F2F5" : "#6E6E73" : Constants.currentTheme ? "#FFFFFF" : "#59595D"
            border.color: Constants.currentTheme ? "#CECECF" : "transparent"
            border.width: Constants.currentTheme ? 1 : 0
            radius: 5

            Row {
                height: parent.height
                anchors.centerIn: parent
                spacing: 10 * screenScaleFactor

                Image {
                    width: 22 * screenScaleFactor
                    height: 22 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    source: Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/usbPrint_light.svg" : "qrc:/UI/photo/rightDrawer/usbPrint_dark.svg"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("USB Printing")
                    color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                }
            }

            MouseArea {
                id: usbBtnArea
                hoverEnabled: true
                anchors.fill: parent
                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: usbButtonClicked()
            }
        }
    }

    Rectangle {
        width: 36 * screenScaleFactor
        height: 36 * screenScaleFactor

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 10 * screenScaleFactor
        anchors.bottomMargin: 10 * screenScaleFactor

        color: arrowBtnArea.containsMouse ? Constants.currentTheme ? "#F2F2F5" : "#6E6E73" : Constants.currentTheme ? "#FFFFFF" : "#59595D"
        border.color: Constants.currentTheme ? "#CECECF" : "transparent"
        border.width: Constants.currentTheme ? 1 : 0
        radius: 5

        Image {
            anchors.centerIn: parent
            source: btnGroupBox.showExtraButtons ? box_downArrow : box_upArrow
            property string box_upArrow: Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/box_upArrow_light.svg" : "qrc:/UI/photo/rightDrawer/box_upArrow_dark.svg"
            property string box_downArrow: Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/box_downArrow_light.svg" : "qrc:/UI/photo/rightDrawer/box_downArrow_dark.svg"
        }

        MouseArea {
            id: arrowBtnArea
            hoverEnabled: true
            anchors.fill: parent
            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

            onClicked: btnGroupBox.showExtraButtons = !btnGroupBox.showExtraButtons
        }
    }
}
