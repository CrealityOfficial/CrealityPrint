import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.13

import "../qml"

Rectangle {
  id: root

  implicitWidth: 50 * screenScaleFactor
  implicitHeight: 108 * screenScaleFactor
  radius: 5 * screenScaleFactor

  color: Constants.leftBarBgColor

  signal pickMode()
  signal ellipseMode()
  signal rectMode()
  signal imageMode()
  signal textMode()
  signal lineMode()

  ButtonGroup {
    id: button_group
  }

  ColumnLayout {
    id: root_layout

    anchors.fill: parent
    anchors.margins: 1
    spacing: 1

    readonly property int buttonWidth: root.width - anchors.margins * 2

    Rectangle {
      Layout.fillWidth: true
      Layout.minimumHeight: 8 * screenScaleFactor

      color: "transparent"
      radius: root.radius

      Image {
        anchors.centerIn: parent
        source: "qrc:/UI/photo/leftBar/horizontalDragBtn.svg"
        width: 19 *screenScaleFactor
        height: 4 * screenScaleFactor
      }
    }

    Rectangle {
      Layout.fillWidth: true
      Layout.fillHeight: true

      radius: root.radius
      color: Constants.leftTabBgColor

      ColumnLayout {
        anchors.fill: parent
        spacing: 1

        CusImglButton {
          Layout.minimumWidth: root_layout.buttonWidth
          Layout.minimumHeight: root_layout.buttonWidth

          checkable: true
          ButtonGroup.group: button_group
          bottonSelected: checked

          borderWidth: 0
          btnRadius: root.radius
          cusTooltip.text: qsTr("Pickup")

          imgWidth: 20 * screenScaleFactor
          imgHeight: 20 * screenScaleFactor
          imgFillMode: Image.PreserveAspectFit

          enabledIconSource: Constants.laserPickImg
          pressedIconSource: Constants.laserPickCheckedImg
          highLightIconSource: Constants.laserPickHoveredImg

          defaultBtnBgColor: Constants.leftBtnBgColor_normal
          hoveredBtnBgColor: Constants.leftBtnBgColor_hovered
          selectedBtnBgColor: Constants.leftBtnBgColor_selected

          onClicked: pickMode()
        }

        CusImglButton {
          Layout.minimumWidth: root_layout.buttonWidth
          Layout.minimumHeight: root_layout.buttonWidth

          borderWidth: 0
          btnRadius: root.radius
          cusTooltip.text: qsTr("Image")

          imgWidth: 20 * screenScaleFactor
          imgHeight: 20 * screenScaleFactor
          imgFillMode: Image.PreserveAspectFit

          enabledIconSource: Constants.laserImageImg
          pressedIconSource: Constants.laserImageCheckedImg
          highLightIconSource: Constants.laserImageHoveredImg

          defaultBtnBgColor: Constants.leftBtnBgColor_normal
          hoveredBtnBgColor: Constants.leftBtnBgColor_hovered
          selectedBtnBgColor: Constants.leftBtnBgColor_selected

          onClicked: imageMode()
        }
      }
    }
  }
}
