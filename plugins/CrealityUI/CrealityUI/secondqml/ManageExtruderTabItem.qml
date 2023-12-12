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

    id: extrusion_2_view
    title: qsTr("Extrusion 2")
    //enabled: root.extruderCount > 1

    function updateMachine(machine,extruderIndex,isAdvance)
    {
        var currentExtruder = machine.extruderObject(extruderIndex)

        generalParam_model = currentExtruder.extruderParameterModel("base", isAdvance)
        lineWidth_model = currentExtruder.extruderParameterModel("line", isAdvance)
        retract_model = currentExtruder.extruderParameterModel("retraction", isAdvance)
        
        idGeneral.update(generalParam_model,currentExtruder.settingsObject())
        idLineWidth.update(lineWidth_model,currentExtruder.settingsObject())
        idRetract.update(retract_model,currentExtruder.settingsObject())
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
            
            ExtruderTabGeneral {
                id:idGeneral
                title: qsTr("General parameters")
            }
            ExtruderTabLineWidth {
                id:idLineWidth
                title: qsTr("Line Width")
            }
            ExtruderTabRetract {
                id:idRetract
                title: qsTr("Retract")
            }

            Item{
                width: 10
                height: 30*screenScaleFactor
            }
        }
    }
}
