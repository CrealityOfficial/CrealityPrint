import QtQuick 2.10
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/components"
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
  
  MouseArea {
    anchors.fill: parent

    onClicked: {
      parent.focus = true
    }
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

        CusStyledSpinBox
        {
            id: thinkness_spin_box
            Layout.preferredWidth: 153 * screenScaleFactor
            Layout.preferredHeight: 28 * screenScaleFactor
            decimals: 2
            from: kernel_parameter_manager.currentMachineObject.currentNozzleSize() * Math.pow(10, decimals)
            to: (com ? com.minLengthInSelectionms / 2  > 0 ?  com.minLengthInSelectionms / 2 : 100* Math.pow(10, decimals)
                    : 100) * Math.pow(10, decimals)
            value: 2 * Math.pow(10, decimals)
            stepSize: 0.5 * Math.pow(10, decimals)
            unitchar: "mm"
            isBindingValue : false
            needTextEditing : false
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

        enabled: true

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
            fill_ratio_spin_box.focus = false
          console.log("thinkness_spin_box.value =" + thinkness_spin_box.realValue);
          console.log("fill_ratio_spin_box.value =" + fill_ratio_spin_box.realValue / 100);

          com.hollow(thinkness_spin_box.value / 100,
                     fill_ratio_spin_box.realValue / 100,
                     fill_enabled_combo_box.currentIndex === 1);
        }
      }
    }
  }
}
