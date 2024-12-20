import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13

import "../qml"

Rectangle {
  id: root

  readonly property string title: qsTr("Temperature")
  readonly property string unit: "℃"

  color: "transparent"

  StyledLabel {
    id: title_label

    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right

    text: "%1 (%2)".arg(root.title).arg(root.unit)
    color: "#ffffff"
    font.family: Constants.labelFontFamily
    font.weight: Constants.labelFontWeight
    font.pointSize: Constants.labelFontPointSize_9
  }

  ListView {
    anchors.top: title_label.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.topMargin: 10 * screenScaleFactor

    clip: true

    model: kernel_slice_model.temperatureModel
    delegate: RowLayout {
      width: title_label.width
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
        id: value_label

        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

        text: String(model_value.toFixed(2))
        color:"#ffffff"
        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: Constants.labelFontPointSize_9
      }

      Item { Layout.fillWidth: true }
    }
  }
}
