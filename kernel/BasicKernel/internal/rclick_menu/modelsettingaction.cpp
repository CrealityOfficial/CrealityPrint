#include "internal/rclick_menu/modelsettingaction.h"

#include <qtusercore/util/settings.h>

#include "external/interface/commandinterface.h"
#include "external/interface/uiinterface.h"

namespace creative_kernel {

  namespace {
    static const auto KEY = QStringLiteral("ui_config/model_setting_tip_dialog_visible");
  }  // namespace

  ModelSettingAction::ModelSettingAction(QObject* parent) : ActionCommand{ parent } {
    setEnabled(true);
    onLanguageChanged({});
    addUIVisualTracer(this,this);
  }

  auto ModelSettingAction::execute() -> void {
    QMetaObject::invokeMethod(getUIAppWindow(),
                              "requestModelSettingPanel",
                              Q_ARG(QVariant, QVariant::fromValue<bool>(true)));

    if (qtuser_core::VersionSettings{}.value(KEY, QVariant::fromValue<bool>(true)).toBool()) {
      requestQmlDialog(this, QStringLiteral("model_setting_tip_dialog"));
    }
  }

  void ModelSettingAction::onTipDialogFinished(bool show_next_time) {
    qtuser_core::VersionSettings{}.setValue(KEY, QVariant::fromValue<bool>(show_next_time));
  }

  auto ModelSettingAction::onThemeChanged(ThemeCategory theme) -> void {
    std::ignore = theme;
  }

  auto ModelSettingAction::onLanguageChanged(MultiLanguage language) -> void {
    std::ignore = language;
    setName(tr("Object Process Setting"));
    sourceChanged();
  }

}  // namespace creative_kernel
