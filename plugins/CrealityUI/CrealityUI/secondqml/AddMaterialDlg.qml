import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0
import QtQuick 2.13
import QtQuick.Controls 2.0 as QQC2
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 1.4 as Control14
import CrealityUI 1.0
import QtQml 2.3
import "../qml"
import ".."

BasicDialogV4
{
    property string curPrinter: "Ender-3 S1"
    property string curMaterial: "PLA"
    property string btnState: "addState"
    property bool isConfirmClose: false
    property int extruderNum: 0
    property var extruderIndex
    property var materialParams
    property var printMaterialsModel

    signal addAccept(var extruderIndex)
    maxBtnVis: false
    onVisibleChanged: {
        if(visible){
            isConfirmClose = false
        }
    }

    onClosed: {
        if(!isConfirmClose){
            printMaterialsModel.cancelModel()
            cloader.item.repeaterView.model = null
            cloader.item.repeaterView.model = Qt.binding(function(){ return  kernel_material_center.types() })
        }
    }

    id: idAddPrinterDlg
    width: 823 * screenScaleFactor
    height: 640 * screenScaleFactor
    titleHeight : 30 * screenScaleFactor
    title: qsTr("Add Filament")

    bdContentItem: Rectangle {
        property alias repeaterView: __materialTypeRepeater
        property alias meterialView: __materialsRepeater
        color: Constants.themeColor_primary
        Column{
            anchors.fill: parent
            spacing: 30*screenScaleFactor
            anchors.margins: 20*screenScaleFactor
            Row{
                StyledLabel{
                    text: qsTr("Printer") + ": " + curPrinter
                    font.pointSize: Constants.labelFontPointSize_12
                    font.weight: Font.Bold
                    color: Constants.model_library_type_button_text_default_color
                }
            }

            Row{
                spacing: 27 * screenScaleFactor
                StyledLabel{
                    text: qsTr("MaterialType") + ": "
                    font.pointSize: Constants.labelFontPointSize_12
                    font.weight: Font.Medium
                    color: Constants.model_library_type_button_text_default_color
                    anchors.top: parent.top
                    anchors.topMargin: 0
                }

                StyleCheckBox {
                    id:checkAllType
                    text: qsTr("Check All")
                    textColor: Constants.right_panel_item_text_default_color
                    fontSize: Constants.labelFontPointSize_10
                    fontWeight: Font.Bold

                    nextCheckState: function() {
                        if (checkState === Qt.Checked)
                            return Qt.Unchecked
                        else
                            return Qt.Checked
                    }

                    onClicked: {
                        printMaterialsModel.checkedAllTypes(checked)
                        __materialTypeRepeater.model = null
                        __materialTypeRepeater.model = kernel_material_center.types()
                    }
                }
            }

            BasicScrollView{
                width: 770*screenScaleFactor
                height: 70*screenScaleFactor
                clip: true
                hpolicyVisible: contentWidth > width
                vpolicyVisible: contentHeight > height

                Flow{
                    id: materialTypeFlow
                    spacing: 10*screenScaleFactor
                    width: parent.width

                    Repeater{
                        id : __materialTypeRepeater
                        delegate:StyleCheckBox {
                            width: 70 * screenScaleFactor
                            height: 30 * screenScaleFactor
                            checked: printMaterialsModel.isVisible(modelData)
                            text: modelData
                            textColor: Constants.right_panel_item_text_default_color
                            font.pointSize: Constants.labelFontPointSize_12
                            font.weight: Font.Medium
                            onStyleCheckChanged:
                            {
                                printMaterialsModel.filterMaterialType(modelData,checked)
                                checkAllType.checked = isAllCheckedFunc(materialTypeFlow.children)

                            }
                        }
                    }
                }
            }

            Rectangle{
                width: parent.width
                height: 1*screenScaleFactor
                color: Constants.splitLineColor
            }

            Column{
                spacing: 30 * screenScaleFactor
                Row{
                    spacing: 27 * screenScaleFactor
                    StyledLabel{
                        text: qsTr("Filament Name")
                        font.pointSize: Constants.labelFontPointSize_12
                        font.weight: Font.Bold
                        color: Constants.model_library_type_button_text_default_color
                    }

                    StyleCheckBox {
                        id:checkAllCB
                        checkState: printMaterialsModel.checkState
                        text: qsTr("Check All")
                        visible: printMaterialsModel.visibleMaterials
                        textColor: Constants.right_panel_item_text_default_color
                        fontSize: Constants.labelFontPointSize_10
                        fontWeight: Font.Bold

                        nextCheckState: function() {
                            if (checkState === Qt.Checked)
                                return Qt.Unchecked
                            else
                                return Qt.Checked
                        }

                        onCheckStateChanged: {
                            printMaterialsModel.checkState = checkState
                        }
                    }
                }

                BasicScrollView{
                    id: materialScroll
                    width: 770*screenScaleFactor
                    height: 180*screenScaleFactor
                    clip: true
                    hpolicyVisible: contentWidth > width
                    vpolicyVisible: contentHeight > height
                    Flow{
                        id: checkBoxGrid
                        spacing: 5*screenScaleFactor
                        width: 770*screenScaleFactor
                        Repeater{
                            id : __materialsRepeater
                            //                            model: kernel_parameter_manager.currentMachineObject.materialsModelProxy()
                            delegate:  StyleCheckBox {
                                width: 160 * screenScaleFactor
                                height: 30 * screenScaleFactor
                                checked: model.meChecked
                                enabled: model.meEnabled
                                text: model.meName
                                textColor: Constants.right_panel_item_text_default_color
                                fontSize: Constants.labelFontPointSize_10
                                fontWeight: Font.Bold

                                onStyleCheckChanged:
                                {
                                    kernel_parameter_manager.currentMachineObject.selectChanged(checked, model.meName, extruderIndex)
                                    printMaterialsModel.setChecked(checked, model.index)
                                }
                            }
                        }
                    }
                }
            }
        }

        Row{
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20*screenScaleFactor
            spacing: 10*screenScaleFactor
            BasicDialogButton {
                id: okBtn
                text: qsTr("OK")
                width: 80* screenScaleFactor
                height: 28* screenScaleFactor
                btnBorderW: 1 * screenScaleFactor
                btnTextColor: Constants.manager_printer_button_text_color
                borderColor: Constants.manager_printer_button_border_color
                defaultBtnBgColor: Constants.manager_printer_button_default_color
                hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                selectedBtnBgColor: Constants.manager_printer_button_checked_color

                onSigButtonClicked: {
                    isConfirmClose = true
                    if(!printMaterialsModel.selectMaterialsCount){
                        let receiver = {}
                        receiver.onOk = function (){
                            __materialTypeRepeater.model = null
                            __materialTypeRepeater.model = kernel_material_center.types()
                            printMaterialsModel.resetModel()

                            kernel_parameter_manager.currentMachineObject.finishMeterialSelect(extruderIndex)
                            printMaterialsModel.confirm()
                            idAddPrinterDlg.close()
                        }

                        receiver.onCancel = function (){

                        }

                        idAllMenuDialog.requestMenuDialog(receiver, "idNoMaterialWarning");
                    }else{
                        printMaterialsModel.confirm()
                        kernel_parameter_manager.currentMachineObject.finishMeterialSelect(extruderIndex)
                        idAddPrinterDlg.close()
                    }
                }
            }

            BasicDialogButton {
                id: resetBtn
                text: qsTr("Reset")
                width: 80* screenScaleFactor
                height: 28* screenScaleFactor

                btnBorderW: 1 * screenScaleFactor
                btnTextColor: Constants.manager_printer_button_text_color
                borderColor: Constants.manager_printer_button_border_color
                defaultBtnBgColor: Constants.manager_printer_button_default_color
                hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                selectedBtnBgColor: Constants.manager_printer_button_checked_color

                onSigButtonClicked: {
                    printMaterialsModel.resetModel()

                    __materialTypeRepeater.model = null
                    __materialTypeRepeater.model = kernel_material_center.types()

                    __materialsRepeater.model = null
                    __materialsRepeater.model = printMaterialsModel
                }
            }

            BasicDialogButton {
                id: cancelBtn
                text: qsTr("Cancel")
                width: 80 * screenScaleFactor
                height: 28* screenScaleFactor

                btnBorderW: 1 * screenScaleFactor
                btnTextColor: Constants.manager_printer_button_text_color
                borderColor: Constants.manager_printer_button_border_color
                defaultBtnBgColor: Constants.manager_printer_button_default_color
                hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                selectedBtnBgColor: Constants.manager_printer_button_checked_color

                onSigButtonClicked: {
                    printMaterialsModel.cancelModel()
                    idAddPrinterDlg.close()

                    cloader.item.repeaterView.model = null
                    cloader.item.repeaterView.model = Qt.binding(function(){ return  kernel_material_center.types() })
                }
            }
        }
    }

    function startShowMaterialDialog(_state, _extruderIndex, materialName, machineObject){
        extruderIndex = _extruderIndex
        curPrinter = kernel_parameter_manager.currentMachineShowName()
        console.log("______startShowMaterialDialog")
        printMaterialsModel = kernel_parameter_manager.currentMachineObject.materialsModelProxy();
        cloader.item.repeaterView.model = null
        cloader.item.repeaterView.model = Qt.binding(function(){ return  kernel_material_center.types() })
        cloader.item.meterialView.model = null
        cloader.item.meterialView.model = Qt.binding(function(){ return  kernel_parameter_manager.currentMachineObject.materialsModelProxy(); })
        visible = true

        cloader.item.meterialView.model.refreshModel()
    }

    function isAllCheckedFunc(childrenItems){
        let isCheckedAll = true
        for(let index = 0; index < childrenItems.length; index++){
            if(childrenItems[index].checked === false){
                isCheckedAll = false
                break;
            }
        }
        return isCheckedAll
    }
}
