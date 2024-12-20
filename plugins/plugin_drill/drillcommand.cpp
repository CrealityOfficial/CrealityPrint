#include "drillcommand.h"

#include "interface/visualsceneinterface.h"

DrillCommand::DrillCommand(QObject* parent)
    : ToolCommand(parent)
    , operate_mode_(nullptr) {
      orderindex = 5;
    }

DrillCommand::~DrillCommand() {
  if (operate_mode_) {
    operate_mode_->deleteLater();
  }
}

void DrillCommand::execute() {
  if (!operate_mode_) {
    operate_mode_ = new DrillOperateMode;
  }

  creative_kernel::setVisOperationMode(operate_mode_);
}

bool DrillCommand::isOneLayerOnly() const {
  if (!operate_mode_) { return false; }
  return operate_mode_->isOneLayerOnly();
}

void DrillCommand::setOneLayerOnly(bool one_layer_only) {
  if (!operate_mode_) { return; }
  if (one_layer_only != isOneLayerOnly()) {
    operate_mode_->setOneLayerOnly(one_layer_only);
    Q_EMIT oneLayerOnlyChanged();
  }
}

float DrillCommand::getRadius() const {
  if (!operate_mode_) { return 1; }
  return operate_mode_->getRadius();
}

void DrillCommand::setRadius(float radius) {
  if (!operate_mode_) { return; }
  if (radius != getRadius()) {
    operate_mode_->setRadius(radius);
    Q_EMIT radiusChanged();
  }
}

float DrillCommand::getDepth() const {
  if (!operate_mode_) { return 1; }
  return operate_mode_->getDepth();
}

void DrillCommand::setDepth(float depth) {
  if (!operate_mode_) { return; }
  if (depth != getDepth()) {
    operate_mode_->setDepth(depth);
    Q_EMIT depthChanged();
  }
}

int DrillCommand::getShape() const {
  if (!operate_mode_) { return 0; }
  return static_cast<int>(operate_mode_->getShape());
}

void DrillCommand::setShape(int shape) {
  if (!operate_mode_) { return; }
  if (shape != getShape()) {
    operate_mode_->setShape(static_cast<DrillOperateMode::Shape>(shape));
    Q_EMIT shapeChanged();
  }
}

int DrillCommand::getDirection() const {
  if (!operate_mode_) { return 0; }
  return static_cast<int>(operate_mode_->getDirection());
}

void DrillCommand::setDirection(int direction) {
  if (!operate_mode_) { return; }
  if (direction != getDirection()) {
    operate_mode_->setDirection(static_cast<DrillOperateMode::Direction>(direction));
    Q_EMIT directionChanged();
  }
}
