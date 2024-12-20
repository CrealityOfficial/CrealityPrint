import QtQuick 2.10
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.12

import "../qml"

Item {
  property string groupId: ""
  property string modelName: ""
  property string btnImgUrl: ""
  property string modelId: ""
  property int    modelSize: 0
  property bool   btnIsSelected: false

  signal sigBtnDetailClicked(var model_id)
  signal sigBtnDownLoadDetailModel(var model_id)
  signal sigBtnImportDetailModel(var model_id)
  signal sigBtnImportDirectModel(var group_id, var model_id)
  signal sigDownloadedTip()
  signal sigLoginTip()

  property string img_download_all_d:  Constants.currentTheme ?"qrc:/UI/photo/model_info/download_all.svg" :"qrc:/UI/photo/model_info/download_all_d.svg"
  property string img_download_one_d:  Constants.currentTheme ? "qrc:/UI/photo/model_info/download_one.svg":"qrc:/UI/photo/model_info/download_one_d.svg"
  property string img_import_all_d:  Constants.currentTheme ?"qrc:/UI/photo/model_info/import_all.svg": "qrc:/UI/photo/model_info/import_all_d.svg"
  property string img_import_one_d: Constants.currentTheme ? "qrc:/UI/photo/model_info/import_one.svg":"qrc:/UI/photo/model_info/import_one_d.svg"

  width: parent.width
  height: 100 * screenScaleFactor
  id:root_download
  RowLayout {
    anchors.fill: parent

    spacing: 10 * screenScaleFactor
    RowLayout{
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        Layout.minimumWidth: 228 * screenScaleFactor
        MouseArea{
          anchors.fill: parent
          cursorShape: Qt.PointingHandCursor
          onClicked:  sigBtnDetailClicked(modelId)

        }
        Rectangle{
            width: 80* screenScaleFactor
            height: 80* screenScaleFactor
            radius: 5* screenScaleFactor
            color: "transparent"
            border.width:btnIsSelected ? 2* screenScaleFactor: 1
            border.color:
                btnIsSelected ? Constants.themeGreenColor:Constants.model_library_action_button_border_default_color
            Image {
                anchors.fill: parent
                source: btnImgUrl
            }
        }

        ColumnLayout{
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.minimumWidth: 150 * screenScaleFactor

            StyledLabel {
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.fillWidth: true
                text: modelName
                font.family: Constants.labelFontFamilyBold
                font.weight: Constants.labelFontWeight
                font.pointSize: Constants.labelFontPointSize_9
                color: Constants.themeTextColor
            }
            StyledLabel {
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                Layout.fillWidth: true
                //    Layout.preferredHeight: 36* screenScaleFactor
                //    Layout.fillHeight: true

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
        }
    }
    BasicDialogButton {
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
      Layout.minimumWidth: 130 * screenScaleFactor
      Layout.preferredHeight: 36* screenScaleFactor
     // Layout.fillHeight: true

      visible: btnIsSelected

      btnBorderW: 1 * screenScaleFactor
      btnRadius: height / 2
      borderColor: hovered ? Constants.model_library_action_button_border_checked_color
                           : Constants.model_library_action_button_border_default_color

      imgWidth: 14 * screenScaleFactor
      imgHeight: 14 * screenScaleFactor
      iconImage.fillMode: Image.PreserveAspectFit
      pressedIconSource: hovered ? img_download_all_d : img_download_one_d

      defaultBtnBgColor: Constants.model_library_action_button_default_color
      hoveredBtnBgColor: Constants.themeGreenColor

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

    BasicDialogButton {
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
      Layout.minimumWidth: 130 * screenScaleFactor
       Layout.preferredHeight: 36* screenScaleFactor
      //Layout.fillHeight: true

      visible: btnIsSelected

      btnBorderW: 1 * screenScaleFactor
      btnRadius: height / 2
      borderColor: hovered ? Constants.model_library_action_button_border_checked_color
                           : Constants.model_library_action_button_border_default_color

      imgWidth: 14 * screenScaleFactor
      imgHeight: 14 * screenScaleFactor
      iconImage.fillMode: Image.PreserveAspectFit
      pressedIconSource: hovered ? img_import_all_d : img_import_one_d
      defaultBtnBgColor: Constants.model_library_action_button_default_color
      hoveredBtnBgColor: Constants.themeGreenColor

      text: qsTr("Import")
      btnTextColor: hovered ? Constants.model_library_action_button_text_checked_color
                            : Constants.model_library_action_button_text_default_color

      onClicked: {
        if (!cxkernel_cxcloud.accountService.logined) {
          cxkernel_cxcloud.downloadService.makeModelDownloadPromise(groupId, modelId, true)
          sigLoginTip()
          return
        }

        if (cxkernel_cxcloud.downloadService.checkModelDownloaded(groupId, modelId)) {
          sigBtnImportDirectModel(groupId, modelId)
        } else {
          sigBtnImportDetailModel(modelId)
        }
      }
    }
  }
}
