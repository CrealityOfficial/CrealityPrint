#include "drillplugin.h"

#include <QCoreApplication>

#include "drillcommand.h"
#include "kernel/kernel.h"
#include "kernel/kernelui.h"
#include "interface/commandinterface.h"

DrillPlugin::DrillPlugin(QObject* parent)
    : QObject(parent)
    , command_(nullptr) {}

DrillPlugin::~DrillPlugin() {
  if (command_) {
    if (command_->parent()) {
      uninitialize();
    }

    command_->deleteLater();
  }
}

QString DrillPlugin::name() {
  return QStringLiteral("DrillPlugin");
}

QString DrillPlugin::info() {
  return QStringLiteral("DrillPlugin");
}

void DrillPlugin::initialize() {
  if (!creative_kernel::getKernel()->globalConst()->isDrillable()) {
    return;
  }

  command_ = new DrillCommand;
  command_->setSource(QStringLiteral("qrc:/drill/drill/info.qml"));
  onThemeChanged(creative_kernel::currentTheme());
  onLanguageChanged(creative_kernel::currentLanguage());
  getKernelUI()->addToolCommand(command_,
    qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_OTHER,
    qtuser_qml::ToolCommandType::DRILL);
  creative_kernel::addUIVisualTracer(this,this);
}

void DrillPlugin::uninitialize() {
  getKernelUI()->removeToolCommand(command_, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_OTHER);
  creative_kernel::removeUIVisualTracer(this);
}

void DrillPlugin::onThemeChanged(creative_kernel::ThemeCategory theme) {
  if (!command_) {
    return;
  }

  switch (theme) {
    case creative_kernel::ThemeCategory::tc_dark:
      command_->setDisabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/drill_dark_disable.svg"));
      command_->setEnabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/drill_dark_default.svg"));
      command_->setHoveredIcon(QStringLiteral("qrc:/UI/photo/cToolBar/drill_dark_default.svg"));
      command_->setPressedIcon(QStringLiteral("qrc:/UI/photo/cToolBar/drill_dark_press.svg"));
      break;
    case creative_kernel::ThemeCategory::tc_light:
      command_->setDisabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/drill_dark_disable.svg"));
      command_->setEnabledIcon(QStringLiteral("qrc:/UI/photo/cToolBar/drill_light_default.svg"));
      command_->setHoveredIcon(QStringLiteral("qrc:/UI/photo/cToolBar/drill_light_default.svg"));
      command_->setPressedIcon(QStringLiteral("qrc:/UI/photo/cToolBar/drill_dark_press.svg"));
      break;
    case creative_kernel::ThemeCategory::tc_num:
      break;
    default:
      break;
  }
}

void DrillPlugin::onLanguageChanged(creative_kernel::MultiLanguage language) {
  if (!command_) {
    return;
  }

  command_->setName(QCoreApplication::translate("info", "Drill"));
}
