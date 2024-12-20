#include "internal/models/parameterenumfilterlistmodel.h"

#include <QtQml/QQmlEngine>

#include "internal/models/parameterdatamodel.h"
#include "internal/models/parameterenumlistmodel.h"

namespace creative_kernel {

  auto ParameterEnumFilterListModel::Create(QPointer<SourceModel> source_model)
      -> std::unique_ptr<ParameterEnumFilterListModel> {
    std::unique_ptr<ParameterEnumFilterListModel> model{ nullptr };

    do {
      if (!source_model) {
        break;
      }

      const auto key = source_model->getDataItem()->getKey();
      if (key.isEmpty()) {
        break;
      }

      if (key == QStringLiteral("support_style")) {
        model.reset(new SupportStyleEnumFilterListModel);
        break;
      }

      if (key == QStringLiteral("sparse_infill_pattern")) {
        model.reset(new SparseInfillPatternEnumFilterListModel);
        break;
      }

    } while (false);

    if (model) {
      model->setSourceModel(source_model);
      QQmlEngine::setObjectOwnership(model.get(), QQmlEngine::ObjectOwnership::CppOwnership);
    }

    return model;
  }

  auto ParameterEnumFilterListModel::getCount() const -> int {
    return rowCount();
  }

  auto ParameterEnumFilterListModel::get(int index) const -> QVariantMap {
    auto source_model = getSourceModel();
    if (!source_model) {
      return {};
    }

    return source_model->get(mapIndexToSource(index));
  }

  int ParameterEnumFilterListModel::mapIndexToSource(int index) const {
    return QSortFilterProxyModel::mapToSource(QSortFilterProxyModel::index(index, 0)).row();
  }

  int ParameterEnumFilterListModel::mapIndexFromSource(int index) const {
    return QSortFilterProxyModel::mapFromSource(source_model_->index(index, 0)).row();
  }

  auto ParameterEnumFilterListModel::getSourceModel() const -> QPointer<SourceModel> {
    return source_model_;
  }

  auto ParameterEnumFilterListModel::setSourceModel(QPointer<SourceModel> source_model) -> void {
    if (source_model_ == source_model) {
      return;
    }

    if (source_model_) {
      source_model_->disconnect(this);
    }

    source_model_ = source_model;
    QAbstractProxyModel::setSourceModel(source_model);

    if (source_model_) {
      connect(source_model_.data(), &SourceModel::countChanged,
              this, &ParameterEnumFilterListModel::countChanged);
    }
  }

  auto ParameterEnumFilterListModel::filterAcceptsRow(
      int source_row, const QModelIndex& source_parent) const -> bool {
    return true;
  }

  auto ParameterEnumFilterListModel::lessThan(const QModelIndex& source_left,
                                              const QModelIndex& source_right) const -> bool {
    return source_left.row() < source_right.row();
  }

  ParameterEnumFilterListModel::ParameterEnumFilterListModel()
      : QSortFilterProxyModel{ nullptr } {
    connect(this, &ParameterEnumFilterListModel::filterInvalidated,
            this, &ParameterEnumFilterListModel::invalidateFilter);
  }





  auto SupportStyleEnumFilterListModel::setSourceModel(QPointer<SourceModel> source_model) -> void {
    if (getSourceModel() == source_model) {
      return;
    }

    if (suport_type_) {
      suport_type_->disconnect(this);
    }

    ParameterEnumFilterListModel::setSourceModel(source_model);
    suport_type_ = nullptr;

    do {
      if (!source_model) {
        break;
      }

      const auto data_item = source_model->getDataItem();
      if (!data_item) {
        break;
      }

      const auto data_model = data_item->getDataModel();
      if (!data_model) {
        break;
      }

      suport_type_ = data_model->findOrMakeDataItem(QStringLiteral("support_type"));
      connect(suport_type_.data(), &SourceModel::DataItem::valueChanged,
              this, &SupportStyleEnumFilterListModel::checkFilterInvalidated);
    } while (false);

    checkFilterInvalidated();
  }

  auto SupportStyleEnumFilterListModel::filterAcceptsRow(
      int source_row, const QModelIndex& source_parent) const -> bool {
    if (!suport_type_) {
      return true;
    }

    auto source_model = getSourceModel();
    if (!source_model) {
      return true;
    }

    const auto option_map = source_model->get(source_row);
    const auto option_iter = option_map.find(QStringLiteral("uid"));
    if (option_iter == option_map.cend()) {
      return true;
    }

    const auto option = option_iter->toString();
    if (option == QStringLiteral("default")) {
      return true;
    }

    const auto value = suport_type_->getValue();
    const auto is_tree_type = value == QStringLiteral("tree(auto)") ||
                              value == QStringLiteral("tree(manual)");
    if (is_tree_type) {
      return option == QStringLiteral("tree_slim") ||
             option == QStringLiteral("tree_strong") ||
             option == QStringLiteral("tree_hybrid") ||
             option == QStringLiteral("organic");
    }

    return option == QStringLiteral("grid") || option == QStringLiteral("snug");
  }

  auto SupportStyleEnumFilterListModel::checkFilterInvalidated() -> void {
    if (!suport_type_) {
      return;
    }

    const auto value = suport_type_->getValue();
    if (current_value_ == value) {
      return;
    }

    const auto is_tree = value.contains(QStringLiteral("tree"));
    const auto is_tree_current = current_value_.contains(QStringLiteral("tree"));

    current_value_ = value;

    if (is_tree_current == is_tree) {
      return;
    }

    filterInvalidated();
    countChanged();
  }





  auto SparseInfillPatternEnumFilterListModel::setSourceModel(QPointer<SourceModel> source_model)
      -> void {
    if (getSourceModel() == source_model) {
      return;
    }

    if (ai_infill_) {
      ai_infill_->disconnect(this);
    }

    ParameterEnumFilterListModel::setSourceModel(source_model);
    ai_infill_ = nullptr;

    do {
      if (!source_model) {
        break;
      }

      const auto data_item = source_model->getDataItem();
      if (!data_item) {
        break;
      }

      const auto data_model = data_item->getDataModel();
      if (!data_model) {
        break;
      }

      ai_infill_ = data_model->findOrMakeDataItem(QStringLiteral("ai_infill"));
      connect(ai_infill_.data(), &SourceModel::DataItem::valueChanged,
              this, &SparseInfillPatternEnumFilterListModel::checkFilterInvalidated);
    } while (false);

    checkFilterInvalidated();
  }

  auto SparseInfillPatternEnumFilterListModel::filterAcceptsRow(
      int source_row, const QModelIndex& source_parent) const -> bool {
    if (!ai_infill_ || !ai_infill_->isChecked()) {
      return true;
    }

    auto source_model = getSourceModel();
    if (!source_model) {
      return true;
    }

    const auto option_map = source_model->get(source_row);
    const auto option_iter = option_map.find(QStringLiteral("uid"));
    if (option_iter == option_map.cend()) {
      return true;
    }

    const auto option = option_iter->toString();
    return option == QStringLiteral("zig-zag") || option == QStringLiteral("grid");
  }

  auto SparseInfillPatternEnumFilterListModel::checkFilterInvalidated() -> void {
    if (!ai_infill_) {
      return;
    }

    const auto checked = ai_infill_->isChecked();
    if (ai_infill_checked_ == checked) {
      return;
    }

    ai_infill_checked_ = checked;
    filterInvalidated();
    countChanged();
  }

}  // namespace creative_kernel
