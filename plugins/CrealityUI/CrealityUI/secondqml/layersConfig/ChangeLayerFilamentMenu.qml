import QtQuick 2.13
import QtQuick.Controls 2.13
import QtGraphicalEffects 1.13

import "./"
import "../"
import "../../qml"
import "../menu"

BasicMenuStyle
{
    id: root
    contentWidth : 163 * screenScaleFactor
    hasIcon: false
    centerText: true

    signal requestDeletePause()
    signal requsetChangeExtruder(int index)

    function updateState() {
        let printer = kernel_reusable_cache.printerManager.selectedPrinter
        let machine = kernel_parameter_manager.currentMachineObject
        if (printer && machine)
            idFilamentSubMenu.enabled = !printer.hasModelInsideUseColors() && machine.extruderAllCount() > 1
        else
            idFilamentSubMenu.enabled = false
    }

    BasicMenuItemStyle {
        text:  qsTr("Delete Filament Change") 
        actionShortcut: ""
        enabled: true
        separatorVisible: false
        hasIcon: false
        centerText: true

        onTriggered: {
            requestDeletePause()
        }
    }
    
    FilamentSubMenu {
        id: idFilamentSubMenu
        enabled: true
        
        onExtruderChanged: {
            requsetChangeExtruder(index)
        }
    }
    
    background: Rectangle {
        implicitWidth:maxImplicitWidth + 20
        color: Constants.itemBackgroundColor//"#061F3B"
        border.width: 1
        border.color: "black"
        radius: 5

        Rectangle
        {
            id: mainLayout
            anchors.fill: parent
            anchors.margins: 5
            color: Constants.themeColor
            opacity: 1
        }

        DropShadow {
            anchors.fill: mainLayout
            horizontalOffset: 5
            verticalOffset: 5
            radius: 8
            samples: 17
            source: mainLayout
            color: Constants.dropShadowColor // "#333333"
        }
    }
}