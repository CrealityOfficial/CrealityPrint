#include "drilloperatemode.h"

#include <QCoreApplication>

#include "kernel/kernelui.h"

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/eventinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "qtuser3d/camera/cameracontroller.h"

#include "qtuser3d/trimesh2/conv.h"

#include "msbase/primitive/primitive.h"

#include "drilljob.h"
#include "renderpass/phongrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"

namespace {

float ShapeToSideNumber(DrillOperateMode::Shape shape) {
  float side_number;

  switch (shape) {
    case DrillOperateMode::Shape::TRIANGLE:
      side_number = 3;
      break;
    case DrillOperateMode::Shape::SQUARE:
      side_number = 4;
      break;
    case DrillOperateMode::Shape::CIRCLE:
    default:
      side_number = 20;
      break;
  }

  return side_number;
}

void SyncDirectionToVector3D(DrillOperateMode::Direction direction, QVector3D& vector_3d) {
  switch (direction) {
    case DrillOperateMode::Direction::NORMAL_DIRECTION:
      break;
    case DrillOperateMode::Direction::PARALLEL_TO_PLATFORM:
      vector_3d.setZ(0);
      break;
    case DrillOperateMode::Direction::PERPENDICULAR_TO_SCREEN:
      vector_3d = creative_kernel::cameraController()->getViewDir();
    default:
      break;
  }
}

} // namespace

DrillOperateMode::DrillOperateMode(QObject* parent)
    : qtuser_3d::SceneOperateMode(parent)
    , one_layer_only_(true)
    , radius_(5.0f)
    , depth_(50.0f)
    , shape_(Shape::CIRCLE)
    , direction_(Direction::NORMAL_DIRECTION)
    , mesh_(nullptr)
    , entity_([]()->decltype(entity_) {
      auto entity = std::make_unique<qtuser_3d::XEntity>();

      qtuser_3d::PhongRenderPass*renderPass = new qtuser_3d::PhongRenderPass(entity.get());
      renderPass->addFilterKeyMask("view", 0);

      qtuser_3d::XEffect* effect = new qtuser_3d::XEffect(entity.get());
      effect->addRenderPass(renderPass);

      entity->setEffect(effect);

      entity->setParameter("color", QVector4D(1.0, 0.753, 0.0, 1.0));
      return entity;
    }()) {}

bool DrillOperateMode::isOneLayerOnly() const {
  return one_layer_only_;
}

void DrillOperateMode::setOneLayerOnly(bool one_layer_only) {
  one_layer_only_ = one_layer_only;
}

float DrillOperateMode::getRadius() const {
  return radius_;
}

void DrillOperateMode::setRadius(float radius) {
  radius_ = radius;
}

float DrillOperateMode::getDepth() const {
  return depth_;
}

void DrillOperateMode::setDepth(float depth) {
  depth_ = depth;
}

auto DrillOperateMode::getShape() const -> Shape {
  return shape_;
}

void DrillOperateMode::setShape(Shape shape) {
  shape_ = shape;
}

auto DrillOperateMode::getDirection() const -> Direction {
  return direction_;
}

void DrillOperateMode::setDirection(Direction direction) {
  direction_ = direction;
}

void DrillOperateMode::onAttach() {
  auto selections = creative_kernel::selectionms();
  if (selections.empty())
    return;

  m_model = selections.first();

  creative_kernel::addSelectTracer(this);
  creative_kernel::addHoverEventHandler(this);
  creative_kernel::prependLeftMouseEventHandler(this);
}

void DrillOperateMode::onDettach() {
  creative_kernel::removeHoverEventHandler(this);
  creative_kernel::removeLeftMouseEventHandler(this);
  creative_kernel::visHide(entity_.get());
  creative_kernel::requestVisUpdate();
  creative_kernel::removeSelectorTracer(this);

  m_model = NULL;
}

void DrillOperateMode::onHoverMove(QHoverEvent* event) {
  creative_kernel::visHide(entity_.get());

  if (m_model == NULL)
    return;

  if (creative_kernel::selectionms().empty()) {
    return;
  }

  QVector3D position_3d{ 0.0f, 0.0f, 0.0f };
  QVector3D direction_3d{ 0.0f, 0.0f, 0.0f };
  auto* model = creative_kernel::checkPickModel(event->pos(), position_3d, direction_3d);
  if (model == nullptr) {
    return;
  }

  SyncDirectionToVector3D(direction_, direction_3d);
  //direction_3d = qtuser_3d::vec2qvector(trimesh::normalized(qtuser_3d::qVector3D2Vec3(direction_3d)));
  direction_3d.normalize();

	QVector3D bottom = position_3d + 2.0f * direction_3d;
	QVector3D top = bottom + (depth_ + 2.0f) * -direction_3d;

	mesh_.reset(msbase::createCylinderMesh(qtuser_3d::qVector3D2Vec3(top),
        qtuser_3d::qVector3D2Vec3(bottom),
                                                      radius_,
                                                      ShapeToSideNumber(shape_)));

	entity_->setGeometry(creative_kernel::createGeometryFromMesh(mesh_.get()));

  creative_kernel::visShow(entity_.get());
  creative_kernel::requestVisUpdate();
}

void DrillOperateMode::onLeftMouseButtonClick(QMouseEvent* event) {
  if (m_model == NULL)
    return;

  if (entity_->parentEntity() == nullptr) {
    return;
  }

  if (!cxkernel::jobExecutorAvaillable()) {
    return;
  }

  QVector3D position_3d{ 0.0f, 0.0f, 0.0f };
  QVector3D direction_3d{ 0.0f, 0.0f, 0.0f };
  auto* model = creative_kernel::checkPickModel(event->pos(), position_3d, direction_3d);
  if (model == nullptr) {
    return;
  }

  SyncDirectionToVector3D(direction_, direction_3d);

  WrapDrillParam param;
  param.cylinder_resolution = ShapeToSideNumber(shape_);
  param.cylinder_radius     = radius_;
  param.cylinder_depth      = one_layer_only_ ? -1 : depth_;
  param.cylinder_startPos   = qtuser_3d::qVector3D2Vec3(position_3d);
  param.cylinder_Dir        = -trimesh::normalized(qtuser_3d::qVector3D2Vec3(direction_3d));

  auto job = QSharedPointer<DrillJob>::create();
  job->setModel(model);
  job->setParam(param);
  connect(job.get(), &DrillJob::finished, this, [=](creative_kernel::ModelN* newModel) {
    m_dirlling = false;
    if (newModel != NULL) 
    { 
      creative_kernel::selectOne(newModel);
      return; 
    }
    getKernelUI()->requestQmlTipDialog(QCoreApplication::translate("info", "Drill failed"));
  });

  m_dirlling = true;
  cxkernel::executeJob(job);
}

void DrillOperateMode::onSelectionsChanged()
{
  if (m_dirlling)
    return;

  auto selections = creative_kernel::selectionms();
  if (selections.empty())
  {
    creative_kernel::visHide(entity_.get());
    m_model = NULL;
    return;
  } 

  m_model = selections.first();
  
}