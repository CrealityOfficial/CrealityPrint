#include "hollowjob.h"

#include "interface/modelinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"

#include "data/fdmsupportgroup.h"
#include "data/modeln.h"

#include "qcxutil/trimesh2/conv.h"

#include "mmesh/trimesh/trimeshutil.h"

#include "utils/convexhullcalculator.h"

#include "qtusercore/module/progressortracer.h"

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
    std::make_shared<trimesh::TriMesh>(),
    std::make_shared<trimesh::TriMesh>(),
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

  for (auto& [model, imesh, omesh] : task_cache_list_) {
    if (model->hasFDMSupport()) {
      model->fdmSupport()->clearSupports();
    }

    try {
      trimesh::fxform xform = qcxutil::qMatrix2Xform(model->globalMatrix());
      *imesh = *model->meshptr(); // deep copy
      for (auto& vertiece : imesh->vertices) {
        vertiece = xform * vertiece;
      }

      imesh->clear_bbox();
      imesh->need_bbox();

      qtuser_core::ProgressorTracer tracer{ progressor };
      omesh.reset(ovdbutil::hollowMeshAndFill(imesh.get(), param, &tracer));
      if (!omesh) { throw "omesh is nullptr!"; }

      ConvexHullCalculator::calculate(omesh.get(), progressor);
      trimesh::apply_xform(omesh.get(), trimesh::xform{ trimesh::inv(xform) });
      omesh->need_bbox();

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
  QList<creative_kernel::ModelN*> model_list;
  QList<creative_kernel::TriMeshPtr> meshe_list;
  QList<QString> name_list;

  for (auto& [model, imesh, omesh] : task_cache_list_) {
    model_list.push_back(model);
    meshe_list.push_back(omesh);
    name_list.push_back(QStringLiteral("hollow_%1").arg(model->objectName()));
  }

  // cacheGeometry();
  creative_kernel::replaceModelsMesh(model_list, meshe_list, name_list, true);

  // clearCache();
  finished(true);
  creative_kernel::requestVisUpdate(true);
}