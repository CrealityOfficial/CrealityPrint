#include "drilljob.h"

#include "interface/modelinterface.h"
#include "interface/visualsceneinterface.h"

#include "qcxutil/trimesh2/conv.h"

#include "mmesh/util/drill.h"

#include "qtusercore/module/progressortracer.h"

DrillJob::DrillJob(QObject* parent)
    : qtuser_core::Job(parent)
    , model_(nullptr)
    , param_({
      0,
      0.0,
      0.0,
      { 0.0f, 0.0f, 0.0f },
      { 0.0f, 0.0f, 0.0f },
    })
    , imesh_(nullptr)
    , omesh_(nullptr) {}

creative_kernel::ModelN* DrillJob::getModel() const {
  return model_;
}

void DrillJob::setModel(creative_kernel::ModelN* model) {
  model_ = model;
}

mmesh::DrillParam DrillJob::getParam() const {
  return param_;
}

void DrillJob::setParam(const mmesh::DrillParam& param) {
  param_ = param;
}

QString DrillJob::name() {
  return QStringLiteral("DrillJob");
}

QString DrillJob::description() {
  return name();
}

void DrillJob::work(qtuser_core::Progressor* progressor) {
  trimesh::fxform xform = qcxutil::qMatrix2Xform(model_->globalMatrix());

  imesh_ = std::make_shared<trimesh::TriMesh>(*model_->meshptr());
  for (auto& vertiece : imesh_->vertices) {
    vertiece = xform * vertiece;
  }
  imesh_->clear_bbox();
  imesh_->need_bbox();

  qtuser_core::ProgressorTracer tracer{ progressor };
  omesh_.reset(mmesh::drillCylinder(imesh_.get(), param_, &tracer, nullptr, true));
  if (omesh_ == nullptr) {
    return;
  }

  trimesh::apply_xform(omesh_.get(), trimesh::xform(trimesh::inv(xform)));
  omesh_->clear_bbox();
  omesh_->need_bbox();
}

void DrillJob::successed(qtuser_core::Progressor* progressor) {
  creative_kernel::replaceModelsMesh({ model_ }, { omesh_ }, { model_->objectName() }, true);
  creative_kernel::requestVisUpdate();
  Q_EMIT finished(true);
}

void DrillJob::failed() {
  creative_kernel::requestVisUpdate();
  Q_EMIT finished(false);
}
