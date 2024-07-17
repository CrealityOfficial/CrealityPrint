import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15

import DockController 1.0

import "../qml"

/// @brief Dock 组件上下文
///        将 Dock 组件的 context 属性设置为该组件, 即绑定该组件与该上下文.
///        同一上下文中的组件将在吸附时层叠.
///        注意 : 同一上下文中的组件对象 title 属性需要唯一.
/// @see DockItem.qml
/// @see DockController.h
/// @see DockController.cpp

Window {
  // ---------- interface [beg] ----------

  // dock 组件吸附后 tabbar 的坐标尺寸及相关属性 (当没有组件吸附时 tab bar 自动隐藏)
  property int tabBarX: 0 * screenScaleFactor
  property int tabBarY: 0 * screenScaleFactor
  property int tabBarWidth: 1280 * screenScaleFactor
  property int tabBarHeight: 30 * screenScaleFactor
  property color tabBarColor: Constants.dock_context_tab_bar_color
  property int tabBarBorderWidth: 0 * screenScaleFactor
  property color tabBarBorderColor: Constants.dock_border_color

  // dock 组件吸附后的 tabbar button 相关属性
  property int tabButtonWidth: 177 * screenScaleFactor
  property color tabButtonDefaultColor: Constants.dock_context_tab_button_default_color
  property color tabButtonCheckedColor: Constants.dock_context_tab_button_checked_color
  property color tabButtonTextDefaultColor: Constants.dock_context_tab_button_text_default_color
  property color tabButtonTextCheckedColor: Constants.dock_context_tab_button_text_checked_color
  property color tabButtonBorderColor: Constants.dock_border_color

  // dock 组件吸附后的坐标尺寸
  property int adsorbedItemX: tabBarX
  property int adsorbedItemY: tabBarY + tabBarHeight
  property int adsorbedItemWidth: tabBarWidth
  property int adsorbedItemHeight: 720 * screenScaleFactor - tabBarHeight

  function closeAllItem() {
    controller.closeAllItem()
  }

  // ---------- interface [end] ----------

  id: root
  flags: Qt.FramelessWindowHint | Qt.Dialog

  x: tabBarX
  y: tabBarY
  width: tabBarWidth
  height: tabBarHeight

  color: "transparent"

  property alias controller: controller
  visible: controller.contextVisible

  DockController {
    id: controller
    context: root
  }

  function append(item) {
    controller.appendItem(item)
  }

  function remove(item) {
    controller.removeItem(item)
  }

  TabBar {
    id: tab_bar

    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right

    height: root.tabBarHeight

    background: Rectangle {
      anchors.fill: parent
      color: root.tabBarColor
      border.width: root.tabBarBorderWidth
      border.color: root.tabBarBorderColor
    }

    Repeater {
      model: controller.contextTitleList

      TabButton {
        id: tab_button

        readonly property string title: model.display
        readonly property int backgroundRadius: 5 * screenScaleFactor

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 1 * screenScaleFactor

        width: root.tabButtonWidth


        background: Rectangle {
          anchors.fill: parent
          radius: tab_button.backgroundRadius
          color: tab_button.checked ? root.tabButtonCheckedColor
                                    : root.tabButtonDefaultColor
          border.width: 1 * screenScaleFactor
          border.color: root.tabButtonBorderColor
        }

        Text {
          id: button_text

          anchors.verticalCenter: tab_button.verticalCenter
          anchors.left: tab_button.left
          anchors.leftMargin: 10 * screenScaleFactor

          text: tab_button.title
          color: tab_button.checked ? root.tabButtonTextCheckedColor
                                    : root.tabButtonTextDefaultColor
          font.family: Constants.labelFontFamily
          font.weight: Constants.labelFontWeight
          font.pointSize: Constants.labelFontPointSize_9
        }

        Button {
          id: unadsorb_button

          anchors.top: tab_button.top
          anchors.right: close_button.left
          anchors.bottom: tab_button.bottom
          anchors.margins: tab_button.anchors.margins

          width: height

          hoverEnabled: true

          background: Rectangle {
            anchors.fill: parent
            color: unadsorb_button.hovered ? Constants.hoveredColor : "transparent"
          }

          Image {
            anchors.centerIn: parent
            width: 10 * screenScaleFactor
            height: 10 * screenScaleFactor
            source: unadsorb_button.hovered ? "qrc:/UI/photo/normalBtn_d.svg"
                                            : "qrc:/UI/photo/normalBtn.svg"
          }

          onClicked: {
            var item = controller.findItemByTitle(tab_button.title)
            if (!item) { return }
            item.adsorbed = false
          }
        }

        Button {
          id: close_button

          anchors.top: tab_button.top
          anchors.right: tab_button.right
          anchors.bottom: tab_button.bottom
          anchors.margins: tab_button.anchors.margins

          width: height

          hoverEnabled: true

          background: CusRoundedBg {
            anchors.fill: parent
            rightTop: true
            rightBottom: true
            radius: tab_button.backgroundRadius
            color: close_button.hovered ? Constants.left_model_list_close_button_hovered_color : "transparent"
          }

          Image {
            anchors.centerIn: parent
            width: 10 * screenScaleFactor
            height: 10 * screenScaleFactor
            source: close_button.hovered ? "qrc:/UI/photo/closeBtn_d.svg"
                                         : "qrc:/UI/photo/closeBtn.svg"
          }

          onClicked: {
            var item = controller.findItemByTitle(tab_button.title)
            if (!item) { return }

            item.closeButtonClicked()
            if (item.autoClose) {
              item.visible = false
            }
          }
        }

        onClicked: {
          if (tab_bar.currentItem == this) {
            tab_bar.currentItemChanged()
          }
        }
      }
    }

    onCurrentItemChanged: {
      if (!currentItem) { return }
      var item = controller.findItemByTitle(currentItem.title)
      if (item) { controller.focusItem(item) }
    }
  }
}
