#include "internal/parameter/processoverridecontext.h"

#include <algorithm>

#include "external/data/modeln.h"
#include "external/interface/selectorinterface.h"
#include "external/kernel/kernel.h"

#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printprofile.h"

namespace creative_kernel {

  ProcessOverrideContext::ProcessOverrideContext(
    std::shared_ptr<TreeModel> tree_model,
    QObject* parent)
      : ParameterOverrideContext{ tree_model, parent } {
    tree_model->setType(TreeModel::Type::PROCESS);

    connect(getKernel()->parameterManager(), &ParameterManager::curMachineIndexChanged,
            this, &ProcessOverrideContext::onCurrentMachineChanged);
    onCurrentMachineChanged();

    addModelNSelectorTracer(this);
    onSelectionsChanged();
  }

  ProcessOverrideContext::ProcessOverrideContext(QObject* parent)
      : ProcessOverrideContext{ TreeModel::CreateRoot(), parent } {}

  ProcessOverrideContext::~ProcessOverrideContext() {
    removeModelNSelectorTracer(this);
  }

  auto ProcessOverrideContext::isValid() const -> bool {
    return valid_;
  }

  auto ProcessOverrideContext::setValid(bool valid) -> void {
    if (valid_ != valid) {
      valid_ = valid;
      validChanged();
    }
  }

  auto ProcessOverrideContext::onCurrentMachineChanged() -> void {
    const auto* parameter_manager = dynamic_cast<ParameterManager*>(sender());
    auto* current_machine = parameter_manager ? parameter_manager->currentMachine() : nullptr;
    if (current_machine_ == current_machine) {
      return;
    }

    if (current_machine_) {
      disconnect(current_machine_, &PrintMachine::curProfileIndexChanged,
                 this, &ProcessOverrideContext::onCurrentProcessChanged);
    }

    current_machine_ = current_machine;

    if (current_machine_) {
      connect(current_machine, &PrintMachine::curProfileIndexChanged,
              this, &ProcessOverrideContext::onCurrentProcessChanged);
    }

    onCurrentProcessChanged();
  }

  auto ProcessOverrideContext::onCurrentProcessChanged() -> void {
    auto* current_process = current_machine_ ? current_machine_->currentProfile() : nullptr;
    if (current_process_ == current_process) {
      return;
    }

    if (current_process_) {
      disconnect(current_process_, &ParameterBase::settingsChanged,
                 this, &ProcessOverrideContext::onCurrentProcessSettingsChanged);
      disconnect(current_process_, &ParameterBase::userSettingsChanged,
                 this, &ProcessOverrideContext::onCurrentProcessSettingsChanged);
    }

    current_process_ = current_process;

    if (current_process_) {
      connect(current_process_, &ParameterBase::settingsChanged,
              this, &ProcessOverrideContext::onCurrentProcessSettingsChanged);
      connect(current_process_, &ParameterBase::userSettingsChanged,
              this, &ProcessOverrideContext::onCurrentProcessSettingsChanged);
    }

    onCurrentProcessSettingsChanged();
  }

  auto ProcessOverrideContext::onCurrentProcessSettingsChanged() -> void {
    auto* current_settings = current_process_ ? current_process_->settings() : nullptr;
    auto* current_user_settings = current_process_ ? current_process_->userSettings() : nullptr;
    if (current_settings_ == current_settings && current_user_settings_ == current_user_settings) {
      return;
    }

    if (current_settings_ != current_settings) {
      current_settings_ = current_settings;
    }

    if (current_user_settings_ != current_user_settings) {
      if (current_user_settings_) {
        disconnect(current_user_settings_, &us::USettings::settingsChanged,
                   this, &ParameterOverrideContext::clearSettingsCache);
      }

      current_user_settings_ = current_user_settings;

      if (current_user_settings_) {
        connect(current_user_settings_, &us::USettings::settingsChanged,
                this, &ParameterOverrideContext::clearSettingsCache);
      }
    }

    // ensure delegate chain like: profile user settings -> profile settings
    if (current_settings_ && current_user_settings_) {
      auto& delegates = current_user_settings_->delegatedSettingsList();
      auto iter = std::find_if(delegates.cbegin(), delegates.cend(), [this](auto&& settings) {
        return settings && settings == current_settings_;
      });

      if (iter == delegates.cend()) {
        delegates.emplace_back(current_settings_);
      }
    }

    if (!current_model_group_) {
      setDefaultSettings(current_user_settings_);
      return;
    }

    auto* default_settings = current_model_group_->setting();
    if (!default_settings) {
      setDefaultSettings(current_user_settings_);
      return;
    }

    // ensure delegate chain like: model group settings -> profile user settings -> profile settings
    auto& delegates = default_settings->delegatedSettingsList();
    auto iter = std::find_if(delegates.begin(), delegates.end(), [](auto&& settings) {
      return settings && dynamic_cast<decltype(current_process_.data())>(settings->parent());
    });

    if (iter != delegates.end() && current_user_settings_) {
      (*iter) = current_user_settings_;
    }

    if (getDefualtSettings() != default_settings) {
      setDefaultSettings(default_settings);
    } else {
      clearSettingsCache();
    }
  }

  auto ProcessOverrideContext::onSelectionsChanged() -> void {
    decltype(getSourceSettingsSet()) source_settings_set{};
    decltype(current_model_group_.data()) current_model_group{ nullptr };

    for (const auto& group : creative_kernel::selectedGroups()) {
      source_settings_set.emplace(group->setting());
    }

    if (source_settings_set.empty()) {
      for (const auto& model : creative_kernel::selectionms()) {
        source_settings_set.emplace(model->setting());
        current_model_group = model->getModelGroup();
      }
    }

    // ensure delegate chain like: model group settings -> profile user settings -> profile settings
    if (current_model_group_ != current_model_group) {
      current_model_group_ = current_model_group;

      if (current_model_group_) {
        auto* default_settings = current_model_group_->setting();

        if (default_settings) {
          auto& delegates = default_settings->delegatedSettingsList();
          auto iter = std::find_if(delegates.cbegin(), delegates.cend(), [](auto&& settings) {
            return settings && dynamic_cast<decltype(current_process_.data())>(settings->parent());
          });

          if (iter == delegates.cend() && current_user_settings_) {
            delegates.emplace_back(current_user_settings_);
          }
        }
      }
    }

    setValid(!source_settings_set.empty());

    if (current_model_group_) {
      auto* default_settings = current_model_group->setting();
      if (default_settings) {
        setDefaultSettings(default_settings);
      }

    } else if (current_user_settings_) {
      setDefaultSettings(current_user_settings_);
    }

    setSourceSettingsSet(std::move(source_settings_set));
  }

}  // namespace creative_kernel
