#pragma once

#ifndef QTUSERCORE_PLIGIN_CUSTOMTREEMODEL_H_
#define QTUSERCORE_PLIGIN_CUSTOMTREEMODEL_H_

#include <deque>
#include <memory>

#include <QtCore/QAbstractListModel>
#include <QtCore/QVariant>
#include <QtCore/QVariantList>

#include "qtusercore/qtusercoreexport.h"

namespace qtuser_qml {

  class CustomTreeModel;
  class FlattenTreeModel;





  /// @brief Qt Model for CustomTreeView.qml
  class QTUSER_CORE_API CustomTreeModel : public QAbstractListModel
                                        , public std::enable_shared_from_this<CustomTreeModel> {
    Q_OBJECT;

   public:
    explicit CustomTreeModel();
    explicit CustomTreeModel(std::shared_ptr<CustomTreeModel> parent_node);
    virtual ~CustomTreeModel() = default;

   public:
    virtual auto getUid() const -> QString;
    virtual auto setUid(const QString& uid) -> void;
    Q_SIGNAL void uidChanged() const;
    Q_PROPERTY(QString uid READ getUid NOTIFY uidChanged);

   public:
    auto isRootNode() const -> bool;
    Q_SIGNAL void isRootNodeChanged() const;
    Q_PROPERTY(bool isRootNode READ isRootNode NOTIFY isRootNodeChanged);

    auto isForkNode() const -> bool;
    Q_SIGNAL void isForkNodeChanged() const;
    Q_PROPERTY(bool isForkNode READ isForkNode NOTIFY isForkNodeChanged);

    auto isLeafNode() const -> bool;
    Q_SIGNAL void isLeafNodeChanged() const;
    Q_PROPERTY(bool isLeafNode READ isLeafNode NOTIFY isLeafNodeChanged);

   public:
    auto parentNode() const -> std::shared_ptr<CustomTreeModel>;
    auto setParentNode(std::weak_ptr<CustomTreeModel> parent_node) -> void;
    auto getParentNode() const -> std::weak_ptr<CustomTreeModel>;
    auto getParentNodeObject() const -> QObject*;
    Q_SIGNAL void parentNodeChanged() const;
    Q_PROPERTY(QObject* parentNode READ getParentNodeObject NOTIFY parentNodeChanged);

   public:
    auto childNodes() const -> const std::deque<std::shared_ptr<CustomTreeModel>>&;
    auto setChildNodes(std::deque<std::shared_ptr<CustomTreeModel>> child_nodes) -> void;
    auto getChildNodes() const -> std::deque<std::shared_ptr<CustomTreeModel>>;
    auto getChildNodesModel() -> QAbstractItemModel*;
    Q_SIGNAL void childNodesChanged() const;
    Q_PROPERTY(QAbstractItemModel* childNodes READ getChildNodesModel NOTIFY childNodesChanged);

    auto getCount() const -> int;
    Q_SIGNAL void countChanged() const;
    Q_PROPERTY(int count READ getCount NOTIFY countChanged);

    auto emplaceBackChildNode(std::shared_ptr<CustomTreeModel> node) -> void;

    auto findNodeRecursived(const QString& uid) const -> std::shared_ptr<CustomTreeModel>;
    Q_INVOKABLE QVariant findNodeObjectRecursived(const QString& uid) const;

    auto findChildNode(const QString& uid) const -> std::shared_ptr<CustomTreeModel>;
    auto eraseChildNode(const QString& uid) -> std::shared_ptr<CustomTreeModel>;
    auto clearChildNodes() -> void;
    Q_INVOKABLE QVariant findChildNodeObject(const QString& uid) const;
    Q_INVOKABLE void eraseChildNodeObject(const QString& uid);
    Q_INVOKABLE void clearChildNodeObjects();

    Q_INVOKABLE QVariantList toArray() const;

   public:
    auto isNodeValid() const -> bool;
    auto setNodeValid(bool valid) -> void;
    Q_SIGNAL void nodeValidChanged() const;
    Q_PROPERTY(bool nodeValid READ isNodeValid WRITE setNodeValid NOTIFY nodeValidChanged);

    auto isChildNodeValid() const -> bool;
    auto setChildNodeValid(bool valid) -> void;
    Q_SIGNAL void childNodeValidChanged() const;
    Q_PROPERTY(bool childNodeValid
               READ isChildNodeValid
               WRITE setChildNodeValid
               NOTIFY childNodeValidChanged);

    /// @return true if any child node nodeValid property is true under current node's sub tree
    auto hasValidChildNode() const -> bool;
    Q_SIGNAL void hasValidLeafNodeChanged() const;
    Q_PROPERTY(bool hasValidChildNode READ hasValidChildNode NOTIFY hasValidLeafNodeChanged);

    /// @return true if any node nodeValid property is false which between root node and parent node
    auto hasUnvalidParentNode() const -> bool;
    Q_SIGNAL void hasUnvalidParentNodeChanged() const;
    Q_PROPERTY(bool hasUnvalidParentNode
               READ hasUnvalidParentNode
               NOTIFY hasUnvalidParentNodeChanged);

    /// @return true if any node childNodeValid property is false
    ///         whitch between root node and parent node
    auto hasChildUnvalidParentNode() const -> bool;
    Q_SIGNAL void hasChildUnvalidParentNodeChanged() const;
    Q_PROPERTY(bool hasChildUnvalidParentNode
               READ hasChildUnvalidParentNode
               NOTIFY hasChildUnvalidParentNodeChanged);

   protected:
    enum class DataRole : int {
      NODE = Qt::ItemDataRole::UserRole,
    };

   protected:
    auto childNodes() -> std::deque<std::shared_ptr<CustomTreeModel>>&;

   protected:
    auto roleNames() const -> QHash<int, QByteArray> override;
    auto rowCount(const QModelIndex& parent) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;

   private:
    QString uid_{};
    bool valid_{ true };
    bool child_valid_{ true };

    std::weak_ptr<CustomTreeModel> parent_node_{};
    std::deque<std::shared_ptr<CustomTreeModel>> child_nodes_{};
  };





  class QTUSER_CORE_API FlattenTreeModel : public CustomTreeModel {
    Q_OBJECT;

   public:
    explicit FlattenTreeModel();
    explicit FlattenTreeModel(std::shared_ptr<CustomTreeModel> parent_node);
    virtual ~FlattenTreeModel() = default;

   public:
    auto isFlattenable() const -> bool;
    auto setFlattenable(bool flattenable) -> void;

    auto flattenChildNodes() const -> const std::deque<std::weak_ptr<CustomTreeModel>>&;

   protected:
    enum class DataRole : int {
      NODE = Qt::ItemDataRole::UserRole,
      LEVEL,
    };

   protected:
    auto roleNames() const -> QHash<int, QByteArray> override;
    auto rowCount(const QModelIndex& parent) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;

   private:
    auto onCountChanged() -> void;

    auto flattenChildNodes() -> std::deque<std::weak_ptr<CustomTreeModel>>&;

   private:
    bool flattenable_{ true };
    bool flatten_child_nodes_valid_{ false };
    mutable std::deque<std::weak_ptr<CustomTreeModel>> flatten_child_nodes_{};
  };

}  // namespace qtuser_qml

#endif
