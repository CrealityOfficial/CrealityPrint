#pragma once

#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "external/us/usetting.h"

#include "internal/models/parameterenumlistmodel.h"

namespace creative_kernel {

  class ParameterDataModel;

  class ParameterDataItem;
  class ParameterMessageContext;





  class ParameterDataItem : public QObject {
    Q_OBJECT;

    auto operator=(const ParameterDataItem&) -> ParameterDataItem = delete;
    auto operator=(ParameterDataItem&&) -> ParameterDataItem = delete;
    ParameterDataItem(const ParameterDataItem&) = delete;
    ParameterDataItem(ParameterDataItem&&) = delete;

   public:
    using DataModel = ParameterDataModel;
    using EnumListModel = ParameterEnumListModel;
    using MessageContext = ParameterMessageContext;
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

    virtual auto isProcessSettable() const -> bool;
    Q_PROPERTY(bool processSettable READ isProcessSettable CONSTANT);

    auto isMeshSettable() const -> bool;
    Q_PROPERTY(bool meshSettable READ isMeshSettable CONSTANT);

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
    auto updateExceededState(bool initialize) -> void;

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
    Q_INVOKABLE void accept();
    Q_INVOKABLE void cancel();

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
    auto isProcessSettable() const -> bool final;

   protected:
    using ParameterDataItem::ParameterDataItem;
  };

}  // namespace creative_kernel
