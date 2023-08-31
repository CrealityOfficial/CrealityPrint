import QtQuick 2.10
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"

LeftPanelDialog {
  id: root

  width: 300 * screenScaleFactor
  height: (kernel_global_const.hollowFillEnabled ? 220 : 150) * screenScaleFactor
  title: qsTr("Hollow")

  property var com: null

  property var _thinknessItem: null
  property var _fillRatioItem: null
  property var _fillIndexItem: null

  function execute() {
    console.log("hollow panel execute");
    if (!com || !_thinknessItem || !_fillRatioItem || !_fillIndexItem) {
      return
    }

    _thinknessItem.realValue = com.getThickness()
    _fillRatioItem.realValue = com.getFillRatio()
    _fillIndexItem.currentIndex = com.isFillEnabled() ? 1 : 0
    _thinknessItem.valueEdited()
    _fillIndexItem.currentIndexChanged()
  }

  Item {
    anchors.fill: root.panelArea

    Component.onCompleted: {
      root._thinknessItem = thinkness_spin_box
      root._fillRatioItem = fill_ratio_spin_box
      root._fillIndexItem = fill_enabled_combo_box
    }

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
          fontText: qsTr("Thickness") + ":"
          fontColor: Constants.textColor
          hAlign: 0
        }

        StyledSpinBox {
          id: thinkness_spin_box

          Layout.alignment: Qt.AlignRight | Qt.AlignHCenter
          Layout.preferredWidth: 153 * screenScaleFactor
          Layout.preferredHeight: 28 * screenScaleFactor
          radius: 5 * screenScaleFactor

          unitchar: "mm"
          realFrom: 2
          realTo: com.minLengthInSelectionms / 2
          realStepSize: 0.5
          realValue: 2

          onValueEdited: {
            if (thinkness_spin_box.realValue > (com.minLengthInSelectionms / 2)) {
              hollow_button.enabled = false;
            } else {
              hollow_button.enabled = true;
            }
          }
        }
      }

      RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 30 * screenScaleFactor

        visible: kernel_global_const.hollowFillEnabled

        CusText {
          Layout.alignment: Qt.AlignLeft | Qt.AlignHCenter
          Layout.fillWidth: true
          fontText: qsTr("Fill Percentage") + ":"
          fontColor: Constants.textColor
          hAlign: 0
        }

        StyledSpinBox {
          id: fill_ratio_spin_box

          Layout.alignment: Qt.AlignRight | Qt.AlignHCenter
          Layout.preferredWidth: 153 * screenScaleFactor
          Layout.preferredHeight: 28 * screenScaleFactor
          radius: 5 * screenScaleFactor

          unitchar: "%"
          realFrom: 5
          realTo: 100
          realStepSize: 0.5
          realValue: 5

          onValueEdited: {
            console.log("fill_ratio_spin_box.value = ", realValue);

            if (thinkness_spin_box.realValue < 0.01) {
              thinkness_spin_box.realValue = 0.01
            }

            if (thinkness_spin_box.realValue > (com.minLengthInSelectionms / 2)) {
              hollow_button.enabled = false;
            } else {
              hollow_button.enabled = true;
            }
          }
        }
      }

      RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 30 * screenScaleFactor

        visible: kernel_global_const.hollowFillEnabled

        CusText {
          Layout.alignment: Qt.AlignLeft | Qt.AlignHCenter
          Layout.fillWidth: true
          fontText: qsTr("Fill Structure") + ":"
          fontColor: Constants.textColor
          hAlign: 0
        }

        BasicCombobox {
          id: fill_enabled_combo_box

          Layout.alignment: Qt.AlignRight | Qt.AlignHCenter
          Layout.preferredWidth: 153 * screenScaleFactor
          Layout.preferredHeight: 28 * screenScaleFactor

          displayText: model.get(currentIndex).text

          model: ListModel {
            ListElement {
              text: qsTr("None")
            }

            ListElement {
              text: qsTr("Grid")
            }
          }

          currentIndex: 0
          onCurrentIndexChanged: {
            if (currentIndex == 0) {
              fill_ratio_spin_box.enabled = false;
            } else {
              fill_ratio_spin_box.enabled = true;
            }
          }
        }
      }

      BasicDialogButton {
        id: hollow_button

        Layout.alignment: Qt.AlignCenter
        Layout.minimumWidth: 258 * screenScaleFactor
        Layout.minimumHeight: 28 * screenScaleFactor
        Layout.topMargin: 15 * screenScaleFactor

        text: qsTr("Hollow")
        btnRadius: height/2
        opacity: enabled ? 1 : 0.8
        defaultBtnBgColor: Constants.leftToolBtnColor_normal
        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
        onSigButtonClicked: {
          console.log("thinkness_spin_box.value =" + thinkness_spin_box.realValue);
          console.log("fill_ratio_spin_box.value =" + fill_ratio_spin_box.realValue / 100);

          com.hollow(thinkness_spin_box.realValue,
                     fill_ratio_spin_box.realValue / 100,
                     fill_enabled_combo_box.currentIndex === 1);
        }
      }
    }
  }
}
