#include "hollowplugin.h"

#include <QCoreApplication>

#include "hollowcommand.h"
#include "kernel/kernel.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"

HollowPlugin::HollowPlugin(QObject* parent)
    : QObject(parent)
    , command_(nullptr) {}

HollowPlugin::~HollowPlugin() {
  if (command_) {
    if (command_->parent()) {
      uninitialize();
    }

    command_->deleteLater();
  }
}

QString HollowPlugin::name() {
  return QStringLiteral("HollowPlugin");
}

QString HollowPlugin::info() {
  return QStringLiteral("HollowPlugin");
}

void HollowPlugin::initialize() {
  if (!creative_kernel::getKernel()->globalConst()->isHollowEnabled()) {
    return;
  }

  command_ = new HollowCommand;
  command_->setSource(QStringLiteral("qrc:/hollow/hollow/info.qml"));
  onThemeChanged(creative_kernel::currentTheme());
  onLanguageChanged(creative_kernel::currentLanguage());
  getKernelUI()->addToolCommand(command_,
    qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_OTHER,
    qtuser_qml::ToolCommandType::HALLOW);
  creative_kernel::addUIVisualTracer(this,this);
}

void HollowPlugin::uninitialize() {
  getKernelUI()->removeToolCommand(command_, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_OTHER);
  creative_kernel::removeUIVisualTracer(this);
}

void HollowPlugin::onThemeChanged(creative_kernel::ThemeCategory theme) {
  if (!command_) {
    return;
  }

  switch (theme) {
    case creative_kernel::ThemeCategory::tc_dark:
      command_->setDisabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/shell_dark_disable.svg"));
      command_->setEnabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/shell_dark_default.svg"));
      command_->setHoveredIcon(QStringLiteral("qrc:/UI/photo/cToolBar/shell_dark_default.svg"));
      command_->setPressedIcon(QStringLiteral("qrc:/UI/photo/cToolBar/shell_dark_press.svg"));
      break;
    case creative_kernel::ThemeCategory::tc_light:
      command_->setDisabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/shell_dark_disable.svg"));
      command_->setEnabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/shell_light_default.svg"));
      command_->setHoveredIcon(QStringLiteral("qrc:/UI/photo/cToolBar/shell_light_default.svg"));
      command_->setPressedIcon(QStringLiteral("qrc:/UI/photo/cToolBar/shell_dark_press.svg"));
      break;
    case creative_kernel::ThemeCategory::tc_num:
      break;
    default:
      break;
  }
}

void HollowPlugin::onLanguageChanged(creative_kernel::MultiLanguage language) {
  if (!command_) {
    return;
  }

  command_->setName(QCoreApplication::translate("info", "Hollow"));
}
