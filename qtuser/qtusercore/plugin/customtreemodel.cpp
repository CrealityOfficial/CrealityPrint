#include "qtusercore/plugin/customtreemodel.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>

#include <QtQml/QQmlEngine>

namespace qtuser_qml {

  CustomTreeModel::CustomTreeModel() : CustomTreeModel{ nullptr } {}

  CustomTreeModel::CustomTreeModel(std::shared_ptr<CustomTreeModel> parent_node)
      : QAbstractListModel{ nullptr }
      , uid_{}
      , valid_{ true }
      , child_valid_{ true }
      , parent_node_{ parent_node }
      , child_nodes_{} {
    connect(this, &QAbstractItemModel::rowsInserted, this, &CustomTreeModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &CustomTreeModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &CustomTreeModel::childNodesChanged);

    if (parent_node) {
      connect(parent_node.get(), &CustomTreeModel::nodeValidChanged,
              this, &CustomTreeModel::hasUnvalidParentNodeChanged);
      connect(parent_node.get(), &CustomTreeModel::hasUnvalidParentNodeChanged,
              this, &CustomTreeModel::hasUnvalidParentNodeChanged);

      connect(parent_node.get(), &CustomTreeModel::childNodeValidChanged,
              this, &CustomTreeModel::hasChildUnvalidParentNodeChanged);
      connect(parent_node.get(), &CustomTreeModel::hasChildUnvalidParentNodeChanged,
              this, &CustomTreeModel::hasChildUnvalidParentNodeChanged);

      connect(this, &CustomTreeModel::nodeValidChanged,
              parent_node.get(), &CustomTreeModel::hasValidLeafNodeChanged);
      connect(this, &CustomTreeModel::hasValidLeafNodeChanged,
              parent_node.get(), &CustomTreeModel::hasValidLeafNodeChanged);
    }
  }

  auto CustomTreeModel::getUid() const -> QString {
    return uid_;
  }

