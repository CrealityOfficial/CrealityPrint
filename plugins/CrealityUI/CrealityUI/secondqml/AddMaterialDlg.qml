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
    property int extruderNum: 0
    property var extruderIndex
    property var materialParams
    property var printMaterialsModel

    signal addAccept(var extruderIndex)
    maxBtnVis: false
    onVisibleChanged: {
        if(visible){
            printMaterialsModel = kernel_parameter_manager.currentMachineObject().materialsModel()
            cloader.item.repeaterView.model = kernel_parameter_manager.currentMachineObject().supportTypes()
        }
    }

    id: idAddPrinterDlg
    width: 1040 * screenScaleFactor
    height: 600 * screenScaleFactor
    titleHeight : 30 * screenScaleFactor
    title: qsTr("Add Material")

    bdContentItem: Rectangle {
        property alias repeaterView: __materialTypeRepeater
        property alias meterialView: __materialsRepeater
        property alias userMeterialView: __userMaterialsRepeater
        color: Constants.themeColor_primary
        Column{
            anchors.fill: parent
            spacing: 30*screenScaleFactor
            anchors.margins: 20*screenScaleFactor
            Row{
                StyledLabel{
                    text: qsTr("Printer") + ": " + curPrinter
                    font.pointSize: Constants.labelFontPointSize_8
                    color: Constants.model_library_type_button_text_default_color
                }
            }

            Row{
                StyledLabel{
                    text: qsTr("MaterialType") + ": "
                    font.pointSize: Constants.labelFontPointSize_8
                    color: Constants.model_library_type_button_text_default_color
                }

                BasicScrollView{
                    width: 854*screenScaleFactor
                    height: 50*screenScaleFactor
                    clip: true
                    vpolicyVisible:(__materialTypeRepeater.height+15)*screenScaleFactor > height
                    Flow{
                        spacing: 15*screenScaleFactor
                        width: parent.width

                        Repeater{
                            id : __materialTypeRepeater
                            model: kernel_parameter_manager.currentMachineObject().supportTypes()
                            delegate:CusCheckBox {
                                checked: printMaterialsModel.isVisible(modelData) //getMaterialTypeStatus(modelData)
                                text: modelData
                                textColor: Constants.right_panel_item_text_default_color
                                font.pointSize: Constants.labelFontPointSize_10
                                onClicked:
                                {
                                    printMaterialsModel.filterMaterialType(modelData,checked)
                                }
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

            BasicGroupBox{
                width: parent.width
                anchors.horizontalCenter: parent.horizontalCenter
                implicitHeight:isExpand ? contentItem.implicitHeight + 30* screenScaleFactor : 40* screenScaleFactor
                title: qsTr("Default Materials")
                backRadius: 5
                borderWidth: 1
                borderColor: Constants.dialogItemRectBgBorderColor
                contentItem:Column{
                    anchors.fill: parent
                    anchors.margins: 10*screenScaleFactor
                    spacing: 5* screenScaleFactor
                    StyleCheckBox {
                        id:checkAllCB
                        text: qsTr("Check All")
                        checked: printMaterialsModel.isCheckedAll
                        textColor: Constants.right_panel_item_text_default_color
                        fontSize: Constants.labelFontPointSize_8
                        onClicked: {
                            printMaterialsModel.isCheckedAll = checked
                        }
                    }

                    BasicScrollView{
                        width: 924*screenScaleFactor
                        height: 100*screenScaleFactor
                        clip: true
                        vpolicyVisible: checkBoxGrid.height > height
                        Grid{
                            id: checkBoxGrid
                            spacing: 100*screenScaleFactor
                            width: parent.width
                            columns: 4
                            columnSpacing: 150*screenScaleFactor
                            rowSpacing: 20*screenScaleFactor
                            Repeater{
                                id : __materialsRepeater
                                model: kernel_parameter_manager.currentMachineObject().materialsModel()
                                delegate:  StyleCheckBox {
                                    checked: model.meChecked
                                    text: model.meName
                                    textColor: Constants.right_panel_item_text_default_color
                                    fontSize: Constants.labelFontPointSize_8

                                    onClicked:
                                    {
                                        kernel_parameter_manager.currentMachineObject().selectChanged(checked, model.meName, extruderIndex)
                                        printMaterialsModel.setChecked(checked, model.index)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            BasicGroupBox{
                width: parent.width
                anchors.horizontalCenter: parent.horizontalCenter
                implicitHeight:isExpand ? contentItem.implicitHeight + 30* screenScaleFactor : 40* screenScaleFactor
                title: qsTr("User Materials")
                backRadius: 5
                borderWidth: 1
                //                isExpand: false
                state: "canExpend"
                borderColor: Constants.dialogItemRectBgBorderColor
                contentItem:Column{
                    anchors.fill: parent
                    anchors.margins: 10*screenScaleFactor
                    BasicScrollView{
                        width: 924*screenScaleFactor
                        height: 100*screenScaleFactor
                        clip: true
                        Grid{
                            spacing: 100*screenScaleFactor
                            width: parent.width
                            columns: 4
                            columnSpacing: 150*screenScaleFactor
                            rowSpacing: 20*screenScaleFactor
                            Repeater{
                                id : __userMaterialsRepeater
                                model: kernel_parameter_manager.currentMachineObject().userMaterialsName
                                delegate:  StyleCheckBox {
                                    enabled: false
                                    checked: true
                                    text: modelData
                                    textColor: Constants.right_panel_item_text_default_color
                                    fontSize: Constants.labelFontPointSize_8

                                    onClicked:
                                    {

                                    }
                                }
                            }
                        }
                    }

                }
            }
        }

        BasicDialogButton {
            id: okBtn
            text: qsTr("OK")
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20*screenScaleFactor
            width: 120* screenScaleFactor
            height: 28* screenScaleFactor

            btnBorderW: 1 * screenScaleFactor
            btnTextColor: Constants.manager_printer_button_text_color
            borderColor: Constants.manager_printer_button_border_color
            defaultBtnBgColor: Constants.manager_printer_button_default_color
            hoveredBtnBgColor: Constants.manager_printer_button_checked_color
            selectedBtnBgColor: Constants.manager_printer_button_checked_color

            onSigButtonClicked: {
                kernel_parameter_manager.currentMachineObject().finishMeterialSelect(extruderIndex)
                idAddPrinterDlg.close()
            }
        }

    }

    function getMaterialTypeStatus(name){
        var materialtypes = kernel_parameter_manager.currentMachineObject().selectTypes();
        console.log("____materialType =",name)
        console.log("____checkTypes =",materialtypes)
        console.log("____ifContains = ", materialtypes.indexOf(name))

        if(materialtypes.indexOf(name)!=-1)
        {
            return true;
        }
        else{
            return false;
        }
    }

    function startShowMaterialDialog(_state, _extruderIndex, materialName, machineObject){
        extruderIndex = _extruderIndex
        curPrinter = kernel_parameter_manager.currentMachineName()
        console.log("______startShowMaterialDialog")
        cloader.item.repeaterView.model = null
        cloader.item.repeaterView.model =  kernel_parameter_manager.currentMachineObject().supportTypes()
        cloader.item.meterialView.model = null
        cloader.item.meterialView.model = Qt.binding(function(){ return  kernel_parameter_manager.currentMachineObject().materialsModel(); })
        cloader.item.userMeterialView.model = Qt.binding(function(){ return  kernel_parameter_manager.currentMachineObject().userMaterialsName; })
        visible = true
    }
}
