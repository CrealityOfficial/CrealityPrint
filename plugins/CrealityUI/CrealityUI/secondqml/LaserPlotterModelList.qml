import QtQuick 2.5
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.12
import QtQml 2.13

import ".."
import "../qml"

Popup {
  id: object_list_popup

  property string titleText: qsTranslate("LaserPanel", "Object List")
  property var control: null
  property var itemModel: null
  property alias listView: object_list_view

  y: parent.height - height
  width: 280 * screenScaleFactor
  height: 180 * screenScaleFactor

  // closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
  closePolicy: Popup.NoAutoClose

  Connections {
    target: kernel_parameter_manager
    onFunctionTypeChanged: {
      object_list_popup.close()
    }
  }

  Connections {
    target: kernel_kernel
    onCurrentPhaseChanged: {
      object_list_popup.close()
    }
  }

  background: Rectangle {
    id: object_list

    border.width: 1
    border.color: Constants.left_model_list_border_color
    color: Constants.left_model_list_background_color
    radius: 5 * screenScaleFactor

    readonly property int dialogMargin: 10 * screenScaleFactor

    ColumnLayout {
      anchors.fill: parent
      anchors.margins: parent.border.width
      spacing: 0

      RowLayout {
        Layout.fillWidth: true
        Layout.minimumHeight: 24 * screenScaleFactor
        Layout.leftMargin: object_list.dialogMargin

        Text {
          Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
          Layout.fillWidth: true

          font.family: Constants.labelFontFamily
          font.pointSize: Constants.labelFontPointSize_9
          color: Constants.left_model_list_title_text_color
          text: object_list_popup.titleText
        }

        CusButton {
          Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
          Layout.minimumHeight: parent.Layout.minimumHeight
          Layout.minimumWidth: Layout.minimumHeight

          rightTop: true
          allRadius: false

          normalColor: Constants.left_model_list_close_button_default_color
          hoveredColor: Constants.left_model_list_close_button_hovered_color
          pressedColor: hoveredColor

          Image {
            anchors.centerIn: parent
            width: 8 * screenScaleFactor
            height: width
            fillMode: Image.PreserveAspectFit
            source: parent.isHovered ? Constants.left_model_list_close_button_hovered_icon
                                     : Constants.left_model_list_close_button_default_icon
          }

          onClicked: {
            object_list_popup.close()
          }
        }
      }

      Rectangle {
        Layout.fillWidth: true
        Layout.minimumHeight: 1
        color: object_list.border.color
      }

      RowLayout {
        Layout.fillWidth: true
        Layout.minimumHeight: 24 * screenScaleFactor
        Layout.margins: object_list.dialogMargin

        spacing: object_list.dialogMargin

        Button {
          id: object_list_all_button

          Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
          Layout.minimumHeight: 14 * screenScaleFactor
          Layout.minimumWidth: Layout.minimumHeight

          checkable: true

          background: Rectangle {
            anchors.fill: parent

            radius: 3 * screenScaleFactor
            border.width: 1
            border.color: parent.hovered ? Constants.left_model_list_all_button_border_color
                                         : object_list.border.color
            color: "transparent"

            Image {
              anchors.centerIn: parent
              width: 8 * screenScaleFactor
              height: width
              fillMode: Image.PreserveAspectFit
              source: Constants.left_model_list_all_button_icon
              visible: object_list_all_button.checked
            }
          }

          onClicked: {
            checked ? object_list_view.selectAllIndex() : object_list_view.unselectAllIndex()
          }

          Connections {
            target: object_list_view
            onIsAllSelectedChanged: {
              object_list_all_button.checked = object_list_view.isAllSelectedChanged
            }
          }
        }

        Text {
          Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
          Layout.fillWidth: true
          verticalAlignment: Text.AlignVCenter
          font.family: Constants.labelFontFamily
          font.pointSize: Constants.labelFontPointSize_9
          color: Constants.left_model_list_all_button_text_color
          text: qsTr("All")
        }

        CusImglButton {
          Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
          Layout.minimumHeight: parent.Layout.minimumHeight
          Layout.minimumWidth: Layout.minimumHeight

          borderWidth: 0
          btnRadius: 5 * screenScaleFactor
          cusTooltip.text: qsTr("Delete")

          defaultBtnBgColor: Constants.left_model_list_action_button_default_color
          hoveredBtnBgColor: Constants.left_model_list_action_button_hovered_color

          enabledIconSource: Constants.left_model_list_del_button_default_icon
          pressedIconSource: Constants.left_model_list_del_button_hovered_icon
          highLightIconSource: pressedIconSource

          onClicked: {
            root.deleteSelectObject()
          }
        }
      }

      Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: object_list.dialogMargin
        Layout.rightMargin: object_list.dialogMargin
        Layout.bottomMargin: object_list.dialogMargin

        border.width: object_list.border.width
        border.color: object_list.border.color
        radius: object_list.radius
        color: "transparent"

        ListView {
          id: object_list_view

          property var selectIndexList: []

          function syncSelectedIndexList(new_index_list) {
            for (let old_idx = 0; old_idx < this.count; ++old_idx) {
              let old_item = this.itemAtIndex(old_idx)
              if (old_item) { old_item.checked = false }
            }

            this.selectIndexList = new_index_list
            for (let new_idx in this.selectIndexList) {
              let new_item = this.itemAtIndex(this.selectIndexList[new_idx])
              if (new_item) { new_item.checked = true }
            }

            object_list_all_button.checked = this.count === this.selectIndexList.length
          }

          function selectAllIndex() {
            object_list_view.selectIndexList = []
            for (let idx = 0; idx < count; ++idx) {
              object_list_view.selectIndexList.push(idx)
              object_list_view.itemAtIndex(idx).checked = true
            }

            object_list_popup.control.selectMulObject(selectIndexList)
          }

          function unselectAllIndex() {
            selectIndexList = []
            for (let idx = 0; idx < count; ++idx) {
              object_list_view.itemAtIndex(idx).checked = false
            }

            object_list_popup.control.selectMulObject(selectIndexList)
          }

          function appendSelectedIndex(index) {
            let found = false
            for (let idx of this.selectIndexList) {
              if (idx === index) {
                found = true
                break
              }
            }
            if (!found) {
              this.selectIndexList.push(index)
            }
          }

          function removeSelectedIndex(index) {
            let found = false
            let new_list = []
            for (let idx of this.selectIndexList) {
              if (idx === index) {
                found = true
              } else {
                new_list.push(idx)
              }
            }

            if (found) {
              this.selectIndexList = new_list
            }
          }

          anchors.fill: parent
          anchors.margins: parent.border.width
          spacing: 1
          clip: true
          boundsBehavior: Flickable.StopAtBounds

          model: object_list_popup.itemModel
          delegate: Button {
            id: item_button

            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 2
            height: 20 * screenScaleFactor

            checkable: true

            background: Rectangle {
              color: parent.hovered || parent.checked ? Constants.left_model_list_item_hovered_color
                                                      : Constants.left_model_list_item_default_color
              radius: object_list.radius

              Text {
                anchors.left: parent.left
                anchors.leftMargin: object_list.dialogMargin
                anchors.verticalCenter: parent.verticalCenter

                font.family: Constants.labelFontFamily
                font.pointSize: Constants.labelFontPointSize_9
                color: item_button.hovered || item_button.checked ? Constants.left_model_list_item_text_hovered_color
                                                                  : Constants.left_model_list_item_text_default_color
                text: name
              }
            }

            onClicked: {
              // update selected index list
              if (item_button.checked) {
                object_list_view.appendSelectedIndex(index)
              } else {
                object_list_view.removeSelectedIndex(index)
              }

              // sync to control
              if (object_list_view.selectIndexList.length === 0) {
                object_list_popup.control.clearSelectAllObject()
              } else {
                object_list_popup.control.selectMulObject(object_list_view.selectIndexList)
              }

              // sync to ui
              object_list_all_button.checked =
                  object_list_view.selectIndexList.length === object_list_view.count
            }
          }

          ScrollBar.vertical: ScrollBar {}
        }
      }
    }
  }
}
