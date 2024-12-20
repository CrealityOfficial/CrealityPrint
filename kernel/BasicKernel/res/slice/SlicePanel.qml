import QtQuick 2.0
import Qt.labs.platform 1.1
import CrealityUI 1.0
import "qrc:/CrealityUI"
import QtQuick.Layouts 1.3

Item {
    id : idslice
    property var autoconfig
    property var settingconfig
    anchors.fill: parent
    property var machineType
    //    implicitWidth: 260
    property var defultLeftToolWidth : 48

    signal sigSliceView(bool visible)

    property var statusObj

    HalotBoxTotalRightUIPanel
    {
        id: paramPanel
		visible: kernel_kernel.currentPhase == 0
        width: 280 * screenScaleFactor
    }

    function showWizards()
    {
        idRightPanel.showWizards()
    }
}

