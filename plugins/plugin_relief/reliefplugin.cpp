#include "reliefplugin.h"

#include <kernel/kernel.h>
#include <kernel/kernelui.h>
#include <interface/commandinterface.h>

ReliefPlugin::ReliefPlugin(QObject* parent) : QObject{ parent }, command_{ nullptr } {}

ReliefPlugin::~ReliefPlugin() {
  uninitialize();
}

auto ReliefPlugin::name() -> QString {
  return QStringLiteral("ReliefPlugin");
}

auto ReliefPlugin::info() -> QString {
  return name();
}

auto ReliefPlugin::initialize() -> void {
  if (!command_) {
    command_ = new ReliefCommand{ this };
  }

  creative_kernel::addUIVisualTracer(command_, this);
  getKernelUI()->addToolCommand(command_,
                                qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW,
                                qtuser_qml::ToolCommandType::RELIEF);
}

auto ReliefPlugin::uninitialize() -> void {
  if (command_) {
    command_->deleteLater();
  }

  getKernelUI()->removeToolCommand(command_, qtuser_qml::ToolCommandGroupType::LEFT_TOOLBAR_DRAW);
  creative_kernel::removeUIVisualTracer(command_);
}
