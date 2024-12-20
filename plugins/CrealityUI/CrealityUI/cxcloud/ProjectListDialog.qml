import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import QtQml 2.15

import ".."
import "../components"
import "../secondqml"

DockItem {
  id: root

  readonly property var focusedModelGroup: cxkernel_cxcloud.modelService.focusedModelGroup

  property var tipDialog: null
  property var userInfoDialog: null

  width: 550 * screenScaleFactor
  height: 512 * screenScaleFactor
  title: focusedModelGroup.name

  ListView {
    id: list_view

    anchors.fill: parent
    anchors.margins: 40 * screenScaleFactor
    anchors.topMargin: 40 * screenScaleFactor + root.currentTitleHeight
    anchors.rightMargin: 20 * screenScaleFactor

    ScrollBar.vertical: ScrollBar {
      policy: list_view.contentHeight > list_view.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
    }

    ScrollBar.horizontal: ScrollBar {
      policy: ScrollBar.AlwaysOff
    }

    clip: true
    spacing: 20 * screenScaleFactor

    model: focusedModelGroup.projects

    delegate: UserProjectItem {
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

      showImportButton: focusedModelGroup.price > 0 ? focusedModelGroup.purchased : true
      showAuthorSelf: true

      onRequestImport: {
        if (!cxkernel_cxcloud.checkNetworkAvailable()) {
          tipDialog.tip(cxTr("cxcloud_network_unvailable_tip"))
          return
        }

        if (!kernel_undo_proxy.canUndo) {
          cxkernel_cxcloud.projectService.importProject(model_uid)
          root.hide()
          userInfoDialog.hide()
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
        userInfoDialog.hide()
      }

      onRequestDelete: {
        tipDialog.exec({
          message: qsTr("Are you sure to delete this configuration?"),
          onAccepted: _ => {
            cxkernel_cxcloud.projectService.deleteUploadedProject(model_uid)
          },
          onCanceled: _ => {},
        })
      }
    }
  }
}
