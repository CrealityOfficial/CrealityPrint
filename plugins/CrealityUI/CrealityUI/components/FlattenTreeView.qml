import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import QtQml 2.15

import ".."
import "../qml"

// @brief 扁平树形组件, 配合 qtuser_qml::FlattenTreeModel 使用
CustomTreeView {
  // ---------- interface [begin] ----------

  // @brief 通过 model 对象找到对应的 delegate 对象并定位到对象对应的位置, 如果找不到则返回 undefined
  // @param model_node model 对象
  // @param position_mode 定位方式枚举, ListView.PositionMode
  function positionDelegate(model_node, position_mode) {
    // 遍历 model_node 到根 root_node 之间的所有节点 (sub_type_node)
    // 展开其中对应组件被折叠的项
    for (let sub_type_node = model_node.parentNode;
         sub_type_node !== treeModel;
         sub_type_node = sub_type_node.parentNode) {
      sub_type_node.childNodeValid = true
    }

    // 定位到 model_node 对应组件处
    const delegate = findDelegate(model_node)
    if (delegate) {
      positionViewAtIndex(delegate.index, position_mode)
    }

    return delegate
  }

  // ---------- interface [end] ----------

  id: root

  model: root.treeModel

  delegate: Item {
    readonly property real spacing: model.index === 0 ? 0 : root.nodeSpacing

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

    width: {
      const fill_width = root.width - x
      const implicit_width = root.nodeFillWidth ? fill_width : item.implicitWidth
      return implicit_width - root.rightMargin
    }

    height: {
      if (!visible || !loader.item || !loader.item.visible) {
        0
      } else if (loader.nodeHeight >= 0.0) {
        loader.nodeHeight + spacing
      } else if (model_node.isLeafNode) {
        (root.leafNodeHeight >= 0.0 ? root.leafNodeHeight : loader.item.implicitHeight) + spacing
      } else {
        (root.forkNodeHeight >= 0.0 ? root.forkNodeHeight : loader.item.implicitHeight) + spacing
      }
    }

    Loader {
      id: loader

      readonly property var nodeModel: model_node
      property real nodeHeight: -1.0

      anchors.fill: parent
      anchors.topMargin: parent.spacing
      anchors.leftMargin: model_level * root.nodeLeftMargin

      clip: true
      asynchronous: root.asynchronous
      sourceComponent: nodeModel.isLeafNode ? root.leafNodeDelegate : root.forkNodeDelegate

      onItemChanged: _ => {
        if (item) {
          root._modelDelegateMap.set(nodeModel, {
            index: model.index,
            object: item,
          })
        } else {
          root._modelDelegateMap.delete(nodeModel)
        }
      }
    }
  }
}
