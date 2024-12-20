import ".."
import "../qml"
import QtQuick 2.10
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3

Item {
    //    color: "transparent"
    //    border.width: 1
    //    border.color: "blue"

    property var printerModel
    property var connectedPrinter

    Row {
        enabled: {
            if (printerModel == null)
                return false;

            if (connectedPrinter.activePrinter && connectedPrinter.activePrinter.activePrintJob) {
                if ((["printing", "pre_print", "resuming", "pausing", "paused", "error", "offline"]).indexOf(connectedPrinter.activePrinter.activePrintJob.state) != -1)
                    return false;

            }
            return true;
        }
        width: parent.width
        height: 28
        spacing: 5

        IconLabel {
            width: 120
            height: parent.height
            iconSource: "qrc:/UI/usbphoto/platformicon.png"
            labelText: qsTr("Build Plate") + ":"
            anchors.verticalCenter: parent.verticalCenter
        }

        StyledLabel {
            width: 35
            height: parent.height
            font.pixelSize: 12
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignVCenter
            text: printerModel != null ? Math.round(printerModel.bedTemperature) + "°C" : "0"
            elide: Text.ElideRight
        }
        
        StyledLabel {
            width: 50
            height: parent.height
            font.pixelSize: 12
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignVCenter
            text: printerModel != null ? "/" + Math.round(printerModel.targetBedTemperature) + "°C" : "0"
            elide: Text.ElideRight
        }

        PrintControlSpinBox {
            id: bedCurrentTemperature

            width: 80
            height: parent.height
            unitchar: ""
            realStepSize: 1
            realFrom: 0
            realTo: 260
            realValue: 60
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
            visible: printerModel != null ? printerModel.canPreHeatBed : true
            enabled: {
                if (!preheatTemperatureControl.enabled)
                    return false;
 //Not connected, not authenticated or printer is busy.
                if (printerModel.isPreheating)
                    return true;

                if (Math.floor(bedCurrentTemperature.realValue) == 0)
                    return false;
 //Setting the temperature to 0 is not allowed (since that cancels the pre-heating).
                return true; //Preconditions are met.
            }
            onClicked: {
                if (!printerModel.isPreheating)
                    printerModel.preheatBed(bedCurrentTemperature.realValue, 900);
                else
                    printerModel.cancelPreheatBed();
            }
        }

    }

}
