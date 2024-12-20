#pragma once

#include <memory>

#include <QtCore/QPointer>
#include <QtCore/QSortFilterProxyModel>

namespace creative_kernel {

  class ParameterDataItem;
  class ParameterEnumListModel;

  class ParameterEnumFilterListModel;
  class SupportStyleEnumFilterListModel;





  class ParameterEnumFilterListModel : public QSortFilterProxyModel {
    Q_OBJECT;

    auto operator=(const ParameterEnumFilterListModel&) -> ParameterEnumFilterListModel = delete;
    auto operator=(ParameterEnumFilterListModel&&) -> ParameterEnumFilterListModel = delete;
    ParameterEnumFilterListModel(const ParameterEnumFilterListModel&) = delete;
    ParameterEnumFilterListModel(ParameterEnumFilterListModel&&) = delete;

   public:
    using SourceModel = ParameterEnumListModel;

   public:
    static auto Create(QPointer<SourceModel> source_model)
        -> std::unique_ptr<ParameterEnumFilterListModel>;

   public:
    virtual ~ParameterEnumFilterListModel() override = default;

   public:
    auto getCount() const -> int;
    Q_SIGNAL void countChanged();
    Q_PROPERTY(int count READ getCount NOTIFY countChanged);

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE int mapIndexToSource(int index) const;
    Q_INVOKABLE int mapIndexFromSource(int index) const;

    Q_SIGNAL void filterInvalidated() const;

    auto filterAcceptsRow(int source_row, const QModelIndex& source_parent) const -> bool override;

   protected:
    explicit ParameterEnumFilterListModel();

    virtual auto getSourceModel() const -> QPointer<SourceModel>;
    virtual auto setSourceModel(QPointer<SourceModel> source_model) -> void;

    using QAbstractProxyModel::sourceModel;
    using QAbstractProxyModel::setSourceModel;

    auto lessThan(const QModelIndex& source_left,
                  const QModelIndex& source_right) const -> bool override;

   private:
    using QSortFilterProxyModel::invalidateFilter;

   private:
    QPointer<SourceModel> source_model_{ nullptr };
  };





  class SupportStyleEnumFilterListModel final : public ParameterEnumFilterListModel {
    Q_OBJECT;

   public:
    using DataItem = ParameterDataItem;
    friend class ParameterEnumFilterListModel;

   public:
    ~SupportStyleEnumFilterListModel() final = default;

    auto filterAcceptsRow(int source_row, const QModelIndex& source_parent) const -> bool final;

   protected:
    using ParameterEnumFilterListModel::ParameterEnumFilterListModel;

    auto setSourceModel(QPointer<SourceModel> source_model) -> void final;

   private:
    auto checkFilterInvalidated() -> void;

   private:
    QString current_value_{};
    QPointer<DataItem> suport_type_{ nullptr };
  };





  class SparseInfillPatternEnumFilterListModel final : public ParameterEnumFilterListModel {
    Q_OBJECT;

   public:
    using DataItem = ParameterDataItem;
    friend class ParameterEnumFilterListModel;

   public:
    ~SparseInfillPatternEnumFilterListModel() final = default;

    auto filterAcceptsRow(int source_row, const QModelIndex& source_parent) const -> bool final;

   protected:
    using ParameterEnumFilterListModel::ParameterEnumFilterListModel;

    auto setSourceModel(QPointer<SourceModel> source_model) -> void final;

   private:
    auto checkFilterInvalidated() -> void;

   private:
    QPointer<DataItem> ai_infill_{ nullptr };
    bool ai_infill_checked_{ false };
  };

}  // namespace creative_kernel
