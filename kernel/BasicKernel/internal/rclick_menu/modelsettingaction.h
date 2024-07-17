#pragma once

#include "external/data/interface.h"
#include "external/menu/actioncommand.h"

namespace creative_kernel {

  class ModelSettingAction : public ActionCommand
                           , public UIVisualTracer {
    Q_OBJECT;

   public:
    explicit ModelSettingAction(QObject* parent = nullptr);
    virtual ~ModelSettingAction() override = default;

   public:
    Q_INVOKABLE void execute();
    Q_SLOT void onTipDialogFinished(bool show_next_time);

   protected:
    auto onThemeChanged(ThemeCategory category) -> void override;
    auto onLanguageChanged(MultiLanguage language) -> void override;
  };

}  // namespace creative_kernel
