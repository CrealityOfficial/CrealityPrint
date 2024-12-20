#pragma once

#ifndef INTERNAL_MODELS_UIPARAMETERSEARCHMODEL_H_
#define INTERNAL_MODELS_UIPARAMETERSEARCHMODEL_H_

#include <QtCore/QPointer>
#include <QtCore/QSortFilterProxyModel>

#include <qtusercore/plugin/datalistmodel.h>

#include "internal/models/parameterdatamodel.h"

namespace creative_kernel {

  class UiParameterSearchListModel;
  class UiParameterSearchProxyModel;





  struct UiParameterSearchItem {
    QString uid{};
    QString key{};
    unsigned int level{ 0u };
    QObject* node{};  // UiParameterTreeModel
    QString text{};
  };

  class UiParameterSearchListModel : public qtuser_qml::DataListModel<UiParameterSearchItem> {
    Q_OBJECT;

    auto operator=(const UiParameterSearchListModel&) -> UiParameterSearchListModel = delete;
    auto operator=(UiParameterSearchListModel&&) -> UiParameterSearchListModel = delete;
    UiParameterSearchListModel(const UiParameterSearchListModel&) = delete;
    UiParameterSearchListModel(UiParameterSearchListModel&&) = delete;

   public:
    using ProxyModel = UiParameterSearchProxyModel;

    enum class Role : int {
      UID = Qt::ItemDataRole::UserRole,
      KEY,
      LEVEL,
      NODE,
      TEXT = Qt::ItemDataRole::DisplayRole,
    };

   public:
    using SuperType::SuperType;
    virtual ~UiParameterSearchListModel() = default;

   public:
    Q_INVOKABLE QVariantMap get(int index) const override;

   public:
    auto getProxyModel() const -> QPointer<ProxyModel>;
    auto getProxyModelObject() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* proxyModel READ getProxyModelObject CONSTANT);

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    mutable QPointer<ProxyModel> proxy_model_{ nullptr };
  };





  class UiParameterSearchProxyModel : public QSortFilterProxyModel {
    Q_OBJECT;

    auto operator=(const UiParameterSearchProxyModel&) -> UiParameterSearchProxyModel = delete;
    auto operator=(UiParameterSearchProxyModel&&) -> UiParameterSearchProxyModel = delete;
    UiParameterSearchProxyModel(const UiParameterSearchProxyModel&) = delete;
    UiParameterSearchProxyModel(UiParameterSearchProxyModel&&) = delete;

   public:
    using SourceModel = UiParameterSearchListModel;

   public:
    explicit UiParameterSearchProxyModel(SourceModel* source_model, QObject* parent = nullptr);
    virtual ~UiParameterSearchProxyModel() = default;

   public:
    auto getSourceModel() const -> QPointer<SourceModel>;
    auto setSourceModel(QPointer<SourceModel> source_model) -> void;

    auto getMinimumLevel() const -> unsigned int;
    auto setMinimumLevel(unsigned int minimum_level) -> void;
    Q_SIGNAL void minimumLevelChanged() const;
    Q_PROPERTY(unsigned int minimumLevel
               READ getMinimumLevel
               WRITE setMinimumLevel
               NOTIFY minimumLevelChanged);

    auto getFilterUidList() const -> QStringList;
    auto setFilterUidList(const QStringList& filter_uid_list) -> void;
    Q_SIGNAL void filterUidListChanged() const;
    Q_PROPERTY(QStringList filterUidList
               READ getFilterUidList
               WRITE setFilterUidList
               NOTIFY filterUidListChanged);

    auto getSearchText() const -> QString;
    auto setSearchText(const QString& search_text) -> void;
    Q_SIGNAL void searchTextChanged() const;
    Q_PROPERTY(QString searchText READ getSearchText WRITE setSearchText NOTIFY searchTextChanged);

    auto getFocusUid() const -> QString;
    auto setFocusUid(const QString& focus_uid) -> void;
    Q_SIGNAL void focusUidChanged() const;
    Q_PROPERTY(QString focusUid READ getFocusUid WRITE setFocusUid NOTIFY focusUidChanged);

    auto getDataModel() const -> QObject*;
    auto setDataModel(QObject* data_model) -> void;
    Q_SIGNAL void dataModelChanged() const;
    Q_PROPERTY(QObject* dataModel READ getDataModel WRITE setDataModel NOTIFY dataModelChanged);

    auto getCount() const -> int;
    Q_SIGNAL void countChanged() const;
    Q_PROPERTY(int count READ getCount NOTIFY countChanged);

    Q_INVOKABLE QVariantMap get(int index) const;

   protected:
    using QAbstractProxyModel::sourceModel;
    using QAbstractProxyModel::setSourceModel;

    auto filterAcceptsRow(int source_row, const QModelIndex& source_parent) const -> bool override;
    auto lessThan(const QModelIndex& source_left,
                  const QModelIndex& source_right) const -> bool override;

   private:
    unsigned int minimum_level_{ 0u };
    QStringList filter_uid_list_{};
    QString search_text_{};
    QString focus_uid_{};
    QPointer<ParameterDataModel> data_model_{ nullptr };
  };

}  // namespace creative_kernel

#endif
