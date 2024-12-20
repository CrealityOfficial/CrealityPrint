import QtQuick 2.10
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import "../qml"
 
ColumnLayout {
    id: rootElement
    property bool isOpen: false
    property string title: ""
    default property alias accordionContent: contentPlaceholder.data
    spacing: 0
 
    Layout.fillWidth: true;
 
    Rectangle {
        id: accordionHeader
        color: Constants.right_panel_button_default_color
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true;
        height: 28*screenScaleFactor
        radius: 5

        border.width: 1
        border.color: Constants.right_panel_border_default_color
 
        StyledLabel{
            anchors.left: parent.left
            anchors.leftMargin: 18*screenScaleFactor
            width: parent.width - 46*screenScaleFactor
            height: parent.height

            text: rootElement.title
            color: Constants.right_panel_text_default_color
            font.pointSize: Constants.labelFontPointSize_9
            font.family: Constants.panelFontFamily
            verticalAlignment: Qt.AlignVCenter
        }

        Rectangle {
          id: imageBackground

          anchors.right: parent.right
          anchors.rightMargin: 16 * screenScaleFactor
          anchors.verticalCenter: parent.verticalCenter

          width: 24 * screenScaleFactor
          height: width
          radius: width / 2

          color: accordionHeader.hovered ? "#545456" : "transparent"

          Image{
              id: idIndicatImg
              anchors.centerIn: parent
              width: 8*screenScaleFactor
              height: 8*screenScaleFactor
              mipmap: true
              smooth: true
              cache: false
              asynchronous: true
              fillMode: Image.PreserveAspectFit
              source: Constants.laserFoldTitleDownImg
          }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
                rootElement.isOpen = !rootElement.isOpen
                if(rootElement.isOpen)
                {
                    idIndicatImg.source = Constants.laserFoldTitleUpImg
                }
                else
                {
                    idIndicatImg.source = Constants.laserFoldTitleDownImg
                }
            }
        }
    }
 
    // This will get filled with the content
    ColumnLayout {
        id: contentPlaceholder
        visible: rootElement.isOpen
        Layout.fillWidth: true;
    }
}
