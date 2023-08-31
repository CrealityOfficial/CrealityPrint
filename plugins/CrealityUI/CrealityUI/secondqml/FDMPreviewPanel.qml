import QtQuick 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5

import "../qml"
import "../secondqml"

Rectangle {
    id: idSlicePreview
    width: 300 * screenScaleFactor
    height: contentColumn.height//634 * screenScaleFactor
    visible: kernel_kernel.currentPhase == 1

    color: Constants.right_panel_menu_background_color
    border.color: Constants.right_panel_menu_border_color
    border.width: 1
    radius: 5

    property var fdmMainObj: ""

    property int defaultMainWidth: fdmMainObj.width
    property int defaultMainHeight: fdmMainObj.height
	property alias aFDMRightPanel : idFDMRightPanel
	
    onVisibleChanged: {
		if(kernel_kernel.currentPhase == 0)
			idFDMRightPanel.showPlatform(true)
    }

    function update()
    {
        idFDMRightPanel.update()
    }

    function endPreview()
    {
        idFDMRightPanel.showPlatform(true)
        idSlicePreview.visible = false
        idFDMRightPanel.revertStructureColor()
    }

    Column
    {
        id: contentColumn
        width: parent.width

        Rectangle {
            width: parent.width
            height: 10 * screenScaleFactor
            color: "transparent"
        }

        FDMRightPanel {
            id: idFDMRightPanel
            color: "transparent"
            width: parent.width
            height: 540 * screenScaleFactor
            visible: idSlicePreview.width > 10 ? true : false
        }
    }
}


