#pragma once

#include <list>
#include <memory>

#include <QtCore/QPointer>
#include <QtGui/QColor>

#include <qtusercore/plugin/datalistmodel.h>

#include "external/data/interface.h"

#include "internal/models/parameterenumfilterlistmodel.h"

namespace creative_kernel {

  class ParameterDataItem;

  struct ParameterEnumItem;
  class ParameterEnumListModel;





  struct ParameterEnumItem {
    QString uid{};
    QString name{};
    QColor color{};
    QString image{};
    QString hovered_image{};
  };





  class ParameterEnumListModel : public qtuser_qml::DataListModel<ParameterEnumItem>
                               , public UIVisualTracer {
    Q_OBJECT;

    auto operator=(const ParameterEnumListModel&) -> ParameterEnumListModel = delete;
    auto operator=(ParameterEnumListModel&&) -> ParameterEnumListModel = delete;
    ParameterEnumListModel(const ParameterEnumListModel&) = delete;
    ParameterEnumListModel(ParameterEnumListModel&&) = delete;

   public:
    using DataItem = ParameterDataItem;
    using FilterModel = ParameterEnumFilterListModel;

    enum class Role : int {
      UID = Qt::ItemDataRole::UserRole,
      NAME = Qt::ItemDataRole::DisplayRole,
      COLOR = UID + 1,
      IMAGE,
      HOVERED_IMAGE,
    };

   public:
    static auto Create(QPointer<const DataItem> data_item)
        -> std::unique_ptr<ParameterEnumListModel>;

   public:
    virtual ~ParameterEnumListModel() override = default;

   public:
    auto getDataItem() const -> QPointer<const DataItem>;

    auto getFilterModel() const -> QPointer<FilterModel>;
    auto getFilterModelObject() const -> QObject*;
    Q_PROPERTY(QObject* filterModel READ getFilterModelObject CONSTANT);

    auto getCount() const -> int;
    Q_SIGNAL void countChanged();
    Q_PROPERTY(int count READ getCount NOTIFY countChanged);

    Q_INVOKABLE QVariantMap get(int row) const override;

   protected:
    explicit ParameterEnumListModel(QPointer<const DataItem> data_item);
    explicit ParameterEnumListModel(const DataContainer& enums, QPointer<const DataItem> data_item);
    explicit ParameterEnumListModel(DataContainer&& enums, QPointer<const DataItem> data_item);

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   protected:
    auto onThemeChanged(ThemeCategory theme) -> void override;
    auto onLanguageChanged(MultiLanguage language) -> void override;

   private:
    QPointer<const DataItem> data_item_{ nullptr };
    mutable bool filter_model_initialized_{ false };
    mutable std::unique_ptr<FilterModel> filter_model_{ nullptr };
    QString theme_{};
  };





  class SupportFilamentEnumListModel final : public ParameterEnumListModel {
    Q_OBJECT;

   public:
    friend class ParameterEnumListModel;

    enum class Role : int {
      UID = Qt::ItemDataRole::UserRole,
      NAME = Qt::ItemDataRole::DisplayRole,
      COLOR = UID + 1,
    };

   public:
    ~SupportFilamentEnumListModel() final;

   protected:
    explicit SupportFilamentEnumListModel(QPointer<const DataItem> data_item);

   private:
    auto onCurrentMachineChanged() -> void;
    auto onExtruderAdded(QObject* extruder) -> void;
    auto onExtruderRemoved(QObject* extruder) -> void;
    auto onExtruderColorChanged() -> void;
    auto onExtruderMaterialChanged() -> void;

   private:
    struct Internal;

   private:
    std::unique_ptr<Internal> internal_{ nullptr };
    std::list<QMetaObject::Connection> connections_{};
  };

}  // namespace creative_kernel
