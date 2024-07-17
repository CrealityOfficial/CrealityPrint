import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import QtQml 2.15

import "../qml"

/// @brief 使用 GridLayout 对标题按钮进行布局的 TabView
///        以下属性需要手动控制: GridLayout 行列数, 按钮坐标, 按钮权重
Item {
  // ---------- interface [begin] ----------

  // 布局
  property int layoutRowSize: 1
  property int layoutColumnSize: 1
  property double layoutRowSpacing: 0 * screenScaleFactor
  property double layoutColumnSpacing: 0 * screenScaleFactor

  // 标题
  property double titleTopMargin: 0  // 首行按钮未选中时顶部向下收缩的高度
  property double titleBottomMargin: 0  // 末行按钮未选中时底部向上收缩的高度

  // 标题-按钮
  property double buttonHeight: 36 * screenScaleFactor
  property double buttonSpacing: 4 * screenScaleFactor
  property string buttonColor: Constants.right_panel_button_default_color
  property string buttonHoveredColor: Constants.right_panel_button_hovered_color
  property string buttonCheckedColor: Constants.right_panel_button_checked_color

  // 标题-文本
  property string textColor: Constants.right_panel_text_default_color
  property string textHoveredColor: Constants.right_panel_text_hovered_color
  property string textCheckedColor: Constants.right_panel_text_checked_color
  property var textFont: Constants.font
  property var textTranslater: _ => _

  // 标题-图片
  property double imageWidth: 14 * screenScaleFactor
  property double imageHeight: imageWidth

  // 面板
  property string panelColor: Constants.right_panel_item_default_color
  property double panelPadding: 12 * screenScaleFactor

  // 边框 (同时作用于标题栏按钮与面板)
  property double borderRadius: 5 * screenScaleFactor
  property double borderWidth: 1
  property string borderColor: Constants.right_panel_border_default_color

  // QtModel (同时作用于标题栏按钮与面板)
  property var itemModel: ListModel {
    ListElement {
      // 以下 role 将作用于 GridTabView 内部
      ui_button_text: "Tab"        // [string / QString] 标题栏按钮的文本 (不携带该 role 或值为空则不显示文本)
      ui_button_image: ""          // [string / QString] 标题栏按钮的图片 (不携带该 role 或值为空则不显示图片)
      ui_button_hovered_image: ""  // [string / QString] 标题栏按钮 hovered 状态图片 (不携带该 role 则等于 ui_button_image)
      ui_button_checked_image: ""  // [string / QString] 标题栏按钮 checked 状态图片 (不携带该 role 则等于 ui_button_image)
      ui_button_row: 0             // [number / unsigned int] 标题栏按钮的坐标-行 (不携带该 role 则默认位于 row 0)
      ui_button_column: 0          // [number / unsigned int] 标题栏按钮的坐标-列 (不携带该 role 则默认位于 column 0)
      ui_button_column_span: 1     // [number / unsigned int] 标题栏按钮的权重-列 (不携带该 role 则默认权重为 1)

      // 其余 role 将传递给 itemDelegate.model
      // other_custom_model: null
    }
  }

  // 面板内部的代理组件
  // 关于懒加载每页 delegate 的方式:
  //   设置 itemDelegate 为 Loader 组件, 并初始化其 active 属性为 false.
  //   当该 delegate 首次被激活时, GridTabView 将自动尝试将其 active 属性设置为 true.
  property Component itemDelegate: Item {}

  // 当前选中的 Tab 索引, 与 model 索引和 delegate 索引一致
  property int currentIndex: 0

  // 通过索引找到 delegate 组件
  function findDelegate(index) {
      return delegate_repeater.itemAt(index)
  }

  // ---------- interface [end] ----------

  id: root

  onCurrentIndexChanged: function() {
    if (currentIndex !== button_group.checkedButton.buttonIndex) {
      tab_button_repeater.itemAt(currentIndex).checked = true
    }
  }

  Connections {
    target: button_group

    function onCheckedButtonChanged() {
      if (root.currentIndex !== button_group.checkedButton.buttonIndex) {
        root.currentIndex = button_group.checkedButton.buttonIndex
      }
    }
  }

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    ButtonGroup {
      id: button_group
    }

    GridLayout {
      rows: root.layoutRowSize
      columns: root.layoutColumnSize

      rowSpacing: root.layoutRowSpacing
      columnSpacing: root.layoutColumnSpacing

      Repeater {
        id: tab_button_repeater

        model: root.itemModel

        delegate: Button {
          id: button

          readonly property int buttonIndex: index

          readonly property string uiButtonText: typeof ui_button_text == "string" ?
            root.textTranslater ? root.textTranslater(ui_button_text) : ui_button_text : ""

          readonly property string uiButtonImage:
            typeof ui_button_image == "string" ? ui_button_image : ""

          readonly property string uiButtonHoveredImage:
            typeof ui_button_hovered_image == "string" ? ui_button_hovered_image : uiButtonImage

          readonly property string uiButtonCheckedImage:
            typeof ui_button_checked_image == "string" ? ui_button_checked_image : uiButtonImage

          readonly property int uiButtonRow: typeof ui_button_row == "number" ? ui_button_row : 0

          readonly property int uiButtonColumn:
            typeof ui_button_column == "number" ? ui_button_column : 0

          readonly property int uiButtonColumnSpan:
            typeof ui_button_column_span == "number" ? ui_button_column_span : 1

          readonly property bool atTopRow: uiButtonRow === 0
          readonly property bool atBottomRow: uiButtonRow === root.layoutRowSize - 1
          readonly property bool atTopColumn: uiButtonColumn === 0
          readonly property bool atBottomColumn: uiButtonColumn === root.layoutColumnSize - 1

          clip: true

          Layout.fillWidth: true
          Layout.minimumHeight: root.buttonHeight
          Layout.maximumHeight: Layout.minimumHeight
          Layout.row: uiButtonRow
          Layout.column: uiButtonColumn
          Layout.columnSpan: uiButtonColumnSpan
          Layout.alignment: atBottomRow ? Qt.AlignBottom : Qt.AlignTop

          checkable: true
          checked: buttonIndex === 0
          ButtonGroup.group: button_group

          onCheckedChanged: function() {
            button_canvas.requestPaint()
          }

          onHoveredChanged: function() {
            button_canvas.requestPaint()
          }

          Connections {
            target: Constants

            function onCurrentThemeChanged() {
              button_canvas.requestPaint()
            }
          }

          background: Canvas {
            id: button_canvas

            readonly property var pathDrawer: QtObject {
              function drawNormalButton(context) {
                context.rect(0, 0, width, height)
              }

              function drawTopRowButton(context) {
                const x = button.checked ? 0 : root.titleTopMargin

                // 左下角处
                context.moveTo(0, height)

                // 左侧边缘
                context.lineTo(0, root.borderRadius + x)

                // 左上角
                context.arcTo(0, x,
                              root.borderRadius, x,
                              root.borderRadius, root.borderRadius)

                // 顶部边缘
                context.lineTo(width - root.borderRadius, x)

                // 右上角
                context.arcTo(width, x,
                              width, root.borderRadius,
                              root.borderRadius, root.borderRadius)

                // 右侧边缘
                context.lineTo(width, height)

                // 底部边缘
                context.lineTo(0, height)
              }

              function drawBottomRowButton(context) {
                const height = button.checked ? button.height
                                              : button.height - root.titleBottomMargin

                // 左上角处
                context.moveTo(0, 0)

                // 左侧边缘
                context.lineTo(0, height - root.borderRadius)

                // 左下角
                context.arcTo(0, height,
                              root.borderRadius, height,
                              root.borderRadius, root.borderRadius)

                // 底部边缘
                context.lineTo(width - root.borderRadius, height)

                // 右下角
                context.arcTo(width, height,
                              width, height - root.borderRadius,
                              root.borderRadius, root.borderRadius)

                // 右侧边缘
                context.lineTo(width, 0)

                // 顶部边缘
                context.lineTo(0, 0)
              }

              function drawLeftButtomButton(context) {
                drawBottomRowButton(context)
                context.moveTo(0, height - root.borderRadius)
                context.lineTo(0, height)
              }

              function drawRightButtomButton(context) {
                drawBottomRowButton(context)
                context.moveTo(width, height - root.borderRadius)
                context.lineTo(width, height)
              }
            }

            antialiasing: true

            onPaint: {
              const context = getContext("2d")

              if (button.atBottomRow) {
                context.fillStyle = root.panelColor
                context.fillRect(0, 0, width, height)
              } else {
                context.clearRect(0, 0, width, height)
              }

              context.lineWidth = root.borderWidth
              context.strokeStyle = root.borderColor
              context.fillStyle = button.checked ? root.buttonCheckedColor
                                                 : button.hovered ? root.buttonHoveredColor
                                                                  : root.buttonColor

              context.beginPath()

              if (button.atTopRow) {
                pathDrawer.drawTopRowButton(context)
              } else if (button.atBottomRow) {
                if (button.atTopColumn) {
                  pathDrawer.drawLeftButtomButton(context)
                } else if (button.atBottomColumn) {
                  pathDrawer.drawRightButtomButton(context)
                } else {
                  pathDrawer.drawBottomRowButton(context)
                }
              } else {
                pathDrawer.drawNormalButton(context)
              }

              context.closePath()
              context.fill()
              context.stroke()
            }
          }

          Row {
            anchors.centerIn: parent
            anchors.topMargin: !button.atTopRow ? 0 : button.checked ? 0 : root.titleTopMargin / 2
            anchors.bottomMargin: !button.atBottomRow ? 0
                                                      : button.checked ? 0
                                                                       : root.titleBottomMargin / 2

            spacing: root.buttonSpacing

            Image {
              width: visible ? root.imageWidth : 0
              height: visible ? root.imageHeight : 0

              anchors.verticalCenter: parent.verticalCenter

              fillMode: Image.PreserveAspectFit
              source: button.checked ? button.uiButtonCheckedImage
                                     : button.hovered ? button.uiButtonHoveredImage
                                                      : button.uiButtonImage
              visible: source !== ""
            }

            Text {
              anchors.verticalCenter: parent.verticalCenter

              horizontalAlignment: Qt.AlignHCenter
              verticalAlignment: Qt.AlignVCenter

              color: button.checked ? root.textCheckedColor
                                    : button.hovered ? root.textHoveredColor : root.textColor
              font: root.textFont
              text: button.uiButtonText
            }
          }
        }
      }
    }

    Canvas {
      id: panel_canvas

      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.alignment: Qt.AlignBottom

      antialiasing: true

      Connections {
        target: Constants

        function onCurrentThemeChanged() {
          panel_canvas.requestPaint()
        }
      }

      onPaint: {
        const context = getContext("2d")

        context.lineWidth = root.borderWidth
        context.strokeStyle = root.borderColor
        context.fillStyle = root.panelColor

        context.clearRect(0, 0, width, height)
        context.beginPath()

        // 右上角处
        context.moveTo(width, 0)

        // 右侧边缘
        context.lineTo(width, height - root.borderRadius)

        // 右下角
        context.arcTo(width, height,
                      width - root.borderRadius, height,
                      root.borderRadius, root.borderRadius)

        // 底部边缘
        context.lineTo(root.borderRadius, height)

        // 左下角
        context.arcTo(0, height,
                      0, height - root.borderRadius,
                      root.borderRadius, root.borderRadius)

        // 左侧边缘
        context.lineTo(0, 0)

        context.fill()
        context.stroke()
      }

      StackLayout {
        anchors.fill: parent
        anchors.bottomMargin: 40 * screenScaleFactor
        anchors.margins: root.panelPadding

        currentIndex: button_group.checkedButton ? button_group.checkedButton.buttonIndex : 0

        Repeater {
          id: delegate_repeater

          model: root.itemModel
          delegate: root.itemDelegate

          readonly property int currentIndex: parent.currentIndex

          onCurrentIndexChanged: function() {
            tryActivateItem(itemAt(currentIndex))
          }

          onItemAdded: function(index, item) {
            if (index === 0) {
              tryActivateItem(item)
            }
          }

          function tryActivateItem(item) {
            if (typeof item.active == "boolean" && !item.active) {
              item.active = true
            }
          }
        }
      }
    }
  }
}
