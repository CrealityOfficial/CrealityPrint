#include "qtusercore/plugin/customtreemodel.h"

namespace qtuser_qml {

  CustomTreeModel::CustomTreeModel()
      : CustomTreeModel{ std::make_shared<CustomTreeModel>(nullptr) } {}

  CustomTreeModel::CustomTreeModel(std::shared_ptr<CustomTreeModel> parent_node)
      : QAbstractListModel{ nullptr }, parent_node_{ parent_node } {}

  CustomTreeModel::~CustomTreeModel() {}

  auto CustomTreeModel::getUid() const -> QString {
    return uid_;
  }

  auto CustomTreeModel::setUid(const QString& uid) -> void {
    if (uid_ == uid) {
      return;
    }

    uid_ = uid;
    Q_EMIT uidChanged();
  }

  auto CustomTreeModel::isRootNode() const -> bool {
    return parent_node_.expired();
  }

  auto CustomTreeModel::isForkNode() const -> bool {
    return !isLeafNode();
  }

  auto CustomTreeModel::isLeafNode() const -> bool {
    return child_nodes_.empty();
  }

  auto CustomTreeModel::parentNode() const -> std::shared_ptr<CustomTreeModel> {
    return parent_node_.lock();
  }

  auto CustomTreeModel::setParentNode(std::weak_ptr<CustomTreeModel> parent_node) -> void {
    if (parent_node_.lock() == parent_node.lock()) {
      return;
    }

    const bool root_node_changed = (isRootNode() && !parent_node.expired()) ||
                                   (!isRootNode() && parent_node.expired());

    parent_node_ = std::move(parent_node);
    Q_EMIT parentNodeChanged();

    if (root_node_changed) {
      Q_EMIT isRootNodeChanged();
    }
  }

  auto CustomTreeModel::getParentNode() const -> std::weak_ptr<CustomTreeModel> {
    return parent_node_;
  }

  auto CustomTreeModel::getParentNodeObject() const -> QObject* {
    return parentNode().get();
  }

  auto CustomTreeModel::childNodes() const -> const std::deque<std::shared_ptr<CustomTreeModel>>& {
    return child_nodes_;
  }

  auto CustomTreeModel::setChildNodes(std::deque<std::shared_ptr<CustomTreeModel>> child_nodes)
      -> void {
    if (child_nodes_ == child_nodes) {
      return;
    }

    const bool leaf_node_changed = (isLeafNode() && !child_nodes.empty()) ||
                                   (!isLeafNode() && child_nodes.empty());

    beginResetModel();
    child_nodes_ = std::move(child_nodes);
    for (const auto& child : child_nodes_) {
      child->setParentNode(shared_from_this());
    }
    endResetModel();
    Q_EMIT childNodesChanged();

    if (leaf_node_changed) {
      Q_EMIT isLeafNodeChanged();
      Q_EMIT isForkNodeChanged();
    }
  }

  auto CustomTreeModel::getChildNodes() const -> std::deque<std::shared_ptr<CustomTreeModel>> {
    return child_nodes_;
  }

  auto CustomTreeModel::getChildNodesModel() -> QAbstractItemModel* {
    return this;
  }

  auto CustomTreeModel::appendChildNode(std::shared_ptr<CustomTreeModel> node) -> void {
    const auto index = std::distance(child_nodes_.cbegin(), child_nodes_.cend());
    beginInsertRows({}, index, index);
    child_nodes_.emplace_back(std::move(node));
    endInsertRows();
  }

  auto CustomTreeModel::findChildNode(const QString& uid) const
      -> std::shared_ptr<CustomTreeModel> {
    for (const auto& child : child_nodes_) {
      if (child->getUid() == uid) {
        return child;
      }
    }

    return nullptr;
  }

  auto CustomTreeModel::eraseChildNode(const QString& uid) -> std::shared_ptr<CustomTreeModel> {
    auto iter = child_nodes_.cbegin();
    for (; iter != child_nodes_.cend(); ++iter) {
      if ((*iter)->getUid() == uid) {
        break;
      }
    }

    if (iter == child_nodes_.cend()) {
      return nullptr;
    }

    const auto index = std::distance(child_nodes_.cbegin(), iter);
    beginRemoveRows({}, index, index);
    auto node = *child_nodes_.erase(iter);
    node->setParentNode(std::make_shared<CustomTreeModel>(nullptr));
    endRemoveRows();
    return node;
  }

  auto CustomTreeModel::clearChildNodes() -> void {
    setChildNodes({});
  }

  int CustomTreeModel::rowCount(const QModelIndex& parent) const {
    return static_cast<int>(child_nodes_.size());
  }

  QVariant CustomTreeModel::data(const QModelIndex& index, int role) const {
    if (index.row() < 0 || index.row() >= child_nodes_.size()) {
      return QVariant::Type::Invalid;
    }

    switch (static_cast<DataRole>(role)) {
      case DataRole::NODE: {
        return QVariant::fromValue(child_nodes_.at(index.row()).get());
        break;
      }
      default: {
        return QVariant::Type::Invalid;
        break;
      }
    }
  }

  QHash<int, QByteArray> CustomTreeModel::roleNames() const {
    return {
      { static_cast<int>(DataRole::NODE), QByteArrayLiteral("model_node") },
    };
  }

}  // namespace qtuser_qml
