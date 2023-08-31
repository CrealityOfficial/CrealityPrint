import QtQuick 2.10
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.12

import "../qml"

Item {
  property string groupId: ""
  property string modelName: ""
  property string modelId: ""
  property int    modelSize: 0
  property bool   btnIsSelected: false

  signal sigBtnDetailClicked(var model_id)
  signal sigBtnDownLoadDetailModel(var model_id)
  signal sigDownloadedTip()
  signal sigLoginTip()

  width: parent.width
  height: 36 * screenScaleFactor

  RowLayout {
    anchors.fill: parent

    spacing: 10 * screenScaleFactor

    BasicButton {
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.minimumWidth: 220 * screenScaleFactor
      Layout.fillHeight: true

      btnRadius: 5 * screenScaleFactor
      btnBorderW: 0

      defaultBtnBgColor: Constants.model_library_item_default_color
      hoveredBtnBgColor: Constants.model_library_item_hovered_color
      selectedBtnBgColor: Constants.model_library_item_checked_color

      btnSelected: btnIsSelected

      text: modelName
      btnText.color: btnIsSelected ? Constants.model_library_item_text_checked_color
                                   : Constants.model_library_item_text_default_color
      btnText.width: width - 20 * screenScaleFactor

      onSigButtonClicked: {
        sigBtnDetailClicked(modelId)
      }
    }

    StyledLabel {
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.fillWidth: true
      Layout.fillHeight: true

      verticalAlignment: Text.AlignVCenter

      font.family: Constants.labelFontFamily
      font.pointSize: Constants.labelFontPointSize_9
      color: Constants.model_library_general_text_color

      text: {
        if (modelSize > 1024 * 1024 * 1024) {
          return Math.ceil(modelSize / (1024 * 1024 * 1024)) + "GB"
        } else if (modelSize > 1024 * 1024) {
          return Math.ceil(modelSize / (1024 * 1024)) + "MB"
        } else if (modelSize > 1024) {
          return Math.ceil(modelSize / 1024) + "KB"
        } else {
          return modelSize + "B"
        }
      }
    }

    BasicDialogButton {
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
      Layout.minimumWidth: 150 * screenScaleFactor
      Layout.fillHeight: true

      visible: btnIsSelected

      btnBorderW: 1 * screenScaleFactor
      btnRadius: height / 2
      borderColor: hovered ? Constants.model_library_action_button_border_checked_color
                           : Constants.model_library_action_button_border_default_color

      imgWidth: 14 * screenScaleFactor
      imgHeight: 14 * screenScaleFactor
      iconImage.fillMode: Image.PreserveAspectFit
      pressedIconSource: hovered ? Constants.model_library_download_checked_image
                                 : Constants.model_library_download_default_image

      defaultBtnBgColor: Constants.model_library_action_button_default_color
      hoveredBtnBgColor: Constants.model_library_action_button_checked_color

      text: qsTr("Download")
      btnTextColor: hovered ? Constants.model_library_action_button_text_checked_color
                            : Constants.model_library_action_button_text_default_color

      onClicked: {
        if (!cxkernel_cxcloud.downloadService.checkModelDownloadable(groupId, modelId)) {
          sigDownloadedTip()
          return
        }

        if (!cxkernel_cxcloud.accountService.logined) {
          cxkernel_cxcloud.downloadService.makeModelDownloadPromise(groupId, modelId)
          sigLoginTip()
          return
        }

        sigBtnDownLoadDetailModel(modelId)
      }
    }
  }
}
