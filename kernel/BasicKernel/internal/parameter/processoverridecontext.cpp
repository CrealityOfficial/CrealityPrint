#include "internal/parameter/processoverridecontext.h"

#include "external/kernel/kernel.h"
#include "external/data/modeln.h"
#include "external/interface/selectorinterface.h"

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

    addSelectTracer(this);
    onSelectionsChanged();
  }

  ProcessOverrideContext::ProcessOverrideContext(QObject* parent)
      : ProcessOverrideContext{ TreeModel::CreateRoot(), parent } {}

  ProcessOverrideContext::~ProcessOverrideContext() {
    removeSelectorTracer(this);
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
    const auto* parameter_manager = qobject_cast<ParameterManager*>(sender());
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

    if (current_settings_ && current_user_settings_) {
      auto& delegated_settings_list = current_user_settings_->delegatedSettingsList();
      auto found = false;
      for (const auto& delegated_settings : delegated_settings_list) {
        if (delegated_settings == current_settings_) {
          found = true;
          break;
        }
      }

      if (!found) {
        delegated_settings_list.emplace_back(current_settings_);
      }
    }

    setDefaultSettings(current_user_settings_);
  }

  auto ProcessOverrideContext::onSelectionsChanged() -> void {
    const auto model_list = creative_kernel::selectionms();

    std::set<QPointer<us::USettings>> settings_set{};
    for (const auto& model : model_list) {
      settings_set.emplace(model->setting());
    }

    setValid(!model_list.empty());
    setSourceSettingsSet(std::move(settings_set));
  }

  auto ProcessOverrideContext::selectChanged(qtuser_3d::Pickable* pickable) -> void {}

}  // namespace creative_kernel
