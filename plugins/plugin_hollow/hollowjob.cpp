#include "hollowjob.h"

#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"

#include "qtuser3d/trimesh2/conv.h"

#include "qtusercore/module/progressortracer.h"
#include "data/spaceutils.h"

// VDB
#include "ovdbutil/hollowing.h"
#include "ovdbutil/init.h"

const float HollowJob::FILL_RATIO_MIN = 0.05f;
const float HollowJob::FILL_RATIO_MAX = 1.0f;

HollowJob::HollowJob(QObject* parent)
    : qtuser_core::Job(parent)
    , thickness_(0.0f)
    , fill_enabled_(false)
    , fill_ratio_(0.5f) {}

HollowJob::~HollowJob() {}

void HollowJob::appendModel(creative_kernel::ModelN* model) {
  task_cache_list_.emplace_back(task_cache_t{
    model,
    std::make_shared<cxkernel::MeshData>(),
  });
}

QString HollowJob::name() { return QStringLiteral("HollowJob"); }

QString HollowJob::description() { return QStringLiteral("HollowJob"); }

float HollowJob::getTihckness() const { return thickness_; }

void HollowJob::setThickness(float thickness) { thickness_ = thickness; }

bool HollowJob::isFillEnabled() const { return fill_enabled_; }

void HollowJob::setFillEnabled(bool enabled) {
  fill_enabled_ = enabled;
}

float HollowJob::getFillRatio() const { return fill_ratio_; }

void HollowJob::setFillRatio(float ratio) {
  if (ratio < FILL_RATIO_MIN || ratio > FILL_RATIO_MAX) {
    return;
  }

  fill_ratio_ = ratio;
}

void HollowJob::work(qtuser_core::Progressor* progressor) {
  ovdbutil::HollowingParameter param;
  param.min_thickness          = thickness_;
  param.quality                = 0.5;
  param.closing_distance       = 0.0;
  param.voxel_size_inout_range = 1.0;
  param.voxel_size             = 1.0;
  param.fill_config.enable     = fill_enabled_;
  param.fill_config.fillratio  = fill_ratio_;

  for (auto& [model, odata] : task_cache_list_) {
    try {
      trimesh::fxform xform = qtuser_3d::qMatrix2Xform(model->globalMatrix());
      TriMeshPtr imesh = model->createGlobalMeshWithNormal();

      imesh->clear_bbox();
      imesh->need_bbox();

      qtuser_core::ProgressorTracer tracer{ progressor };
      TriMeshPtr omesh(ovdbutil::hollowMeshAndFill(imesh.get(), param, &tracer));
      if (!omesh) { throw "omesh is nullptr!"; }

      TriMeshPtr invertedMesh(new trimesh::TriMesh());
      *invertedMesh = *(omesh.get());

      trimesh::apply_xform(invertedMesh.get(), trimesh::xform(trimesh::inv(xform)));

      trimesh::fxform normalXf = qtuser_3d::qMatrix2Xform(model->modelNormalMatrix());

      creative_kernel::changeMeshFaceNormal(invertedMesh.get(), omesh.get(), normalXf);

      trimesh::apply_xform(omesh.get(), trimesh::xform{ trimesh::inv(xform) });
      omesh->need_bbox();

      odata.reset(new cxkernel::MeshData(omesh, false));
    } catch (...) {
      assert(false && "exception catched!");
      progressor->failed("HollowJob failed!");
    }
  }
}

void HollowJob::failed() {
  qDebug() << "HollowJob::failed()";
  finished(false);
  creative_kernel::requestVisUpdate(true);
}

void HollowJob::successed(qtuser_core::Progressor* progressor) {

    QList<creative_kernel::ModelNPropertyMeshChange> changes;
  for (auto& [model, odata] : task_cache_list_) {
      if (model && odata)
      {
          creative_kernel::ModelNPropertyMeshChange change;
          change.model = model;
          change.name = QStringLiteral("%1").arg(model->name());
          change.mesh = odata;
          changes.append(change);
      }
  }
  creative_kernel::replaceModelNMeshes(changes, true);

  finished(true);
  creative_kernel::requestVisUpdate(true);
}