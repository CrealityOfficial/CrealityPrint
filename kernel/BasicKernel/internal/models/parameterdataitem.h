#pragma once

#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "external/us/usetting.h"
#include "external/message/parameterexceededmessage.h"

#include "internal/models/parameterenumlistmodel.h"

namespace creative_kernel {

  class ParameterDataModel;

  class ParameterDataItem;
  class SparseInfillDensityDataItem;
  class FlushIntoObjectsDataItem;
  class EnablePrimeTowerDataItem;
  class IroningSpacingDataItem;





  class ParameterDataItem : public QObject {
    Q_OBJECT;

    auto operator=(const ParameterDataItem&) -> ParameterDataItem = delete;
    auto operator=(ParameterDataItem&&) -> ParameterDataItem = delete;
    ParameterDataItem(const ParameterDataItem&) = delete;
    ParameterDataItem(ParameterDataItem&&) = delete;

   public:
    using DataModel = ParameterDataModel;
    using EnumListModel = ParameterEnumListModel;
    using ExceededMessage = ParameterExceededMessage;
    friend class ParameterDataModel;

   public:
    static auto Create(QPointer<us::USetting> default_setting,
                       QPointer<us::USetting> modifyed_setting,
                       QPointer<DataModel> data_model) -> std::unique_ptr<ParameterDataItem>;

   public:
    virtual ~ParameterDataItem() override;

   public:  // basic property
    auto getKey() const -> QString;
    Q_PROPERTY(QString key READ getKey CONSTANT);

    auto getLabel() const -> QString;
    Q_PROPERTY(QString label READ getLabel CONSTANT);

    auto getDescription() const -> QString;
    Q_PROPERTY(QString description READ getDescription CONSTANT);

    auto getUnit() const -> QString;
    Q_PROPERTY(QString unit READ getUnit CONSTANT);

    auto getType() const -> QString;
    Q_PROPERTY(QString type READ getType CONSTANT);

    virtual auto isGlobalSettable() const -> bool;
    Q_PROPERTY(bool globalSettable READ isGlobalSettable CONSTANT);

    // unused
    auto isExtruderSettable() const -> bool;
    Q_PROPERTY(bool extruderSettable READ isExtruderSettable CONSTANT);

    auto isModelSettable() const -> bool;
    Q_PROPERTY(bool modelSettable READ isModelSettable CONSTANT);

    auto isModelGroupSettable() const -> bool;
    Q_PROPERTY(bool modelGroupSettable READ isModelGroupSettable CONSTANT);

    auto isEnabled() const -> bool;
    Q_SIGNAL void enabledChanged();
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged);

    auto isModifyed() const -> bool;
    auto setModifyed(bool modifyed) -> void;
    Q_SIGNAL void modifyedChanged();
    Q_PROPERTY(bool modifyed READ isModifyed NOTIFY modifyedChanged);

    auto isPartiallyModifyed() const -> bool;
    Q_SIGNAL void partiallyModifyedChanged();
    Q_PROPERTY(bool partiallyModifyed READ isPartiallyModifyed NOTIFY partiallyModifyedChanged);

    auto isExceeded() const -> bool;
    Q_SIGNAL void exceededChanged();
    Q_PROPERTY(bool exceeded READ isExceeded NOTIFY exceededChanged);

    auto isMinimumExceeded() const -> bool;
    Q_SIGNAL void minimumExceededChanged();
    Q_PROPERTY(bool minimumExceeded READ isMinimumExceeded NOTIFY minimumExceededChanged);

    auto isMaximumExceeded() const -> bool;
    Q_SIGNAL void maximumExceededChanged();
    Q_PROPERTY(bool maximumExceeded READ isMaximumExceeded NOTIFY maximumExceededChanged);

