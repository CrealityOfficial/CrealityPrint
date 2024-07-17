import QtQuick 2.10
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13

import QtQml 2.13

import QtGraphicalEffects 1.13

import ".."
import "../components"
import "../secondqml"

DockItem {
  id: root

  property var tipDialog: null
  property var shareDialog: null
  property var downloadManageDialog: null

  width: 1200 * screenScaleFactor
  height: 764 * screenScaleFactor
  title: qsTr("Personal Center")

  enum Page {
    UPLOADED_MODEL = 0,
    COLLECTED_MODEL = 1,
    PURCHASED_MODEL = 2,
    UPLOADED_SLICE = 3,
    CLOUD_SLICE = 4,
    PRINTER = 5
  }

  function setPage(page) {
    let button = null

    switch (page) {
      case UserInfoDialog.Page.UPLOADED_MODEL: button = uploaded_model_button; break;
      case UserInfoDialog.Page.COLLECTED_MODEL: button = collected_model_button; break;
      case UserInfoDialog.Page.PURCHASED_MODEL: button = purchased_model_button; break;
      case UserInfoDialog.Page.UPLOADED_SLICE: button = uploaded_slice_button; break;
      case UserInfoDialog.Page.CLOUD_SLICE: button = cloud_slice_button; break;
      case UserInfoDialog.Page.PRINTER: button = printer_button; break;
    }

    if (button) {
      button.checked = true
    }
  }

  ColumnLayout {
    id: root_layout

    anchors.fill: parent
    anchors.margins: 20 * screenScaleFactor
    anchors.topMargin: 20 * screenScaleFactor + root.currentTitleHeight
    spacing: 20 * screenScaleFactor

    RowLayout {
      id: account_layout

      readonly property var service: cxkernel_cxcloud.accountService

      Layout.fillWidth: true
      Layout.minimumHeight: 70 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight
      Layout.alignment: Qt.AlignHCenter | Qt.AlignTop

      spacing: 0

      BaseCircularImage {
        Layout.minimumWidth: account_layout.Layout.minimumHeight
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        img_src: account_layout.service.avatar
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
          font.pointSize: Constants.labelFontPointSize_9
          color: Constants.textColor
          text: account_layout.service.nickName
        }

        Text {
          horizontalAlignment: Qt.AlignLeft
          verticalAlignment: Qt.AlignTop

          font.family: Constants.labelFontFamily
          font.weight: Constants.labelFontWeight
          font.pointSize: Constants.labelFontPointSize_9
          color: Constants.textColor
          text: "ID: %1".arg(account_layout.service.userId)
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
        font.pointSize: Constants.labelFontPointSize_9
        color: Constants.textColor
        text: "%1:".arg(qsTr("Remaining Space"))
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
            (account_layout.service.usedStorageSpace / account_layout.service.maxStorageSpace)
          radius: parent.radius
          color: "#009CFF"
        }

        Text {
          anchors.bottom: parent.top
          anchors.horizontalCenter: parent.horizontalCenter
          anchors.bottomMargin: 4 * screenScaleFactor

          horizontalAlignment: Qt.AlignHCenter
          verticalAlignment: Qt.AlignVCenter
          font.family: Constants.labelFontFamily
          font.weight: Constants.labelFontWeight
          font.pointSize: Constants.labelFontPointSize_9
          color: Constants.textColor
          text: "%1/%2".arg(translateStorage(account_layout.service.usedStorageSpace, 2))
                        .arg(translateStorage(account_layout.service.maxStorageSpace, 0))

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
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.textColor
            text: qsTr("Quit")
          }
        }

        onClicked: {
          account_layout.service.logout()
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
              font.pointSize: Constants.labelFontPointSize_9
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
              font.pointSize: Constants.labelFontPointSize_9
              color: uploaded_model_button.checked ? "#00A3FF" : Constants.textColor
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
              font.pointSize: Constants.labelFontPointSize_9
              color: collected_model_button.checked ? "#00A3FF" : Constants.textColor
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
              font.pointSize: Constants.labelFontPointSize_9
              color: purchased_model_button.checked ? "#00A3FF" : Constants.textColor
              text: qsTr("My Purchased")
            }
          }
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
              font.pointSize: Constants.labelFontPointSize_9
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
              font.pointSize: Constants.labelFontPointSize_9
              color: uploaded_slice_button.checked ? "#00A3FF" : Constants.textColor
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
              font.pointSize: Constants.labelFontPointSize_9
              color: cloud_slice_button.checked ? "#00A3FF" : Constants.textColor
              text: qsTr("Cloud Slicing")
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
              font.pointSize: Constants.labelFontPointSize_9
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
              font.pointSize: Constants.labelFontPointSize_9
              color: printer_button.checked ? "#00A3FF" : Constants.textColor
              text: qsTr("My Devices")
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

        currentIndex: page_button_group.checkedButton.index

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
          id: uploaded_model_view

          itemModel: cxkernel_cxcloud.modelService.uploadedModelGroupListModel

          itemDelegate: UserFileItem {
            state: "uploaded_model"

            itemImage: model_image
            itemName: model_name

            onRequestImport: {
              if (!cxkernel_cxcloud.downloadService.checkModelGroupDownloadable(model_uid)) {
                root.tipDialog.tipModelDownloaded(root.hide)
                return
              }

              cxkernel_cxcloud.downloadService.startModelGroupDownloadTask(model_uid, true)
              root.downloadManageDialog.show()
              root.hide()
            }

            onRequestExport: {
              root.shareDialog.share(model_uid, this)
            }

            onRequestDelete: {
              cxkernel_cxcloud.modelService.deleteUploadedModelGroup(model_uid)
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

            itemImage: model_image
            itemName: model_name

            onRequestImport: {
              if (!cxkernel_cxcloud.downloadService.checkModelGroupDownloadable(model_uid)) {
                root.tipDialog.tipModelDownloaded(root.hide)
                return
              }

              cxkernel_cxcloud.downloadService.startModelGroupDownloadTask(model_uid, true)
              root.downloadManageDialog.show()
              root.hide()
            }

            onRequestExport: {
              root.shareDialog.share(model_uid, this)
            }

            onRequestDelete: {
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

            itemImage: model_image
            itemName: model_name

            onRequestImport: {
              if (!cxkernel_cxcloud.downloadService.checkModelGroupDownloadable(model_uid)) {
                root.tipDialog.tipModelDownloaded(root.hide)
                return
              }

              cxkernel_cxcloud.downloadService.startModelGroupDownloadTask(model_uid, true)
              root.downloadManageDialog.show()
              root.hide()
            }

            onRequestExport: {
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
          id: uploaded_slice_view

          itemModel: cxkernel_cxcloud.sliceService.uploadedSliceListModel

          itemDelegate: UserFileItem {
            state: "uploaded_slice"

            itemImage: model_image
            itemName: model_name

            onRequestImport: {
              cxkernel_cxcloud.sliceService.importSlice(model_uid)
              root.hide()
            }

            onRequestExport: {
              cxkernel_cxcloud.sliceService.openPrintSliceWebpage(model_uid)
            }

            onRequestDelete: {
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

            onRequestImport: {
              cxkernel_cxcloud.sliceService.importSlice(model_uid)
              root.hide()
            }

            onRequestExport: {
              cxkernel_cxcloud.sliceService.openPrintSliceWebpage(model_uid)
            }

            onRequestDelete: {
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

            onRequestControl: {
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
      }
    }
  }
}
