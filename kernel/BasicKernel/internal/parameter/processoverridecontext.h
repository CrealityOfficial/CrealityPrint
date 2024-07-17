#pragma once

#include <QtCore/QPointer>

#include <qtuser3d/module/pickableselecttracer.h>

#include "internal/parameter/parameteroverridecontext.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printprofile.h"

namespace creative_kernel {

  class ProcessOverrideContext : public ParameterOverrideContext
                               , public qtuser_3d::SelectorTracer {
    Q_OBJECT;

   public:
    explicit ProcessOverrideContext(std::shared_ptr<TreeModel> tree_model,
                                    QObject* parent = nullptr);
    explicit ProcessOverrideContext(QObject* parent = nullptr);
    virtual ~ProcessOverrideContext() override;

   public:
    auto isValid() const -> bool;
    auto setValid(bool valid) -> void;
    Q_SIGNAL void validChanged();
    Q_PROPERTY(bool valid READ isValid NOTIFY validChanged);

   protected:
    auto onCurrentMachineChanged() -> void;
    auto onCurrentProcessChanged() -> void;
    auto onCurrentProcessSettingsChanged() -> void;

   private:
    auto onSelectionsChanged() -> void override;
    auto selectChanged(qtuser_3d::Pickable* pickable) -> void override;

   private:
    bool valid_{ false };
    QPointer<PrintMachine> current_machine_{ nullptr };
    QPointer<PrintProfile> current_process_{ nullptr };
    QPointer<us::USettings> current_settings_{ nullptr };
    QPointer<us::USettings> current_user_settings_{ nullptr };
  };

}  // namespace creative_kernel
