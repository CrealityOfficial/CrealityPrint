#include "internal/models/uiparametersearchmodel.h"

namespace creative_kernel {

  auto UiParameterSearchListModel::get(int index) const -> QVariantMap {
    if (rawData().size() <= index) {
      return {};
    }

    const auto model_index = QAbstractListModel::index(index);
    return {
      { QStringLiteral("model_uid"), data(model_index, static_cast<int>(Role::UID)) },
      { QStringLiteral("model_key"), data(model_index, static_cast<int>(Role::KEY)) },
      { QStringLiteral("model_level"), data(model_index, static_cast<int>(Role::LEVEL)) },
      { QStringLiteral("model_node"), data(model_index, static_cast<int>(Role::NODE)) },
      { QStringLiteral("model_text"), data(model_index, static_cast<int>(Role::TEXT)) },
    };
  }

  auto UiParameterSearchListModel::getProxyModel() const -> QPointer<ProxyModel> {
    if (!proxy_model_) {
      auto* self = const_cast<UiParameterSearchListModel*>(this);
      proxy_model_ = new ProxyModel{ self, self };
    }

    return proxy_model_.data();
  }

  auto UiParameterSearchListModel::getProxyModelObject() const -> QAbstractItemModel* {
    return getProxyModel().data();
  }

  auto UiParameterSearchListModel::data(const QModelIndex& index, int role) const-> QVariant {
    if (index.row() < 0 || index.row() >= rawData().size()) {
      return QVariant::Type::Invalid;
    }

    QVariant result;
    const auto data = rawData().at(index.row());
    switch (static_cast<Role>(role)) {
      case Role::UID: {
        result = QVariant::fromValue(data.uid);
        break;
      }
      case Role::KEY: {
        result = QVariant::fromValue(data.key);
        break;
      }
      case Role::LEVEL: {
        result = QVariant::fromValue(data.level);
        break;
      }
      case Role::NODE: {
        result = QVariant::fromValue(data.node);
        break;
      }
      case Role::TEXT: {
        result = QVariant::fromValue(data.text);
        break;
      }
      default: {
        break;
      }
    }

    return result;
  }

  auto UiParameterSearchListModel::roleNames() const -> QHash<int, QByteArray> {
    static const QHash<int, QByteArray> ROLE_NAME{
      { static_cast<int>(Role::UID), QByteArrayLiteral("model_uid") },
      { static_cast<int>(Role::KEY), QByteArrayLiteral("model_key") },
      { static_cast<int>(Role::LEVEL), QByteArrayLiteral("model_level") },
      { static_cast<int>(Role::NODE), QByteArrayLiteral("model_node") },
      { static_cast<int>(Role::TEXT), QByteArrayLiteral("model_text") },
    };

    return ROLE_NAME;
  }





  UiParameterSearchProxyModel::UiParameterSearchProxyModel(SourceModel* source_model,
                                                           QObject* parent)
      : QSortFilterProxyModel{ parent } {
    setSourceModel(source_model);
  }

  auto UiParameterSearchProxyModel::getSourceModel() const -> QPointer<SourceModel> {
    return static_cast<SourceModel*>(sourceModel());
  }

  auto UiParameterSearchProxyModel::setSourceModel(QPointer<SourceModel> source_model) -> void {
    QAbstractProxyModel::setSourceModel(source_model);
  }

  auto UiParameterSearchProxyModel::getMinimumLevel() const -> unsigned int {
    return minimum_level_;
  }

  auto UiParameterSearchProxyModel::setMinimumLevel(unsigned int minimum_level) -> void {
    if (minimum_level_ != minimum_level) {
      minimum_level_ = minimum_level;
      minimumLevelChanged();
      invalidateFilter();
      countChanged();
    }
  }

  auto UiParameterSearchProxyModel::getFilterUidList() const -> QStringList {
    return filter_uid_list_;
  }

  auto UiParameterSearchProxyModel::setFilterUidList(const QStringList& filter_uid_list) -> void {
    if (filter_uid_list_ != filter_uid_list) {
      filter_uid_list_ = filter_uid_list;
      filterUidListChanged();
      invalidateFilter();
      countChanged();
    }
  }

  auto UiParameterSearchProxyModel::getSearchText() const -> QString {
    return search_text_;
  }

  auto UiParameterSearchProxyModel::setSearchText(const QString& search_text) -> void {
    if (search_text_ != search_text) {
      search_text_ = search_text;
      searchTextChanged();
      invalidateFilter();
      countChanged();
    }
  }

  auto UiParameterSearchProxyModel::getFocusUid() const -> QString {
    return focus_uid_;
  }

  auto UiParameterSearchProxyModel::setFocusUid(const QString& focus_uid) -> void {
    if (focus_uid_ != focus_uid) {
      focus_uid_ = focus_uid;
      focusUidChanged();
      sort(0, Qt::SortOrder::AscendingOrder);
    }
  }

  auto UiParameterSearchProxyModel::getDataModel() const -> QObject* {
    return data_model_;
  }

  auto UiParameterSearchProxyModel::setDataModel(QObject* data_model_object) -> void {
    auto* data_model = qobject_cast<ParameterDataModel*>(data_model_object);
    if (data_model_ != data_model) {
      data_model_ = data_model;
      dataModelChanged();
      invalidateFilter();
      countChanged();
    }
  }

  auto UiParameterSearchProxyModel::getCount() const -> int {
    return rowCount();
  }

  auto UiParameterSearchProxyModel::get(int index) const -> QVariantMap {
    const auto source_index = mapToSource(QSortFilterProxyModel::index(index, 0));
    return getSourceModel()->get(source_index.row());
  }

  auto UiParameterSearchProxyModel::filterAcceptsRow(
      int source_row, const QModelIndex& source_parent) const -> bool {
    const auto* model = sourceModel();
    const auto index = model->index(source_row, 0);

    const auto level = model->data(index, static_cast<int>(SourceModel::Role::LEVEL)).toUInt();
    if (level < minimum_level_) {
      return false;
    }

    const auto uid = model->data(index, static_cast<int>(SourceModel::Role::UID)).toString();
    for (const auto& filter_uid : filter_uid_list_) {
      if (uid.startsWith(filter_uid, filterCaseSensitivity())) {
        return false;
      }
    }

    if (!search_text_.isEmpty()) {
      const auto text = model->data(index, static_cast<int>(SourceModel::Role::TEXT)).toString();
      if (!text.contains(search_text_, filterCaseSensitivity())) {
        return false;
      }
    }

    if (data_model_) {
      const auto key = model->data(index, static_cast<int>(SourceModel::Role::KEY)).toString();
      auto data_item = data_model_->findOrMakeDataItem(key);
      if (!data_item || !data_item->isEnabled()) {
        return false;
      }
    }

    return true;
  }

  auto UiParameterSearchProxyModel::lessThan(const QModelIndex& source_left,
                                             const QModelIndex& source_right) const -> bool {
    if (focus_uid_.isEmpty()) {
      return source_left.row() < source_right.row();
    }

    const auto* model = sourceModel();
    constexpr auto ROLE = static_cast<int>(SourceModel::Role::UID);
    const auto focused_left = model->data(source_left, ROLE).toString().startsWith(focus_uid_);
    const auto focused_right = model->data(source_right, ROLE).toString().startsWith(focus_uid_);

    if (focused_left == focused_right) {
      return source_left.row() < source_right.row();
    }

    return focused_left;
  }

}  // namespace creative_kernel
