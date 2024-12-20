#include "internal/models/parameterdatamodel.h"

#include <mutex>

#include <QtQml/QQmlEngine>

#include "internal/models/parameterdatatool.h"
#include "internal/parameter/parameterbase.h"
#include "us/usettings.h"

namespace creative_kernel {

  namespace {

    auto GlobalSettings() -> us::USettings* {
      static std::once_flag init_once{};
      static std::unique_ptr<us::USettings> settings{ nullptr };

      std::call_once(init_once, []() {
        settings = std::make_unique<us::USettings>();
        settings->loadCompleted();
      });

      return settings.get();
    }

  }  // namespace

  ParameterDataModel::ParameterDataModel(QPointer<us::USettings> default_settings,
                                         QPointer<us::USettings> modifyed_settings,
                                         QObject* parent)
      : QObject{ parent }
      , global_settings_{ GlobalSettings() }
      , default_settings_{ default_settings }
      , modifyed_settings_{ modifyed_settings } {}

  auto ParameterDataModel::getDataType() const -> DataType {
    return data_type_;
  }

  auto ParameterDataModel::setDataType(DataType data_type) -> void {
    if (data_type_ != data_type) {
      data_type_ = data_type;
      dataTypeChanged();
    }
  }

  auto ParameterDataModel::getDataTypeString() const -> QString {
    static const std::map<DataType, QString> MAP{
      { DataType::PROFILE, QStringLiteral("profile") },
      { DataType::MODEL, QStringLiteral("model") },
      { DataType::MODEL_GROUP, QStringLiteral("model_group") },
    };

    const auto iter = MAP.find(data_type_);
    return iter != MAP.cend() ? iter->second : MAP.at(DataType::PROFILE);
  }

  auto ParameterDataModel::findOrMakeDataItem(const QString& key) -> QPointer<DataItem> {
    if (key.isEmpty()) {
      return nullptr;
    }

    auto iter = key_data_map_.find(key);
    if (iter != key_data_map_.cend()) {
      return iter->second.get();
    }

    if (!default_settings_ || !modifyed_settings_) {
      return nullptr;
    }

    auto default_setting = QPointer<us::USetting>{ default_settings_->findSettingDelegated(key) };
    if (!default_setting) {
      return nullptr;
    }

    auto modifyed_setting = QPointer<us::USetting>{ modifyed_settings_->findSetting(key) };

    auto&& item_data = DataItem::Create(default_setting, modifyed_setting, this);
    iter = key_data_map_.emplace(std::make_pair(key, std::move(item_data))).first;
    return iter->second.get();
  }

  auto ParameterDataModel::getDefaultSettings() const -> QPointer<us::USettings> {
    return default_settings_;
  }

  auto ParameterDataModel::hasSettingModifyed() const -> bool {
    if (true/*getDataType() != DataType::PROFILE*/) {
      return !CheckEmptySettings(modifyed_settings_, {
        QStringLiteral("flush_volumes_matrix"),
        QStringLiteral("flush_multiplier"),
        QStringLiteral("extruder"),
      });
    }

    return !CheckEmptySettings(modifyed_settings_);
  }

  auto ParameterDataModel::isSettingModifyed(const QString& key) const -> bool {
    if (CheckEmptySettings(modifyed_settings_)) {
      return false;
    }

    return modifyed_settings_->findSetting(key) != nullptr;
  }

  auto ParameterDataModel::emplaceModifyedSetting(const QString& key,
                                                  QPointer<us::USetting> setting) -> void {
    if (!modifyed_settings_ || isSettingModifyed(key)) {
      return;
    }

    const auto settings_modifyed = hasSettingModifyed();

    modifyed_settings_->emplace(key, setting);

    if (settings_modifyed != hasSettingModifyed()) {
      settingsModifyedChanged();
    }
  }

  auto ParameterDataModel::eraseModifyedSetting(const QString& key) -> void {
    if (!modifyed_settings_ || !isSettingModifyed(key)) {
      return;
    }

    const auto settings_modifyed = hasSettingModifyed();

    modifyed_settings_->erase(key);

    if (settings_modifyed != hasSettingModifyed()) {
      settingsModifyedChanged();
    }
  }

  auto ParameterDataModel::getModifyedSettings() const -> QPointer<us::USettings> {
    return modifyed_settings_;
  }

