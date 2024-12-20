import QtQuick 2.7
import  QtQuick.Controls 2.1

Button{
    id: control

    property var normalIconSource
    property var hoverIconSource
    property int iconWidth: 10
    property int iconHeight: 10
    property var hoverBgColor:Constants.tittleBarBtnColor
    property var borderwidth: 1
    property int buttonRadius: 3
    property color hoveredBdColor: "#4D4D4D"

    font.family: Constants.labelFontFamily
    font.weight: Constants.labelFontWeight
    contentItem: Item
    {
        anchors.fill: parent
        Image
        {
            anchors.centerIn: parent
            width: control.iconWidth
            height: control.iconHeight
            source:  normalIconSource
            fillMode: Image.PreserveAspectFit
        }
    }

    background: Rectangle
    {
        anchors.fill: parent
        implicitWidth: 30
        implicitHeight: 30

        color: control.hovered? hoverBgColor:"transparent"
        //opacity: enabled ? 1 : 0.3
        visible: true
        border.width : control.hovered ? borderwidth : 0
        border.color : hovered? hoveredBdColor : "#4D4D4D"
        radius : control.buttonRadius
    }
}
