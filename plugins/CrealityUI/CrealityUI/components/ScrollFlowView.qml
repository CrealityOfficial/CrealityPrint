import QtQuick 2.15
import QtQuick.Controls 2.15

import QtQml 2.15

/// @brief ScrollView 与 Flow 的结合体
///        delegate 组件按从左至右, 从上到下的顺序依次放置在 view 中.
///        当宽度不足时, delegate 组件自动放置到下一行;
///        当高度不足时, 可使用垂直滚动条翻页.
/// @example cxcloud/UserInfoDialog.qml

ScrollView {
  // ---------- interface [beg] ----------

  property alias itemModel: repeater.model
  property alias itemDelegate: repeater.delegate

  // visible 变化后的行为
  property bool autoLoadWhenShow: true
  property bool autoClearWhenHide: true

  readonly property int currentPage: flow.currentPage

  signal requestLoadItem(int page_index, int page_size)
  signal requestClearItem()

  function resetToFirstPage() {
    flow.clearAll()
    flow.loadNextPage()
  }

  // 直接访问内部的 ScrollView 与 Flow
  property alias scrollView: root
  property alias flowView: flow

  // ---------- interface [end] ----------

  id: root

  ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
  ScrollBar.vertical.policy: (contentHeight > height) ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
  spacing: 0
  clip: true

  Flow {
    id: flow
    width: root.availableWidth
    spacing: 20 * screenScaleFactor

    property int currentPage: 0

    function loadNextPage() {
      currentPage++
      root.requestLoadItem(currentPage, 24)
    }

    function clearAll() {
      root.ScrollBar.vertical.position = 0.0
      currentPage = 0
      root.requestClearItem()
    }

    onVisibleChanged: function() {
      if (!visible && root.autoClearWhenHide) {
        clearAll()
        return
      }

      if (root.autoLoadWhenShow) {
        loadNextPage()
      }
    }

    Connections {
      target: root.ScrollBar.vertical
      function onPositionChanged() {
        if (!target.visible) { return }
        if (target.position + target.size !== 1.0) { return }
        flow.loadNextPage()
      }
    }

    Repeater {
      id: repeater
    }
  }
}
