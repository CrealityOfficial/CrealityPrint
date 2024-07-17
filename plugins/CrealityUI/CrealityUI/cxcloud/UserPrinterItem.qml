import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13

import ".."
import "../qml"

Button {
  id: root

  signal requestControl

  property string printerUid: ""
  property string printerIp: ""
  property string printerName: ""
  property string printerImage: ""
  property bool printerOnline: true
  property bool printerConnected: true
  property bool tfCardExist: true

  width: 470 * screenScaleFactor
  height: 285 * screenScaleFactor

  onClicked: {
    requestControl()
  }

  background: Rectangle {
    radius: 10 * screenScaleFactor

    color: "#FFFFFF"
    border.color: root.hovered ? "#00A3FF" : Constants.dock_border_color
    border.width: root.hovered ? 4 * screenScaleFactor : 1 * screenScaleFactor

    Rectangle {
      id: image

      anchors.top: parent.top
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.margins: 10 * screenScaleFactor

      width: 200 * screenScaleFactor

      color: "#E0E0E0"

      Image {
        anchors.fill: parent

        fillMode: Image.PreserveAspectFit

        source: printerImage !== "" ? printerImage : "qrc:/UI/photo/userInfo_imageDefault_dark.png"
      }
    }

    Text {
      id: text

      anchors.top: parent.top
      anchors.bottom: parent.bottom
      anchors.left: image.right
      anchors.right: parent.right
      anchors.margins: 20 * screenScaleFactor

      verticalAlignment: Text.AlignTop
      horizontalAlignment: Text.AlignLeft

      font.weight: Font.Medium
      font.family: Constants.mySystemFont.name
      font.pointSize: Constants.labelFontPointSize_9

      lineHeightMode: Text.FixedHeight
      lineHeight: 32 * screenScaleFactor

      text: "<font color='#999999'>%1: </font><font color='#333333'>%2</font><br>".arg(qsTr("Device Name")).arg(printerName)
          + "<font color='#999999'>%1: </font><font color='#333333'>%2</font><br>".arg(qsTr("Device ID")).arg(printerUid)
          + "<font color='#999999'>%1: </font><font color='#333333'>%2</font><br>".arg(qsTr("Status")).arg(printerOnline ? qsTr("Online") : qsTr("Offline"))
          + "<font color='#999999'>%1: </font><font color='#333333'>%2</font><br>".arg(qsTr("TF Card Status")).arg(tfCardExist ? qsTr("Card Inserted") : qsTr("No Card Inserted"))
          + "<font color='#999999'>%1: </font><font color='#333333'>%2</font><br>".arg(qsTr("Printer Status")).arg(printerConnected ? qsTr("Connected") : qsTr("Disconnected"))
          + "<font color='#999999'>%1: </font><font color='#333333'>%2</font>".arg(qsTr("IP")).arg(printerIp)
    }
  }
}
