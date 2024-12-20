import QtQuick 2.0
import QtQuick.Layouts 1.13
import CrealityUI 1.0
import "qrc:/CrealityUI"

Rectangle {
    color: "transparent"
    width: 58 * screenScaleFactor
    height: 58 * screenScaleFactor
    property real dialogPos: 0 //0 right; 1 left

    CusImglButton {
        id: mouseButton
        enabledIconSource:"qrc:/UI/photo/mouseIcon.png"
        pressedIconSource:"qrc:/UI/photo/mouseIcon_d.png"
        borderWidth: 0
        //borderColor: "#3e4448"
        defaultBtnBgColor: "#4b4b4d"
        hoveredBtnBgColor: "#68686b"
        selectedBtnBgColor: "#1e9be2"
        width: parent.width
        height: parent.height
        anchors.centerIn: parent
        onClicked:{
            mouseInfo.visible = !mouseInfo.visible
        }
    }

    MouseInfo{
        id: mouseInfo

    }
}
