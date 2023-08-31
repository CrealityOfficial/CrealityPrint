import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtQml 2.13

import ".."
import "../qml"
CustomTabViewItem {
    property var basicParams_model
    property var currentMachineObj
    id: basic_param_view
    title: qsTr("Basic Parameters")
    anchors.margins: 10
    width: 800* screenScaleFactor
    ParameterContext {
        id : idParameterContext
    }

    function updateMachine(machine)
    {
        currentMachineObj = machine
        idParameterContext.settings = currentMachineObj.settingsObject()
        idParameterContext.currentProfile = currentMachineObj.machineParameterModel("base", "")
        basicParams_model = currentMachineObj.machineParameterModel("base", "")
    }

    ParameterGeneralCom{
        id:gCom
        parameterContext: idParameterContext
    }

    BasicScrollView{
        id: bsv
        anchors.fill: parent
        padding: 10* screenScaleFactor
        hpolicyVisible: false
        vpolicyVisible: contentHeight > height
        clip: true

        ColumnLayout{
            width: bsv.contentItem.width
            Repeater{
                model: basicParams_model
                delegate: gCom.itemRow
            }

            Item{
                Layout.fillHeight: true
            }
        }
    }
}



