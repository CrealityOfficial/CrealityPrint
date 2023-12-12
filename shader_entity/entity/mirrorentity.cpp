#include "mirrorentity.h"

#include "qtuser3d/geometry/basicshapecreatehelper.h"

namespace qtuser_3d {

namespace {

static const QVector4D RGBA_RED  { 1.0f   , 0.1098f, 0.1098f, 1.0f };
static const QVector4D RGBA_GREEN{ 0.2470f, 0.933f , 0.1098f, 1.0f };
static const QVector4D RGBA_BLUE { 0.4117f, 0.243f , 1.0f   , 1.0f };
static const QVector4D RGBA_GOLD { 1.0f   , 0.79f  , 0.0f   , 1.0f };

QMatrix4x4 CreateRotetedScaledMatrix(QVector3D&& to_direction) {
  static const QVector3D FROM_DIRECTION{ 0.0f, 1.0f, 0.0f };

  to_direction.normalize();
  
  QMatrix4x4 matrix;
  matrix.rotate(QQuaternion::rotationTo(FROM_DIRECTION, to_direction));
  matrix.scale(70.0f, 70.0f, 70.0f);

  return matrix;
}

} // namespace

MirrorEntity::MirrorEntity(QPointer<CameraController> camera_controller, Qt3DCore::QNode* parent)
    : XEntity(parent)
    , camera_controller_(camera_controller)
    , space_box_(QVector3D{ 0.0f, 0.0f, 0.0f })
    , transform_(std::make_unique<Qt3DCore::QTransform>(this))
    , geometry_([this]()->decltype(geometry_) {
      auto* raw_ptr = BasicShapeCreateHelper::createInstructions(0.0f, 0.75f, 0.075f, 0.3f, this);
      return decltype(geometry_){ raw_ptr };
    }())
    , x_positive_entity_(std::make_unique<qtuser_3d::ManipulateEntity>(this, ManipulateEntity::Pickable | ManipulateEntity::Overlay))
    , y_negative_entity_(std::make_unique<qtuser_3d::ManipulateEntity>(this, ManipulateEntity::Pickable | ManipulateEntity::Overlay))
    , z_positive_entity_(std::make_unique<qtuser_3d::ManipulateEntity>(this, ManipulateEntity::Pickable | ManipulateEntity::Overlay))
    , x_negative_entity_(std::make_unique<qtuser_3d::ManipulateEntity>(this, ManipulateEntity::Pickable | ManipulateEntity::Overlay))
    , y_positive_entity_(std::make_unique<qtuser_3d::ManipulateEntity>(this, ManipulateEntity::Pickable | ManipulateEntity::Overlay))
    , z_negative_entity_(std::make_unique<qtuser_3d::ManipulateEntity>(this, ManipulateEntity::Pickable | ManipulateEntity::Overlay))
    , x_positive_pickable_(std::make_shared<qtuser_3d::ManipulatePickable>(this))
    , x_negative_pickable_(std::make_shared<qtuser_3d::ManipulatePickable>(this))
    , y_positive_pickable_(std::make_shared<qtuser_3d::ManipulatePickable>(this))
    , y_negative_pickable_(std::make_shared<qtuser_3d::ManipulatePickable>(this))
    , z_positive_pickable_(std::make_shared<qtuser_3d::ManipulatePickable>(this))
    , z_negative_pickable_(std::make_shared<qtuser_3d::ManipulatePickable>(this)) {
  addComponent(transform_.get());

  x_positive_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{  1.0f,  0.0f,  0.0f }));
  x_negative_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{ -1.0f,  0.0f,  0.0f }));
  y_positive_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{  0.0f,  1.0f,  0.0f }));
  y_negative_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{  0.0f, -1.0f,  0.0f }));
  z_positive_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{  0.0f,  0.0f,  1.0f }));
  z_negative_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{  0.0f,  0.0f, -1.0f }));

  x_positive_entity_->setGeometry(geometry_.get());
  x_negative_entity_->setGeometry(geometry_.get());
  y_positive_entity_->setGeometry(geometry_.get());
  y_negative_entity_->setGeometry(geometry_.get());
  z_positive_entity_->setGeometry(geometry_.get());
  z_negative_entity_->setGeometry(geometry_.get());
  
  x_positive_entity_->setColor(RGBA_RED);
  x_negative_entity_->setColor(RGBA_RED);
  y_positive_entity_->setColor(RGBA_GREEN);
  y_negative_entity_->setColor(RGBA_GREEN);
  z_positive_entity_->setColor(RGBA_BLUE);
  z_negative_entity_->setColor(RGBA_BLUE);

  x_positive_entity_->setTriggeredColor(RGBA_GOLD);
  x_negative_entity_->setTriggeredColor(RGBA_GOLD);
  y_positive_entity_->setTriggeredColor(RGBA_GOLD);
  y_negative_entity_->setTriggeredColor(RGBA_GOLD);
  z_positive_entity_->setTriggeredColor(RGBA_GOLD);
  z_negative_entity_->setTriggeredColor(RGBA_GOLD);
  
  x_positive_entity_->setTriggerible(true);
  x_negative_entity_->setTriggerible(true);
  y_positive_entity_->setTriggerible(true);
  y_negative_entity_->setTriggerible(true);
  z_positive_entity_->setTriggerible(true);
  z_negative_entity_->setTriggerible(true);
  
  x_positive_pickable_->setPickableEntity(x_positive_entity_.get());
  x_negative_pickable_->setPickableEntity(x_negative_entity_.get());
  y_positive_pickable_->setPickableEntity(y_positive_entity_.get());
  y_negative_pickable_->setPickableEntity(y_negative_entity_.get());
  z_positive_pickable_->setPickableEntity(z_positive_entity_.get());
  z_negative_pickable_->setPickableEntity(z_negative_entity_.get());

  x_positive_pickable_->setEnableSelect(false);
  x_negative_pickable_->setEnableSelect(false);
  y_positive_pickable_->setEnableSelect(false);
  y_negative_pickable_->setEnableSelect(false);
  z_positive_pickable_->setEnableSelect(false);
  z_negative_pickable_->setEnableSelect(false);
}

