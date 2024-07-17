import "../qml"
import "../secondqml"
import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13

Rectangle {
    id: idSlicePreview

    property var fdmMainObj: ""
    property int defaultMainWidth: fdmMainObj.width
    property int defaultMainHeight: fdmMainObj.height
    property alias aFDMRightPanel: idFDMRightPanel

    function update() {
        idFDMRightPanel.update();
    }

    function endPreview() {
        idFDMRightPanel.showPlatform(true);
        idSlicePreview.visible = false;
        idFDMRightPanel.revertStructureColor();
    }

    width: 300 * screenScaleFactor
    height: contentColumn.height //634 * screenScaleFactor
    visible: kernel_kernel.currentPhase === 1
    color: "transparent"
    //    color: Qt.rgba(0, 0, 0, 0.5)
    border.width: 0
    radius: 5 * screenScaleFactor
    onVisibleChanged: {
        if (kernel_kernel.currentPhase === 0)
            idFDMRightPanel.showPlatform(true);

    }

    Column {
        id: contentColumn

        width: parent.width

        // Rectangle {
        //     width: parent.width
        //     height: 10 * screenScaleFactor
        //     color: "transparent"
        // }

        FDMRightPanel {
            id: idFDMRightPanel

            color: "transparent"
            width: parent.width
            height: screenScaleFactor<=1? 650 * screenScaleFactor :500 * screenScaleFactor
            visible: idSlicePreview.width > 10 ? true : false
        }

    }

}
