import QtQuick 2.5
import QtQuick.Controls 2.12

import QtGraphicalEffects 1.13

import "qrc:/CrealityUI"
import ".."
import "../qml"

Button {
  id: root

  width: 84 * screenScaleFactor
  height: 68 * screenScaleFactor

  property bool isTableShow: false
  signal sigButtonClicked(var isMoreBtn, var id, var name, var count, var author, var avtar, var ctime)

  property string btnNameText: "name"
  property string btnAuthorText: "author"
  property string btnModelImage: ""
  property string btnAvtarImage: ""
  property string modelGroupId: ""
  property int modelCount: 0
  property string modelTime: ""

  onClicked: {
    sigButtonClicked(modelGroupId, btnModelImage, btnNameText, modelCount,
                     btnAuthorText, btnAvtarImage, modelTime)
  }

  background: Rectangle {
    anchors.fill: parent

    clip: true

    color: root.isTableShow ? Constants.modelAlwaysMoreBtnBgColor : "transparent"
    border.width: root.hovered ? 2 * screenScaleFactor : 1 * screenScaleFactor
    border.color: root.hovered ? "#1E9BE2" : Constants.lpw_BtnBorderColor

    radius: 10 * screenScaleFactor

    BaseCircularImage {
      z: -1

      anchors.fill: parent
      anchors.margins: 1 * screenScaleFactor

      radiusImg: parent.radius
      visible: !root.isTableShow
      img_src: root.btnModelImage
      isPreserveAspectCrop: true
    }

    Text {
      anchors.centerIn: parent
      visible: root.isTableShow
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      font.pointSize: Constants.labelFontPointSize_12
      font.family: Constants.labelFontFamily
      color: "#333333"
      text: qsTr("More>>")
    }
  }
}
