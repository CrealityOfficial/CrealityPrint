import QtQml 2.13

import QtQuick 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.0

import Qt.labs.platform 1.1

import "../components"
import "../qml"
import "./"

DockItem {
  id: root

  title: "设置切片高度"
  width: 150 * screenScaleFactor
  height: 480 * screenScaleFactor

  onVisibleChanged: {
    if (visible) {
      move_to_bottom_check_box.checked = false
      slider.second.value = plugin_info_sliderinfo.maxLayer
      slider.first.value = 0
    }
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 10 * screenScaleFactor
    anchors.topMargin: 10 * screenScaleFactor + root.currentTitleHeight

    spacing: 10 * screenScaleFactor

    ZSliderRange {
      id: slider

      Layout.alignment: Qt.AlignRight | Qt.AlignTop
      Layout.fillHeight: true
      Layout.margins: 10 * screenScaleFactor

      orientation: Qt.Vertical

      from: 0
      to: plugin_info_sliderinfo.maxLayer + 0.01
      stepSize: 0.1
      first.value: from
      second.value: to

      firstVisible: true
      secondVisible: true
      firstLabelText: first.value.toFixed(1)
      secondLabelText: second.value.toFixed(1)

      first.onValueChanged: {
        plugin_info_sliderinfo.setBottomCurrentLayer(first.value)
      }

      second.onValueChanged: {
        plugin_info_sliderinfo.setTopCurrentLayer(second.value)
      }
    }

    StyleCheckBox {
      id: move_to_bottom_check_box

      Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
      text: "置底"
    }

    BasicDialogButton {
      Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
      Layout.minimumHeight: 36 * screenScaleFactor
      Layout.fillWidth: true

      btnRadius: height / 2
      text: "切片"

      onClicked: {
        cf_slice.reset()
        cf_slice.setSliceType(0)
        cf_slice.setToPlate(move_to_bottom_check_box.checked)
        cf_slice.setTopHeight(slider.second.value)
        cf_slice.setBottomHeight(slider.first.value)
		    kernel_slice_flow.sliceChengfei()

        root.close()
      }
    }
  }
}
