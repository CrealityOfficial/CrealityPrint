import QtQuick 2.15

import QtQml 2.15

// @brief 自定义 TabView 组件子项
// 与 CustomTabView 结合使用 @see CustomTabView.qml
//
// 示例:
//   @see CustomTabView.qml
//
// 注意:
//   1. 只接受 CustomTabView 作为 parent;
//   2. 以下属性受所属 CusTabView 控制:
//     1) x
//     2) y
//     3) width
//     4) height
//     5) visible
//   3. 以下属性 CustomTabView 提供默认值:
//     1) color
Rectangle {
  // -------------------- interface [begin] --------------------

  // 对应的标题
  property string title: ""

  // 启用选项 (未启用时隐藏对应标题栏按钮)
  enabled: true

  // -------------------- interface [end] --------------------

  id: root

  property var _button: null

  color: "transparent"

  Component.onCompleted: function() {
    parent._initItem(this)
  }
}
