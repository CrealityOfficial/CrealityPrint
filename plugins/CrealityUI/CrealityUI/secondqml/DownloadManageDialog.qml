import QtQml 2.10

import QtQuick 2.10
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.0

import Qt.labs.platform 1.1

import "../qml"
import "./"

DockItem {
  id: root

  readonly property int downloadTaskCount: download_item_view.vaildItemCount
  readonly property int finishedTaskCount: finished_item_view.vaildItemCount

  signal requestToOpenModelLibrary()

  property var control: cxkernel_cxcloud.downloadService

  title: qsTranslate("DownloadManage", "Download Manage")
  width: 1040 * screenScaleFactor
  height: 614 * screenScaleFactor

  onVisibleChanged: {
    if (!this.visible) {
      return
    }

    tab_view.selectDefaultPanel()
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

        onRequestToOpenModelLibrary: {
          root.visible = false
          root.requestToOpenModelLibrary()
        }
      }

      DownloadManageTabViewItem {
        id: finished_item_view
        downloadTaskModel: root.control.downloadTaskModel
        mode: "FinishedMode"

        onRequestToOpenModelLibrary: {
          root.visible = false
          root.requestToOpenModelLibrary()
        }
      }
    }



    CusButton {
        anchors.right: root_layout.right
        anchors.rightMargin: 11* screenScaleFactor
        anchors.top: root_layout.top
        radius: 5
        borderWidth: 0
        Layout.preferredWidth: 24 * screenScaleFactor
        Layout.preferredHeight: 24 * screenScaleFactor
        toolTipText: qsTranslate("DownloadManage", "Open folder")
        toolTipPosition: BasicTooltip.Position.LEFT
        normalColor: "transparent"
        hoveredColor: Constants.currentTheme ? "#ECECED" : "#3e3e41"
        pressedColor: "transparent"

        Image {
            anchors.centerIn: parent
            width: sourceSize.width* screenScaleFactor
            height: sourceSize.height* screenScaleFactor
            source: parent.isHovered ? "qrc:/UI/photo/model_library_file-hover.svg" : "qrc:/UI/photo/model_library_file-default.svg"
        }

        onClicked: {
            control.openModelCacheDir()
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
              control.cancelModelDownloadTask(group_id, model_id)
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

        // group_model_list : [ "group_id#model_id", ... ]
        // @see clouddownloadcenter.h clouddownloadcenter.cpp
        onSigButtonClicked: {
          let delimiter = "#"
          let group_model_list = []

          let map = finished_item_view.getSelectedModelUid()
          for (let group_id in map) {
            for (let index = 0; index < map[group_id].length; ++index) {
              let model_id = map[group_id][index]
              group_model_list.push("%1%2%3".arg(group_id).arg(delimiter).arg(model_id))
            }
          }

          control.importModelCache(group_model_list)
          kernel_kernel.setKernelPhase(0)
          root.close()
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
            let map = finished_item_view.getSelectedModelUid()
            let save_path = save_dir_select_dialog.currentFolder
            let successed = false

            for (let group_id in map) {
              for (let index = 0; index < map[group_id].length; ++index) {
                let model_id = map[group_id][index]
                successed = control.exportModelCache(group_id, model_id, save_path) || successed
              }
            }

            save_dir_finished_dialog.successed = successed
            save_dir_finished_dialog.path = save_path
            save_dir_finished_dialog.visible = true
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

        // group_model_list : [ "group_id#model_id", ... ]
        // @see clouddownloadcenter.h clouddownloadcenter.cpp
        onSigButtonClicked: {
          let delimiter = "#"
          let group_model_list = []

          let map = finished_item_view.getSelectedModelUid()
          for (let group_id in map) {
            for (let index = 0; index < map[group_id].length; ++index) {
              let model_id = map[group_id][index]
              group_model_list.push("%1%2%3".arg(group_id).arg(delimiter).arg(model_id))
            }
          }

          control.deleteModelCache(group_model_list)
        }
      }
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

    messageText: successed ? qsTranslate("DownloadManage", "Save successed, do you want open the folder now?")
                           : qsTranslate("DownloadManage", "Save failed, please try again.")

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
