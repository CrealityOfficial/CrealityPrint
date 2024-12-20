#pragma once

#include <map>
#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "external/us/usetting.h"
#include "external/us/usettings.h"

#include "internal/models/parameterdataitem.h"
#include "internal/models/parameterenumlistmodel.h"

namespace creative_kernel {

  class ParameterDataModel : public QObject {
    Q_OBJECT;

    auto operator=(const ParameterDataModel&) -> ParameterDataModel = delete;
    auto operator=(ParameterDataModel&&) -> ParameterDataModel = delete;
    ParameterDataModel(const ParameterDataModel&) = delete;
    ParameterDataModel(ParameterDataModel&&) = delete;

   public:
    using DataItem = ParameterDataItem;
    friend class ParameterDataItem;

    enum class DataType : int {
      PROFILE,
      MODEL,
      MODEL_GROUP,
    };

   public:
    explicit ParameterDataModel(QPointer<us::USettings> default_settings,
                                QPointer<us::USettings> modifyed_settings,
                                QObject* parent = nullptr);
    virtual ~ParameterDataModel() override = default;

   public:
    /// @breif one of: [
    ///   "profile",
    ///   "model",
    ///   "model_group",
    /// ]
    auto getDataType() const -> DataType;
    auto setDataType(DataType data_type) -> void;
    auto getDataTypeString() const -> QString;
    Q_SIGNAL void dataTypeChanged() const;
    Q_PROPERTY(QString dataType READ getDataTypeString NOTIFY dataTypeChanged);

    auto findOrMakeDataItem(const QString& key) -> QPointer<DataItem>;

    auto getDefaultSettings() const -> QPointer<us::USettings>;

    auto hasSettingModifyed() const -> bool;
    auto isSettingModifyed(const QString& key) const -> bool;
    auto emplaceModifyedSetting(const QString& key, QPointer<us::USetting> setting) -> void;
    auto eraseModifyedSetting(const QString& key) -> void;
    auto getModifyedSettings() const -> QPointer<us::USettings>;
    auto setModifyedSettings(QPointer<us::USettings> settings) -> void;

    Q_SIGNAL void settingsModifyedChanged() const;
    Q_PROPERTY(bool settingsModifyed READ hasSettingModifyed NOTIFY settingsModifyedChanged);

    auto resetValue(const QString& key) -> bool;

   public:  // for qml
    Q_INVOKABLE QObject* getDataItem(const QString& key);
    Q_INVOKABLE QString value(const QString& key);
    Q_INVOKABLE void reset();

   private:
    auto updateAffectedSettings(const QString& key) -> void;

    auto enableChanged(const QString& key, bool enabled) -> void;
    auto strChanged(const QString& key, const QString& str) -> void;
    auto save() -> void;

   private:
    DataType data_type_{ DataType::PROFILE };
    std::map<QString, std::unique_ptr<DataItem>> key_data_map_{};
    QPointer<us::USettings> global_settings_{ nullptr };
    QPointer<us::USettings> default_settings_{ nullptr };
    QPointer<us::USettings> modifyed_settings_{ nullptr };
  };

}  // namespace creative_kernel
