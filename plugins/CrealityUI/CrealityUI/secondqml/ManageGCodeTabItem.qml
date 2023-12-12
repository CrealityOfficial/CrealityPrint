import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtQml 2.13

import ".."
import "../qml"
CustomTabViewItem {
    property var generalParam_model
    property var lineWidth_model
    property var retract_model

    property var currentMachineObj
    property int extruderCount: 1

    id: extrusion_2_view
    title: qsTr("Extrusion 2")

    function updateMachine(machine)
    {
        //gCodeModel.clear()
        currentMachineObj = machine
        idParameterContext.settings = currentMachineObj.settingsObject()
        idParameterContext.currentProfile = currentMachineObj.machineParameterModel("base", "")
        if(extruderCount>1)
        {
            idParameterContext1.settings = currentMachineObj.extruderObject(0).settingsObject()
            idParameterContext1.currentProfile = currentMachineObj.extruderObject(0).extruderParameterModel("base", true)

            idParameterContext2.settings = currentMachineObj.extruderObject(1).settingsObject()
            idParameterContext2.currentProfile = currentMachineObj.extruderObject(1).extruderParameterModel("base", true)
        }else{
            idParameterContext1.currentProfile = null;
            idParameterContext2.currentProfile = null;

        }
    }

    ParameterContext {
        id : idParameterContext
    }
    ParameterContext {
        id : idParameterContext1
    }
    ParameterContext {
        id : idParameterContext2
    }

    ScrollView{
        anchors.fill: parent
        anchors.margins: 10* screenScaleFactor
//        hpolicyVisible: false
//        vpolicyVisible: contentHeight > height
        clip: true
        Column{
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 10
            spacing: 10*screenScaleFactor
            Repeater{
                model:idParameterContext.currentProfile
                delegate: BasicGroupBox{
                    width: 802* screenScaleFactor
                    height: 198* screenScaleFactor
                    defaultBgColor: "transparent"
                    textBgColor: Constants.custom_tabview_panel_color
                    title: {
                        if(model.key === "machine_start_gcode")
                        {
                            return qsTr("Printer Start of G-code")
                        }
                        else if(model.key === "machine_end_gcode")
                        {
                            return qsTr("Printer End of G-code")
                        }
                        return qsTr("Printer Start of G-code")
                    }
                    state:"canNotExpend"
                    backRadius: 5
                    borderWidth: 1
                    visible: model.key === "machine_start_gcode" || model.key === "machine_end_gcode"
                    borderColor: Constants.dialogItemRectBgBorderColor
                    contentItem: ScrollView{
                      //  vpolicyVisible: start_gcode_editor.height > 180* screenScaleFactor
                        BasicDialogTextArea{
                            id: start_gcode_editor
                            width: 200
                            height: 100
                            borderWidth: 0
                            font.pointSize:Constants.labelFontPointSize_10
                            color: Constants.themeTextColor
                            text: model.str
                            onEditingFinished: {
                                console.log("+++param = ", model.index)
                                idParameterContext.textFieldEditFinish(model.key, this)
                            }
                        }
                    }
                }
            }
            Repeater{
                model:idParameterContext1.currentProfile
                delegate: BasicGroupBox{
                    width: 802* screenScaleFactor
                    height: 198* screenScaleFactor
                    defaultBgColor: "transparent"
                    textBgColor: Constants.custom_tabview_panel_color
                    title: {
                        if(model.key === "machine_extruder_start_code")
                        {
                            return qsTr("Extruder 1 Start of G-code")
                        }
                        if(model.key === "machine_extruder_end_code")
                        {
                            return qsTr("Extruder 1 End of G-code")
                        }
                    }
                    state:"canNotExpend"
                    backRadius: 5
                    borderWidth: 1
                    visible: model.key === "machine_extruder_start_code" || model.key === "machine_extruder_end_code"
                    borderColor: Constants.dialogItemRectBgBorderColor
                    contentItem: ScrollView{
                        BasicDialogTextArea{
                            id: start_gcode_editor2
                            width: 200
                            height: 100
                            borderWidth: 0
                            font.pointSize:Constants.labelFontPointSize_10
                            color: Constants.themeTextColor
                            text: model.str
                            onEditingFinished: {
                                console.log("+++param = ", model.index)
                                idParameterContext1.textFieldEditFinish(model.key, this)
                            }
                        }
                    }
                }
            }
            Repeater{
                model:idParameterContext2.currentProfile
                delegate: BasicGroupBox{
                    width: 802* screenScaleFactor
                    height: 198* screenScaleFactor
                    defaultBgColor: "transparent"
                    textBgColor: Constants.custom_tabview_panel_color
                    title: {
                        if(model.key === "machine_extruder_start_code")
                        {
                            return qsTr("Extruder 2 Start of G-code")
                        }
                        if(model.key === "machine_extruder_end_code")
                        {
                            return qsTr("Extruder 2 End of G-code")
                        }
                    }
                    state:"canNotExpend"
                    backRadius: 5
                    borderWidth: 1
                    visible: model.key === "machine_extruder_start_code" || model.key === "machine_extruder_end_code"
                    borderColor: Constants.dialogItemRectBgBorderColor
                    contentItem: ScrollView{
                     //   vpolicyVisible: start_gcode_editor.height > 180* screenScaleFactor
                        BasicDialogTextArea{
                            id: start_gcode_editor3
                            width: 200
                            height: 100
                            borderWidth: 0
                            font.pointSize:Constants.labelFontPointSize_10
                            color: Constants.themeTextColor
                            text: model.str
                            onEditingFinished: {
                                console.log("+++param = ", model.index)
                                idParameterContext2.textFieldEditFinish(model.key, this)
                            }
                        }
                    }
                }
            }

//            Item{
//                width: 10
//                height: 20*screenScaleFactor
//            }
        }
    }
}