  auto CustomTreeModel::setUid(const QString& uid) -> void {
    if (uid_ == uid) {
      return;
    }

    uid_ = uid;
    uidChanged();
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
    parentNodeChanged();

    if (root_node_changed) {
      isRootNodeChanged();
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

  auto CustomTreeModel::childNodes() -> std::deque<std::shared_ptr<CustomTreeModel>>& {
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

    if (leaf_node_changed) {
      isLeafNodeChanged();
      isForkNodeChanged();
    }
  }

  auto CustomTreeModel::getChildNodes() const -> std::deque<std::shared_ptr<CustomTreeModel>> {
    return child_nodes_;
  }

  auto CustomTreeModel::getChildNodesModel() -> QAbstractItemModel* {
    return this;
  }

  auto CustomTreeModel::getCount() const -> int {
    return rowCount({});
  }

  auto CustomTreeModel::emplaceBackChildNode(std::shared_ptr<CustomTreeModel> node) -> void {
    const auto index = std::distance(child_nodes_.cbegin(), child_nodes_.cend());
    beginInsertRows({}, index, index);
    child_nodes_.emplace_back(std::move(node));
    endInsertRows();
  }

  auto CustomTreeModel::findNodeRecursived(const QString& uid) const
      -> std::shared_ptr<CustomTreeModel> {
    for (const auto& child : child_nodes_) {
      const auto& child_uid = child->getUid();
      if (child_uid == uid) {
        return child;
      }

      if (uid.startsWith(child_uid)) {
        return child->findNodeRecursived(uid);
      }
    }

    return nullptr;
  }

  auto CustomTreeModel::findNodeObjectRecursived(const QString& uid) const -> QVariant {
    return QVariant::fromValue<QObject*>(findNodeRecursived(uid).get());
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

  auto CustomTreeModel::findChildNodeObject(const QString& uid) const -> QVariant {
    return QVariant::fromValue<QObject*>(findChildNode(uid).get());
  }

  auto CustomTreeModel::eraseChildNodeObject(const QString& uid) -> void {
    eraseChildNode(uid);
  }

  auto CustomTreeModel::clearChildNodeObjects() -> void {
    clearChildNodes();
  }

  auto CustomTreeModel::toArray() const -> QVariantList {
    QVariantList array{};

    std::transform(child_nodes_.cbegin(), child_nodes_.cend(), std::back_inserter(array),
        [](auto&& item) {
          return QVariant::fromValue<QObject*>(item.get());
        });

    return array;
  }

  auto CustomTreeModel::isNodeValid() const -> bool {
    return valid_;
  }

  auto CustomTreeModel::setNodeValid(bool valid) -> void {
    if (valid_ == valid) {
      return;
    }

    valid_ = valid;
    nodeValidChanged();
  }

  auto CustomTreeModel::isChildNodeValid() const -> bool {
    return child_valid_;
  }

  auto CustomTreeModel::setChildNodeValid(bool valid) -> void {
    if (child_valid_ == valid) {
      return;
    }

    child_valid_ = valid;
    childNodeValidChanged();
  }

  auto CustomTreeModel::hasValidChildNode() const -> bool {
    for (const auto& child_node : childNodes()) {
      if (child_node && child_node->isNodeValid()) {
        if (child_node->isLeafNode() || child_node->hasValidChildNode()) {
          return true;
        }
      }
    }

    return false;
  }

  auto CustomTreeModel::hasUnvalidParentNode() const -> bool {
    for (auto parent = parentNode(); parent; parent = parent->parentNode()) {
      if (!parent->isNodeValid()) {
        return true;
      }
    }

    return false;
  }

  auto CustomTreeModel::hasChildUnvalidParentNode() const -> bool {
    for (auto parent = parentNode(); parent; parent = parent->parentNode()) {
      if (!parent->isChildNodeValid()) {
        return true;
      }
    }

    return false;
  }

  auto CustomTreeModel::roleNames() const -> QHash<int, QByteArray> {
    return {
      { static_cast<int>(DataRole::NODE), QByteArrayLiteral("model_node") },
    };
  }

  auto CustomTreeModel::rowCount(const QModelIndex& parent) const -> int {
    return static_cast<int>(childNodes().size());
  }

  auto CustomTreeModel::data(const QModelIndex& index, int role) const -> QVariant {
    if (index.row() < 0 || index.row() >= getCount()) {
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





  FlattenTreeModel::FlattenTreeModel() : FlattenTreeModel{ nullptr } {}

  FlattenTreeModel::FlattenTreeModel(std::shared_ptr<CustomTreeModel> parent_node)
      : CustomTreeModel{ std::move(parent_node) }
      , flattenable_{ true }
      , flatten_child_nodes_valid_{ false }
      , flatten_child_nodes_{} {
    connect(this, &CustomTreeModel::countChanged, this, &FlattenTreeModel::onCountChanged);
  }

  auto FlattenTreeModel::isFlattenable() const -> bool {
    return flattenable_;
  }

  auto FlattenTreeModel::setFlattenable(bool flattenable) -> void {
    if (flattenable_ == flattenable) {
      return;
    }

    flattenable_ = flattenable;

    beginResetModel();
    countChanged();
    endResetModel();
  }

  auto FlattenTreeModel::flattenChildNodes() const
      -> const std::deque<std::weak_ptr<CustomTreeModel>>& {
    return const_cast<FlattenTreeModel*>(this)->flattenChildNodes();
  }

  auto FlattenTreeModel::flattenChildNodes() -> std::deque<std::weak_ptr<CustomTreeModel>>& {
    if (!flatten_child_nodes_valid_) {
      flatten_child_nodes_.clear();

      std::function<void(std::shared_ptr<CustomTreeModel> node)> recursiver{ nullptr };

      recursiver = [this, &recursiver](auto node) -> void {
        if (!node) {
          return;
        }

        if (node.get() != this) {
          flatten_child_nodes_.emplace_back(node);
        }

        auto const_node = std::const_pointer_cast<const CustomTreeModel>(node);
        for (auto child_node : const_node->childNodes()) {
          recursiver(child_node);
        }
      };

      recursiver(shared_from_this());
    }

    return flatten_child_nodes_;
  }

  auto FlattenTreeModel::roleNames() const -> QHash<int, QByteArray> {
    return {
      { static_cast<int>(DataRole::NODE), QByteArrayLiteral("model_node") },
      { static_cast<int>(DataRole::LEVEL), QByteArrayLiteral("model_level") },
    };
  }

  auto FlattenTreeModel::rowCount(const QModelIndex& parent) const -> int {
    if (!flattenable_) {
      return CustomTreeModel::rowCount(parent);
    }

    return static_cast<int>(flattenChildNodes().size());
  }

  auto FlattenTreeModel::data(const QModelIndex& index, int role) const -> QVariant {
    if (!flattenable_) {
      return CustomTreeModel::data(index, role);
    }

    if (index.row() < 0 || index.row() >= getCount()) {
      return QVariant::Type::Invalid;
    }

    switch (static_cast<DataRole>(role)) {
      case DataRole::NODE: {
        auto node = flattenChildNodes().at(index.row()).lock();
        return QVariant::fromValue(node.get());
        break;
      }
      case DataRole::LEVEL: {
        auto node = flattenChildNodes().at(index.row()).lock();

        auto level = 0;
        for (auto parent = node->parentNode();
             parent && parent.get() != this;
             parent = parent->parentNode()) {
          ++level;
        }

        return QVariant::fromValue(level);
        break;
      }
      default: {
        return QVariant::Type::Invalid;
        break;
      }
    }
  }

  auto FlattenTreeModel::onCountChanged() -> void {
    flatten_child_nodes_valid_ = false;
  }

}  // namespace qtuser_qml
