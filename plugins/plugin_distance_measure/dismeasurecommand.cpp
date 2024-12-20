#include "dismeasurecommand.h"

#include "interface/visualsceneinterface.h"

DistanceMeasureCommand::DistanceMeasureCommand(QObject* parent)
    : ToolCommand(parent),
		operate_mode_(nullptr) {
      orderindex = 8;
    }

DistanceMeasureCommand::~DistanceMeasureCommand() {
	if (operate_mode_) {
    operate_mode_->deleteLater();
  }
}

void DistanceMeasureCommand::execute() {
  if (!operate_mode_) {
    operate_mode_ = new DistanceMeasureOp;
  }
  creative_kernel::setVisOperationMode(operate_mode_);
}
