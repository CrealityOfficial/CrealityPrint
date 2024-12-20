import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import QtQml 2.15

import QtGraphicalEffects 1.15

import ".."
import "../components"
import "../secondqml"

DockItem {
  id: root

  property var tipDialog: null
  property var shareDialog: null
  property var downloadManageDialog: null
  property var projectListDialog: null

  width: 1200 * screenScaleFactor
  height: 770 * screenScaleFactor
  title: qsTr("Personal Center")

  enum Page {
    UPLOADED_SLICE = 0,
    CLOUD_SLICE = 1,
    UPLOADED_MODEL = 2,
    COLLECTED_MODEL = 3,
    PURCHASED_MODEL = 4,
    PRINTER = 5,
    UPLOADED_PROJECT = 6
  }

  function setPage(page) {
    let button = null

    switch (page) {
      case UserInfoDialog.Page.UPLOADED_SLICE: button = uploaded_slice_button; break;
      case UserInfoDialog.Page.CLOUD_SLICE: button = cloud_slice_button; break;
      case UserInfoDialog.Page.UPLOADED_MODEL: button = uploaded_model_button; break;
      case UserInfoDialog.Page.COLLECTED_MODEL: button = collected_model_button; break;
      case UserInfoDialog.Page.PURCHASED_MODEL: button = purchased_model_button; break;
      case UserInfoDialog.Page.PRINTER: button = printer_button; break;
      case UserInfoDialog.Page.UPLOADED_PROJECT: button = uploaded_project_button; break;
    }

    if (button) {
      button.checked = true
    }
  }

  function invokeOnAcceptRestricted(restricted, callback) {
    if (typeof callback !== "function") {
      return
    }

    if (!restricted) {
      callback()
      return
    }

    if (cxkernel_cxcloud.modelRestriction == "ignore" || !cxkernel_cxcloud.restrictedTipEnabled) {
      callback()
      return
    }

    tipDialog.exec({
      message: cxTr("cxcloud_nsfw_model_tip"),
      rememberButtonVisible: true,
      acceptButtonText: cxTr("cxcloud_nsfw_model_accept"),
      onAccepted: remember => {
        if (remember) {
          cxkernel_cxcloud.restrictedTipEnabled = false
        }
        callback()
      },
      onCanceled: _ => {},
    })
  }

  function invokeOnModelGroupInfoChecked(group_uid, callback) {
    if (typeof callback !== "function") {
      return
    }

    if (!cxkernel_cxcloud.checkNetworkAvailable()) {
      tipDialog.tip(cxTr("cxcloud_network_unvailable_tip"))
      return
    }

    cxkernel_cxcloud.modelService.loadModelGroupInfo(group_uid, group_info => {
      if (!group_info) {
        tipDialog.tip(cxTr("cxcloud_model_group_doesnt_exist_tip"))
        return
      }

      if (group_info.status === 'deleted') {
        tipDialog.tip(cxTr('cxcloud_model_group_deleted_tip'))
        return
      }

      if (group_info.status === 'removed') {
        tipDialog.tip(cxTr('cxcloud_model_group_removed_tip'))
        return
      }

      if (group_info.price > 0 && !group_info.purchased) {
        Qt.openUrlExternally(cxkernel_cxcloud.modelGroupUrlHead + group_uid)
        return
      }

      callback(group_info)
    })
  }

  function importModelGroup(group_uid) {
    invokeOnModelGroupInfoChecked(group_uid, group_info => {
      invokeOnAcceptRestricted(group_info.restricted, () => {
        cxkernel_cxcloud.modelService.loadModelGroupModelList(group_uid, group_info, group_info => {
          if (!group_info || group_info.modelCount == 0) {
            tipDialog.tip(cxTr('cxcloud_model_group_none_model_file_tip'))
            return
          }

          if (group_info.modelCount === group_info.modelBrokenCount) {
            tipDialog.tip(cxTr('cxcloud_model_group_model_file_all_broken_tip'))
            return
          }

          const import_model_group = function(group_uid) {
            if (cxkernel_cxcloud.downloadService.checkModelGroupDownloaded(group_uid)) {
              cxkernel_cxcloud.downloadService.importModelGroupCache(group_uid)
            } else if (cxkernel_cxcloud.downloadService.checkModelGroupDownloading(group_uid)) {
              cxkernel_cxcloud.downloadService.markModelGroupImportLater(group_uid, true)
            } else {
              cxkernel_cxcloud.downloadService.startModelGroupDownloadTask(group_uid, true)
            }

            root.hide()
          }

          if (group_info.modelBrokenCount !== 0) {
            tipDialog.exec({
              message: cxTr('cxcloud_model_group_model_file_broken_tip'),
              onAccepted: function() {
                import_model_group(group_uid)
              },
              onCanceled: function() {},
            })
            return
          }

          import_model_group(group_uid)
        })
      })
    })
  }

  function execProjectListDialog(group_uid) {
    invokeOnModelGroupInfoChecked(group_uid, group_info => {
      invokeOnAcceptRestricted(group_info.restricted, () => {
        if (group_info.projectCount <= 0) {
          tipDialog.tip(cxTr("cxcloud_model_group_none_project_file_tip"))
          return
        }

        cxkernel_cxcloud.modelService.setFocusedModelGroupUid(group_uid, 1, 24)
        projectListDialog.show()
      })
    })
  }

  ColumnLayout {
    id: root_layout

    anchors.fill: parent
    anchors.margins: 20 * screenScaleFactor
    anchors.topMargin: 20 * screenScaleFactor + root.currentTitleHeight
    spacing: 20 * screenScaleFactor

    RowLayout {
      id: account_layout

      Layout.fillWidth: true
      Layout.minimumHeight: 70 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight
      Layout.alignment: Qt.AlignHCenter | Qt.AlignTop

      spacing: 0

      ImageRectangle {
        Layout.minimumWidth: account_layout.Layout.minimumHeight
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

        radius: height / 2
        imageSource: cxkernel_cxcloud.accountService.avatar
      }

      ColumnLayout {
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        Layout.leftMargin: 10 * screenScaleFactor

        spacing: 8 * screenScaleFactor

        Text {
          horizontalAlignment: Qt.AlignLeft
          verticalAlignment: Qt.AlignBottom

          font.family: Constants.labelFontFamily
          font.weight: Constants.labelFontWeight
          font.pointSize: Constants.labelFontPointSize_12
          color: Constants.textColor
          text: cxkernel_cxcloud.accountService.nickName
        }

        Text {
          horizontalAlignment: Qt.AlignLeft
          verticalAlignment: Qt.AlignTop

          font.family: Constants.labelFontFamily
          font.weight: Constants.labelFontWeight
          font.pointSize: Constants.labelFontPointSize_10
          color: Constants.textColor
          text: "ID: %1".arg(cxkernel_cxcloud.accountService.userId)
        }
      }

      Rectangle {
        Layout.fillHeight: true
        Layout.minimumWidth: 1 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.topMargin: 20 * screenScaleFactor
        Layout.bottomMargin: 20 * screenScaleFactor
        Layout.leftMargin: 50 * screenScaleFactor
        Layout.rightMargin: 50 * screenScaleFactor
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        color: Constants.dock_border_color
      }

      Text {
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        horizontalAlignment: Qt.AlignRight
        verticalAlignment: Qt.AlignVCenter
        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: Constants.labelFontPointSize_10
        color: Constants.textColor
        text: "%1:".arg(qsTr("Used"))
      }

      Rectangle {
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        Layout.minimumWidth: 200 * screenScaleFactor
        Layout.minimumHeight: 4 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.maximumHeight: Layout.minimumHeight
        Layout.leftMargin: 10 * screenScaleFactor

        radius: height / 2
        color: Constants.dock_border_color

        Rectangle {
          anchors.left: parent.left
          anchors.top: parent.top
          anchors.bottom: parent.bottom

          width: parent.width *
            (cxkernel_cxcloud.accountService.usedStorageSpace / cxkernel_cxcloud.accountService.maxStorageSpace)
          radius: parent.radius
          color: Constants.themeGreenColor
        }

        Text {
          anchors.bottom: parent.top
          anchors.horizontalCenter: parent.horizontalCenter
          anchors.bottomMargin: 4 * screenScaleFactor

          horizontalAlignment: Qt.AlignHCenter
          verticalAlignment: Qt.AlignVCenter
          font.family: Constants.labelFontFamily
          font.weight: Constants.labelFontWeight
          font.pointSize: Constants.labelFontPointSize_19
          color: Constants.textColor
          text: "%1/%2".arg(translateStorage(cxkernel_cxcloud.accountService.usedStorageSpace, 2))
                       .arg(translateStorage(cxkernel_cxcloud.accountService.maxStorageSpace, 0))

          function translateStorage(bytes, precision) {
            if (bytes < 1024) { return "%1B".arg(bytes.toFixed(precision)) }

            let kbytes = bytes / 1024
            if (kbytes < 1024) { return "%1KB".arg(kbytes.toFixed(precision)) }

            let mbytes = kbytes / 1024
            if (mbytes < 1024) { return "%1MB".arg(mbytes.toFixed(precision)) }

            return "%1GB".arg(Number(mbytes / 1024).toFixed(precision))
          }
        }
      }

      Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
      }

      Button {
        Layout.minimumWidth: 68 * screenScaleFactor
        Layout.minimumHeight: 28 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.maximumHeight: Layout.minimumHeight
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

        background: Rectangle {
          radius: parent.height / 2
          border.width: 1 * screenScaleFactor
          border.color: Constants.dock_border_color
          color: parent.hovered ? Constants.dock_border_color : "transparent"

          Text {
            anchors.centerIn: parent
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            font.pointSize: Constants.labelFontPointSize_10
            color: Constants.textColor
            text: qsTranslate(cxContext, "Sign Out")
          }
        }

        onClicked: {
          cxkernel_cxcloud.accountService.logout()
          root.visible = false
        }
      }
    }

    RowLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true

      ColumnLayout {
        id: button_layout

        Layout.minimumWidth: Math.max(140 * screenScaleFactor)
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true

        ButtonGroup {
          id: page_button_group
        }

        // "My Slices" group begin

        Button {
          id: slice_type_button

          Layout.minimumWidth: 140 * screenScaleFactor
          Layout.minimumHeight: 34 * screenScaleFactor

          checked: true
          checkable: true

          background: Item {
            Image {
              id: slice_type_button_image

              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter

              width: 13 * screenScaleFactor
              height: 13 * screenScaleFactor
              sourceSize: Qt.size(width, height)
              source: slice_type_button.checked ? "qrc:/UI/photo/treeView_minus_dark.png"
                                                : "qrc:/UI/photo/treeView_plus_dark.png"
            }

            Text {
              anchors.left: slice_type_button_image.right
              anchors.verticalCenter: parent.verticalCenter
              anchors.leftMargin: 10 * screenScaleFactor

              horizontalAlignment: Qt.AlignLeft
              verticalAlignment: Qt.AlignVCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_11
              color: Constants.textColor
              text: qsTr("My Slices")
            }
          }
        }

        Button {
          id: uploaded_slice_button

          property int index: UserInfoDialog.Page.UPLOADED_SLICE

          Layout.minimumWidth: 140 * screenScaleFactor
          Layout.minimumHeight: 28 * screenScaleFactor

          checked: true
          checkable: true
          ButtonGroup.group: page_button_group
          visible: slice_type_button.checked

          background: Item {
            Image {
              id: uploaded_slice_button_image

              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter

              width: 13 * screenScaleFactor
              height: 13 * screenScaleFactor
              sourceSize: Qt.size(width, height)
              source: ""
            }

            Text {
              anchors.left: uploaded_slice_button_image.right
              anchors.verticalCenter: parent.verticalCenter
              anchors.leftMargin: 10 * screenScaleFactor

              horizontalAlignment: Qt.AlignLeft
              verticalAlignment: Qt.AlignVCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_10
              color: uploaded_slice_button.checked ? Constants.themeGreenColor : Constants.textColor
              text: qsTr("My Uploads")
            }
          }
        }

        Button {
          id: cloud_slice_button

          property int index: UserInfoDialog.Page.CLOUD_SLICE

          Layout.minimumWidth: 140 * screenScaleFactor
          Layout.minimumHeight: 28 * screenScaleFactor

          checked: true
          checkable: true
          ButtonGroup.group: page_button_group
          visible: slice_type_button.checked

          background: Item {
            Image {
              id: cloud_slice_button_image

              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter

              width: 13 * screenScaleFactor
              height: 13 * screenScaleFactor
              sourceSize: Qt.size(width, height)
              source: ""
            }

            Text {
              anchors.left: cloud_slice_button_image.right
              anchors.verticalCenter: parent.verticalCenter
              anchors.leftMargin: 10 * screenScaleFactor

              horizontalAlignment: Qt.AlignLeft
              verticalAlignment: Qt.AlignVCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_10
              color: cloud_slice_button.checked ? Constants.themeGreenColor : Constants.textColor
              text: qsTr("Cloud Slicing")
            }
          }
        }

        // "My Model" group begin

        Button {
          id: model_type_button

          Layout.minimumWidth: 140 * screenScaleFactor
          Layout.minimumHeight: 34 * screenScaleFactor

          checked: true
          checkable: true

          background: Item {
            Image {
              id: model_type_button_image

              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter

              width: 13 * screenScaleFactor
              height: 13 * screenScaleFactor
              sourceSize: Qt.size(width, height)
              source: model_type_button.checked ? "qrc:/UI/photo/treeView_minus_dark.png"
                                                : "qrc:/UI/photo/treeView_plus_dark.png"
            }

            Text {
              anchors.left: model_type_button_image.right
              anchors.verticalCenter: parent.verticalCenter
              anchors.leftMargin: 10 * screenScaleFactor

              horizontalAlignment: Qt.AlignLeft
              verticalAlignment: Qt.AlignVCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_11
              color: Constants.textColor
              text: qsTr("My Model")
            }
          }
        }

        Button {
          id: uploaded_model_button

          property int index: UserInfoDialog.Page.UPLOADED_MODEL

          Layout.minimumWidth: 140 * screenScaleFactor
          Layout.minimumHeight: 28 * screenScaleFactor

          checked: true
          checkable: true
          ButtonGroup.group: page_button_group
          visible: model_type_button.checked

          background: Item {
            Image {
              id: uploaded_model_button_image

              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter

              width: 13 * screenScaleFactor
              height: 13 * screenScaleFactor
              sourceSize: Qt.size(width, height)
              source: ""
            }

            Text {
              anchors.left: uploaded_model_button_image.right
              anchors.verticalCenter: parent.verticalCenter
              anchors.leftMargin: 10 * screenScaleFactor

              horizontalAlignment: Qt.AlignLeft
              verticalAlignment: Qt.AlignVCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_10
              color: uploaded_model_button.checked ? Constants.themeGreenColor : Constants.textColor
              text: qsTr("My Uploads")
            }
          }
        }

        Button {
          id: collected_model_button

          property int index: UserInfoDialog.Page.COLLECTED_MODEL

          Layout.minimumWidth: 140 * screenScaleFactor
          Layout.minimumHeight: 28 * screenScaleFactor

          checkable: true
          ButtonGroup.group: page_button_group
          visible: model_type_button.checked

          background: Item {
            Image {
              id: collected_model_button_image

              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter

              width: 13 * screenScaleFactor
              height: 13 * screenScaleFactor
              sourceSize: Qt.size(width, height)
              source: ""
            }

            Text {
              anchors.left: collected_model_button_image.right
              anchors.verticalCenter: parent.verticalCenter
              anchors.leftMargin: 10 * screenScaleFactor

              horizontalAlignment: Qt.AlignLeft
              verticalAlignment: Qt.AlignVCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_10
              color: collected_model_button.checked ? Constants.themeGreenColor : Constants.textColor
              text: qsTr("My Favorited")
            }
          }
        }

        Button {
          id: purchased_model_button

          property int index: UserInfoDialog.Page.PURCHASED_MODEL

          Layout.minimumWidth: 140 * screenScaleFactor
          Layout.minimumHeight: 28 * screenScaleFactor

          checkable: true
          ButtonGroup.group: page_button_group
          visible: model_type_button.checked

          background: Item {
            Image {
              id: purchased_model_button_image

              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter

              width: 13 * screenScaleFactor
              height: 13 * screenScaleFactor
              sourceSize: Qt.size(width, height)
              source: ""
            }

            Text {
              anchors.left: purchased_model_button_image.right
              anchors.verticalCenter: parent.verticalCenter
              anchors.leftMargin: 10 * screenScaleFactor

              horizontalAlignment: Qt.AlignLeft
              verticalAlignment: Qt.AlignVCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_10
              color: purchased_model_button.checked ? Constants.themeGreenColor : Constants.textColor
              text: qsTr("My Purchased")
            }
          }
        }

        // "My Devices" group begin

        Button {
          id: printer_type_button

          Layout.minimumWidth: 140 * screenScaleFactor
          Layout.minimumHeight: 34 * screenScaleFactor

          checked: true
          checkable: true

          background: Item {
            Image {
              id: printer_type_button_image

              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter

              width: 13 * screenScaleFactor
              height: 13 * screenScaleFactor
              sourceSize: Qt.size(width, height)
              source: printer_type_button.checked ? "qrc:/UI/photo/treeView_minus_dark.png"
                                                  : "qrc:/UI/photo/treeView_plus_dark.png"
            }

            Text {
              anchors.left: printer_type_button_image.right
              anchors.verticalCenter: parent.verticalCenter
              anchors.leftMargin: 10 * screenScaleFactor

              horizontalAlignment: Qt.AlignLeft
              verticalAlignment: Qt.AlignVCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_11
              color: Constants.textColor
              text: qsTr("My Devices")
            }
          }
        }

        Button {
          id: printer_button

          property int index: UserInfoDialog.Page.PRINTER

          Layout.minimumWidth: 140 * screenScaleFactor
          Layout.minimumHeight: 28 * screenScaleFactor

          checked: true
          checkable: true
          ButtonGroup.group: page_button_group
          visible: printer_type_button.checked

          background: Item {
            Image {
              id: printer_button_image

              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter

              width: 13 * screenScaleFactor
              height: 13 * screenScaleFactor
              sourceSize: Qt.size(width, height)
              source: ""
            }

            Text {
              anchors.left: printer_button_image.right
              anchors.verticalCenter: parent.verticalCenter
              anchors.leftMargin: 10 * screenScaleFactor

              horizontalAlignment: Qt.AlignLeft
              verticalAlignment: Qt.AlignVCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_10
              color: printer_button.checked ? Constants.themeGreenColor : Constants.textColor
              text: qsTr("My Devices")
            }
          }
        }

        // "My Projects" group begin

        Button {
          id: project_type_button

          Layout.minimumWidth: 140 * screenScaleFactor
          Layout.minimumHeight: 34 * screenScaleFactor

          checked: true
          checkable: true

          background: Item {
            Image {
              id: project_type_button_image

              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter

              width: 13 * screenScaleFactor
              height: 13 * screenScaleFactor
              sourceSize: Qt.size(width, height)
              source: project_type_button.checked ? "qrc:/UI/photo/treeView_minus_dark.png"
                                                  : "qrc:/UI/photo/treeView_plus_dark.png"
            }

            Text {
              anchors.left: project_type_button_image.right
              anchors.verticalCenter: parent.verticalCenter
              anchors.leftMargin: 10 * screenScaleFactor

              horizontalAlignment: Qt.AlignLeft
              verticalAlignment: Qt.AlignVCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_11
              color: Constants.textColor
              text: qsTr("Print Configuration")
            }
          }
        }

        Button {
          id: uploaded_project_button

          property int index: UserInfoDialog.Page.UPLOADED_PROJECT

          Layout.minimumWidth: 140 * screenScaleFactor
          Layout.minimumHeight: 28 * screenScaleFactor

          checked: true
          checkable: true
          ButtonGroup.group: page_button_group
          visible: project_type_button.checked

          background: Item {
            Image {
              id: uploaded_project_button_image

              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter

              width: 13 * screenScaleFactor
              height: 13 * screenScaleFactor
              sourceSize: Qt.size(width, height)
              source: ""
            }

            Text {
              anchors.left: uploaded_project_button_image.right
              anchors.verticalCenter: parent.verticalCenter
              anchors.leftMargin: 10 * screenScaleFactor

              horizontalAlignment: Qt.AlignLeft
              verticalAlignment: Qt.AlignVCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_10
              color: uploaded_project_button.checked ? Constants.themeGreenColor : Constants.textColor
              text: qsTr("My Uploads")
            }
          }
        }

        Item {
          Layout.fillWidth: true
          Layout.fillHeight: true
        }
      }

      Rectangle {
        Layout.fillHeight: true
        Layout.minimumWidth: 1 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.leftMargin: 20 * screenScaleFactor
        Layout.rightMargin: 20 * screenScaleFactor
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        color: Constants.dock_border_color
      }

      StackLayout {
        id: page_layout

        Layout.fillWidth: true
        Layout.fillHeight: true

        currentIndex: root.visible ? page_button_group.checkedButton.index : -1

        Connections {
          target: root
          onVisibleChanged: {
            let visible_view = null

            switch (page_button_group.checkedButton) {
              case uploaded_model_button: visible_view = uploaded_model_view; break;
              case collected_model_button: visible_view = collected_model_view; break;
              case purchased_model_button: visible_view = purchased_model_view; break;
              case uploaded_slice_button: visible_view = uploaded_slice_view; break;
              case cloud_slice_button: visible_view = cloud_slice_view; break;
              case printer_button: visible_view = printer_view; break;
              default: break;
            }

            if (!visible_view) {
              return
            }

            if (root.visible) {
              visible_view.resetToFirstPage()
            } else {
              visible_view.requestClearItem()
            }
          }
        }

        ScrollFlowView {
          id: uploaded_slice_view

          itemModel: cxkernel_cxcloud.sliceService.uploadedSliceListModel

          itemDelegate: UserFileItem {
            state: "uploaded_slice"

            itemImage: model_image
            itemName: model_name

            onRequestImport: function() {
              if (!cxkernel_cxcloud.checkNetworkAvailable()) {
                tipDialog.tip(cxTr("cxcloud_network_unvailable_tip"))
                return
              }

              cxkernel_cxcloud.sliceService.importSlice(model_uid)
              root.hide()
            }

            onRequestExport: function() {
              cxkernel_cxcloud.sliceService.openPrintSliceWebpage(model_uid)
            }

            onRequestDelete: function() {
              cxkernel_cxcloud.sliceService.deleteUploadedSlice(model_uid)
            }
          }

          onRequestLoadItem: function(page_index, page_size) {
            cxkernel_cxcloud.sliceService.loadUploadedSliceList(page_index, page_size)
          }

          onRequestClearItem: function() {
            cxkernel_cxcloud.sliceService.clearUploadedSliceListModel()
          }
        }

        ScrollFlowView {
          id: cloud_slice_view

          itemModel: cxkernel_cxcloud.sliceService.cloudSliceListModel

          itemDelegate: UserFileItem {
            state: "cloud_slice"

            itemImage: model_image
            itemName: model_name

            onRequestImport: function() {
              if (!cxkernel_cxcloud.checkNetworkAvailable()) {
                tipDialog.tip(cxTr("cxcloud_network_unvailable_tip"))
                return
              }

              cxkernel_cxcloud.sliceService.importSlice(model_uid)
              root.hide()
            }

            onRequestExport: function() {
              cxkernel_cxcloud.sliceService.openPrintSliceWebpage(model_uid)
            }

            onRequestDelete: function() {
              cxkernel_cxcloud.sliceService.deleteCloudSlice(model_uid)
            }
          }

          onRequestLoadItem: function(page_index, page_size) {
            cxkernel_cxcloud.sliceService.loadCloudSliceList(page_index, page_size)
          }

          onRequestClearItem: function() {
            cxkernel_cxcloud.sliceService.clearCloudSliceListModel()
          }
        }

        ScrollFlowView {
          id: uploaded_model_view

          itemModel: cxkernel_cxcloud.modelService.uploadedModelGroupListModel

          itemDelegate: UserFileItem {
            id: umufi
            state: "uploaded_model"

            itemUid: model_uid
            itemName: model_name
            itemImage: model_image
            itemRestricted: model_restricted

            onRequestDetail: function() {
              execProjectListDialog(model_uid)
            }

            onRequestImport: function() {
              importModelGroup(model_uid)
            }

            onRequestExport: function() {
              root.shareDialog.share(model_uid, this)
            }

            onRequestDelete: function() {
              const accepted_callback = () => {
                cxkernel_cxcloud.modelService.deleteUploadedModelGroup(model_uid)
                cxkernel_cxcloud.projectService.deleteModelGroupProject(model_uid)
              }

              if (itemProjectCount <= 0) {
                accepted_callback()
                return
              }

              tipDialog.exec({
                message: qsTr("When you delete the model, the configuration you uploaded will be deleted simultaneously."),
                onAccepted: accepted_callback,
                onCanceled: _ => {},
              })
            }
          }

          onRequestLoadItem: function(page_index, page_size) {
            cxkernel_cxcloud.modelService.loadUploadedModelGroupList(page_index, page_size)
          }

          onRequestClearItem: function() {
            cxkernel_cxcloud.modelService.clearUploadedModelGroupListModel()
          }
        }

        ScrollFlowView {
          id: collected_model_view

          itemModel: cxkernel_cxcloud.modelService.collectedModelGroupListModel

          itemDelegate: UserFileItem {
            state: "collected_model"

            itemUid: model_uid
            itemName: model_name
            itemImage: model_image
            itemRestricted: model_restricted

            onRequestDetail: function() {
              execProjectListDialog(model_uid)
            }

            onRequestImport: function() {
              importModelGroup(model_uid)
            }

            onRequestExport: function() {
              root.shareDialog.share(model_uid, this)
            }

            onRequestDelete: function() {
              cxkernel_cxcloud.modelLibraryService.uncollectModelGroup(model_uid)
            }
          }

          onRequestLoadItem: function(page_index, page_size) {
            cxkernel_cxcloud.modelService.loadCollectedModelGroupList(page_index, page_size)
          }

          onRequestClearItem: function() {
            cxkernel_cxcloud.modelService.clearCollectedModelGroupListModel()
          }
        }

        ScrollFlowView {
          id: purchased_model_view

          itemModel: cxkernel_cxcloud.modelService.purchasedModelGroupListModel

          itemDelegate: UserFileItem {
            state: "purchased_model"

            itemUid: model_uid
            itemName: model_name
            itemImage: model_image
            itemRestricted: model_restricted

            onRequestDetail: function() {
              execProjectListDialog(model_uid)
            }

            onRequestImport: function() {
              importModelGroup(model_uid)
            }

            onRequestExport: function() {
              root.shareDialog.share(model_uid, this)
            }
          }

          onRequestLoadItem: function(page_index, page_size) {
            cxkernel_cxcloud.modelService.loadPurchasedModelGroupList(page_index, page_size)
          }

          onRequestClearItem: function() {
            cxkernel_cxcloud.modelService.clearPurchasedModelGroupListModel()
          }
        }

        ScrollFlowView {
          id: printer_view

          itemModel: cxkernel_cxcloud.printerService.printerListModel

          itemDelegate: UserPrinterItem {
            printerUid: model_device_id
            printerIp: model_ip
            printerName: model_nickname
            printerImage: model_image
            printerOnline: model_online
            printerConnected: model_connected
            tfCardExist: model_tf_card_exisit

            onRequestControl: function() {
              cxkernel_cxcloud.printerService.openCloudPrintWebpage()
            }
          }

          onRequestLoadItem: function(page_index, page_size) {
            cxkernel_cxcloud.printerService.loadPrinterList(page_index, page_size)
          }

          onRequestClearItem: function() {
            cxkernel_cxcloud.printerService.clearPrinterListModel()
          }
        }

        ScrollFlowView {
          id: uploaded_project_view

          itemModel: cxkernel_cxcloud.projectService.uploadedProjectListModel

          itemDelegate: UserProjectItem {
            itemUid: model_uid
            itemName: model_name
            itemImage: model_image
            itemPrinterName: model_printer_name
            itemLayerHeight: model_layer_height
            itemSparseInfillDensity: model_sparse_infill_density
            itemPlateCount: model_plate_count
            itemPrintDuraction: model_print_duraction
            itemMaterialWeight: model_material_weight
            itemAuthorId: model_author_id
            itemAuthorName: model_author_name
            itemAuthorImage: model_author_image
            itemCreationTimepoint: model_creation_timepoint

            onRequestImport: function() {
              if (!cxkernel_cxcloud.checkNetworkAvailable()) {
                tipDialog.tip(cxTr("cxcloud_network_unvailable_tip"))
                return
              }

              if (!kernel_undo_proxy.canUndo) {
                cxkernel_cxcloud.projectService.importProject(model_uid)
                root.hide()
                return
              }

              standaloneWindow.menuDialog.requestMenuDialog({
                projectUid: model_uid,
                projectLocalManager: project_kernel,
                projectCloudService: cxkernel_cxcloud.projectService,
                onOkClicked: function() {
                  this.projectLocalManager.saveCX3D(true)
                  this.projectCloudService.importProject(this.projectUid)
                },
                onCancelClicked: function() {
                  this.projectCloudService.importProject(this.projectUid)
                }
              }, "newProjectDialog")

              root.hide()
            }

            onRequestDelete: function() {
              tipDialog.exec({
                message: qsTr("Are you sure to delete this profile?"),
                onAccepted: _ => {
                  cxkernel_cxcloud.projectService.deleteUploadedProject(model_uid)
                },
                onCanceled: _ => {},
              })
            }
          }

          onRequestLoadItem: function(page_index, page_size) {
            cxkernel_cxcloud.projectService.loadUploadedProjectList(page_index, page_size)
          }

          onRequestClearItem: function() {
            cxkernel_cxcloud.projectService.clearUploadedProjectListModel()
          }
        }
      }
    }
  }
}
