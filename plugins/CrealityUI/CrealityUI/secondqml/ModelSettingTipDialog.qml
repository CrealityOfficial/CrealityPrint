import QtQuick 2.15
import QtQuick.Controls 2.15

import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/qml"

DockItem {
  signal confirmButtonClicked(bool show_next_time)

  id: root

  width: 600 * screenScaleFactor
  height: 160 * screenScaleFactor
  title: qsTr("Edit Process Settings")

  Item {
    id: panel
    anchors.fill: parent
    anchors.margins: 20 * screenScaleFactor
    anchors.topMargin: 20 * screenScaleFactor + root.currentTitleHeight

    Text {
      id: message_text
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.top: parent.top
      anchors.topMargin: 10 * screenScaleFactor
      verticalAlignment: Qt.AlignVCenter
      horizontalAlignment: Qt.AlignHCenter
      font: Constants.font
      color: Constants.textColor
      text: qsTr("Switched to object settings mode, where you can edit the parameters of the selected object.")
    }

    Button {
      id: confirm_button
      anchors.horizontalCenter: parent.horizontalCenter
      anchors.bottom: parent.bottom
      width: implicitContentWidth + 40 * screenScaleFactor
      height: 28 * screenScaleFactor

      onClicked: {
        confirmButtonClicked(check_box.checkState !== Qt.Checked)
        root.close()
      }

      background: Rectangle {
        radius: height / 2
        color: parent.hovered ? Constants.profileBtnHoverColor : Constants.profileBtnColor
      }

      contentItem: Text {
        verticalAlignment: Qt.AlignVCenter
        horizontalAlignment: Qt.AlignHCenter
        font: Constants.font
        color: Constants.textColor
        text: qsTr("Confirm")
      }
    }

    CheckBox {
      id: check_box

      anchors.left: parent.left
      anchors.bottom: parent.bottom
      width: 16 * screenScaleFactor
      height: width

      text: qsTr("Don't ask me again")
      spacing: 5 * screenScaleFactor

      background: Rectangle {
        radius: 3 * screenScaleFactor
        border.width: 1 * screenScaleFactor
        border.color: parent.hovered ? Constants.textRectBgHoveredColor
                                     : Constants.dialogItemRectBgBorderColor
        color: parent.checkState === Qt.Checked ? Constants.themeGreenColor : "transparent"
      }

      contentItem: Text {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.right
        anchors.leftMargin: parent.spacing
        verticalAlignment: Qt.AlignVCenter
        horizontalAlignment: Qt.AlignLeft
        font: Constants.font
        color: Constants.textColor
        text: parent.text
      }

      indicator: Image {
        width: 9 * screenScaleFactor
        height: 6 * screenScaleFactor
        anchors.centerIn: parent
        visible: check_box.checkState === Qt.Checked
        source: "qrc:/UI/images/check2.png"
      }
    }
  }
}
