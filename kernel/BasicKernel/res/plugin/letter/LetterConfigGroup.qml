import QtQuick 2.10
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12
import CrealityUI 1.0
import "qrc:/CrealityUI"
ColumnLayout {
    id: rootElement
    property bool isOpen: false
    property string title: ""
    property string icon: ""
    default property alias accordionContent: contentPlaceholder.data
    spacing: 0
 
    Layout.fillWidth: true
 
    Rectangle {
        id: accordionHeader
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        height: 28 * screenScaleFactor
        radius: 5
        color: "transparent"

        Image {
            id: idImage
            anchors.left: parent.left
            anchors.leftMargin: 7
            anchors.verticalCenter: parent.verticalCenter
            height: 20 * screenScaleFactor
            width: 20 * screenScaleFactor
            source: icon
            sourceSize.width: 20
            sourceSize.height: 20
        }

        CusText {
            id: idText
            anchors.left: idImage.right
            anchors.leftMargin: 10 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            height: parent.height
            fontPointSize: Constants.labelFontPointSize_12
            hAlign: 0
            fontText:title
            fontColor: Constants.textColor
            fontWeight : Font.Medium
            font.pointSize: Constants.labelFontPointSize_12
            isDefault: true
        }

        Rectangle {
            anchors.left: idText.right
            anchors.leftMargin: 10 * screenScaleFactor
            anchors.right: parent.right
            anchors.rightMargin: 8 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            height: 1
            color: Constants.rectBorderColor
        }
 
    }
 
    // This will get filled with the content
    ColumnLayout {
        id: contentPlaceholder
        Layout.fillWidth: true
    }
}
