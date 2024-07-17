import QtQml 2.10

import QtQuick 2.10
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.0

import Qt.labs.platform 1.1

import "./"
import "../components"
import "../qml"
import "../secondqml"

DockItem {
  id: root

  readonly property int downloadTaskCount: download_item_view.vaildItemCount
  readonly property int finishedTaskCount: finished_item_view.vaildItemCount

  property var modelLibraryDialog: null

  property var control: cxkernel_cxcloud.downloadService

  title: qsTranslate("DownloadManage", "Download Manage")
  width: 1040 * screenScaleFactor
  height: 614 * screenScaleFactor

  onVisibleChanged: {
    if (!this.visible) {
      return
    }

    tab_view.currentPanel = tab_view.defaultPanel
  }

  ColumnLayout {
    id: root_layout

    anchors.fill: parent
    anchors.margins: 10 * screenScaleFactor
    anchors.topMargin: 10 * screenScaleFactor + root.currentTitleHeight

    spacing: 10 * screenScaleFactor

    CustomTabView {
      id: tab_view

      Layout.fillWidth: true
      Layout.fillHeight: true

      panelColor: "transparent"
      defaultPanel: download_item_view
      borderColor: Constants.splitLineColor
      buttonColor: Constants.download_manage_tab_button_color
      buttonTextDefaultColor: Constants.manager_printer_tabview_default_color
      buttonTextCheckedColor: Constants.manager_printer_tabview_checked_color
      titleBarRightMargin: this.width - (160 * screenScaleFactor) * 2

      DownloadManageTabViewItem {
        id: download_item_view
        downloadTaskModel: root.control.downloadTaskModel
        mode: "DownloadMode"

        onRequestToOpenModelLibrary: function() {
          root.modelLibraryDialog.show()
          root.hide()
        }
      }

      DownloadManageTabViewItem {
        id: finished_item_view
        downloadTaskModel: root.control.downloadTaskModel
        mode: "FinishedMode"

        onRequestToOpenModelLibrary: function() {
          root.modelLibraryDialog.show()
          root.hide()
        }
      }
    }

    RowLayout {
      id: button_layout

      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight
      Layout.alignment: Qt.AlignCenter

      spacing: 10 * screenScaleFactor

      BasicDialogButton {
        id: cancel_button

        Layout.minimumWidth: 120 * screenScaleFactor
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignCenter

        enabled: download_item_view.selectedItemCount > 0
        visible: tab_view.currentPanel === download_item_view &&
                 download_item_view.vaildItemCount > 0
        text: qsTranslate("DownloadManage", "Cancel")

        btnRadius: height / 2
        btnBorderW: 1 * screenScaleFactor
        btnTextColor: Constants.manager_printer_button_text_color
        borderColor: Constants.manager_printer_button_border_color
        defaultBtnBgColor: Constants.manager_printer_button_default_color
        hoveredBtnBgColor: Constants.manager_printer_button_checked_color
        selectedBtnBgColor: Constants.manager_printer_button_checked_color

        onSigButtonClicked: {
          let map = download_item_view.getSelectedModelUid()
          for (let group_id in map) {
            for (let index = 0; index < map[group_id].length; ++index) {
              let model_id = map[group_id][index]
              root.control.cancelModelDownloadTask(group_id, model_id)
            }
          }
        }
      }

      BasicDialogButton {
        id: import_button

        Layout.minimumWidth: 120 * screenScaleFactor
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignCenter

        enabled: finished_item_view.selectedItemCount > 0
        visible: tab_view.currentPanel === finished_item_view &&
                 finished_item_view.vaildItemCount > 0
        text: qsTranslate("DownloadManage", "Import")

        btnRadius: height / 2
        btnBorderW: 1 * screenScaleFactor
        btnTextColor: Constants.manager_printer_button_text_color
        borderColor: Constants.manager_printer_button_border_color
        defaultBtnBgColor: Constants.manager_printer_button_default_color
        hoveredBtnBgColor: Constants.manager_printer_button_checked_color
        selectedBtnBgColor: Constants.manager_printer_button_checked_color

        onSigButtonClicked: {
          /// [
          ///   {
          ///      group_uid: "xxx",
          ///      model_uid: "yyy"
          ///   },
          ///   ...
          /// ]
          let uid_map_list = []

          const group_uid_map = finished_item_view.getSelectedModelUid()
          for (let group_uid in group_uid_map) {
            const model_uid_list = group_uid_map[group_uid]
            for (let model_uid of model_uid_list) {
              uid_map_list.push({
                group_uid: group_uid,
                model_uid: model_uid
              })
            }
          }

          root.control.importModelCache(uid_map_list)
          kernel_kernel.setKernelPhase(0)
          root.hide()
        }
      }

      BasicDialogButton {
        id: save_button

        Layout.minimumWidth: 120 * screenScaleFactor
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignCenter

        enabled: finished_item_view.selectedItemCount > 0
        visible: tab_view.currentPanel === finished_item_view &&
                 finished_item_view.vaildItemCount > 0
        text: qsTranslate("DownloadManage", "Save")

        btnRadius: height / 2
        btnBorderW: 1 * screenScaleFactor
        btnTextColor: Constants.manager_printer_button_text_color
        borderColor: Constants.manager_printer_button_border_color
        defaultBtnBgColor: Constants.manager_printer_button_default_color
        hoveredBtnBgColor: Constants.manager_printer_button_checked_color
        selectedBtnBgColor: Constants.manager_printer_button_checked_color

        onSigButtonClicked: {
          save_dir_select_dialog.open()
        }

        Connections {
          target: save_dir_select_dialog
          onAccepted: {
            const group_uid_map = finished_item_view.getSelectedModelUid()
            const save_path = save_dir_select_dialog.currentFolder
            let successed = false

            for (let group_uid in group_uid_map) {
              const model_uid_list = group_uid_map[group_uid]
              for (let model_uid of model_uid_list) {
                if (root.control.exportModelCache(group_uid, model_uid, save_path)) {
                  successed = true
                }
              }
            }

            save_dir_finished_dialog.successed = successed
            save_dir_finished_dialog.path = save_path
            // save_dir_finished_dialog.visible = true
            save_dir_finished_dialog.open()
          }
        }
      }

      BasicDialogButton {
        id: delete_button

        Layout.minimumWidth: 120 * screenScaleFactor
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignCenter

        enabled: finished_item_view.selectedItemCount > 0
        visible: tab_view.currentPanel === finished_item_view &&
                 finished_item_view.vaildItemCount > 0
        text: qsTranslate("DownloadManage", "Delete")

        btnRadius: height / 2
        btnBorderW: 1 * screenScaleFactor
        btnTextColor: Constants.manager_printer_button_text_color
        borderColor: Constants.manager_printer_button_border_color
        defaultBtnBgColor: Constants.manager_printer_button_default_color
        hoveredBtnBgColor: Constants.manager_printer_button_checked_color
        selectedBtnBgColor: Constants.manager_printer_button_checked_color

        onSigButtonClicked: {
          /// [
          ///   {
          ///      group_uid: "xxx",
          ///      model_uid: "yyy"
          ///   },
          ///   ...
          /// ]
          let uid_map_list = []

          const group_uid_map = finished_item_view.getSelectedModelUid()
          for (let group_uid in group_uid_map) {
            const model_uid_list = group_uid_map[group_uid]
            for (let model_uid of model_uid_list) {
              uid_map_list.push({
                group_uid: group_uid,
                model_uid: model_uid
              })
            }
          }

          root.control.deleteModelCache(uid_map_list)
        }
      }
    }
  }

  Button {
    anchors.top: parent.top
    anchors.right: parent.right
    anchors.topMargin: 5 * screenScaleFactor + root.currentTitleHeight
    anchors.rightMargin: 10 * screenScaleFactor

    width: 24 * screenScaleFactor
    height: 24 * screenScaleFactor
    padding: 4 * screenScaleFactor

    background: Rectangle {
      radius: 5 * screenScaleFactor
      color: !parent.hovered ? "transparent" : Constants.manager_printer_button_checked_color
    }

    contentItem: Image {
      fillMode: Image.PreserveAspectFit
      source: parent.hovered ? "qrc:/UI/photo/model_library_file-hover.svg"
                             : "qrc:/UI/photo/model_library_file-default.svg"
    }

    BasicTooltip {
      visible: parent.hovered
      position: BasicTooltip.Position.LEFT
      text: qsTranslate("DownloadManage", "Open folder")
    }

    onClicked: {
      root.control.openModelCacheDir()
    }
  }

  FolderDialog {
    id: save_dir_select_dialog
    options: Qt.platform.os !== "linux" ? FolderDialog.ShowDirsOnly
                                        : FolderDialog.ShowDirsOnly |
                                          FolderDialog.DontUseNativeDialog
    folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
  }

  BasicMessageDialog {
    id: save_dir_finished_dialog

    property bool successed: false
    property string path: ""

    title: successed ? qsTranslate("DownloadManage", "Save Successed")
                     : qsTranslate("DownloadManage", "Save Failed")

    messageText: successed ? qsTranslate("DownloadManage", "save_successed_tip")
                           : qsTranslate("DownloadManage", "save_failed_tip")

    yesBtnText: successed ? qsTranslate("DownloadManage", "Yes")
                          : qsTranslate("DownloadManage", "Ok")

    noBtnText: qsTranslate("DownloadManage", "No")

    onSuccessedChanged: {
      successed ? showDoubleBtn() : showSingleBtn()
    }

    onAccept: {
      if (!successed) { return }
      Qt.openUrlExternally(path)
    }
  }
}