QPointer<CameraController> MirrorEntity::getCameraController() const {
  return camera_controller_;
}

void MirrorEntity::setCameraController(QPointer<CameraController> camera_controller) {
  camera_controller_ = camera_controller;
}

Box3D MirrorEntity::getSpaceBox() const {
  return space_box_;
}

void MirrorEntity::setSpaceBox(const Box3D& space_box) {
  if (space_box_ == space_box) { return; }

  space_box_ = space_box;
  
  QMatrix4x4 matrix;
  matrix.translate(space_box_.center());
  transform_->setMatrix(matrix);
}

float MirrorEntity::getScaleFactor() const {
  return transform_->scale();
}

void MirrorEntity::setScaleFactor(float scale_factor) {
  transform_->setScale(scale_factor);
}

std::weak_ptr<Pickable> MirrorEntity::xPositivePickable() const {
  return std::dynamic_pointer_cast<Pickable>(x_positive_pickable_);
}

std::weak_ptr<Pickable> MirrorEntity::xNegativePickable() const {
  return std::dynamic_pointer_cast<Pickable>(x_negative_pickable_);
}

std::weak_ptr<Pickable> MirrorEntity::yPositivePickable() const {
  return std::dynamic_pointer_cast<Pickable>(y_positive_pickable_);
}

std::weak_ptr<Pickable> MirrorEntity::yNegativePickable() const {
  return std::dynamic_pointer_cast<Pickable>(y_negative_pickable_);
}

std::weak_ptr<Pickable> MirrorEntity::zPositivePickable() const {
  return std::dynamic_pointer_cast<Pickable>(z_positive_pickable_);
}

std::weak_ptr<Pickable> MirrorEntity::zNegativePickable() const {
  return std::dynamic_pointer_cast<Pickable>(z_negative_pickable_);
}

void MirrorEntity::attach() {
  if (camera_controller_ != nullptr) {
    connect(camera_controller_, &CameraController::signalCameraChaged,
            this , &MirrorEntity::onCameraChanged, Qt::ConnectionType::UniqueConnection);
    onCameraChanged(camera_controller_->getViewPosition(), camera_controller_->getViewupVector());
  }
}

void MirrorEntity::detach() {
  if (camera_controller_ != nullptr) {
    camera_controller_->disconnect(this);
    this->disconnect(camera_controller_);
  }
}

void MirrorEntity::onCameraChanged(QVector3D position, QVector3D up_vector) {
  std::ignore = up_vector;

  x_positive_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{  position.x(), 0.0f, 0.0f }));
  x_negative_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{ -position.x(), 0.0f, 0.0f }));
  y_positive_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{ 0.0f,  position.y(), 0.0f }));
  y_negative_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{ 0.0f, -position.y(), 0.0f }));
  z_positive_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{ 0.0f, 0.0f,  position.z() }));
  z_negative_entity_->setPose(CreateRotetedScaledMatrix(QVector3D{ 0.0f, 0.0f, -position.z() }));
}

}  // namespace qtuser_3d
