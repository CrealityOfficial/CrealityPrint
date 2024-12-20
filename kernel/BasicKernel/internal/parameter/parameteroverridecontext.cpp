#include "internal/parameter/parameteroverridecontext.h"

#include <map>

#include <QtQml/QQmlEngine>

#include "external/data/modelgroup.h"

namespace creative_kernel {

  ParameterOverrideContext::ParameterOverrideContext(
    std::shared_ptr<TreeModel> tree_model,
    QObject* parent)
      : QObject{ parent }
      , tree_model_{ std::move(tree_model) } {}

  ParameterOverrideContext::ParameterOverrideContext(QObject* parent)
      : ParameterOverrideContext{ TreeModel::CreateRoot(), parent } {}

  auto ParameterOverrideContext::getTreeModel() const -> std::weak_ptr<TreeModel> {
    return tree_model_;
  }

  auto ParameterOverrideContext::getTreeModelObject() const -> QObject* {
    return tree_model_.get();
  }

  auto ParameterOverrideContext::getDataModel() const -> DataModel* {
    if (!default_settings_ || !current_settings_) {
      return nullptr;
    }

    if (!data_model_) {
      data_model_ = std::make_unique<DataModel>(default_settings_, current_settings_);
      data_model_->setDataType([this]() {
        const auto settings = *source_settings_set_.cbegin();
        const auto from_group = dynamic_cast<ModelGroup*>(settings ? settings->parent() : nullptr);
        return from_group ? DataModel::DataType::MODEL_GROUP : DataModel::DataType::MODEL;
      }());

      QQmlEngine::setObjectOwnership(data_model_.get(), QQmlEngine::ObjectOwnership::CppOwnership);
    }

    return data_model_.get();
  }

  auto ParameterOverrideContext::getDataModelObject() const -> QObject* {
    return getDataModel();
  }

  auto ParameterOverrideContext::clearSettingsCache() -> void {
    DataModel* temp = data_model_.release();
    temp->disconnect(this);
    temp->deleteLater();
    dataModelChanged();
  }

  auto ParameterOverrideContext::getDefualtSettings() const -> QPointer<us::USettings> {
    return default_settings_;
  }

  auto ParameterOverrideContext::setDefaultSettings(QPointer<us::USettings> settings) -> void {
    if (default_settings_ != settings) {
      default_settings_ = settings;
      clearSettingsCache();
    }
  }

  auto ParameterOverrideContext::getSourceSettingsSet() const
      -> std::set<QPointer<us::USettings>> {
    return source_settings_set_;
  }

  auto ParameterOverrideContext::setSourceSettingsSet(
      std::set<QPointer<us::USettings>> settings_set) -> void {
    if (!settings_set.empty() && source_settings_set_ != settings_set) {
    // if (source_settings_set_ != settings_set) {
      source_settings_set_ = settings_set;
      resetCurrentSettings();
      clearSettingsCache();
    }
  }

  auto ParameterOverrideContext::getCurrentSettings() const -> QPointer<us::USettings> {
    return current_settings_;
  }

  auto ParameterOverrideContext::resetCurrentSettings() -> void {
    if (current_settings_ && current_settings_->parent() == this) {
      delete current_settings_;
    }

    current_settings_ = nullptr;

    if (source_settings_set_.empty()) {
      return;
    }

    if (source_settings_set_.size() == 1) {
      current_settings_ = *source_settings_set_.begin();
      return;
    }

    current_settings_ = new us::USettings{ this };

    std::map<QString, std::multiset<QString>> key_values_map{};
    for (const auto& setting_define : us::SettingDef::instance().getHashItemDef()) {
      const auto& key = setting_define->name;
      auto&& pair = std::make_pair(key, std::multiset<QString>{});
      auto& values = key_values_map.emplace(std::move(pair)).first->second;

      for (const auto& source_settings : source_settings_set_) {
        auto* setting = source_settings->findSetting(key);
        if (!setting) {
          continue;
        }

        values.emplace(setting->str());
      }
    }

    for (const auto& pair : key_values_map) {
      const auto& key = pair.first;
      const auto& values = pair.second;
      if (values.empty()) {
        continue;
      }

      auto first_value = *values.begin();
      const auto value = [this, values, &first_value]() -> QString {
        if (values.size() != source_settings_set_.size()) {
          return {};
        }

        for (const auto& value : values) {
          if (value != first_value) {
            return {};
          }
        }

        return first_value;
      }();

      if (first_value.isEmpty() && value.isEmpty()) {
        continue;
      }

      current_settings_->emplace(key, us::SettingDef::instance().create(key, value));
    }

    for (const auto& source_settings : source_settings_set_) {
      connect(source_settings.data(), &us::USettings::settingEmplaced,
              this, &ParameterOverrideContext::onSourceSettingEmplaced);

      connect(source_settings.data(), &us::USettings::settingValueChanged,
              this, &ParameterOverrideContext::onSourceSettingValueChanged);

      connect(source_settings.data(), &us::USettings::settingErased,
              this, &ParameterOverrideContext::onSourceSettingErased);
    }

    connect(current_settings_.data(), &us::USettings::settingEmplaced,
            this, &ParameterOverrideContext::onCurrnetSettingEmplaced);

    connect(current_settings_.data(), &us::USettings::settingValueChanged,
            this, &ParameterOverrideContext::onCurrnetSettingValueChanged);

    connect(current_settings_.data(), &us::USettings::settingErased,
            this, &ParameterOverrideContext::onCurrnetSettingErased);
  }

  auto ParameterOverrideContext::onCurrnetSettingEmplaced(const QString& key,
                                                          us::USetting* setting) -> void {
    const auto value = setting->str();
    for (auto& srouce_settings : source_settings_set_) {
      srouce_settings->emplace(key, us::SettingDef::instance().create(key, value));
    }
  }

  auto ParameterOverrideContext::onCurrnetSettingValueChanged(const QString& key,
                                                              us::USetting* setting) -> void {
    const auto value = setting->str();
    if (value.isEmpty()) {
      return;
    }

    for (auto& source_settings : source_settings_set_) {
      auto* source_setting = source_settings->findSetting(key);
      if (source_setting) {
        source_setting->setStr(value);
        continue;
      }

      source_settings->emplace(key, us::SettingDef::instance().create(key, value));
    }
  }

  auto ParameterOverrideContext::onCurrnetSettingErased(const QString& key) -> void {
    for (auto& source_settings : source_settings_set_) {
      source_settings->erase(key);
    }
  }

  auto ParameterOverrideContext::onSourceSettingEmplaced(const QString& key,
                                                         us::USetting* setting) -> void {
    std::ignore = key;
    std::ignore = setting;
  }

  auto ParameterOverrideContext::onSourceSettingValueChanged(const QString& key,
                                                             us::USetting* setting) -> void {
    auto* current_setting = current_settings_->findSetting(key);
    if (!current_setting) {
      return;
    }

    const auto value = setting->str();
    for (auto& source_settings : source_settings_set_) {
      auto* source_setting = source_settings->findSetting(key);
      if (!source_setting || source_setting == setting) {
        continue;
      }

      if (source_setting->str() != value) {
        current_setting->setStr({});
        return;
      }
    }

    current_setting->setStr(value);
  }

  auto ParameterOverrideContext::onSourceSettingErased(const QString& key) -> void {
    std::ignore = key;
  }

}  // namespace creative_kernel
