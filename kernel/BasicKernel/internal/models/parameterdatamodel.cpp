#include "internal/models/parameterdatamodel.h"

#include <QtQml/QQmlEngine>

#include "internal/models/parameterdatatool.h"
#include "internal/parameter/parameterbase.h"

namespace creative_kernel {

  ParameterDataModel::ParameterDataModel(QPointer<us::USettings> default_settings,
                                         QPointer<us::USettings> modifyed_settings,
                                         QObject* parent)
      : QObject{ parent }
      , global_settings_{ new us::USettings{ this } }
      , default_settings_{ default_settings }
      , modifyed_settings_{ modifyed_settings } {
    global_settings_->loadCompleted();
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
    if (modifyed_settings_) {
      const auto& hash = modifyed_settings_->hashSettings();
      if (hash.size() == 2 &&
          hash.contains(QStringLiteral("flush_volumes_matrix")) &&
          hash.contains(QStringLiteral("flush_multiplier"))) {
        return false;
      }
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

  auto ParameterDataModel::value(const QString& key) const -> QString {
    for (const auto& settings : { modifyed_settings_, default_settings_, global_settings_ }) {
      if (settings && settings->hasKey(key)) {
        auto* const setting = settings->findSetting(key);
        return CalculateFuzzyValue(setting->type(), key, setting->str());
      }
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
