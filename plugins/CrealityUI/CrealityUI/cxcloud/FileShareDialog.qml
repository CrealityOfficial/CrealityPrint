import QtQuick 2.0
import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Window 2.0
import QtGraphicalEffects 1.12

import "./"
import "../qml"
import "../secondqml"

Window {
  id: root
  width: 408 * screenScaleFactor
  height: 148 * screenScaleFactor

  property string groupUid: ""
  property var service: cxkernel_cxcloud.modelLibraryService
  property var tipDialog: null

  function share(group_uid, item) {
    groupUid = group_uid

    let pos = item.mapToGlobal(item.width - width, item.height)
    x = pos.x + 10 * screenScaleFactor
    y = pos.y + 10 * screenScaleFactor
    visible = true
  }

  property string contentBackground: Constants.dialogContentBgColor
  property alias linkText: idShareLink.text
  signal copyButtonClicked
  signal cancelButtonClicked

  flags: Qt.FramelessWindowHint | Qt.Dialog
  modality: Qt.ApplicationModal
  color: "transparent" //Constants.themeColor

  Image {
    width: parent.width
    height: parent.height
    //mipmap: true
    smooth: true
    cache: false
    asynchronous: true
    fillMode: Image.PreserveAspectFit
    source: "qrc:/UI/photo/mode_library_share_bg.png"
  }

  Column {
    x: 31 * screenScaleFactor
    y: 36 * screenScaleFactor
    spacing: 10 * screenScaleFactor

    Row {
      spacing: 5 * screenScaleFactor

      CusText {
        id: idContentText
        anchors.verticalCenter: parent.verticalCenter
        fontText: qsTr("Link") + ": "
        fontPointSize: Constants.labelFontPointSize_9
        fontFamily: Constants.labelFontFamily
        fontWeight: Constants.labelFontWeight
        fontWidth: 50 * screenScaleFactor
        width: 50 * screenScaleFactor
        hAlign: 2
      }

      StyledLabel {
        id: idShareLink
        width: 294 * screenScaleFactor
        height: 30 * screenScaleFactor
        font.pointSize: Constants.labelFontPointSize_9
        color: "#333333"
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
        Rectangle {
          anchors.fill: parent
          color: "transparent"
          border.width: 1
          border.color: "#E7E7E7"
        }

        text: root.service.createModelGroupUrl(root.groupUid)
      }
    }

    Row {
      x: 55 * screenScaleFactor
      spacing: 14 * screenScaleFactor

      BasicDialogButton {
        width: 140 * screenScaleFactor
        height: 36 * screenScaleFactor
        text: qsTr("Copy Link")
        btnRadius: 18 * screenScaleFactor
        btnBorderW: 0
        font.pointSize: Constants.labelFontPointSize_normal
        defaultBtnBgColor: "#009CFF"
        hoveredBtnBgColor: "#1EB6E2"
        onSigButtonClicked: {
          root.tipDialog.tipCopyLinkSuccessed()
          root.service.shareModelGroup(root.groupUid)
          root.visible = false
        }
      }

      BasicDialogButton {
        width: 140 * screenScaleFactor
        height: 36 * screenScaleFactor
        text: qsTr("Cancel")
        btnRadius: 18 * screenScaleFactor
        btnBorderW: 0
        font.pointSize: Constants.labelFontPointSize_normal
        defaultBtnBgColor: "#BBBBBB"
        hoveredBtnBgColor: "#009CFF"

        onSigButtonClicked: {
          root.visible = false
        }
      }
    }
  }
}
