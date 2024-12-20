#include "adaptivelayercommand.h"

#include <algorithm>

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "slice/adaptiveslice.h"
#include <interface/selectorinterface.h>
#include <data/modeln.h>

AdaptiveLayerCommand::AdaptiveLayerCommand(QObject *parent)
    : ToolCommand(parent)
    , operation_mode_(nullptr) {
      orderindex = 4;
      m_speedVal = 0.50;
      m_radius = 5;
      m_bKeepMin = false;
}

AdaptiveLayerCommand::~AdaptiveLayerCommand() {
  if (operation_mode_) {
    operation_mode_->deleteLater();
  }
}

bool AdaptiveLayerCommand::checkSelect() {
  return !creative_kernel::selectionms().empty();
}

void AdaptiveLayerCommand::execute() {
 
    if (!operation_mode_) {
        operation_mode_ = new AdaptiveLayerMode(this);
    }

    creative_kernel::setVisOperationMode(!operation_mode_->isPopupVisible() ? operation_mode_ : nullptr);
}
void AdaptiveLayerCommand::adapted()
{
    if (operation_mode_) {
        operation_mode_->adapted(m_speedVal);
    }
}

void AdaptiveLayerCommand::smooth()
{
    if (operation_mode_) {
        operation_mode_->smooth(m_radius,m_bKeepMin);
    }
}

void AdaptiveLayerCommand::reset()
{
    if (operation_mode_) {
        operation_mode_->reset();
    }
}