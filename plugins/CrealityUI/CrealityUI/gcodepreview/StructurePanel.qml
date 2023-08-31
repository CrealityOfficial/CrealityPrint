import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13

import "../qml"

Rectangle {
  id : root

  readonly property string title: qsTr("Structure Type")
  property bool isReset: false

  function resetChecked() {
    isReset = true
    isReset = false
  }

  color: "transparent"

  RowLayout {
    id: title_layout

    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.leftMargin: 25 * screenScaleFactor
    anchors.rightMargin: 25 * screenScaleFactor
    height: 20 * screenScaleFactor
    spacing: 5 * screenScaleFactor

    Label {
      id: type_title_label

      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.fillWidth: true
      horizontalAlignment: Qt.AlignLeft
      verticalAlignment: Qt.AlignVCenter

      text: qsTr("Type")
      color: Constants.textColor
      font.family: Constants.labelFontFamily
      font.weight: Constants.labelFontWeight
      font.pointSize: Constants.labelFontPointSize_9
    }

    Label {
      id: time_title_label

      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
      Layout.minimumWidth: 60 * screenScaleFactor
      horizontalAlignment: Qt.AlignHCenter
      verticalAlignment: Qt.AlignVCenter

      text: qsTr("Time")
      color: Constants.textColor
      font.family: Constants.labelFontFamily
      font.weight: Constants.labelFontWeight
      font.pointSize: Constants.labelFontPointSize_9
    }

    Label {
      id: percent_title_label

      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
      Layout.minimumWidth: 60 * screenScaleFactor
      horizontalAlignment: Qt.AlignHCenter
      verticalAlignment: Qt.AlignVCenter

      text: qsTr("Percent")
      color: Constants.textColor
      font.family: Constants.labelFontFamily
      font.weight: Constants.labelFontWeight
      font.pointSize: Constants.labelFontPointSize_9
    }
  }

  ListView {
    anchors.top: title_layout.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.topMargin: 10 * screenScaleFactor

    clip: true

    model: kernel_slice_model.structureModel
    delegate: RowLayout {
      width: parent.width
      height: 20 * screenScaleFactor
      spacing: 5 * screenScaleFactor

      Rectangle {
        id: color_rect

        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        height: parent.height
        width: height

        color: model_color
        border.width: 1 * screenScaleFactor
      }

      Label {
        id: name_label

        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        Layout.fillWidth: true
        horizontalAlignment: Qt.AlignLeft
        verticalAlignment: Qt.AlignVCenter

        text: qsTr(model_name)
        color: Constants.textColor
        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: Constants.labelFontPointSize_9
      }

      Label {
        id: time_label

        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        Layout.minimumWidth: time_title_label.Layout.minimumWidth
        horizontalAlignment: Qt.AlignRight
        verticalAlignment: Qt.AlignVCenter

        text: model_time
        color: Constants.textColor
        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: Constants.labelFontPointSize_9
      }

      Label {
        id: percent_label

        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        Layout.minimumWidth: percent_title_label.Layout.minimumWidth
        horizontalAlignment: Qt.AlignRight
        verticalAlignment: Qt.AlignVCenter

        text: "%1%".arg(model_percent.toFixed(1))
        color: Constants.textColor
        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: Constants.labelFontPointSize_9
      }

      CheckBox {
        id: check_box

        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        height: parent.height
        width: height

        checked: model_checked
        onCheckedChanged: {
          kernel_slice_model.structureModel.checkItem(model_type, this.checked)
        }

        indicator: Rectangle {
          anchors.verticalCenter: parent.verticalCenter
          anchors.right: parent.right
          width: 16 * screenScaleFactor
          height: 16 * screenScaleFactor

          opacity: check_box.enabled ? 1 : 0.7
          radius: 3 * screenScaleFactor
          border.width: 1 * screenScaleFactor
          border.color: check_box.checked || check_box.hovered
                          ? "#1E9BE2" : Constants.right_panel_border_default_color

          color: check_box.checked ? "#1E9BE2" : "transparent"

          Image {
            width: 9 * screenScaleFactor
            height: 6 * screenScaleFactor
            anchors.centerIn: parent
            source: "qrc:/UI/images/check2.png"
            visible: check_box.checked
          }
        }
      }
    }
  }
}