   public:  // general value property
    auto getValue() const -> QString;
    auto setValue(const QString& value) -> void;
    Q_SIGNAL void valueChanged();
    Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged);

    auto getDefaultValue() const -> QString;
    Q_SIGNAL void defaultValueChanged();
    Q_PROPERTY(QString defaultValue READ getDefaultValue NOTIFY defaultValueChanged);

    auto getMinimumValue() const -> QString;
    Q_SIGNAL void minimumValueChanged();
    Q_PROPERTY(QString minimumValue READ getMinimumValue NOTIFY minimumValueChanged);

    auto getMaximumValue() const -> QString;
    Q_SIGNAL void maximumValueChanged();
    Q_PROPERTY(QString maximumValue READ getMaximumValue NOTIFY maximumValueChanged);

   public:  // boolean value property
    auto isChecked() const -> bool;
    auto setChecked(bool checked) -> void;
    Q_SIGNAL void checkedChanged();
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged);

   public:  // enum value property
    auto getEnumListModel() const -> QPointer<EnumListModel>;
    auto getEnumListModelObject() const -> QAbstractListModel*;
    Q_PROPERTY(QAbstractListModel* enumListModel READ getEnumListModelObject CONSTANT);

    auto getDefaultIndex() const -> int;
    Q_SIGNAL void defaultIndexChanged();
    Q_PROPERTY(int defaultIndex READ getDefaultIndex NOTIFY defaultIndexChanged);

    /// @return < 0 if current value is not in option list
    auto getCurrentIndex() const -> int;
    auto setCurrentIndex(int index) -> void;
    Q_SIGNAL void currentIndexChanged();
    Q_PROPERTY(int currentIndex
               READ getCurrentIndex
               WRITE setCurrentIndex
               NOTIFY currentIndexChanged);

   public:  // for qml
    Q_INVOKABLE void resetValue();

    auto getUiModel() const -> QObject*;
    auto setUiModel(QObject* ui_model) -> void;
    Q_SIGNAL void uiModelChanged();
    Q_PROPERTY(QObject* uiModel READ getUiModel WRITE setUiModel NOTIFY uiModelChanged);

   public:
    auto getDefaultSetting() const -> QPointer<us::USetting>;
    auto getModifyedSetting() const -> QPointer<us::USetting>;
    auto getDataModel() const -> QPointer<DataModel>;

   protected:
    explicit ParameterDataItem(QPointer<us::USetting> default_setting,
                               QPointer<us::USetting> modifyed_setting,
                               QPointer<DataModel> data_model);

   protected:
    auto setModifyedValue(bool modifyed, const QString& value) -> bool;
    auto setAffectedValue(const QString& value) -> void;

    virtual auto updateExceededState(bool block_signal) -> void;
    virtual auto createMessage() const -> std::unique_ptr<ExceededMessage>;
    auto getMessagePropertyKey() const -> QString;

    auto onUiModelChanged() -> void;
    auto onEnumFilterInvalidated() -> void;

   protected:
    QPointer<us::USetting> default_setting_{ nullptr };
    QPointer<us::USetting> modifyed_setting_{ nullptr };
    QPointer<DataModel> data_model_{ nullptr };

    mutable std::unique_ptr<EnumListModel> enum_list_model_{ nullptr };

    bool minimum_exceeded_{ false };
    bool maximum_exceeded_{ false };

    QPointer<QObject> ui_model_{ nullptr };
  };





  class SparseInfillDensityDataItem final : public ParameterDataItem {
    Q_OBJECT;

   public:
    friend class ParameterDataItem;

   public:
    ~SparseInfillDensityDataItem() final = default;

   public:  // dialog receiver interface
    Q_INVOKABLE QString getMessageText() const;
    Q_INVOKABLE bool isAcceptable() const;
    Q_INVOKABLE bool isCancelable() const;
    Q_INVOKABLE bool isIgnoreable() const;
    Q_INVOKABLE void onAccepted();
    Q_INVOKABLE void onCanceled();

   protected:
    explicit SparseInfillDensityDataItem(QPointer<us::USetting> default_setting,
                                         QPointer<us::USetting> modifyed_setting,
                                         QPointer<DataModel> data_model);

   private:
    auto updateVaildState() -> void;

   private:
    QPointer<ParameterDataItem> sparse_infill_pattern_{ nullptr };
  };





  class FlushIntoObjectsDataItem final : public ParameterDataItem {
    Q_OBJECT;

   public:
    friend class ParameterDataItem;

   public:
    ~FlushIntoObjectsDataItem() final = default;

   public:
    auto isGlobalSettable() const -> bool final;

   protected:
    using ParameterDataItem::ParameterDataItem;
  };





  class EnablePrimeTowerDataItem final : public ParameterDataItem
                                       , public SpaceTracer {
    Q_OBJECT;

   public:
    using Message = EnablePrimeTowerExceededMessage;
    friend class ParameterDataItem;

   public:
    ~EnablePrimeTowerDataItem() final;

   protected:
    explicit EnablePrimeTowerDataItem(QPointer<us::USetting> default_setting,
                                      QPointer<us::USetting> modifyed_setting,
                                      QPointer<DataModel> data_model);

   protected:
    auto updateExceededState(bool block_signal) -> void override;
    auto createMessage() const -> std::unique_ptr<ExceededMessage> override;

   protected:
    auto onModelGroupAdded(ModelGroup* group) -> void override;
    auto onModelGroupRemoved(ModelGroup* group) -> void override;
    auto onModelGroupModified(ModelGroup*           group,
                              const QList<ModelN*>& removed_models,
                              const QList<ModelN*>& added_models) -> void override;

   private:
    auto onCurrentPanelChanged(Printer* panel) -> void;

    using ModelGroupPairList = std::list<std::pair<ModelN*, ModelGroup*>>;
    auto generlateModelGroupPairList() const -> ModelGroupPairList;

    struct ExceededPremise {
      bool multi_color{ false };
      bool multi_group{ false };
    };
    auto generlateExceededPremise(const ModelGroupPairList& pair_list) const -> ExceededPremise;

    auto isSupportEnabled() const -> bool;
    auto checkSupportEnabled(us::USettings* settings) const -> bool;

    auto getSupportFilament() const -> int;
    auto checkSupportFilament(us::USettings* settings) const -> int;

    auto getSupportInterfaceFilament() const -> int;
    auto checkSupportInterfaceFilament(us::USettings* settings) const -> int;

   private:
    QPointer<ParameterDataItem> enable_support_{ nullptr };
    QPointer<ParameterDataItem> support_filament_{ nullptr };
    QPointer<ParameterDataItem> support_interface_filament_{ nullptr };

    QPointer<QObject> model_space_{ nullptr };
    QPointer<Printer> current_panel_{};

    Message::State message_state_{ Message::State::UNVALID };
    QPointer<ModelGroup> layer_height_model_group_{ nullptr };
    QPointer<QObject> layer_height_ui_model_{ nullptr };
  };





  class IroningSpacingDataItem final : public ParameterDataItem {
    Q_OBJECT;

   public:
    friend class ParameterDataItem;

   public:
    ~IroningSpacingDataItem() final = default;

   public:  // dialog receiver interface
    Q_INVOKABLE QString getMessageText() const;
    Q_INVOKABLE bool isAcceptable() const;
    Q_INVOKABLE bool isCancelable() const;
    Q_INVOKABLE bool isIgnoreable() const;
    Q_INVOKABLE void onAccepted();

   protected:
    explicit IroningSpacingDataItem(QPointer<us::USetting> default_setting,
                                    QPointer<us::USetting> modifyed_setting,
                                    QPointer<DataModel> data_model);

   private:
    auto onMinimumExceededChanged() -> void;

   private:
    QPointer<ParameterDataItem> ironing_type_{ nullptr };
  };





  class TreeSupportBranchDiameterDataItem final : public ParameterDataItem {
    Q_OBJECT;

   public:
    friend class ParameterDataItem;

   public:
    ~TreeSupportBranchDiameterDataItem() final = default;

   protected:
    explicit TreeSupportBranchDiameterDataItem(QPointer<us::USetting> default_setting,
                                               QPointer<us::USetting> modifyed_setting,
                                               QPointer<DataModel> data_model);

   protected:
    auto createMessage() const -> std::unique_ptr<ExceededMessage> override;

   private:
    QPointer<ParameterDataItem> tip_diameter_{ nullptr };
  };





  class SupportStyleDataItem final : public ParameterDataItem
                                   , public SpaceTracer {
    Q_OBJECT;

   public:
    using Message = SupportStyleExceededMessage;
    friend class ParameterDataItem;

   public:
    virtual ~SupportStyleDataItem() final;

   protected:
    explicit SupportStyleDataItem(QPointer<us::USetting> default_setting,
                                  QPointer<us::USetting> modifyed_setting,
                                  QPointer<DataModel> data_model);

   protected:
    auto updateExceededState(bool block_signal) -> void override;
    auto createMessage() const -> std::unique_ptr<ExceededMessage> override;

   private:
    auto onCurrentPanelChanged(Printer* panel) -> void;

    auto onModelGroupAdded(ModelGroup* group) -> void override;
    auto onModelGroupRemoved(ModelGroup* group) -> void override;

   private:
    auto isSupportEnabled() const -> bool;
    auto checkSupportEnabled(us::USettings* settings) const -> bool;

    auto isTreeSupportTypeSelected() const -> bool;
    auto checkTreeSupportTypeSelected(us::USettings* settings) const -> bool;

    auto isOrganicTreeStyleSelected() const -> bool;
    auto checkOrganicTreeStyleSelected(us::USettings* settings) const -> bool;

   private:
    QPointer<ParameterDataItem> enable_support_{ nullptr };
    QPointer<ParameterDataItem> support_type_{ nullptr };

    QPointer<QObject> model_space_{ nullptr };
    QPointer<Printer> current_panel_{};
    QPointer<ModelGroup> exceeded_model_group_{ nullptr };
  };

}  // namespace creative_kernel
