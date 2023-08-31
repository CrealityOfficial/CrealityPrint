import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtQml 2.13

import ".."
import "../qml"
CustomTabViewItem {
    property var moveAbility_speed_model
    property var moveAbility_acceleration_model
    property var moveAbility_jitter_model
    id: moveAbility
    title: qsTr("Move Ability")

    ParameterContext {
        id : idFeedrateParameterContext
    }
    ParameterContext {
        id : idAccelerationParameterContext
    }
    ParameterContext {
        id : idJerkParameterContext
    }
    function updateMachine(machine)
    {
        currentMachineObj = machine

        idFeedrateParameterContext.settings = currentMachineObj.settingsObject()
        idAccelerationParameterContext.settings = currentMachineObj.settingsObject()
        idJerkParameterContext.settings = currentMachineObj.settingsObject()

        moveAbility_speed_model = currentMachineObj.machineParameterModel("move", "feedrate")
        moveAbility_acceleration_model = currentMachineObj.machineParameterModel("move", "acceleration")
        moveAbility_jitter_model = currentMachineObj.machineParameterModel("move", "jerk")

        idFeedrateParameterContext.currentProfile = moveAbility_speed_model
        idAccelerationParameterContext.currentProfile = moveAbility_acceleration_model
        idJerkParameterContext.currentProfile = moveAbility_jitter_model
    }

    ParameterGeneralCom{
        id:gComFeedrate
        parameterContext: idFeedrateParameterContext
    }
    ParameterGeneralCom{
        id:gComAcceleration
        parameterContext: idAccelerationParameterContext
    }
    ParameterGeneralCom{
        id:gComJerk
        parameterContext: idJerkParameterContext
    }

    BasicScrollView{
        anchors.fill: parent
        anchors.margins: 10* screenScaleFactor
        hpolicyVisible: false
        vpolicyVisible: contentHeight > height
        clip: true
        Column{
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 10* screenScaleFactor
            spacing: 10*screenScaleFactor
            BasicGroupBox{
                width: 836* screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                implicitHeight:isExpand ? contentItem.implicitHeight + 30* screenScaleFactor : 40* screenScaleFactor
                title: qsTr("Speed Limit")
                backRadius: 5* screenScaleFactor
                borderWidth: 1* screenScaleFactor
                borderColor: Constants.dialogItemRectBgBorderColor
                defaultBgColor: "transparent"
                textBgColor: Constants.custom_tabview_panel_color
                contentItem:ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10 * screenScaleFactor
                    spacing: 1* screenScaleFactor
                    Repeater{
                        model: moveAbility_speed_model
                        delegate: gComFeedrate.itemRow
                    }
                    Item{
                        Layout.fillHeight: true
                    }
                }
            }
            BasicGroupBox{
                width: 836* screenScaleFactor
                implicitHeight:isExpand ? contentItem.implicitHeight + 30* screenScaleFactor : 40* screenScaleFactor
                title: qsTr("Acceleration Limit")
                backRadius: 5* screenScaleFactor
                borderWidth: 1* screenScaleFactor
                borderColor: Constants.dialogItemRectBgBorderColor
                defaultBgColor: "transparent"
                textBgColor:Constants.custom_tabview_panel_color
                contentItem:ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10 * screenScaleFactor
                    spacing: 2* screenScaleFactor
                    Repeater{
                        model: moveAbility_acceleration_model
                        delegate: gComAcceleration.itemRow
                    }
                    Item{
                        Layout.fillHeight: true
                    }
                }
            }
            BasicGroupBox{
                width: 836* screenScaleFactor
                implicitHeight:isExpand ? contentItem.implicitHeight + 30* screenScaleFactor : 40* screenScaleFactor
                title: qsTr("Jitter Limit")
                backRadius: 5* screenScaleFactor
                borderWidth: 1* screenScaleFactor
                borderColor: Constants.dialogItemRectBgBorderColor
                defaultBgColor: "transparent"
                textBgColor: Constants.custom_tabview_panel_color
                contentItem:ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10 * screenScaleFactor
                    spacing: 2* screenScaleFactor
                    Repeater{
                        model: moveAbility_jitter_model
                        delegate: gComJerk.itemRow
                    }
                    Item{
                        Layout.fillHeight: true
                    }
                }
            }
            Item{
                height: 20* screenScaleFactor
                width: 20* screenScaleFactor
            }
        }
    }
}
