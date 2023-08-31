import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtQml 2.13

import ".."
import "../qml"

Item {
    id: root
    property string currentMachine: ""
    property var currentMachineObj: null

    property var generalParam_model
    property var lineWidth_model
    property var retract_model
    property var basicParams_model //基本参数
    property var moveAbility_speed_model //移动能力_速度限制
    property var moveAbility_acceleration_model //移动能力_加速度限制
    property var moveAbility_jitter_model //移动能力_抖动限制
    property var currentExtruder
    property var extruderModel
    property bool isAdvanced: false

    property int extruderCount: 0

    function saveMachineParam(){
        currentExtruder.save()
    }

    function updateFromMachine(machine){
        currentMachineObj = machine
        if(!currentMachineObj)
            return            
        basic_param_view.updateMachine(machine)
        move_view.updateMachine(machine)
        updateExtruder(machine) 
        gCode_view.extruderCount = extruderCount
        gCode_view.updateMachine(machine)
    }

    function updateExtruder(machine){
        var strCount = machine.settingsObject().settingObject("machine_extruder_count").str()
        extruderCount = Number(strCount)

        extrusion_1_view.updateMachine(machine,0,isAdvanced)
        if(extruderCount>1)
        {
            extrusion_2_view.updateMachine(machine,1,isAdvanced)
        }
    }

    function valchanged(key, value){
    }

    function valChanged_Extruder(key, value, index){
    }



    function selectTabViewDefaultPanel() {
        tab_view.selectDefaultPanel()
    }


    ColumnLayout {
        anchors.fill: parent
        spacing: 10 * screenScaleFactor      

        CustomTabView {
            id: tab_view
            Layout.fillWidth: true
            Layout.fillHeight: true
            panelColor: Constants.custom_tabview_panel_color
            defaultPanel: basic_param_view
            borderColor: Constants.splitLineColor
            buttonTextDefaultColor: Constants.manager_printer_tabview_default_color
            buttonTextCheckedColor: Constants.manager_printer_tabview_checked_color
            titleBarRightMargin: width - (240 * screenScaleFactor) * (extrusion_2_view.enabled ? 3 : 2)

            onCurrentPanelChanged: {
                switch (this.currentPanel) {
                case basic_param_view:
                    break
                case extrusion_1_view:
                    break
                case extrusion_2_view:
                    break
                default:
                    break
                }
            }

            readonly property int rowSpacing: 2 * screenScaleFactor
            readonly property int columnSpacing: 20 * screenScaleFactor
            readonly property int itemHeight: 28 * screenScaleFactor
            readonly property int labelWidth: 160 * screenScaleFactor
            readonly property int editorWidth: 230 * screenScaleFactor
            ManageBasicTabItem {
                id: basic_param_view
                title: qsTr("Basic Parameters")
            }

            ManageMoveTabItem {
                id:move_view
                title: qsTr("Move Ability")
            }

            ManageGCodeTabItem {
                id:gCode_view
                title: qsTr("G-code")
            }

            ManageExtruderTabItem {
                id:extrusion_1_view
                title: qsTr("Extrusion 1")
            }

            ManageExtruderTabItem {
                id:extrusion_2_view
                enabled: extruderCount > 1
                title: qsTr("Extrusion 2")
            }
        }
    }
}
