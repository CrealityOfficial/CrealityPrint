import QtQuick 2.0
import ".."
import "../qml"
Item {
    property string iconSource: ""
    property string labelText: ""
    implicitHeight: parent.height
    implicitWidth: parent.width
    property real imgWidth: 14
    Row
    {
        anchors.fill: parent
        spacing: 6
        Image {
            id: __image
            anchors.verticalCenter: parent.verticalCenter
            height: imgWidth
            width: imgWidth
              fillMode: Image.PreserveAspectFit
              source: iconSource
        }
        Item
        {
            width: imgWidth
            visible: !(iconSource == "")
        }

        StyledLabel
        {
            text: labelText
            anchors.verticalCenter: parent.verticalCenter
            color: enabled ? Constants.textColor : Constants.disabledTextColor
            font.pixelSize:12
        }
    }

}
