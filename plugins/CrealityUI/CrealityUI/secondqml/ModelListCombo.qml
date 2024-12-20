import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.13
import QtQml 2.13

import ".."
import "../qml"

Item {
    id: mlRec
    width: 58 * screenScaleFactor
    height: 58 * screenScaleFactor
    property real sizeX
    property real sizeY
    property real sizeZ

    onSizeXChanged: {

    }

    CusImglButton {
        visible: true
        id: mouseButton
        width: mlRec.width
        height: mlRec.height
        anchors.centerIn: mlRec

        borderWidth: Constants.currentTheme ? 1 : (hovered ? 0 : 1)
        borderColor: Constants.currentTheme ? "#D6D6DC" : (hovered ? "transparent" : "#56565C")
        enabledIconSource: Constants.currentTheme ? "qrc:/UI/photo/modelListIcon_lite.svg" :  "qrc:/UI/photo/modelLIstIcon.svg"
        highLightIconSource: Constants.currentTheme ?  "qrc:/UI/photo/modelListIcon_lite.svg" : "qrc:/UI/photo/modelLIstIcon_h.svg"
        pressedIconSource: "qrc:/UI/photo/modelLIstIcon.svg"
        defaultBtnBgColor: Constants.currentTheme ? "#F2F2F5" : "#363638"
        hoveredBtnBgColor: Constants.currentTheme ? "#DBF2FC" : "#6D6D71"
        selectedBtnBgColor: "transparent"
        onClicked: mouseOptPopup.open()
        cusTooltip.text : qsTr("Model List")
        state: "imgOnly"
    }

    Rectangle {
        radius: height / 2
        width: 20 * screenScaleFactor
        height: 20 * screenScaleFactor
        anchors.rightMargin: -10 * screenScaleFactor
        anchors.right: parent.right
        anchors.top: parent.top

        visible: !mouseOptPopup.visible
        color: Constants.currentTheme ? "#1E9BE2" : "#FFFFFF"

        Text {
            anchors.centerIn: parent
            verticalAlignment: Text.AlignVCenter
            font.weight: Font.Bold
            font.family: Constants.mySystemFont.name
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.currentTheme ? "#F2F2F5" : "#363638"
            text: kernel_modelspace.modelNNum
        }
    }

    ModelListPop{
        id: mouseOptPopup
        x: mouseButton.x
        y: mouseButton.y - mouseOptPopup.height + mouseButton.height
    }
}
