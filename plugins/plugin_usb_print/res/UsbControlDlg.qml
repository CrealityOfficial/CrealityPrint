import ".."
import "../qml"
import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0

BasicDialog {
    id: idDialog

    property var filePath
    property var connectedDevice: plugin_usb_printer.printerOutputDevices.length >= 1 ? plugin_usb_printer.printerOutputDevices[0] : null
    property var activePrinter: connectedDevice != null ? connectedDevice.activePrinter : null
    property var activePrintJob: activePrinter != null ? activePrinter.activePrintJob : null

    function strPadLeft(string, pad, length) {
        return (new Array(length + 1).join(pad) + string).slice(-length);
    }

    function getPrettyTime(time) {
        var hours = Math.floor(time / 3600);
        time -= hours * 3600;
        var minutes = Math.floor(time / 60);
        time -= minutes * 60;
        var seconds = Math.floor(time);
        var finalTime = strPadLeft(hours, "0", 2) + ":" + strPadLeft(minutes, "0", 2) + ":" + strPadLeft(seconds, "0", 2);
        return finalTime;
    }

    width: 860
    height: 407
    titleHeight: 29
    title: qsTr("USB Printing")
    onDialogClosed: {
    }
    titleBackground: Constants.itemBackgroundColor
    contentBackground: Constants.itemBackgroundColor
    Component.onCompleted: {
        console.log("connectedDevice", connectedDevice);
        console.log("activePrinter", activePrinter);
        console.log("activePrintJob", activePrintJob);
    }

    Column {
        y: 50
        x: 30
        spacing: 20
        width: parent.width - 60
        height: parent.height - 50

        Row {
            spacing: 30
            height: 257

            Control {
                //                color: "transparent"
                //                border.width: 1
                //                border.color: "red"
                width: 335
                height: parent.height

                ColumnLayout {
                    spacing: 10
                    anchors.fill: parent

                    Row {
                        Layout.preferredHeight: 28

                        IconLabel {
                            labelText: qsTr("Deveice") + ":"
                            width: 150
                            iconSource: "qrc:/UI/usbphoto/devicon.png"
                        }

                        StyledLabel {
                            text: connectedDevice != null ? connectedDevice.activePrinter.name : ""
                            anchors.verticalCenter: parent.verticalCenter
                            color: Constants.textColor
                            font.pixelSize: 12
                            width: 160
                        }

                    }

                    Row {
                        Layout.preferredHeight: 28

                        IconLabel {
                            labelText: qsTr("Port") + ":"
                            width: 150
                        }

                        StyledLabel {
                            text: connectedDevice.address
                            anchors.verticalCenter: parent.verticalCenter
                            color: Constants.textColor
                            font.pixelSize: 12
                            width: 160
                        }

                    }

                    Row {
                        Layout.preferredHeight: 28
                        spacing: 10

                        IconLabel {
                            labelText: qsTr("Send G-code") + ":"
                            width: 140
                        }

                        BasicDialogTextField {
                            id: gcodeText

                            height: 28
                            width: 121
                            text: ""
                            radius: 5
                            onAccepted: {
                                activePrinter.sendRawCommand(text);
                            }

                            validator: RegExpValidator { regExp: /^[G|M|T][0-9]([ A-Z0-9_\-\.]+)$/ }
                        }

                        BasicDialogButton {
                            id: sendButton

                            text: qsTr("Send")
                            height: 24
                            width: 64
                            anchors.verticalCenter: parent.verticalCenter
                            onClicked: {
                                activePrinter.sendRawCommand(gcodeText.text);
                            }
                        }

                    }

                    Row {
                        Layout.preferredHeight: 28

                        IconLabel {
                            labelText: qsTr("Printing File") + ":"
                            width: 150
                            iconSource: "qrc:/UI/usbphoto/fileicon.png"
                        }

                        StyledLabel {
                            text: activePrintJob != null ? activePrintJob.name : ""
                            anchors.verticalCenter: parent.verticalCenter
                            color: Constants.textColor
                            font.pixelSize: 12
                            width: 160
                        }

                    }

                    Row {
                        Layout.preferredHeight: 28

                        IconLabel {
                            labelText: qsTr("Printing Time") + ":"
                            width: 150
                            iconSource: "qrc:/UI/usbphoto/printtimeicon.png"
                        }

                        StyledLabel {
                            text: activePrintJob != null ? getPrettyTime(activePrintJob.timeElapsed) : ""
                            anchors.verticalCenter: parent.verticalCenter
                            color: Constants.textColor
                            font.pixelSize: 12
                            width: 160
                        }

                    }

                    Row {
                        Layout.preferredHeight: 28

                        IconLabel {
                            labelText: qsTr("Estimated Time Left") + ":"
                            width: 150
                            iconSource: "qrc:/UI/usbphoto/retimeicon.png"
                        }

                        StyledLabel {
                            text: activePrintJob != null ? getPrettyTime(activePrintJob.timeTotal - activePrintJob.timeElapsed) : ""
                            anchors.verticalCenter: parent.verticalCenter
                            color: Constants.textColor
                            font.pixelSize: 12
                            width: 160
                        }

                    }

                    Row {
                        Layout.preferredHeight: 28
                        spacing: 10

                        IconLabel {
                            labelText: qsTr("Printing Progress") + ":"
                            width: 140
                            height: parent.height
                            iconSource: "qrc:/UI/usbphoto/progressicon.png"
                        }

                        ProgressBar {
                            id: progressBar

                            from: 0
                            to: 100
                            value: activePrintJob != null ? activePrintJob.progress : 0
                            width: 160
                            height: 3
                            anchors.verticalCenter: parent.verticalCenter

                            background: Rectangle {
                                anchors.fill: parent
                                color: Constants.progressBarBgColor
                            }

                            contentItem: Item {
                                Rectangle {
                                    width: progressBar.visualPosition * progressBar.width
                                    height: progressBar.height
                                    color: "#1E9BE2"
                                }

                            }

                        }

                        StyledLabel {
                            text: activePrintJob != null ? activePrintJob.progress + "%" : ""
                            anchors.verticalCenter: parent.verticalCenter
                            color: Constants.textColor
                            font.pixelSize: 12
                            width: 24
                        }

                    }

                }

            }

            Item {
                width: 2
                height: 237

                Rectangle {
                    width: 2
                    height: parent.height
                    color: Constants.splitLineColor

                    Rectangle {
                        width: 1
                        height: parent.height
                        color: Constants.splitLineColorDark
                    }

                }

            }

            Item {
                //                color: "transparent"
                //                border.width: 1
                //                border.color: "red"
                width: 395
                height: parent.height

                ColumnLayout {
                    width: 365
                    height: 257
                    spacing: 10

                    Item {
                        Layout.fillHeight: true
                    }

                    Flow {
                        id: extrudersGrid

                        spacing: 2
                        width: parent.width
                        height: 30

                        Repeater {
                            id: extrudersRepeater

                            model: activePrinter != null ? activePrinter.extruders : null

                            delegate: CusExtruderBox {
                                width: parent.width
                                height: 30
                                extruderModel: modelData
                                connectedPrinter: idDialog.connectedDevice
                            }

                        }

                    }

                    CusHeatBedBox {
                        Layout.preferredHeight: 28
                        Layout.preferredWidth: parent.width
                        visible: {
                            if (activePrinter != null && activePrinter.bedTemperature != -1)
                                return true;

                            return false;
                        }
                        printerModel: activePrinter
                        connectedPrinter: idDialog.connectedDevice
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    CusManualPrinterControlBox {
                        printerModel: activePrinter
                        Layout.preferredHeight: 140
                        Layout.preferredWidth: parent.width
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                }

            }

        }

        Item {
            width: parent.width
            height: 2

            Rectangle {
                // anchors.left: idCol.left
                // anchors.leftMargin: -10
                width: parent.width
                height: 2
                color: Constants.splitLineColor

                Rectangle {
                    width: parent.width
                    height: 1
                    color: Constants.splitLineColorDark
                }

            }

        }

        Item {
            width: parent.width
            height: 30

            //            color: "transparent"
            //            border.width: 1
            //            border.color: "red"
            Row {
                spacing: 10
                anchors.centerIn: parent

                BasicDialogButton {
                    id: idSuspend

                    text: {
                        if (activePrintJob == null)
                            return qsTr("Complete");

                        if (activePrintJob.state == "paused")
                            return qsTr("Resume");
                        else
                            return qsTr("Pause");
                    }
                    visible: activePrinter != null
                    enabled: (activePrintJob == null) || (["paused", "printing"].indexOf(activePrintJob.state) >= 0)
                    width: 160
                    height: 28
                    btnRadius: 14
                    btnBorderW: 0
                    defaultBtnBgColor: Constants.profileBtnColor
                    hoveredBtnBgColor: Constants.profileBtnHoverColor
                    onClicked: {
                        if (activePrintJob == null){                            
                            idDialog.close()
                            return
                        }

                        if (activePrintJob.state == "paused")
                            activePrintJob.setState("print");
                        else if (activePrintJob.state == "printing")
                            activePrintJob.setState("pause");
                    }
                }

                BasicDialogButton {
                    id: idDisConnect

                    text: qsTr("Stop")
                    visible: activePrintJob != null
                    enabled: activePrintJob != null && (["paused", "printing", "pre_print"].indexOf(activePrintJob.state) >= 0)
                    width: 160
                    height: 28
                    btnRadius: 14
                    defaultBtnBgColor: Constants.lpw_BtnColor
                    hoveredBtnBgColor: Constants.lpw_BtnHoverColor
                    borderColor: Constants.lpw_BtnBorderColor
                    borderHoverColor: Constants.lpw_BtnBorderHoverColor
                    onClicked: {
                        if (activePrintJob != null) {
                            idDoubleMessageDlg.showDoubleBtn();
                            idDoubleMessageDlg.messageText = qsTr("Stop") + "?";
                            idDoubleMessageDlg.show();
                        }
                    }
                }

            }

        }

    }

    BasicMessageDialog {
        id: idDoubleMessageDlg

        onAccept: {
            activePrintJob.setState("abort");
            idDialog.close();
        }
        onCancel: {
            close();
        }
    }

}
