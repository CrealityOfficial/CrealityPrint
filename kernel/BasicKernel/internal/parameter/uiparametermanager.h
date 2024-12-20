#pragma once

#ifndef INTERNAL_PARAMETER_UIPARAMETERMANAGER_H_
#define INTERNAL_PARAMETER_UIPARAMETERMANAGER_H_

#include <map>
#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "external/data/interface.h"
#include "external/slice/sliceflow.h"

#include "internal/models/parameterdatatool.h"
#include "internal/models/processlevellistmodel.h"
#include "internal/models/uiparametertreemodel.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/parameteroverridecontext.h"

namespace creative_kernel {

  class UiParameterManager final : public QObject
                                 , public UIVisualTracer {
    Q_OBJECT;

    auto operator=(const UiParameterManager&) -> UiParameterManager = delete;
    auto operator=(UiParameterManager&&) -> UiParameterManager = delete;
    UiParameterManager(const UiParameterManager&) = delete;
    UiParameterManager(UiParameterManager&&) = delete;

   public:
    explicit UiParameterManager(QPointer<ParameterManager> parameter_manager,
                                SliceFlow::EngineType engine_type,
                                QObject* parent = nullptr);
    virtual ~UiParameterManager() final;

   public:
    auto initialize() -> void;
    auto uninitialize() -> void;
    auto isInitialized() const -> bool;

   public:
    auto getMachineTreeModel() const -> std::shared_ptr<UiParameterTreeModel>;
    auto getMachineTreeModelObject() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* machineTreeModel READ getMachineTreeModelObject CONSTANT);

