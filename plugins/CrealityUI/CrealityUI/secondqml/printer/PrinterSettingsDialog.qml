import QtQuick 2.10
import QtQuick.Window 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.12
import QtQuick.Controls 2.5 as QQC2
import QtGraphicalEffects 1.12

import "../../qml"
import "../../secondqml"
import "../../components"

BasicDialogV4
{
    id: root
    //color: "transparent"
    width: 600 * screenScaleFactor
    // height: 292 * screenScaleFactor + (idSequenceEditor.visible * idSequenceEditor.height)
    height: 292 * screenScaleFactor
    // flags: Qt.FramelessWindowHint | Qt.Dialog
    maxBtnVis: false
    title: qsTr("Plate Settings") + "——" + printerName
    titleIcon: "qrc:/scence3d/res/logo.png"
    property var printer
    property var printerName: {
        if (!printer || printer.name == '') {
            return qsTr("None")
        } else {
            return printer.name
        }
    }
    property var bedTypeItem
    property var printSequenceItem
    property var firstLayerPrintSequenceModeItem
    property var firstLayerPrintSequenceItem
    property var defaultBedType: qsTr("Same as Global Plate Type")

    function init() {
        if (!printer)
            return

        let type_index = 0
        let type_model = [ defaultBedType ]
        let type_keys = kernel_parameter_manager.currrentBedKeys();
        let type_values = kernel_parameter_manager.currrentBedTypes();
        let current_type_key = printer.parameter('curr_bed_type')
        for (let index = 0; index < type_keys.length; ++index) {
            const key = type_keys[index]
            const value = type_values[index]
            if (key == current_type_key) {
                type_index = index + 1
            }
            type_model.push(qsTranslate("ParameterComponent", value))
        }
        bedTypeItem.model = type_model
        bedTypeItem.currentIndex = type_index

        let printSequnence = printer.parameter('print_sequence')
        if (printSequnence == "by layer") {
            printSequenceItem.currentIndex = 1
        } else if (printSequnence == "by object") {
            printSequenceItem.currentIndex = 2
        } else {
            printSequenceItem.currentIndex = 0
        }

        let useCustomSequence = printer.parameter('custom')
        if (useCustomSequence == "true") {
            firstLayerPrintSequenceModeItem.currentIndex = 1
        } else {
            firstLayerPrintSequenceModeItem.currentIndex = 0
        }

        let sequenceStr = printer.parameter('first_layer_print_sequence')
        firstLayerPrintSequenceItem.init(sequenceStr)

    }

    function apply() {
        var bedIndex = bedTypeItem.currentIndex - 1
        if (bedIndex < 0) {
            printer.setParameter('curr_bed_type', "")
        } else {
            let keys = kernel_parameter_manager.currrentBedKeys();
            printer.setParameter('curr_bed_type', keys[bedIndex])
        }

        if (printSequenceItem.currentIndex == 0) {
            printer.setParameter('print_sequence', "")
        } else if (printSequenceItem.currentIndex == 1) {
            printer.setParameter('print_sequence', "by layer")
        } else if (printSequenceItem.currentIndex == 2) {
            printer.setParameter('print_sequence', "by object")
        }

        if (firstLayerPrintSequenceModeItem.currentIndex == 1) {
            printer.setParameter('custom', true)
        } else {
            printer.setParameter('custom', false)
        }

        printer.setParameter('first_layer_print_sequence', firstLayerPrintSequenceItem.getSequenceString())
        standaloneWindow.parameterPanel.initializePlateSetting()
    }


    bdContentItem: Rectangle {
        id: idSettings
        anchors.fill: parent
        color: Constants.themeColor_primary


        Rectangle {
            anchors.fill: parent
            anchors.leftMargin: 40
            anchors.rightMargin: 40
            anchors.topMargin: 38
            anchors.bottomMargin: 80
            color: Qt.rgba(0, 0, 0, 0)

            Grid {
                columns : 2
                rows : 4
                columnSpacing : 10
                rowSpacing : 9

                StyledLabel {
                    id: idTypeTitle
                    text: qsTr("Plate type") + ":"
                    width: 145 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignLCenter
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_9
                }

                CustomComboBox
                {
                    id: idTypeEdit
                    width: 360 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    font.pointSize: Constants.labelFontPointSize_9
                    model: [
                        // qsTr("Same as Global Plate Type"),
                        // qsTr("Cool Plate / PLA Plate"),
                        // qsTr("Engineering Plate"),
                        // qsTr("Smooth PEI Plate / Hight Tempertrue PEI Plate"),
                        // qsTr("Textured PEI Plate")
                    ]
                    currentIndex:0

                    Component.onCompleted: {
                        root.bedTypeItem = idTypeEdit
                    }
                }

                StyledLabel {
                    id: idPrinterTitle
                    text: qsTr("Print sequence") + ":"
                    width: 145 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignLCenter
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_9
                }

                CustomComboBox
                {
                    id: idPrinterEdit
                    width: 360 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    font.pointSize: Constants.labelFontPointSize_9
                    model: [
                        qsTr("Same as Global Print Sequence"),
                        qsTranslate("ParameterComponent", "By layer"),
                        qsTranslate("ParameterComponent", "By object"),

                    ]
                    currentIndex:0

                    Component.onCompleted: {
                        root.printSequenceItem = idPrinterEdit
                    }
                }

                StyledLabel {
                    id: idPrinterModeTitle
                    text: qsTr("First layer filament sequence") + ":"
                    width: 145 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignLCenter
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_9
                }

                CustomComboBox
                {
                    id: idPrinterModeEdit
                    width: 360 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    font.pointSize: Constants.labelFontPointSize_9
                    model: [
                        qsTr("Auto"),
                        qsTr("Customize"),

                    ]
                    currentIndex:0

                    Component.onCompleted: {
                        root.firstLayerPrintSequenceModeItem = idPrinterModeEdit
                    }
                }

                /* 占位控件 */
                Item {
                    visible: idSequenceEditor.visible
                    width: 145 * screenScaleFactor
                    height: 28 * screenScaleFactor
                }

                PrintSequenceEditor {
                    id: idSequenceEditor
                    visible: idPrinterModeEdit.currentIndex == 1
                    width: 360 * screenScaleFactor

                    onVisibleChanged: {
                        root.height = 292 * screenScaleFactor + visible * (height + parent.rowSpacing)
                    }

                    Component.onCompleted: {
                        root.firstLayerPrintSequenceItem = idSequenceEditor
                    }
                }

            }
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30 * screenScaleFactor
        width: 170 * screenScaleFactor
        height: 28 * screenScaleFactor
        color: Qt.rgba(0, 0, 0, 0)

        BasicButton {
            id: idOkBtn
            anchors.left: parent.left
            width: 80* screenScaleFactor
            height: 28* screenScaleFactor
            text : qsTr("OK")
            btnRadius:14
            btnBorderW:1
            pointSize: 9
            borderColor: Constants.lpw_BtnBorderColor
            borderHoverColor: Constants.lpw_BtnBorderHoverColor
            defaultBtnBgColor: Constants.lpw_BtnColor
            hoveredBtnBgColor: Constants.lpw_BtnHoverColor
            visible: true

            onSigButtonClicked: {
                root.apply()
                root.visible = false
            }
        }

        BasicButton {
            id: idCancelBtn
            anchors.right: parent.right
            width: 80* screenScaleFactor
            height: 28* screenScaleFactor
            text : qsTr("Cancel")
            btnRadius:14
            btnBorderW:1
            pointSize: 9
            borderColor: Constants.lpw_BtnBorderColor
            borderHoverColor: Constants.lpw_BtnBorderHoverColor
            defaultBtnBgColor: Constants.lpw_BtnColor
            hoveredBtnBgColor: Constants.lpw_BtnHoverColor
            visible: true

            onSigButtonClicked: {
                root.visible = false
            }
        }
    }
}
