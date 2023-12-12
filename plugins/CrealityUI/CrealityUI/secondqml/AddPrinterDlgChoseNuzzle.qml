import "../qml"
import "../secondqml"
import QtQml 2.13
import QtQuick 2.10
import QtQuick.Controls 2.5
import QtQuick.Controls 1.4 as Control14
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.13

BasicDialogV4 {
    id: idAddPrinterDlg

    property string machineName: "Ender-3"
    property int nozzleNum: 1
    property bool isGranular: false
    property int nozzle1Index: 0
    property int nozzle2Index: 0

    signal choseAccepted()
    signal choseCanceled()

    title: machineName
    width: 800 * screenScaleFactor
    height: 600 * screenScaleFactor
    maxBtnVis: false

    bdContentItem: Rectangle {
        color: Constants.lpw_bgColor

        ButtonGroup {
            id: btnGroup1
        }

        ButtonGroup {
            id: btnGroup2
        }

        StyledLabel {
            id: machineLabel

            anchors.top: parent.top
            anchors.topMargin: 30 * screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 30 * screenScaleFactor
            text: qsTr("Device Type") + ": " + idAddPrinterDlg.machineName
            font.pointSize: Constants.labelFontPointSize_12
            font.weight: Font.Medium
        }

        Connections {
            target: idAddPrinterDlg
            onVisibleChanged: {
                if (visible) {
                    nozzleList.model = kernel_parameter_manager.extruderSupportDiameters(idAddPrinterDlg.machineName, 0);
                    nozzleList1.model = kernel_parameter_manager.extruderSupportDiameters(idAddPrinterDlg.machineName, 0);
                    nozzleList2.model = kernel_parameter_manager.extruderSupportDiameters(idAddPrinterDlg.machineName, 1);
                }
            }
        }

        StackLayout {
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 30 * screenScaleFactor
            anchors.right: parent.right
            anchors.rightMargin: 30 * screenScaleFactor
            anchors.bottom: btnsRow.top
            anchors.bottomMargin: 30 * screenScaleFactor
            currentIndex: idAddPrinterDlg.nozzleNum - 1
            anchors.margins: 20 * screenScaleFactor

            Item {
                id: norzzle1Stack

                Image {
                    height: 340 * screenScaleFactor
                    width: 718 * screenScaleFactor
                    anchors.top: parent.top
                    anchors.topMargin: 50 * screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: idAddPrinterDlg.isGranular ? "qrc:/UI/photo/addPrinterSource/extruder1_granular_1.png" : "qrc:/UI/photo/addPrinterSource/extruder1_1.png"

                    StyledLabel {
                        id: no1

                        text: qsTr("Nozzle Diameter") + "(mm)"
                        anchors.right: parent.right
                        anchors.rightMargin: 113 * screenScaleFactor
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 65 * screenScaleFactor
                    }

                }

                Row {
                    id: nozzleGroup

                    property string nozzleDiameter: "0.4"

                    spacing: 10 * screenScaleFactor
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: norzzle1Stack.horizontalCenter

                    StyledLabel {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Nozzle Diameter") + ": "
                    }

                    Repeater {
                        //property int currentIndex: idAddPrinterDlg.nozzle1Index
                        id: nozzleList

                        model: []
                        anchors.verticalCenter: parent.verticalCenter

                        delegate: CusImglButton {
                            width: 100 * screenScaleFactor
                            height: 40 * screenScaleFactor
                            btnRadius: 5 * screenScaleFactor
                            bottonSelected: idAddPrinterDlg.nozzle1Index === model.index
                            state: "wordsOnly"
                            checkable: true
                            textAlign: 1
                            defaultBtnBgColor: Constants.dialogContentBgColor
                            hoverBorderColor: Constants.typeBtnHoveredColor
                            selectBorderColor: Constants.typeBtnHoveredColor
                            ButtonGroup.group: btnGroup1
                            text: qsTr(modelData) + "mm"
                            onClicked: {
                                //nozzleList.currentIndex = model.index
                                idAddPrinterDlg.nozzle1Index = model.index;
                                nozzleGroup.nozzleDiameter = modelData;
                            }
                        }

                    }

                }

            }

            Item {
                id: norzzle2Stack

                Image {
                    id: nozzleImg2

                    height: 340 * screenScaleFactor
                    width: 718 * screenScaleFactor
                    anchors.top: parent.top
                    anchors.topMargin: 0
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: idAddPrinterDlg.isGranular ? "qrc:/UI/photo/addPrinterSource/extruder2_granular_1.png" : "qrc:/UI/photo/addPrinterSource/extruder2_1.png"

                    StyledLabel {
                        id: no2_1

                        text: qsTr("Nozzle 1 Diameter") + "(mm)"
                        anchors.right: parent.right
                        anchors.rightMargin: 75 * screenScaleFactor
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 82 * screenScaleFactor
                    }

                    StyledLabel {
                        id: no2_2

                        text: qsTr("Nozzle 2 Diameter") + "(mm)"
                        anchors.right: parent.right
                        anchors.rightMargin: 60 * screenScaleFactor
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 52 * screenScaleFactor
                    }

                }

                Column {
                    anchors.top: nozzleImg2.bottom
                    anchors.horizontalCenter: nozzleImg2.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 30 * screenScaleFactor
                    spacing: 20 * screenScaleFactor

                    Row {
                        id: nozzleGroup1

                        property string nozzleDiameter: "0.4"

                        spacing: 10 * screenScaleFactor

                        StyledLabel {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Nozzle 1 Diameter") + ": "
                        }

                        Repeater {
                            //property int currentIndex: idAddPrinterDlg.nozzle1Index
                            id: nozzleList1

                            model: []

                            delegate: CusImglButton {
                                width: 100 * screenScaleFactor
                                height: 40 * screenScaleFactor
                                btnRadius: 5 * screenScaleFactor
                                bottonSelected: idAddPrinterDlg.nozzle1Index === model.index
                                state: "wordsOnly"
                                checkable: true
                                textAlign: 1
                                defaultBtnBgColor: Constants.dialogContentBgColor
                                hoverBorderColor: Constants.typeBtnHoveredColor
                                selectBorderColor: Constants.typeBtnHoveredColor
                                ButtonGroup.group: btnGroup1
                                text: qsTr(modelData) + "mm"
                                onClicked: {
                                    idAddPrinterDlg.nozzle1Index = model.index;
                                    nozzleGroup1.nozzleDiameter = modelData;
                                }
                            }

                        }

                    }

                    Row {
                        id: nozzleGroup2

                        property string nozzleDiameter: "0.4"

                        spacing: 10 * screenScaleFactor

                        StyledLabel {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Nozzle 2 Diameter") + ": "
                        }

                        Repeater {
                            //property int currentIndex: idAddPrinterDlg.nozzle2Index
                            id: nozzleList2

                            model: []

                            delegate: CusImglButton {
                                width: 100 * screenScaleFactor
                                height: 40 * screenScaleFactor
                                btnRadius: 5 * screenScaleFactor
                                bottonSelected: idAddPrinterDlg.nozzle2Index === model.index
                                state: "wordsOnly"
                                checkable: true
                                textAlign: 1
                                defaultBtnBgColor: Constants.dialogContentBgColor
                                hoverBorderColor: Constants.typeBtnHoveredColor
                                selectBorderColor: Constants.typeBtnHoveredColor
                                ButtonGroup.group: btnGroup2
                                text: qsTr(modelData) + "mm"
                                onClicked: {
                                    idAddPrinterDlg.nozzle2Index = model.index;
                                    nozzleGroup2.nozzleDiameter = modelData;
                                }
                            }

                        }

                    }

                }

            }

        }

        Row {
            id: btnsRow

            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40 * screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10 * screenScaleFactor

            BasicDialogButton {
                text: qsTr("Ok")
                width: 120 * screenScaleFactor
                height: 28 * screenScaleFactor
                btnRadius: height / 2
                btnBorderW: 1 * screenScaleFactor
                btnTextColor: Constants.manager_printer_button_text_color
                borderColor: Constants.manager_printer_button_border_color
                defaultBtnBgColor: Constants.manager_printer_button_default_color
                hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                selectedBtnBgColor: Constants.manager_printer_button_checked_color
                onSigButtonClicked: {
                    var nozzleString = idAddPrinterDlg.nozzleNum == 2 ? nozzleList.model[idAddPrinterDlg.nozzle1Index] + "-" + nozzleList.model[idAddPrinterDlg.nozzle2Index] : nozzleList.model[idAddPrinterDlg.nozzle1Index]; //"0.4-0.4" : "0.4"
                    console.log("---Name = ", idAddPrinterDlg.machineName, " ", nozzleString);
                    kernel_parameter_manager.addMachine(idAddPrinterDlg.machineName, nozzleString);
                    idAddPrinterDlg.close();
                    idAddPrinterDlg.choseAccepted();
                }
            }

            BasicDialogButton {
                text: qsTr("Cancel")
                width: 120 * screenScaleFactor
                height: 28 * screenScaleFactor
                btnRadius: height / 2
                btnBorderW: 1 * screenScaleFactor
                btnTextColor: Constants.manager_printer_button_text_color
                borderColor: Constants.manager_printer_button_border_color
                defaultBtnBgColor: Constants.manager_printer_button_default_color
                hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                selectedBtnBgColor: Constants.manager_printer_button_checked_color
                onSigButtonClicked: {
                    idAddPrinterDlg.close();
                    idAddPrinterDlg.choseCanceled();
                }
            }

        }

    }

}