    auto getProcessTreeModel() const -> std::shared_ptr<UiParameterTreeModel>;
    auto getProcessTreeModelObject() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* processTreeModel READ getProcessTreeModelObject CONSTANT);

    auto getMaterialTreeModel() const -> std::shared_ptr<UiParameterTreeModel>;
    auto getMaterialTreeModelObject() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* materialTreeModel READ getMaterialTreeModelObject CONSTANT);

    auto getOverrideTreeModel() const -> std::shared_ptr<UiParameterTreeModel>;
    auto getOverrideTreeModelObject() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* overrideTreeModel READ getOverrideTreeModelObject CONSTANT);

    auto getMachineCompareTreeModel() const -> std::shared_ptr<UiParameterTreeModel>;
    auto getMachineCompareTreeModelObject() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* machineCompareTreeModel
               READ getMachineCompareTreeModelObject CONSTANT);

    auto getProcessCompareTreeModel() const -> std::shared_ptr<UiParameterTreeModel>;
    auto getProcessCompareTreeModelObject() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* processCompareTreeModel
               READ getProcessCompareTreeModelObject CONSTANT);

    auto getMaterialCompareTreeModel() const -> std::shared_ptr<UiParameterTreeModel>;
    auto getMaterialCompareTreeModelObject() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* materialCompareTreeModel
               READ getMaterialCompareTreeModelObject CONSTANT);

    auto getOverrideCompareTreeModel() const -> std::shared_ptr<UiParameterTreeModel>;
    auto getOverrideCompareTreeModelObject() const -> QAbstractItemModel*;
    Q_PROPERTY(QAbstractItemModel* overrideCompareTreeModel
               READ getOverrideCompareTreeModelObject CONSTANT);

    auto getFocusedMaterialName() const -> QString;
    auto setFocusedMaterialName(const QString& name) -> void;
    Q_SIGNAL void focusedMaterialNameChanged() const;
    Q_PROPERTY(QString focusedMaterialName
               READ getFocusedMaterialName
               WRITE setFocusedMaterialName
               NOTIFY focusedMaterialNameChanged);

    auto getFocusedProcessName() const -> QString;
    auto setFocusedProcessName(const QString& name) -> void;
    Q_SIGNAL void focusedProcessNameChanged() const;
    Q_PROPERTY(QString focusedProcessName
               READ getFocusedProcessName
               WRITE setFocusedProcessName
               NOTIFY focusedProcessNameChanged);

    auto getProcessOverrideContextObject() const -> QObject*;
    Q_PROPERTY(QObject* processOverrideContext READ getProcessOverrideContextObject CONSTANT);

    auto getProcessLevelListModelObject() const -> QObject*;
    Q_PROPERTY(QObject* processLevelListModel READ getProcessLevelListModelObject CONSTANT);

    auto isAdvanceSettingDefault() -> bool;
    auto setAdvanceSettingDefault(bool is_default) -> void;
    Q_SIGNAL void advanceSettingDefaultChanged() const;
    Q_PROPERTY(bool advanceSettingDefault
               READ isAdvanceSettingDefault
               WRITE setAdvanceSettingDefault
               NOTIFY advanceSettingDefaultChanged);

    auto isAdvanceSettingEnabled() -> bool;
    auto setAdvanceSettingEnabled(bool enabled) -> void;
    Q_SIGNAL void advanceSettingEnabledChanged() const;
    Q_PROPERTY(bool advanceSettingEnabled
               READ isAdvanceSettingEnabled
               WRITE setAdvanceSettingEnabled
               NOTIFY advanceSettingEnabledChanged);

   public:
    Q_INVOKABLE void saveProcessCustomizedKeys();

    Q_INVOKABLE bool emplaceOverrideParameter(const QString& key);
    Q_INVOKABLE bool eraseOverrideParameter(const QString& key);
    Q_INVOKABLE bool clearOverrideParameter();

    /// @param tree_model_object UiParameterTreeModel*
    /// @return nullptr if failed, else UiParameterSearchListModel*
    /// @note qml engine takes the ownership
    Q_INVOKABLE QObject* generateSearchContext(QObject* tree_model_object,
                                               const QString& translate_context) const;

   public:
    auto getMessageContext() const -> QPointer<ParameterMessageContext>;

    auto uiImageMap() const -> const std::map<QString, QString>&;

   private:
    auto onThemeChanged(ThemeCategory theme) -> void final;
    auto onLanguageChanged(MultiLanguage language) -> void final;

   private:
    auto loadUiImageMap() -> bool;
    auto loadKeyLabelMap() -> bool;

    auto loadMachineTreeModel() -> bool;
    auto loadProcessTreeModel() -> bool;
    auto loadMaterialTreeModel() -> bool;
    auto loadOverrideTreeModel() -> bool;
    auto syncOverrideTreeModel() -> bool;

   private:
    QPointer<ParameterManager> parameter_manager_{ nullptr };
    const SliceFlow::EngineType engine_type_{ SliceFlow::EngineType::CURA };

    bool initialized_{ false };

    QString focused_material_name_{};
    QString focused_process_name_{};

    std::unique_ptr<ParameterMessageContext> message_context_{ nullptr };

    std::map<QString, QString> ui_image_map_{};

    std::map<QString, QString> key_label_map_{};
    std::map<QString, std::weak_ptr<UiParameterTreeModel>> key_override_node_map_{};

    std::shared_ptr<UiParameterTreeModel> machine_tree_model_{ nullptr };
    std::shared_ptr<UiParameterTreeModel> process_tree_model_{ nullptr };
    std::shared_ptr<UiParameterTreeModel> material_tree_model_{ nullptr };
    std::shared_ptr<UiParameterTreeModel> override_tree_model_{ nullptr };

    std::shared_ptr<UiParameterTreeModel> machine_compare_tree_model_{ nullptr };
    std::shared_ptr<UiParameterTreeModel> process_compare_tree_model_{ nullptr };
    std::shared_ptr<UiParameterTreeModel> material_compare_tree_model_{ nullptr };
    std::shared_ptr<UiParameterTreeModel> override_compare_tree_model_{ nullptr };

    std::unique_ptr<ParameterOverrideContext> process_override_context_{ nullptr };

    std::unique_ptr<ProcessLevelListModel> process_level_list_model_{ nullptr };

    bool advance_setting_enabled_{ false };
  };

}  // namespace creative_kernel

#endif
