
import QtQuick 2.6
import QtQuick.Controls 2.0

TextField {
    id: control
    property alias unitchar : unit.text
    property color unitColor: Constants.unitColor
    text:"0.0"
    property color borderNormalColor: Constants.rectBorderColor
    property color borderHoverColor: Constants.hoveredColor
    property var value:0
    property var userWidth: 160
    property var uesrHeight: 28
    property alias fontPointSize: unit.font.pointSize
    selectByMouse: true
    signal valueEdited
    font.family: Constants.labelFontFamily
    font.weight: Constants.labelFontWeight
    //placeholderText: qsTr("Enter description")
    color: Constants.textColor
    padding: 0
    background: Rectangle {
        radius: 5
        anchors.fill: parent
        color:Constants.textBackgroundColor
        border.color: control.focus  ? borderHoverColor : borderNormalColor
        Text {
            id: unit
            anchors.right: parent.right
            anchors.rightMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            //anchors.horizontalCenter: parent.horizontalCenter
            color: unitColor
            text: "mm"
//            font:control.font
            verticalAlignment: Text.AlignVCenter
        }
    }
    validator:DoubleValidator {
        locale: "C"
    }


    onTextChanged: {
        value = text.valueOf()

        valueEdited()
    }
}
