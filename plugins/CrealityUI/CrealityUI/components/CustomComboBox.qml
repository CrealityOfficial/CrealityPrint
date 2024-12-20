import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/CrealityUI/qml"

/// @brief 自定义样式的 ComboBox 兼容原生 ComboBox
ComboBox {
  // ---------- interface [begin] ----------

  // 默认 popup 中的 ListView.
  property alias view: list_view

  // 翻译函数, 将用于默认 displayText 与默认 delegate 内部的 Text.text 中进行翻译.
  // source 为待翻译的原文. 翻译时将逐一检查以下表达式, 将首个成立的表达式取值作为 source 传参给 translater:
  //   modelData
  //   model
  //   modelData[textRole]
  //   model[textRole]
  // 示例: CustomComboBox {
  //   textRole: 'my_text_role'
  //   translater: source => qsTranslate('my context', source)
  // }
  property var translater: source => source

  // view 展示 delegate 的最大数量(不包括 InlineLabel ssection), < 0 则不做限制.
  // 该属性用于自动调整 popup 的高度, view 的 header 与 footer 高度将计算在内.
  property int maximumVisibleCount: -1

  // 统一的圆角半径, 将作用于默认 contextItem, 默认 popup 与默认 delegate 的 background 中.
  property real radius: 5 * screenScaleFactor

  // delegate 的相关属性, 将作用于默认 delegate 中.
  property real implicitDelegateWidth: root.popup.width - root.popup.padding * 2
  property real implicitDelegateHeight: root.height

  // Text 的相关属性, 将作用于默认 contentItem 与默认 delegate 内部的 Text 中.
  property real textTopPadding: 0
  property real textBottomPadding: 0
  property real textLeftPadding: 5 * screenScaleFactor
  property real textRightPadding: 5 * screenScaleFactor
  property int textVerticalAlignment: Qt.AlignVCenter
  property int textHorizontalAlignment: Qt.AlignLeft
  property int textElide: Text.ElideRight
  property var textFont: Constants.font
  property color textColor: Constants.right_panel_text_default_color

  // ---------- interface [end] ----------

  id: root

  displayText: translater ? translater(currentText) : currentText

  background: Rectangle {
    radius: root.radius
    border.width: 1 * screenScaleFactor
    border.color: hovered ? Constants.right_panel_border_hovered_color
                          : Constants.right_panel_border_default_color
    color: 'transparent'
  }

  indicator: Canvas {
    property color defaultColor: Constants.currentTheme ? '#333333' : '#C9C9C9'
    property color hoveredColor: Constants.right_panel_border_hovered_color
    property color checkedColor: Constants.right_panel_border_checked_color

    anchors.right: root.background.right
    anchors.rightMargin: 10 * screenScaleFactor
    anchors.verticalCenter: root.verticalCenter
    width: 7 * screenScaleFactor
    height: 5 * screenScaleFactor

    contextType: '2d'

    onPaint: {
      const context = getContext('2d')
      context.clearRect(0, 0, width, height)
      context.beginPath()
      if (!root.down) {
        context.moveTo(0, 0)
        context.lineTo(width, 0)
        context.lineTo(width / 2, height)
      } else {
        context.moveTo(0, height)
        context.lineTo(width, height)
        context.lineTo(width / 2, 0)
      }
      context.closePath()
      context.fillStyle = root.down ? checkedColor
                                    : root.hovered ? hoveredColor
                                                   : defaultColor
      context.fill()
    }

    Connections {
      target: root

      function onDownChanged() {
        root.indicator.requestPaint()
      }

      function onHoveredChanged() {
        root.indicator.requestPaint()
      }
    }

    Connections {
      target: Constants

      function onCurrentThemeChanged() {
        root.indicator.requestPaint()
      }
    }
  }

  contentItem: RowLayout {
    anchors.top: root.background.top
    anchors.bottom: root.background.bottom
    anchors.left: root.background.left
    anchors.right: root.indicator.left

    Text {
      Layout.fillWidth: true
      Layout.fillHeight: true

      topPadding: root.textTopPadding
      bottomPadding: root.textBottomPadding
      leftPadding: root.textLeftPadding
      rightPadding: root.textRightPadding
      verticalAlignment: root.textVerticalAlignment
      horizontalAlignment: root.textHorizontalAlignment

      elide: root.textElide
      color: root.textColor
      font: root.textFont
      text: root.displayText
    }
  }

  popup: Popup {
    y: root.height + 5 * screenScaleFactor
    width: root.width
    height: {
      let total_height = implicitContentHeight

      if (root.maximumVisibleCount >= 0) {
        let currnet_count = root.maximumVisibleCount

        if (!root.model) {
          currnet_count = 0
        }

        if (typeof root.model.count === 'number') {
          currnet_count = root.model.count
        }

        const visible_count = Math.min(currnet_count, root.maximumVisibleCount)

        total_height = root.implicitDelegateHeight * visible_count

        if (list_view.headerItem && list_view.headerPositioning === ListView.OverlayHeader) {
          total_height += list_view.headerItem.height + padding
        }

        if (list_view.footerItem && list_view.footerPositioning === ListView.OverlayFooter) {
          total_height += list_view.footerItem.height + padding
        }
      }

      return total_height + padding * 2
    }

    padding: background.border.width

    background: Rectangle {
      radius: root.radius
      border.width: 1 * screenScaleFactor
      border.color: Constants.right_panel_border_default_color
      color: Constants.right_panel_item_default_color
    }

    contentItem: ListView {
      id: list_view

      implicitHeight: contentHeight

      ScrollBar.vertical: ScrollBar {
        policy: list_view.contentHeight > list_view.height ? ScrollBar.AlwaysOn
                                                           : ScrollBar.AlwaysOff
      }

      ScrollBar.horizontal: ScrollBar {
        policy: ScrollBar.AlwaysOff
      }

      clip: true
      model: root.delegateModel
    }
  }

  delegate: ItemDelegate {
    id: item_delegate

    enabled: !ListView.view.ScrollBar.vertical.active

    implicitWidth: root.implicitDelegateWidth
    implicitHeight: root.implicitDelegateHeight

    background: Rectangle {
      radius: root.radius
      color: parent.hovered ? Constants.right_panel_item_checked_color : 'transparent'
    }

    contentItem: RowLayout {
      anchors.fill: parent

      Text {
        readonly property string source: {
          if (typeof modelData === 'string') {
            return modelData
          } else if (typeof model === 'string') {
            return model
          }

          if (root.textRole.length !== 0) {
            if (typeof modelData !== 'undefined' &&
                typeof modelData !== 'null' &&
                root.textRole in modelData) {
              return modelData[root.textRole]
            } else if (typeof model !== 'undefined' &&
                       typeof model !== 'null' &&
                       root.textRole in model) {
              return model[root.textRole]
            }
          }

          return ''
        }

        Layout.fillWidth: true
        Layout.fillHeight: true

        topPadding: root.textTopPadding
        bottomPadding: root.textBottomPadding
        leftPadding: root.textLeftPadding
        rightPadding: root.textRightPadding
        verticalAlignment: root.textVerticalAlignment
        horizontalAlignment: root.textHorizontalAlignment

        font: root.textFont
        elide: root.textElide
        color: item_delegate.hovered ? '#FFFFFF' : root.textColor
        text: root.translater ? root.translater(source) : source
      }
    }
  }
}
