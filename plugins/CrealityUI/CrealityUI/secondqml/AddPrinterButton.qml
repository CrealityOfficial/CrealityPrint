import QtQuick 2.10
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13

import "../qml"

Button {
  id: root

  property string typeName: ""
  property string printerName: ""

  readonly property bool isTypeButton: printerName == ""
  readonly property bool isPrinterButton: !isTypeButton

  readonly property bool isSelected: {
    kernel_add_printer_model.currentTypeName === this.typeName &&
    kernel_add_printer_model.currentPrinterName === this.printerName
  }

  readonly property alias isOpened: left_button.checked

  property string subButtonDefaultImage: ""
  property string subButtonSelectedImage: ""
  property color textColor: "black"

  implicitWidth: left_button.width + spacer.width + typeText.width //   125 * screenScaleFactor
  implicitHeight: 36 * screenScaleFactor

  background: Rectangle {
    color: "transparent"

    Button {
      id: left_button

      anchors.verticalCenter: parent.verticalCenter
      anchors.left: parent.left
      width: root.subButtonDefaultImage?13 * screenScaleFactor:0
      height: 13 * screenScaleFactor

      checkable: root.isTypeButton

      background: Rectangle {
        color: "transparent"
        Image {
          anchors.fill: parent
          source: root.isTypeButton ? root.isOpened ? root.subButtonSelectedImage
                                                    : root.subButtonDefaultImage
                                    : root.isSelected ? root.subButtonSelectedImage
                                                      : root.subButtonDefaultImage
          sourceSize: Qt.size(left_button.width, left_button.height)
        }
      }
    }

    Rectangle {
      id: spacer

      anchors.top: parent.top
      anchors.bottom: parent.bottom
      anchors.left: left_button.right

      width: root.subButtonDefaultImage?10 * screenScaleFactor:0
      color: "transparent"
    }

    Button {
      id: right_button

      anchors.top: parent.top
      anchors.bottom: parent.bottom
      anchors.left: spacer.right
      anchors.right: parent.right

      background: Rectangle {
        color: "transparent"
        Text {
          anchors.verticalCenter: parent.verticalCenter
          anchors.left: parent.left
          horizontalAlignment: Text.AlignLeft
          id:typeText
          font.weight: Font.Medium
          font.family: Constants.mySystemFont.name
          font.pointSize: Constants.labelFontPointSize_9
          color: root.hovered?  Constants.themeGreenColor: root.isSelected ?  Constants.themeGreenColor : root.textColor
          text: root.isTypeButton ? qsTranslate("AddPrinterDlgNew", root.typeName) : root.printerName
        }
      }

      onClicked: {
        kernel_add_printer_model.selectPrinter(root.typeName, root.printerName)
      }
    }
  }
}
