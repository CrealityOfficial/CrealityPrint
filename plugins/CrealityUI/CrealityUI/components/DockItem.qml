import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

import QtQml 2.15

import Qt.labs.platform 1.1

import "./"
import "../qml"

/// @brief Dock 组件
///        将 Dock 组件的 context 属性设置为某个 Dock 组件上下文, 即绑定该组件与该上下文.
///        组件吸附后, 将停留在上下文所在窗口, 并由上下文控制坐标与尺寸;
///        取消吸附后, 将作为独立的弹窗, 自行控制坐标与尺寸.
/// @see DockContext.qml
/// @see DockController.h
/// @see DockController.cpp

Window {
  // ---------- interface [beg] ----------

  // dock 组件的上下文, 若不设置则该组件为普通弹窗
  property var context: null

  // 当前是否吸附于上下文
  property bool adsorbed: false

  // 不吸附 (即窗口化) 时标题栏的相关属性 (吸附 (即最大化) 时标题栏将自动隐藏)
  property int titleHeight: 30 * screenScaleFactor
  property color titleColor: Constants.dialogTitleColor
  property color titleTextColor: Constants.dialogTitleTextColor
  property string titleFontFamily: Constants.labelFontFamily
  readonly property int currentTitleHeight: adsorbed ? 0 : titleHeight
  property var minimumX: undefined
  property var maximumX: undefined
  property var minimumY: undefined
  property var maximumY: undefined

  // 窗口相关属性 (吸附时 border 与 radius 自动设置为 0)
  property color panelColor: Constants.themeColor
  property color borderColor: Constants.dock_border_color
  property int borderWidth: 1 * screenScaleFactor
  property int windowRadius: 5 * screenScaleFactor

  // 按下关闭按钮后是否自动关闭 (无论是否自动关闭, 都会先发出 closeButtonClicked 信号)
  property bool autoClose: true
  signal closeButtonClicked()

  // ---------- interface [end] ----------

  id: root

  readonly property var _context: Qt.platform.os === "osx" ? null : context

  signal itemAdsorbedChanged(bool absorbed)

  flags: Qt.FramelessWindowHint | Qt.Dialog
  modality: _context == null ? Qt.ApplicationModal : Qt.NonModal

  Connections {
    target: root._context
    enabled: root.adsorbed

    function onAdsorbedItemXChanged() {
      x = root._context.adsorbedItemX
    }

    function onAdsorbedItemYChanged() {
      y = root._context.adsorbedItemY
    }

    function onAdsorbedItemWidthChanged() {
      width = root._context.adsorbedItemWidth
    }

    function onAdsorbedItemHeightChanged() {
      height = root._context.adsorbedItemHeight
    }
  }

  QtObject {
    id: cache

    property bool contextChanging: false
    property var context: null

    property int x: 0
    property int y: 0
    property int width: 0
    property int height: 0
  }

  onContextChanged: {
    cache.contextChanging = true

    let cache_adsorbed = root.adsorbed
    root.adsorbed = false

    if (cache.context) {
      cache.context.remove(root)
    }

    if (root._context) {
      root._context.append(root)
    }

    cache.context = root._context
    root.adsorbed = cache_adsorbed

    cache.contextChanging = false
  }

  onAdsorbedChanged: {
    itemAdsorbedChanged(adsorbed)

    if (!cache.contextChanging) {
      visible = true
    }

    if (adsorbed) {
      cache.x = root.x
      cache.y = root.y
      cache.width = root.width
      cache.height = root.height

      root.x = root._context.adsorbedItemX
      root.y = root._context.adsorbedItemY
      root.width = root._context.adsorbedItemWidth
      root.height = root._context.adsorbedItemHeight

    } else {
      root.x = cache.x
      root.y = cache.y
      root.width = cache.width
      root.height = cache.height
    }

    if (root._context && root._context.controller) {
      root._context.controller.syncContext()
    }
  }

  onVisibleChanged: {
    if (root._context && root._context.controller) {
      root._context.controller.syncContext()
    }
  }

  color:"transparent"

  Rectangle {
    id: background
    anchors.fill: parent

    color: root.titleColor
    radius: adsorbed ? 0 : root.windowRadius
    border.color: adsorbed ? root.panelColor : root.borderColor
    border.width: root.borderWidth

    Rectangle {
      id: title
      visible: !root.adsorbed

      anchors.top: parent.top
      anchors.left: parent.left
      anchors.right: parent.right

      height: root.adsorbed ? 0 : root.titleHeight

      color: "transparent"

      MouseArea {
        id: title_area

        height: parent.height
        width: parent.width

        property point clickPos: "0,0"

        onPressed: {
          clickPos = Qt.point(mouse.x, mouse.y)
        }

        onPositionChanged: {
          var cursorPos = WizardUI.cursorGlobalPos()
          root.x = cursorPos.x - clickPos.x
          root.y = cursorPos.y - clickPos.y
        }

        onReleased: {
          if (typeof root.minimumX === "number" && root.x < root.minimumX) {
            root.x = root.minimumX
          }

          if (typeof root.maximumX === "number" && root.x > root.maximumX) {
            root.x = root.maximumX
          }

          if (typeof root.minimumY === "number" && root.y < root.minimumY) {
            root.y = root.minimumY
          }

          if (typeof root.maximumY === "number" && root.y > root.maximumY) {
            root.y = root.maximumY
          }
        }

        Text {
          id: title_text

          anchors.verticalCenter: title_area.verticalCenter
          anchors.left: title_area.left
          anchors.leftMargin: 10 * screenScaleFactor

          text: root.title
          color: root.titleTextColor
          font.family: root.titleFontFamily
          font.weight: Constants.labelFontWeight
          font.pointSize: Constants.labelFontPointSize_9
        }

        Button {
          id: adsorb_button

          visible: root._context != null

          anchors.top: title_area.top
          anchors.right: close_button.left
          anchors.bottom: title_area.bottom

          width: height

          hoverEnabled: true

          background: Rectangle {
            anchors.fill: parent
            anchors.topMargin: root.borderWidth
            color: adsorb_button.hovered ? Constants.hoveredColor : "transparent"
          }

          Image {
            anchors.centerIn: parent
            width: 10 * screenScaleFactor
            height: 10 * screenScaleFactor
            source: adsorb_button.hovered ? "qrc:/UI/photo/maxBtn_d.svg" : "qrc:/UI/photo/maxBtn.svg"
          }

          onClicked: {
            if (!root._context) { return }
            root.adsorbed = true
          }
        }

        Button {
          id: close_button

          anchors.top: title_area.top
          anchors.right: title_area.right
          anchors.bottom: title_area.bottom

          width: height

          hoverEnabled: true

          background: CusRoundedBg {
            anchors.fill: parent
            anchors.topMargin: root.borderWidth
            anchors.rightMargin: root.borderWidth
            radius: 5 * screenScaleFactor
            rightTop: true
            color: close_button.hovered ? "#ff365c" : "transparent"
          }

          Image {
            anchors.centerIn: parent
            width: 10 * screenScaleFactor
            height: 10 * screenScaleFactor
            source: close_button.hovered ? "qrc:/UI/photo/closeBtn_d.svg" : "qrc:/UI/photo/closeBtn.svg"
          }

          onClicked: {
            closeButtonClicked()
            if (root.autoClose) {
              root.visible = false
            }
          }
        }
      }
    }

    Rectangle {
      id: split_line

      anchors.top: title.bottom
      anchors.left: background.left
      anchors.right: background.right
      height: root.borderWidth

      color: adsorbed ? root.panelColor : root.borderColor
    }

    Rectangle {
      id: panel

      anchors.top: split_line.bottom
      anchors.left: background.left
      anchors.right: background.right
      anchors.bottom: background.bottom
      anchors.leftMargin: root.borderWidth
      anchors.rightMargin: root.borderWidth
      anchors.bottomMargin: background.radius

      color: root.panelColor
    }

    Rectangle {
      id: panel_bottom

      anchors.left: background.left
      anchors.right: background.right
      anchors.bottom: background.bottom
      anchors.leftMargin: root.borderWidth
      anchors.rightMargin: root.borderWidth
      anchors.bottomMargin: root.borderWidth

      height: background.radius * 2

      radius: background.radius
      color: root.panelColor
    }
  }
}
