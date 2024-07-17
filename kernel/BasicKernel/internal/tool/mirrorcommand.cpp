#include "mirrorcommand.h"

#include <QCoreApplication>

#include "interface/commandinterface.h"
#include "interface/visualsceneinterface.h"
#include "kernel/kernel.h"
#include "kernel/kernelui.h"

namespace creative_kernel {

MirrorCommand::MirrorCommand(QObject* parent)
    : ToolCommand(parent)
    // delay initlize the operate mode until the command first execution
    , operate_mode_(nullptr) {
  creative_kernel::addUIVisualTracer(this,this);
  orderindex = 2;
  onThemeChanged(creative_kernel::currentTheme()); 
  onLanguageChanged(creative_kernel::currentLanguage());
}

MirrorCommand::~MirrorCommand() {
  creative_kernel::removeUIVisualTracer(this);
}

void MirrorCommand::execute() {
  if (!operate_mode_) {
    operate_mode_ = std::make_unique<MirrorOperateMode>();
  }

  creative_kernel::setVisOperationMode(operate_mode_.get());
}

void MirrorCommand::onThemeChanged(ThemeCategory theme) {
  switch (theme) {
    case ThemeCategory::tc_dark:
      setDisabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/mirror_dark_disable.svg"));
      setEnabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/mirror_dark_default.svg"));
      setHoveredIcon(QStringLiteral("qrc:/UI/photo/cToolBar/mirror_dark_default.svg"));
      setPressedIcon(QStringLiteral("qrc:/UI/photo/cToolBar/mirror_dark_press.svg"));
      break;
    case ThemeCategory::tc_light:
      setDisabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/mirror_dark_disable.svg"));
      setEnabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/mirror_light_default.svg"));
      setHoveredIcon(QStringLiteral("qrc:/UI/photo/cToolBar/mirror_light_default.svg"));
      setPressedIcon(QStringLiteral("qrc:/UI/photo/cToolBar/mirror_dark_press.svg"));
      break;
    case ThemeCategory::tc_num:
      break;
    default:
      break;
  }
}

void MirrorCommand::onLanguageChanged(MultiLanguage language) {
  std::ignore = language;
  setName(QCoreApplication::translate("MirrorCommand", "Mirror")+": M");
}

}  // namespace creative_kernel
