#include "hollowoperatemode.h"

#include <QSharedPointer>

#include "interface/visualsceneinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/selectorinterface.h"
#include "interface/eventinterface.h"
#include "interface/spaceinterface.h"
#include "interface/modelinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/uiinterface.h"

#include "kernel/kernel.h"

#include "data/fdmsupportgroup.h"
#include "data/modelspace.h"
#include "data/modeln.h"

#include "hollowjob.h"

HollowOperateMode::HollowOperateMode(QObject* parent)
    : SceneOperateMode(parent)
    , unvisible_height_percent_(1.0f)
    , model_max_height_(0.0f)
    , fill_ratio_(0.5f)
    , fill_enabled_(false)
    , popup_visible_(false)
    , message_text_() {}

HollowOperateMode::~HollowOperateMode() {
  if (timer_.isActive()) {
    timer_.stop();
  }
}

void HollowOperateMode::onAttach() {
  bool has_support{ false };
  for (auto* model : creative_kernel::selectionms()) {
    if (model->hasFDMSupport()) {
      has_support = true;
      break;
    }
  }

  if (has_support) {
    setMessageText(tr("This operation will delete the support.Do you want to continue?"));
    creative_kernel::requestQmlDialog(this, "messageDlg");
  }

  popup_visible_ = true;
}

void HollowOperateMode::onDettach() {
  timer_.stop();
  creative_kernel::setModelEffectClipMaxZSceneTop();
  popup_visible_ = false;
}

void HollowOperateMode::jobFinished(bool successed) {
  if (successed) {
    using namespace std::chrono_literals;
    unvisible_height_percent_ = 1.0f;
    timer_.callOnTimeout(std::bind(&HollowOperateMode::timerUpdate, this));
    timer_.setSingleShot(false);
    timer_.setInterval(30ms);
    timer_.start();
  } else {

    resetModelsVisibility();
    creative_kernel::requestVisUpdate(true);
    setMessageText(tr("Hollow failed"));
    creative_kernel::requestQmlDialog("messageDlg");
  }
}

void HollowOperateMode::timerUpdate() {
  if (unvisible_height_percent_ > 0) {
    float max_z = model_max_height_ * unvisible_height_percent_;
    creative_kernel::setModelEffectClipMaxZ(max_z);
    unvisible_height_percent_ -= 0.03f;

  } else {
    timer_.stop();
    resetModelsVisibility();
    creative_kernel::setModelEffectClipMaxZSceneTop();
  }

  creative_kernel::requestVisUpdate(true);
}

void HollowOperateMode::hollow(float thinkness, int type) {
  auto selected_model_list = creative_kernel::selectionms();
  if (selected_model_list.empty()) {
    return;
  }
  
  for (auto* model: creative_kernel::modelns()) {
    if (!model->isVisible()) {
      unvisible_model_set_.emplace(model);
    }
  }

  recordModelsVisibility();
  setNoSelectedModelsVisiable(false);

  auto job = QSharedPointer<HollowJob>::create();
  connect(job.get(), &HollowJob::finished, this, &HollowOperateMode::jobFinished);
  job->setThickness(thinkness);
  job->setFillEnabled(fill_enabled_);
  job->setFillRatio(fill_ratio_);

  qtuser_3d::Box3D box;
  for (auto* model : selected_model_list) { // 选中的且可见的模型才打洞
    box += model->boxWithSup();

    if (model->hasFDMSupport()) {
        model->fdmSupport()->clearSupports();
    }
    if (model->isVisible()) {
      job->appendModel(model);
    }
  }

  creative_kernel::requestVisUpdate(true);
  model_max_height_ = box.max.z();
  cxkernel::executeJob(job);
}

void HollowOperateMode::setNoSelectedModelsVisiable(bool visiable)
{
  auto selected_model_list = creative_kernel::selectionms();
  auto model_list = creative_kernel::modelns();
  for (auto model: model_list) {
    if (!selected_model_list.contains(model)) {
      model->setVisibility(visiable);
    }
  }
}

void HollowOperateMode::recordModelsVisibility() {
  unvisible_model_set_.clear();
  for (auto* model: creative_kernel::modelns()) {
    if (!model->isVisible()) {
      unvisible_model_set_.emplace(model);
    }
  }
}

void HollowOperateMode::resetModelsVisibility() const {
  for (auto* model : creative_kernel::modelns()) {
    model->setVisibility(unvisible_model_set_.find(model) == unvisible_model_set_.cend());
  }
}

bool HollowOperateMode::isPopupVisible() {
  return popup_visible_;
}

void HollowOperateMode::accept() {
  for (auto* model : creative_kernel::selectionms()) {
    model->fdmSupport()->clearSupports();
  }
}

void HollowOperateMode::cancel() {
  //AbstractKernelUI::getSelf()->switchPickMode();
}

QString HollowOperateMode::getMessageText() {
  //return tr("This operation will delete the support.Do you want to continue?");
  return message_text_;
}

void HollowOperateMode::setMessageText(const QString& message) {
  message_text_ = message;
}

bool HollowOperateMode::isFillEnabled() const {
  return fill_enabled_;
}

void HollowOperateMode::setFillEnabled(bool fill) {
  fill_enabled_ = fill;
}

float HollowOperateMode::getFillRatio() const {
  return fill_ratio_;
}

void HollowOperateMode::setFillRatio(float ratio) {
  if (qFuzzyCompare(fill_ratio_, ratio)) {
    return;
  }

  fill_ratio_ = ratio;
}
