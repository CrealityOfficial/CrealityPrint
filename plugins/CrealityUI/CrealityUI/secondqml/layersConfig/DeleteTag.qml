import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Templates 2.12 as T
import QtGraphicalEffects 1.12
import "../../qml"
import "../../components"

Rectangle{
    id: idRoot
    
    signal requestDelete()

    width: 20* screenScaleFactor
    height: 20* screenScaleFactor
    anchors.left: parent.left
    anchors.leftMargin: 20 * screenScaleFactor
    anchors.verticalCenter: idHandle_second.verticalCenter
    visible: false
    radius: height/2
    color: dele_img.isHover ? "#FD265A" : "transparent"
    border.width: dele_img.isHover ? 0 : 1
    border.color: "#4B4B4D"
    Image {
        id:dele_img
        source: "qrc:/UI/photo/slider/gcode_delete.svg"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        sourceSize.width: 8* screenScaleFactor
        sourceSize.height: 8* screenScaleFactor
        property bool isHover: false
        MouseArea{
            hoverEnabled: true
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onEntered: dele_img.isHover = true
            onExited: dele_img.isHover = false
            onClicked: {
                // kernel_slice_model.delCustomGcode()
                // let obj = gcodeImage.find(f=>Math.abs(parseInt(f.y)-parseInt(idHandle_second.y)) < 3)
                // if(obj) {
                //     obj.destroy();
                //     delete obj;
                // }
                // gcodeDele.visible =false
                idRoot.requestDelete()
                idRoot.visible = false
            }

        }
    }

}