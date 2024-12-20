import QtQml 2.10

import QtQuick 2.10
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.0

import "./"
import "../components"
import "../qml"
import "../secondqml"

CustomTabViewItem {
  id: root

  property string mode: "DownloadMode" // vaild value: ["DownloadMode", "FinishedMode"]

  property var downloadTaskModel: null

  readonly property int vaildItemCount: {
    switch (root.mode) {
      case "DownloadMode": {
        return root.downloadTaskModel.downloadingCount
      }
      case "FinishedMode": {
        return root.downloadTaskModel.finishedCount
      }
      default: {
        return 0
      }
    }
  }

  readonly property int selectedItemCount: _selectedItemCount

  function getSelectedModelUid() {
    let result = { "": [] }

    for (let index = 0; index < this._checkboxList.length; ++index) {
      let checkbox = this._checkboxList[index]
      if (checkbox === null) {
        continue
      } else if (!checkbox.visible) {
        continue
      } else if (!checkbox.checked) {
        continue
      }

      let group_uid = checkbox.getGroupUid()
      let model_uid = checkbox.getModelUid()

      if (result[group_uid] === undefined) {
        result[group_uid] = []
      }

      result[group_uid].push(model_uid)
    }

    return result
  }

  title: "%1 (%2)".arg(this._titleMap[this.mode]).arg(this.vaildItemCount)

  signal requestToOpenModelLibrary()

  // ------------------------------ //

  property int _selectedItemCount: 0

  function _updateSelectedModelCount() {
    let count = 0

    for (let index = 0; index < this._checkboxList.length; ++index) {
      let checkbox = this._checkboxList[index]

      if (checkbox === null) {
        continue
      } else if (!checkbox.visible) {
        continue
      } else if (!checkbox.checked) {
        continue
      }

      ++count
    }

    _selectedItemCount = count
  }

  property var _checkboxList: []

  readonly property var _titleMap: {
    "DownloadMode": qsTranslate("DownloadManage", "Downloading"),
    "FinishedMode": qsTranslate("DownloadManage", "Finished"),
  }

  StackLayout {
    anchors.fill: parent
    currentIndex: root.vaildItemCount > 0 ? 0 : 1

    Rectangle {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: "transparent"

      ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20 * screenScaleFactor
        anchors.rightMargin: 0

        spacing: 10 * screenScaleFactor

        RowLayout {
          Layout.fillWidth: true
          Layout.minimumHeight: 13 * screenScaleFactor
          Layout.maximumHeight: Layout.minimumHeight
          Layout.rightMargin: 20 * screenScaleFactor

          spacing: 10 * screenScaleFactor

          StyleCheckBox {
            id: select_all_checkbox

            function updateCheckedState() {
              let is_all_selected = root._checkboxList.length > 0

              for (let index = 0; index < root._checkboxList.length; ++index) {
                let checkbox = root._checkboxList[index]
                if (checkbox === null) {
                  continue
                } else if (!checkbox.visible) {
                  continue
                } else if (!checkbox.checked) {
                  is_all_selected = false
                  break
                }
              }

              select_all_checkbox.checked = is_all_selected
            }

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

            indicatorWidth: height
            indicatorHeight: height

            fontWeight: Font.Normal
            textColor: Constants.manager_printer_label_color
            text: qsTranslate("DownloadManage", "Select All")

            indicatorTextSpacing: 10 * screenScaleFactor

            onClicked: {
              root._selectedItemCount = this.checked ? root.vaildItemCount : 0
            }
          }

          Text {
            Layout.minimumWidth: 180 * screenScaleFactor
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter

            color: Constants.download_manage_title_text_color
            font.pointSize: Constants.labelFontPointSize_9
            font.family: Constants.labelFontFamily
            font.weight: Font.Normal
            text: qsTranslate("DownloadManage", "File Size")
          }

          Text {
            Layout.minimumWidth: 180 * screenScaleFactor
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter

            visible: root.mode === "DownloadMode"

            color: Constants.download_manage_title_text_color
            font.pointSize: Constants.labelFontPointSize_9
            font.family: Constants.labelFontFamily
            font.weight: Font.Normal
            text: qsTranslate("DownloadManage", "Download Speed")
          }

          Text {
            Layout.minimumWidth: 180 * screenScaleFactor
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter

            visible: root.mode === "FinishedMode"

            color: Constants.download_manage_title_text_color
            font.pointSize: Constants.labelFontPointSize_9
            font.family: Constants.labelFontFamily
            font.weight: Font.Normal
            text: qsTranslate("DownloadManage", "Download Time")
          }
        }

        Rectangle {
          id: title_split_line
          Layout.fillWidth: true
          Layout.minimumHeight: 1 * screenScaleFactor
          Layout.maximumHeight: Layout.minimumHeight
          Layout.rightMargin: 20 * screenScaleFactor
          color: Constants.splitLineColor
        }

        BasicScrollView {
          Layout.fillWidth: true
          Layout.fillHeight: true
          Layout.topMargin: 10 * screenScaleFactor
          Layout.rightMargin: 8 * screenScaleFactor

          hpolicyVisible: false
          vpolicyVisible: contentHeight > height

          clip: true

          ColumnLayout {
            width: title_split_line.width
            anchors.rightMargin: 12 * screenScaleFactor
            spacing: 20 * screenScaleFactor

            Repeater {
              model: root.downloadTaskModel

              delegate: ColumnLayout {
                id: group_item

                readonly property string uid: model_uid

                property var checkboxList: []

                Layout.fillWidth: true
                Layout.minimumHeight: 20 * screenScaleFactor
                Layout.maximumHeight: group_button.checked ? Layout.preferredHeight
                                                            : Layout.minimumHeight

                clip: true
                spacing: 10 * screenScaleFactor
                visible: item_list.vaildItemCount > 0

                RowLayout {
                  id: group_info_row

                  Layout.fillWidth: true
                  Layout.fillHeight: true

                  spacing: 10 * screenScaleFactor

                  StyleCheckBox {
                    id: select_group_checkbox

                    function getGroupUid() { return group_item.uid }

                    function updateCheckedState() {
                      let is_all_selected = group_item.checkboxList.length > 0

                      for (let index = 0; index < group_item.checkboxList.length; ++index) {
                        let checkbox = group_item.checkboxList[index]
                        if (checkbox === null) {
                          continue
                        } else if (!checkbox.visible) {
                          continue
                        } else if (!checkbox.checked) {
                          is_all_selected = false
                          break
                        }
                      }

                      select_group_checkbox.checked = is_all_selected
                    }

                    Layout.minimumWidth: 13 * screenScaleFactor
                    Layout.minimumHeight: 13 * screenScaleFactor
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                    indicatorWidth: Layout.minimumWidth
                    indicatorHeight: Layout.minimumHeight

                    fontWeight: Font.Bold
                    textColor: Constants.download_manage_model_text_color

                    Connections {
                      target: select_all_checkbox
                      function onClicked() {
                        select_group_checkbox.checked = select_all_checkbox.checked
                      }
                    }
                  }

                  Button {
                    id: group_button

                    Layout.fillWidth: true
                    Layout.minimumHeight: parent.Layout.minimumHeight
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                    checked: true
                    checkable: true

                    background: RowLayout {
                      anchors.fill: parent
                      spacing: 10 * screenScaleFactor

                      Image {
                        Layout.minimumWidth: 13 * screenScaleFactor
                        Layout.minimumHeight: 13 * screenScaleFactor
                        Layout.maximumWidth: Layout.minimumWidth
                        Layout.maximumHeight: Layout.minimumHeight
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        sourceSize.width: Layout.minimumWidth
                        sourceSize.height: Layout.minimumHeight
                        source: group_button.checked ? Constants.download_manage_group_close_image
                                                     : Constants.download_manage_group_open_image
                      }

                      Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        color: Constants.download_manage_model_text_color
                        font.family: Constants.labelFontFamily
                        font.pointSize: Constants.labelFontPointSize_9
                        font.weight: Font.Bold
                        text: "%1 (%2)".arg(model_name).arg(item_list.vaildItemCount)
                      }
                    }
                  }
                }

                Repeater {
                  id: item_list

                  readonly property int vaildItemCount: {
                    switch (root.mode) {
                      case "DownloadMode": {
                        return model_items.downloadingCount
                      }
                      case "FinishedMode": {
                        return model_items.finishedCount
                      }
                      default: {
                        return 0
                      }
                    }
                  }

                  model: model_items

                  delegate: Rectangle{
                    id:itemRect
                    color: "transparent"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 74 * screenScaleFactor
                    radius: 5 * screenScaleFactor

                    visible: {
                      switch (root.mode) {
                        case "DownloadMode": {
                          return model_state === 0 || model_state === 1
                        }
                        case "FinishedMode": {
                          return model_state === 4
                        }
                        default: {
                          return false
                        }
                      }
                    }

                    MouseArea{
                      anchors.fill: parent
                      hoverEnabled: true
                      onEntered:itemRect.color =Constants.currentTheme === 0 ? "#434345" :"#F2F2F5"
                      onExited: itemRect.color = "transparent"

                      onClicked: {
                        select_model_checkbox.checked = !select_model_checkbox.checked
                        select_group_checkbox.updateCheckedState()
                        select_all_checkbox.updateCheckedState()
                        root._updateSelectedModelCount()
                      }

                      Connections {
                        target: select_all_checkbox
                       function onClicked() {
                          select_model_checkbox.checked = select_all_checkbox.checked
                        }
                      }

                      Connections {
                        target: select_group_checkbox
                        function onClicked() {
                          select_model_checkbox.checked = select_group_checkbox.checked
                          select_all_checkbox.updateCheckedState()
                          root._updateSelectedModelCount()
                        }
                      }
                    }

                    RowLayout {
                      id: model_item
                      anchors.fill: parent

                      Layout.leftMargin: select_group_checkbox.width + group_info_row.spacing + 5 * screenScaleFactor
                      Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                      spacing: 10 * screenScaleFactor

                      // @see cxcloud/service/download_model.h
                      // struct ModelInfo {
                      //   enum class State : int {
                      //     UNREADY     = -1,
                      //     READY       = 0,
                      //     DOWNLOADING = 1,
                      //     PAUSED      = 2,
                      //     FAILED      = 3,
                      //     FINISHED    = 4,
                      //     BROKEN      = 5,
                      //   };
                      //   ...
                      // };

                      StyleCheckBox {
                        id: select_model_checkbox

                        function getGroupUid() { return group_item.uid }
                        function getModelUid() { return model_uid }

                        Layout.minimumWidth: 13 * screenScaleFactor
                        Layout.minimumHeight: 13 * screenScaleFactor
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        Layout.leftMargin: 10 * screenScaleFactor
                        indicatorWidth: Layout.minimumWidth
                        indicatorHeight: Layout.minimumHeight

                        fontWeight: Font.Bold
                        textColor: Constants.download_manage_model_text_color

                        onClicked: {
                          select_group_checkbox.updateCheckedState()
                          select_all_checkbox.updateCheckedState()
                          root._updateSelectedModelCount()
                        }

                        Connections {
                          target: select_all_checkbox
                         function onClicked() {
                            select_model_checkbox.checked = select_all_checkbox.checked
                          }
                        }

                        Connections {
                          target: select_group_checkbox
                          function onClicked() {
                            select_model_checkbox.checked = select_group_checkbox.checked
                            select_all_checkbox.updateCheckedState()
                            root._updateSelectedModelCount()
                          }
                        }

                        Component.onCompleted: {
                          root._checkboxList.push(this)
                          group_item.checkboxList.push(this)
                          this.clicked()
                        }

                        Component.onDestruction: {
                          let index = root._checkboxList.indexOf(this)
                          root._checkboxList[index] = null

                          index = group_item.checkboxList.indexOf(this)
                          group_item.checkboxList[index] = null

                          this.clicked()

                          root._updateSelectedModelCount()
                        }

                        onVisibleChanged: {
                          this.checked = false
                          this.clicked()
                        }
                      }

                      Rectangle {
                        Layout.minimumWidth: 62 * screenScaleFactor
                        Layout.minimumHeight: 62 * screenScaleFactor
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                        clip: true

                        color: "transparent"
                        border.width: 1 * screenScaleFactor
                        border.color: Constants.splitLineColor

                        radius: 5 * screenScaleFactor

                        Image {
                          anchors.fill: parent
                          source:  kernel_vld_enabled ? "" : model_image
                        }
                      }

                      ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                        spacing: 8 * screenScaleFactor

                        Text {
                          Layout.fillWidth: true
                          Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                          horizontalAlignment: Text.AlignLeft
                          verticalAlignment: Text.AlignVCenter

                          color: Constants.download_manage_model_text_color
                          font.pointSize: Constants.labelFontPointSize_9
                          font.family: Constants.labelFontFamily
                          font.weight: Font.Bold
                          text: model_name
                        }

                        ProgressBar {
                          id: download_progress

                          Layout.fillWidth: true
                          Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                          from: 0
                          to: 100
                          value: model_progress

                          visible: root.mode === "DownloadMode"

                          background: Item {
                            anchors.fill: parent

                            Rectangle {
                              id: progress_background

                              anchors.left: parent.left
                              anchors.right: progress_text.left
                              anchors.rightMargin: 10 * screenScaleFactor
                              anchors.verticalCenter: parent.verticalCenter
                              height: 4 * screenScaleFactor
                              radius: 2 * screenScaleFactor
                              color: Constants.download_manage_prograss_right_color
                            }

                            Rectangle {
                              anchors.top: progress_background.top
                              anchors.bottom: progress_background.bottom
                              anchors.left: progress_background.left
                              width: download_progress.position * progress_background.width
                              radius: progress_background.radius
                              color: Constants.download_manage_prograss_left_color
                            }

                            Text {
                              id: progress_text

                              width: 32 * screenScaleFactor
                              anchors.right: parent.right
                              anchors.verticalCenter: parent.verticalCenter
                              horizontalAlignment: Text.AlignLeft
                              verticalAlignment: Text.AlignVCenter

                              color: Constants.download_manage_model_text_color
                              font.pointSize: Constants.labelFontPointSize_9
                              font.family: Constants.labelFontFamily
                              font.weight: Font.Bold
                              text: "%1%".arg(download_progress.value)
                            }
                          }

                          contentItem: Item {}
                        }
                      }

                      Text {
                        Layout.minimumWidth: 180 * screenScaleFactor
                        Layout.fillHeight: true
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter

                        color: Constants.download_manage_model_text_color
                        font.pointSize: Constants.labelFontPointSize_9
                        font.family: Constants.labelFontFamily
                        font.weight: Font.Bold
                        text: model_size_with_unit
                      }

                      Text {
                        Layout.minimumWidth: 180 * screenScaleFactor
                        Layout.fillHeight: true
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter

                        visible: root.mode === "DownloadMode"

                        color: Constants.download_manage_model_text_color
                        font.pointSize: Constants.labelFontPointSize_9
                        font.family: Constants.labelFontFamily
                        font.weight: Font.Bold
                        text: model_speed_with_unit
                      }

                      Text {
                        Layout.minimumWidth: 180 * screenScaleFactor
                        Layout.fillHeight: true
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter

                        visible: root.mode === "FinishedMode"

                        color: Constants.download_manage_model_text_color
                        font.pointSize: Constants.labelFontPointSize_9
                        font.family: Constants.labelFontFamily
                        font.weight: Font.Bold
                        text: model_date
                        Layout.rightMargin: 10 * screenScaleFactor
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    Rectangle {
      Layout.fillWidth: true
      Layout.fillHeight: true
      color: "transparent"

      ColumnLayout {
        anchors.centerIn: parent

        spacing: 20 * screenScaleFactor

        Text {
          Layout.alignment: Qt.AlignCenter
          horizontalAlignment: Text.AlignHCenter
          verticalAlignment: Text.AlignVCenter

          color: Constants.download_manage_empty_text_color
          font.pointSize: Constants.labelFontPointSize_12
          font.family: Constants.labelFontFamily
          font.weight: Font.Black
          text: qsTranslate("DownloadManage", "The Download task list is empty")
        }

        Button {
          Layout.minimumWidth: 300 * screenScaleFactor
          Layout.minimumHeight: 48 * screenScaleFactor
          Layout.alignment: Qt.AlignCenter

          background: Rectangle {
            anchors.fill: parent

            radius: this.height / 2

            color: parent.hovered ? Constants.download_manage_empty_button_checked_color
                                  : Constants.download_manage_empty_button_default_color

            RowLayout {
              anchors.centerIn: parent

              spacing: 10 * screenScaleFactor

              Image {
                Layout.alignment: Qt.AlignCenter
                Layout.minimumWidth: 24 * screenScaleFactor
                Layout.minimumHeight: 22 * screenScaleFactor
                Layout.maximumWidth: Layout.minimumWidth
                Layout.maximumHeight: Layout.minimumHeight
                source: Constants.download_manage_empty_button_image
              }

              Text {
                Layout.alignment: Qt.AlignCenter
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                color: parent.hovered ? Constants.download_manage_empty_button_text_checked_color
                                      : Constants.download_manage_empty_button_text_default_color
                font.pointSize: Constants.labelFontPointSize_12
                font.family: Constants.labelFontFamily
                font.weight: Font.Bold
                text: qsTranslate("DownloadManage", "Go to the model library")
              }
            }
          }

          onClicked: {
            root.requestToOpenModelLibrary()
          }
        }
      }
    }
  }
}
