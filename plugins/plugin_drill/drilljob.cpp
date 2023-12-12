#include "drilljob.h"

#include "interface/modelinterface.h"
#include "interface/visualsceneinterface.h"

#include "qtuser3d/trimesh2/conv.h"
#include "msbase/utils/drill.h"

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
    , data_(nullptr) {}

creative_kernel::ModelN* DrillJob::getModel() const {
  return model_;
}

void DrillJob::setModel(creative_kernel::ModelN* model) {
  model_ = model;
}

void DrillJob::setParam(const WrapDrillParam& param) {
  param_ = param;
}

QString DrillJob::name() {
  return QStringLiteral("DrillJob");
}

QString DrillJob::description() {
  return name();
}

void DrillJob::work(qtuser_core::Progressor* progressor) {
  trimesh::fxform xform = qtuser_3d::qMatrix2Xform(model_->globalMatrix());

  imesh_ = std::make_shared<trimesh::TriMesh>(*model_->meshptr());
  for (auto& vertiece : imesh_->vertices) {
    vertiece = xform * vertiece;
  }
  imesh_->clear_bbox();
  imesh_->need_bbox();

  qtuser_core::ProgressorTracer tracer{ progressor };

  msbase::DrillParam param;
  memcpy(&param, &param_, sizeof(msbase::DrillParam));

  TriMeshPtr omesh_(msbase::drillCylinder(imesh_.get(), param, &tracer, nullptr, true));
  if (omesh_ == nullptr) {
    return;
  }

  trimesh::apply_xform(omesh_.get(), trimesh::xform(trimesh::inv(xform)));
  omesh_->clear_bbox();
  omesh_->need_bbox();

  data_ = cxkernel::createModelNData(omesh_, model_->objectName(), cxkernel::ModelNDataType::mdt_algrithm);
}

void DrillJob::successed(qtuser_core::Progressor* progressor) {
  auto newModels = creative_kernel::replaceModelsMesh({ model_ }, { data_ }, true);
  creative_kernel::requestVisUpdate();
  Q_EMIT finished(newModels.empty() ? NULL : newModels.first());
}

void DrillJob::failed() {
  creative_kernel::requestVisUpdate();
  Q_EMIT finished(NULL);
}
