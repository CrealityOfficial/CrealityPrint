import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13

import "../qml"

Rectangle {
  id: root

  readonly property string title: qsTr("Nozzle")
  property int nozzlenum: 1

  color: "transparent"

  ListView {
    anchors.fill: parent
    clip: true

    model: kernel_slice_model.extruderModel

    delegate: RowLayout {
      width: parent.width
      height: 20 * screenScaleFactor
      spacing: 5 * screenScaleFactor

      visible: index < root.nozzlenum

      Rectangle {
        id: color_rect

        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

        height: parent.height
        width: height
        color: model_color
        border.width: 1 * screenScaleFactor
      }

      Label {
        id: text_label

        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

        text: "%1 %2".arg(root.title).arg(model_index)
        color: Constants.textColor
        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: Constants.labelFontPointSize_9
      }

      Item { Layout.fillWidth: true }
    }
  }
}