  auto ParameterDataModel::setModifyedSettings(QPointer<us::USettings> settings) -> void {
    modifyed_settings_ = settings;
    settingsModifyedChanged();
  }

  auto ParameterDataModel::resetValue(const QString& key) -> bool {
    const auto iter = key_data_map_.find(key);
    if (iter == key_data_map_.cend()) {
      return false;
    }

    auto* item = iter->second.get();
    if (!item) {
      return false;
    }

    item->resetValue();
    return true;
  }

  auto ParameterDataModel::getDataItem(const QString& key) -> QObject* {
    return findOrMakeDataItem(key);
  }

  auto ParameterDataModel::value(const QString& key) -> QString {
    for (const auto& settings : { modifyed_settings_, default_settings_, global_settings_ }) {
      if (!settings) {
        continue;
      }

      auto* const setting = settings->findSettingDelegated(key);
      if (!setting) {
        continue;
      }

      return CalculateFuzzyValue(setting->type(), key, setting->str());
    }

    return {};
  }

  auto ParameterDataModel::reset() -> void {
    for (const auto& pair : key_data_map_) {
      resetValue(pair.first);
    }
  }

  auto ParameterDataModel::updateAffectedSettings(const QString& key) -> void {
    const auto find_data = [this](const QString& key) {
      DataItem* data = nullptr;

      const auto iter = key_data_map_.find(key);
      if (iter != key_data_map_.cend()) {
        data = iter->second.get();
      }

      return data;
    };

    const auto find_setting = [this](const QString& key) {
      us::USetting* setting = nullptr;

      if (!setting && default_settings_) {
        setting = default_settings_->findSettingDelegated(key);
      }

      if (!setting && global_settings_) {
        setting = global_settings_->findSetting(key);
      }

      return setting;
    };

    for (auto* global_setting : global_settings_->settings()) {
      us::SettingItemDef* define = global_setting->def();

      // if (isSettingModifyed(define->name)) {
      //   continue;
      // }

      auto* data = find_data(define->name);
      auto* const setting = find_setting(define->name);
      const auto find_key = [double_quotes_key = QStringLiteral("\"%1\"").arg(key),
                             single_quotes_key = QStringLiteral("'%1'").arg(key)]
                            (const QString& string) {
        return string.contains(single_quotes_key) || string.contains(double_quotes_key);
      };

      // value affected
      if (find_key(define->valueStr)) {
        const auto js_value = InvokeJsScript(define->valueStr, this);
        if (!js_value.isNull()) {
          const auto value = js_value.toString();

          if (data) {
            if (data->getType() == QStringLiteral("float")) {
              data->setAffectedValue(QString::number(value.toFloat(), 'f', 2));
            } else {
              data->setAffectedValue(value);
            }
          } else if (setting) {
            if (setting->def()->type == QStringLiteral("float")) {
              setting->setValue(QString::number(value.toFloat(), 'f', 2));
            } else {
              setting->setValue(value);
            }
          }

          updateAffectedSettings(define->name);
        }
      }

      // enabled affected
      if (find_key(define->enableStatus)) {
        if (!data) {
          data = findOrMakeDataItem(key);
        }

        data->enabledChanged();
      }

      // minimum value affected
      if (find_key(setting->minStr())) {
        if (!data) {
          data = findOrMakeDataItem(key);
        }

        data->minimumValueChanged();
      }

      // maximum value affected
      if (find_key(setting->maxStr())) {
        if (!data) {
          data = findOrMakeDataItem(key);
        }

        data->maximumValueChanged();
      }

      if (data) {
        data->updateExceededState(false);
      }
    }
  }

  auto ParameterDataModel::enableChanged(const QString& key, bool enabled) -> void {
    auto* parameter_base = dynamic_cast<ParameterBase*>(parent());
    if (parameter_base) {
      parameter_base->enableChanged(key, enabled);
    }
  }

  auto ParameterDataModel::strChanged(const QString& key, const QString& str) -> void {
    auto* parameter_base = dynamic_cast<ParameterBase*>(parent());
    if (parameter_base) {
      parameter_base->strChanged(key, str);
    }
  }

  auto ParameterDataModel::save() -> void {
    auto* parameter_base = dynamic_cast<ParameterBase*>(parent());
    if (parameter_base) {
      parameter_base->setDirty();
      parameter_base->save();
    }
  }

}  // namespace creative_kernel
