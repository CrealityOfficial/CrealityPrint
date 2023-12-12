#pragma once

#ifndef QTUSERCORE_PLIGIN_CUSTOMTREEMODEL_H_
#define QTUSERCORE_PLIGIN_CUSTOMTREEMODEL_H_

#include <deque>
#include <memory>

#include <QtCore/QAbstractListModel>

#include "qtusercore/qtusercoreexport.h"

namespace qtuser_qml {

  /// @brief Qt Model for CustomTreeView.qml
  class QTUSER_CORE_API CustomTreeModel : public QAbstractListModel
                                        , public std::enable_shared_from_this<CustomTreeModel> {
    Q_OBJECT;

   public:
    explicit CustomTreeModel();
    explicit CustomTreeModel(std::shared_ptr<CustomTreeModel> parent_node);
    virtual ~CustomTreeModel();

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

    auto appendChildNode(std::shared_ptr<CustomTreeModel> node) -> void;

    auto findChildNode(const QString& uid) const -> std::shared_ptr<CustomTreeModel>;
    auto eraseChildNode(const QString& uid) -> std::shared_ptr<CustomTreeModel>;
    auto clearChildNodes() -> void;

   protected:
    int rowCount(const QModelIndex& parent) const final;
    QVariant data(const QModelIndex& index, int role) const final;
    QHash<int, QByteArray> roleNames() const final;

   private:
    enum class DataRole : int {
      NODE = Qt::ItemDataRole::UserRole,
    };

   private:
    QString uid_{};
    std::weak_ptr<CustomTreeModel> parent_node_{};
    std::deque<std::shared_ptr<CustomTreeModel>> child_nodes_{};
  };

}  // namespace qtuser_qml

#endif
