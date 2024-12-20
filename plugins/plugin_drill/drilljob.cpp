#include "drilljob.h"

#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"

#include "qtuser3d/trimesh2/conv.h"
#include "msbase/utils/drill.h"
#include "kernel/kernelui.h"

#include <QtCore/QCoreApplication>

#include "qtusercore/module/progressortracer.h"
#include "msbase/mesh/tinymodify.h"
#include "data/spaceutils.h"

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
    {}

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
  TriMeshPtr imesh_ = model_->createGlobalMeshWithNormal();

  imesh_->clear_bbox();
  imesh_->need_bbox();

  qtuser_core::ProgressorTracer tracer{ progressor };

  msbase::DrillParam param;
  memcpy(&param, &param_, sizeof(msbase::DrillParam));

  TriMeshPtr omesh_(msbase::drillCylinder(imesh_.get(), param, &tracer, nullptr, true));
  if (omesh_ == nullptr || omesh_->faces.size() <= 0) {
    return;
  }

  TriMeshPtr invertedMesh(new trimesh::TriMesh());
  *invertedMesh = *(omesh_.get());

  trimesh::apply_xform(invertedMesh.get(), trimesh::xform(trimesh::inv(xform)));

  trimesh::fxform normalXf = qtuser_3d::qMatrix2Xform(model_->modelNormalMatrix());

  creative_kernel::changeMeshFaceNormal(invertedMesh.get(), omesh_.get(), normalXf);

  trimesh::apply_xform(omesh_.get(), trimesh::xform(trimesh::inv(xform)));
  omesh_->clear_bbox();
  omesh_->need_bbox();

  change.model = model_;
  change.name = QStringLiteral("%1").arg(model_->name());
  change.mesh.reset(new cxkernel::MeshData(omesh_, false));

}

void DrillJob::successed(qtuser_core::Progressor* progressor) {
    if (!change.model)
        return;

    creative_kernel::replaceModelNMesh(change, true);
    getKernelUI()->setAutoResetOperateMode(true);
}

void DrillJob::failed() {
  getKernelUI()->requestQmlTipDialog(QCoreApplication::translate("info", "Drill failed"));
  creative_kernel::requestVisUpdate();
}
