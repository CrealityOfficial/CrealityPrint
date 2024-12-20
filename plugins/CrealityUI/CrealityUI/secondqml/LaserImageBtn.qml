import QtQuick 2.10
import QtQuick.Layouts 1.13
import "../qml"
 
Item{
    width: 52
    height: 60

    property var btnSelected: false
    property var isHovered: false
    property var btnText: "test"
    property var imgSrc: "qrc:/UI/photo/machineImage_Printer_Box.png"

    signal sigBtnClicked()

    MouseArea{
        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            sigBtnClicked()
        }

        onEntered: {
            isHovered = true
        }

        onExited:{
            isHovered = false
        }
    }

    Column{
        width: parent.width
        height: parent.height
        spacing: 8

        Rectangle{
            width: parent.width
            height: 40
            radius: 3
            border.color: (isHovered || btnSelected) ? "#1E9BE2" : Constants.modelAlwaysItemBorderColor
            border.width: 1

            BaseCircularImage{
                x: 1
                y: 1
                width: parent.width - 2
                height: parent.height - 2
                radiusImg: 3
                img_src: imgSrc
            }
        }

        StyledLabel{
            width: parent.width
            height: 12
            text: btnText
            color: (isHovered || btnSelected) ? "#1E9BE2" : Constants.textColor
            font.pointSize: Constants.labelFontPointSize_10
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
        }
    }

}
