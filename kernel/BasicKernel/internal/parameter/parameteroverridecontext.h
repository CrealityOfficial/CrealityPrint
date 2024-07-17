#pragma once

#include <memory>
#include <set>

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "external/us/usettings.h"

#include "internal/models/parameterdatamodel.h"
#include "internal/models/uiparametertreemodel.h"

namespace creative_kernel {

  class ParameterOverrideContext : public QObject {
    Q_OBJECT;

    auto operator=(const ParameterOverrideContext&) -> ParameterOverrideContext = delete;
    auto operator=(ParameterOverrideContext&&) -> ParameterOverrideContext = delete;
    ParameterOverrideContext(const ParameterOverrideContext&) = delete;
    ParameterOverrideContext(ParameterOverrideContext&&) = delete;

   public:
    using TreeModel = UiParameterTreeModel;
    using DataModel = ParameterDataModel;

   public:
    explicit ParameterOverrideContext(std::shared_ptr<TreeModel> tree_model,
                                      QObject* parent = nullptr);
    explicit ParameterOverrideContext(QObject* parent = nullptr);
    virtual ~ParameterOverrideContext() override = default;

   public:
    auto getTreeModel() const -> std::weak_ptr<TreeModel>;
    auto getTreeModelObject() const -> QObject*;
    Q_PROPERTY(QObject* treeModel READ getTreeModelObject CONSTANT);

    auto getDataModel() const -> DataModel*;
    auto getDataModelObject() const -> QObject*;
    Q_SIGNAL void dataModelChanged() const;
    Q_PROPERTY(QObject* dataModel READ getDataModelObject NOTIFY dataModelChanged);

    Q_INVOKABLE void clearSettingsCache();

   public:
    auto getDefualtSettings() const -> QPointer<us::USettings>;
    auto setDefaultSettings(QPointer<us::USettings> settings) -> void;

    auto getSourceSettingsSet() const -> std::set<QPointer<us::USettings>>;
    auto setSourceSettingsSet(std::set<QPointer<us::USettings>> settings_set) -> void;

    auto getCurrentSettings() const -> QPointer<us::USettings>;

   private:
    auto resetCurrentSettings() -> void;

    auto onCurrnetSettingEmplaced(const QString& key, us::USetting* setting) -> void;
    auto onCurrnetSettingValueChanged(const QString& key, us::USetting* setting) -> void;
    auto onCurrnetSettingErased(const QString& key) -> void;

    auto onSourceSettingEmplaced(const QString& key, us::USetting* setting) -> void;
    auto onSourceSettingValueChanged(const QString& key, us::USetting* setting) -> void;
    auto onSourceSettingErased(const QString& key) -> void;

   private:
    std::shared_ptr<TreeModel> tree_model_{ nullptr };
    QPointer<us::USettings> default_settings_{ nullptr };
    QPointer<us::USettings> current_settings_{ nullptr };
    std::set<QPointer<us::USettings>> source_settings_set_{ nullptr };
    mutable std::unique_ptr<DataModel> data_model_{ nullptr };
  };

}  // namespace creative_kernel
