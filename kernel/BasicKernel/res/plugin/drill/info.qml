import QtQuick 2.10
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5

import CrealityUI 1.0

import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"

LeftPanelDialog {
  id: root

  width: 300 * screenScaleFactor
  height: 230 * screenScaleFactor
  title: qsTr("Drill")

  property var com: null

  function execute() {
    console.log("drill panel execute");
  }

  MouseArea{//捕获鼠标点击空白地方的事件
      anchors.fill: parent
  }

  Item {
    anchors.fill: root.panelArea

    ColumnLayout {
      anchors.fill: parent
      anchors.margins: 20 * screenScaleFactor
      spacing: 5 * screenScaleFactor

      RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 30 * screenScaleFactor

        CusText {
          Layout.alignment: Qt.AlignLeft | Qt.AlignHCenter
          Layout.fillWidth: true

          fontText: qsTr("Shape:")
          fontColor: Constants.textColor
          hAlign: 0
        }

        BasicCombobox {
          Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
          Layout.preferredWidth: 153 * screenScaleFactor
          Layout.preferredHeight: 28 * screenScaleFactor

          model: ListModel {
            ListElement { text: qsTr("Circle") }
            ListElement { text: qsTr("Triangle") }
            ListElement { text: qsTr("Square") }
          }

          currentIndex: com.shape
          onCurrentIndexChanged: {
            com.shape = this.currentIndex;
          }
        }
      }

      RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 30 * screenScaleFactor

        CusText {
          Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
          Layout.fillWidth: true

          fontText: qsTr("Radius:")
          fontColor: Constants.textColor
          hAlign: 0
        }

        StyledSpinBox {
          Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
          Layout.preferredWidth: 153 * screenScaleFactor
          Layout.preferredHeight: 28 * screenScaleFactor
          radius: 5 * screenScaleFactor

          unitchar: "mm"
          realFrom: 1
          realTo: 20
          realStepSize: 0.05
          realValue: com.radius

          onRealValueChanged: {
            com.radius = this.realValue;
          }
        }
      }

      RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 30 * screenScaleFactor

        CusText {
          Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
          Layout.fillWidth: true

          enabled: !one_layer_only_box.checked
          opacity: enabled ? 1 : 0.8

          fontText: qsTr("Depth:")
          fontColor: Constants.textColor
          hAlign: 0
        }

        StyledSpinBox {
          Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
          Layout.preferredWidth: 153 * screenScaleFactor
          Layout.preferredHeight: 28 * screenScaleFactor
          radius: 5 * screenScaleFactor

          enabled: !one_layer_only_box.checked
          opacity: enabled ? 1 : 0.8

          unitchar: "mm"
          realFrom: 1
          realTo: 100
          realStepSize: 0.01
          realValue: com.depth

          onRealValueChanged: {
            com.depth = this.realValue;
          }
        }
      }

      RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 30 * screenScaleFactor

        CusText {
          Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
          Layout.fillWidth: true

          fontText: qsTr("Direction:")
          fontColor: Constants.textColor
          hAlign: 0
        }

        BasicCombobox {
          Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
          Layout.preferredWidth: 153 * screenScaleFactor
          Layout.preferredHeight: 28 * screenScaleFactor

          model: ListModel {
            ListElement { text: qsTr("Normal Direction") }
            ListElement { text: qsTr("Parallel to Platform") }
            ListElement { text: qsTr("Perpendicular to Screen") }
          }

          currentIndex: com.direction
          onCurrentIndexChanged: {
            com.direction = this.currentIndex;
          }
        }
      }

      StyleCheckBox {
        id: one_layer_only_box

        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        Layout.topMargin: 10 * screenScaleFactor

        checked: com.oneLayerOnly
        text: qsTr("Pierce One Layer Only")

        onClicked: {
          com.oneLayerOnly = this.checked
        }
      }
    }
  }
}
