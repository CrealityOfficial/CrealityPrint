#include "hollowcommand.h"

#include <algorithm>

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"

HollowCommand::HollowCommand(QObject *parent)
    : ToolCommand(parent)
    , thinkness_(2.f)
    , operation_mode_(nullptr) {
      orderindex = 4;
  creative_kernel::addModelNSelectorTracer(this);
}

HollowCommand::~HollowCommand() {
  creative_kernel::removeModelNSelectorTracer(this);
  if (operation_mode_) {
    operation_mode_->deleteLater();
  }
}

bool HollowCommand::checkSelect() {
  return !creative_kernel::selectionms().empty();
}

void HollowCommand::execute() {
  if (!operation_mode_) {
    operation_mode_ = new HollowOperateMode;
  }

  creative_kernel::setVisOperationMode(operation_mode_->isPopupVisible() ? operation_mode_ : nullptr);
}

void HollowCommand::hollow(float thinkness, float fill_ratio, bool fill_enabled) {
  thinkness_ = thinkness;
  setFillRatio(fill_ratio);
  setFillEnabled(fill_enabled);

  operation_mode_->hollow(thinkness_);
}

float HollowCommand::getMinLengthInSelectionms() {
  float min_length{ -1.f };

  for (creative_kernel::ModelN* model : creative_kernel::selectionms()) {
    QVector3D size = model->globalSpaceBox().size();
    float min_length_temp = std::min({ size.x(), size.y(), size.z() });
    if (min_length_temp < min_length || min_length < 0.f) {
      min_length = min_length_temp;
    }
  }

  return min_length;
}

float HollowCommand::getThickness() const {
  return thinkness_;
}

//void HollowCommand::setThickness(float thinkness) {
//  thinkness_ = thinkness;
//  operation_mode_->hollow(thinkness);
//}

bool HollowCommand::isFillEnabled() const {
  return operation_mode_->isFillEnabled();
}

void HollowCommand::setFillEnabled(bool enabled) {
  operation_mode_->setFillEnabled(enabled);
}

float HollowCommand::getFillRatio() const {
  return operation_mode_->getFillRatio();
}

void HollowCommand::setFillRatio(float ratio) {
  operation_mode_->setFillRatio(ratio);
}

void HollowCommand::onSelectionsChanged() {
  Q_EMIT minLengthInSelectionmsChanged();
}
