import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Layouts 1.13

import "./"
import "../qml"
import "../secondqml"

Popup {
  id: root

  width: 298 * screenScaleFactor
  height: 277 * screenScaleFactor

  visible: false

  property var modelLibarayDialog: null
  readonly property int randomModelGroupCount: 8

  Component.onCompleted: {
    cxkernel_cxcloud.modelLibraryService.setRandomModelGroupListMaxSize(randomModelGroupCount)
    cxkernel_cxcloud.modelLibraryService.loadRandomModelGroupList(randomModelGroupCount)
  }

  onVisibleChanged: {
    if (!visible) { return }
    cxkernel_cxcloud.modelLibraryService.loadRandomModelGroupList(randomModelGroupCount)
  }

  background: Rectangle {
    id: background

    color: Constants.lpw_bgColor
    radius: 5 * screenScaleFactor
    border.width: 1 * screenScaleFactor
    border.color: Constants.dock_border_color

    CusPopViewTitle {
      id: title

      anchors.top: parent.top
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.margins: 1 * screenScaleFactor
      height: 24 * screenScaleFactor

      leftTop: true
      rightTop: true
      radius: background.radius

      color: Constants.lpw_titleColor
      borderColor: "transparent"
      fontColor: Constants.themeTextColor
      closeBtnNColor: "transparent"

      title: qsTranslate("ModelLibraryDialog", "Model Library")

      maxBtnVis: false
      clickedable: false
      onCloseClicked: {
        root.close()
      }
    }

    GridLayout {
      id: panel

      anchors.top: title.bottom
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.margins: 15 * screenScaleFactor

      rows: 3
      columns: 3
      rowSpacing: 8 * screenScaleFactor
      columnSpacing: 8 * screenScaleFactor

      Repeater {
        model: cxkernel_cxcloud.modelLibraryService.randomModelGroupListModel

        delegate: Loader {
          sourceComponent: model_button_component
          Layout.preferredWidth: 84 * screenScaleFactor
          Layout.preferredHeight: 69 * screenScaleFactor
          onLoaded: {
            item.groupUid = model_uid
            item.groupImage = model_image
          }
        }
      }

      Loader {
        sourceComponent: model_button_component
        Layout.preferredWidth: 84 * screenScaleFactor
        Layout.preferredHeight: 69 * screenScaleFactor
        onLoaded: {
          item.isMoreGroupButton = true
        }
      }
    }

    Component {
      id: model_button_component

      Button {
        id: model_button

        property string groupUid: ""
        property string groupImage: ""
        property bool isMoreGroupButton: false

        background: Rectangle {
          radius: 10 * screenScaleFactor

          color: Constants.modelAlwaysMoreBtnBgColor

          BaseCircularImage {
            anchors.fill: parent
            visible: !model_button.isMoreGroupButton
            radiusImg: parent.radius
            isPreserveAspectCrop: true
            img_src: model_button.groupImage
          }
        }

        contentItem: Rectangle {
          anchors.fill: model_button.background
          radius: model_button.background.radius

          color: "transparent"
          border.color: model_button.hovered ? "#2891D1" : Constants.lpw_BtnBorderColor
          border.width: model_button.hovered ? 2 * screenScaleFactor : 1 * screenScaleFactor

          Text {
            anchors.centerIn: parent
            visible: model_button.isMoreGroupButton
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            font.pointSize: Constants.labelFontPointSize_12
            color: "#333333"
            text: "%1>>".arg(qsTranslate("ModelLibraryDialog", "More"))
          }
        }

        onClicked: {
          root.close()

          if (!isMoreGroupButton && groupUid !== "") {
            root.modelLibarayDialog.currentGroupUid = groupUid
          }

          root.modelLibarayDialog.show()
        }
      }
    }
  }
}
