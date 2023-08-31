import QtQuick 2.5
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0

import ".."
import "../qml"

Popup {
  id: idModelAlwaysDlg
  width: 298 * screenScaleFactor
  height: 277 * screenScaleFactor

  visible: false

  property var modelGroupMap: {
    "0": 0
  }

  signal clickItem(var id, var image, var name, var count, var author, var avtar, var ctime)
  signal clickMoreItem()

  function setRecommendModelsData(strJson) {
    var componentButton = Qt.createComponent("../secondqml/ModelAlwaysShowItem.qml")
    if (componentButton.status === Component.Ready) {
      var objectArray = JSON.parse(strJson)
      if (objectArray.code === 0) {
        deleteModelGroupComponent()

        let max_count = 8

        var objResult = objectArray.result.list
        for (var key in objResult) {
          if (objResult[key].model == null) {
            continue
          }

          var objCovers = objResult[key].covers
          var obj = componentButton.createObject(idModelLibraryList, {
            "btnNameText"  : objResult[key].model.groupName,
            "btnModelImage": objResult[key].model.covers[0].url,
            "modelGroupId" : objResult[key].model.id,
            "btnAuthorText": objResult[key].model.userInfo.nickName,
            "btnAvtarImage": objResult[key].model.userInfo.avatar,
            "modelCount"   : objResult[key].model.modelCount,
            "modelTime"    : objResult[key].model.createTime
          })

          modelGroupMap[objResult[key].model.id] = obj
          obj.sigButtonClicked.connect(onItemClicked)

          --max_count
          if (max_count == 0) {
            break
          }
        }

        var obj1 = componentButton.createObject(idModelLibraryList, {
          "isTableShow": true,
          "modelGroupId": "-1"
        })

        modelGroupMap["-1"] = obj1
        obj1.sigButtonClicked.connect(onMoreItemClicked)
      }
    }
  }

  function onItemClicked(id, image, name, count, author, avtar, ctime) {
    hide()
    clickItem(id, image, name, count, author, avtar, ctime)
  }

  function onMoreItemClicked(id, image, name, count, author, avtar, ctime) {
    hide()
    clickMoreItem()
  }

  function deleteModelGroupComponent() {
    for (var key in modelGroupMap) {
      var strkey = "-%1-".arg(key)
      if (strkey != "-0-") {
        modelGroupMap[key].destroy()
        delete modelGroupMap[key]
      } else {
        delete modelGroupMap[key]
      }
    }
  }

  function show(x, y) {
    idModelAlwaysDlg.x = x
    idModelAlwaysDlg.y = y
    idModelAlwaysDlg.visible = true
  }

  function hide() {
    idModelAlwaysDlg.visible = false
  }

  background: Rectangle {
    color: Constants.lpw_bgColor
    radius: 5 * screenScaleFactor
    border.width: 1 * screenScaleFactor
    border.color: Constants.dock_border_color

    clip: true

    CusPopViewTitle {
      id: idTitle

      anchors.top: parent.top
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.margins: 1 * screenScaleFactor
      height: 24 * screenScaleFactor

      leftTop: true
      rightTop: true
      color: Constants.lpw_titleColor
      radius: 5 * screenScaleFactor

      fontColor: Constants.themeTextColor
      title: qsTranslate("ModelLibraryInfoDlg", "Model Library")

      borderColor: "transparent"

      closeBtnNColor: "transparent"
      closeBtnWidth: 8 * screenScaleFactor
      closeBtnHeight: 8 * screenScaleFactor

      maxBtnVis: false
      clickedable: false
      onCloseClicked: {
        idModelAlwaysDlg.visible = false
      }
    }

    Rectangle {
      anchors.top: idTitle.bottom
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right

      color: "transparent"

      Grid {
        id: idModelLibraryList
        anchors.fill: parent
        anchors.margins: 15 * screenScaleFactor
        spacing: 8 * screenScaleFactor
        columns: 3
        rows: 3
      }
    }
  }
}
