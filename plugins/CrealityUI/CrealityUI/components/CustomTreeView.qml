import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import QtQml 2.15

import "../qml"

// @brief 树形组件, 配合 qtuser_qml::CustomTreeModel 使用
ListView {
  // ---------- interface [begin] ----------

  // 是否异步加载节点
  property bool asynchronous: false

  // 节点相关属性
  // 节点是否填充宽度, 若为 true 则所有节点宽度向 View 对齐
  property bool nodeFillWidth: false
  // 节点高度, 若小于 0 则每个节点高度由自身 implicitHeight 决定
  property real forkNodeHeight: 20 * screenScaleFactor
  property real leafNodeHeight: forkNodeHeight
  // 每一级节点左侧的偏移量 (只在父节点不为 root 时生效)
  property real nodeLeftMargin: 20 * screenScaleFactor
  // 节点之间的间距
  property real nodeSpacing: 5 * screenScaleFactor

  // 树形 Qt Model (qtuser_qml::CustomTreeModel)
  property var treeModel: null

  // 树杈节点的代理组件, 提供以下属性:
  // readonly property var nodeModel: 由 Qt Model 传入的节点数据对象
  // property bool nodeModel.nodeValid: 用于控制当前节点合法性. @see 节点合法性规则
  // property bool nodeModel.childNodeValid: 用于控制当前节点的子节点合法性. @see 节点合法性规则
  // property real nodeHeight: 用于控制当前节点高度, 若值 < 0.0 则使用 forkNodeHeight
  property Component forkNodeDelegate: Button {
    checkable: true
    checked: true

    Component.onCompleted: {
      childNodeVisible = Qt.binding(() => checked)
    }

    background: Item {}

    contentItem: RowLayout {
      spacing: 5 * screenScaleFactor

      Text {
        Layout.fillHeight: true
        width: height

        horizontalAlignment: Qt.AlignLeft
        verticalAlignment: Qt.AlignVCenter

        font.pointSize: 16
        font.family: Constants.font.family
        color: Constants.right_panel_text_default_color
        text: parent.parent.checked ? "-" : "+"
      }

      Text {
        Layout.fillHeight: true
        Layout.minimumWidth: contentWidth

        horizontalAlignment: Qt.AlignLeft
        verticalAlignment: Qt.AlignVCenter

        font: Constants.font
        color: Constants.right_panel_text_default_color
        text: nodeModel.uid
      }
    }
  }

  // 叶子节点的代理组件, 提供以下属性:
  // readonly property var nodeModel: 由 Qt Model 传入的节点数据对象
  // property bool nodeModel.nodeValid: 用于控制当前节点合法性. @see 节点合法性规则
  // property real nodeHeight: 用于控制当前节点高度, 若值 < 0.0 则使用 leafNodeHeight
  property Component leafNodeDelegate: Text {
    horizontalAlignment: Qt.AlignLeft
    verticalAlignment: Qt.AlignVCenter

    font: Constants.font
    color: Constants.right_panel_text_default_color
    text: nodeModel.uid
  }

  // 节点合法性规则:
  // 1. node delegate 中, 可用通过属性 nodeValid 与 childNodeValid 控制当前节点与子节点的合法性.
  //    View 将通过合法性间接控制节点显示与否.
  // 2. 对于 leaf node delegate, 以下任意条件成立时将被隐藏:
  //   a. [root_node, current_node] 之间存在任意 nodeModel.nodeValid === false;
  //   b. [root_node, current_node.parent_node] 之间存在任意 nodeModel.childNodeValid === false;
  // 3. 对于 fowk node delegate, 以下任意条件成立时将被隐藏:
  //   a. [root_node, current_node] 之间存在任意 nodeModel.nodeValid === false;
  //   b. [root_node, current_node.parent_node] 之间存在任意 nodeModel.childNodeValid === false;
  //   b. current_node.hasValidChildNode === true;

  // @brief 通过 model 对象找到对应的 delegate 对象, 如果找不到则返回 undefined
  // @param model_node model 对象
  function findDelegate(model_node) {
    return _modelDelegateMap.get(model_node)
  }

  // ---------- interface [end] ----------

  id: root

  readonly property var _modelDelegateMap: new Map()

  implicitHeight: contentHeight

  clip: true
  spacing: 0
  cacheBuffer: (model ? model.count : 0) * 1000 * screenScaleFactor

  model: root.treeModel.childNodes
  delegate: node_component

  Component {
    id: node_component

    ListView {
      readonly property var nodeView: this
      readonly property var nodeModel: model_node
      readonly property var nodeItem: headerItem

      readonly property var parentNodeView: ListView.view
      readonly property var parentNodeModel: parentNodeView.nodeModel
      readonly property var parentNodeItem: parentNodeView.headerItem

      x: parentNodeView === root ? 0 : root.nodeLeftMargin
      width: parentNodeView.width - x
      height: contentHeight

      visible: {
        if (!model_node.nodeValid) {
          return false
        }

        if (model_node.hasUnvalidParentNode || model_node.hasChildUnvalidParentNode) {
          return false
        }

        if (model_node.isForkNode && !model_node.hasValidChildNode) {
          return false
        }

        return true
      }

      clip: true
      spacing: 0
      interactive: false
      cacheBuffer: (model ? model.count : 0) * 1000 * screenScaleFactor

      model: model_node.childNodes
      delegate: node_component

      header: Loader {
        width: root.nodeFillWidth ? nodeView.width : undefined
        height: {
          if (!visible || !item || !item.visible) {
            0
          } else if (nodeHeight >= 0.0) {
            nodeHeight
          } else if (model_node.isLeafNode) {
            root.leafNodeHeight >= 0.0 ? root.leafNodeHeight : item.implicitHeight
          } else {
            root.forkNodeHeight >= 0.0 ? root.forkNodeHeight : item.implicitHeight
          }
        }

        clip: true
        asynchronous: root.asynchronous
        sourceComponent: nodeModel.isLeafNode ? root.leafNodeDelegate : root.forkNodeDelegate

        readonly property var nodeModel: nodeView.nodeModel
        property real nodeHeight: -1.0

        onItemChanged: _ => {
          if (item) {
            root._modelDelegateMap.set(nodeModel, item)
          } else {
            root._modelDelegateMap.delete(nodeModel)
          }
        }
      }
    }
  }
}
