//Copyright (c) 2019 Ultimaker B.V.
//Cura is released under the terms of the LGPLv3 or higher.

import ".."
import "../qml"
import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Layouts 1.1

Item {
    property var extruderModel
    property var connectedPrinter
    property var position: index

    implicitWidth: parent.width

    Rectangle {
        id: background

        color: "transparent"
        anchors.fill: parent

        Row {
            enabled: {
                if (extruderModel == null)
                    return false; //Can't preheat if not connected.

                if (connectedPrinter.activePrinter && connectedPrinter.activePrinter.activePrintJob) {
                    if ((["printing", "pre_print", "resuming", "pausing", "paused", "error", "offline"]).indexOf(connectedPrinter.activePrinter.activePrintJob.state) != -1)
                        return false; //Printer is in a state where it can't react to pre-heating.

                }
                return true;
            }
            width: parent.width
            height: 28
            spacing: 5

            IconLabel {
                width: 120
                height: parent.height
                iconSource: "qrc:/UI/usbphoto/extemp.png"
                labelText: qsTr("Extruder") + ":"
                anchors.verticalCenter: parent.verticalCenter
            }

            StyledLabel {
                width: 35
                height: parent.height
                font.pixelSize: 12
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignVCenter
                text: Math.round(extruderModel.hotendTemperature) + "°C"
                elide: Text.ElideRight
            }

            StyledLabel {
                width: 50
                height: parent.height
                font.pixelSize: 12
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignVCenter
                text: "/" + Math.round(extruderModel.targetHotendTemperature) + "°C"
                elide: Text.ElideRight
            }

            PrintControlSpinBox {
                id: idNozzleTempTargetLabel

                width: 80
                height: parent.height
                unitchar: ""
                realStepSize: 1
                realFrom: 0
                realTo: 260
                realValue: 200
                bgColor: Constants.dialogItemRectBgColor
                borderColor: Constants.dialogItemRectBgBorderColor
                bgRadius: 5
            }

            //The pre-heat button.
            BasicDialogButton {
                id: preheatButton

                text: qsTr("Warm Up")
                height: 24
                width: 64
                anchors.verticalCenter: parent.verticalCenter
                onClicked: {
                    if (!extruderModel.isPreheating)
                        extruderModel.preheatHotend(idNozzleTempTargetLabel.realValue, 900);
                    else
                        extruderModel.cancelPreheatHotend();
                }
            }

        }

    }

}
