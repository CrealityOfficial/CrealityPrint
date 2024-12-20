#pragma once

#include <memory>

#include <QNode>
#include <QPointer>
#include <QGeometry>
#include <QTransform>

#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/refactor/xentity.h"
#include "entity/manipulateentity.h"
#include "qtuser3d/module/manipulatepickable.h"
#include "qtuser3d/math/box3d.h"
#include "shader_entity_export.h"
#include "qtuser3d/camera/screencamera.h"

namespace qtuser_3d {

class SHADER_ENTITY_API MirrorEntity : public XEntity, public qtuser_3d::ScreenCameraObserver {
  Q_OBJECT;

public:
  explicit MirrorEntity(QPointer<CameraController> camera_controller = nullptr,
                        Qt3DCore::QNode* parent = nullptr);
  virtual ~MirrorEntity() = default;

public:
  QPointer<CameraController> getCameraController() const;
  void setCameraController(QPointer<CameraController> camera_controller);

public:
  Box3D getSpaceBox() const;
  void setSpaceBox(const Box3D& space_box);

protected:
  float getScaleFactor() const;
  void setScaleFactor(float scale_factor);

  void onCameraChanged(ScreenCamera* camera) override;

public:
  std::weak_ptr<Pickable> xPositivePickable() const;
  std::weak_ptr<Pickable> xNegativePickable() const;

  std::weak_ptr<Pickable> yPositivePickable() const;
  std::weak_ptr<Pickable> yNegativePickable() const;

  std::weak_ptr<Pickable> zPositivePickable() const;
  std::weak_ptr<Pickable> zNegativePickable() const;

public:
  void attach();
  void detach();

protected:
    void adaptScale();

private:
  Q_SLOT void onCameraChanged(QVector3D position, QVector3D up_vector);

private:
  QPointer<CameraController> camera_controller_;
  Box3D space_box_;

  std::unique_ptr<Qt3DCore::QTransform> transform_;
  std::unique_ptr<Qt3DRender::QGeometry> geometry_;
  
  std::unique_ptr<ManipulateEntity> x_positive_entity_;
  std::unique_ptr<ManipulateEntity> x_negative_entity_;
  std::unique_ptr<ManipulateEntity> y_positive_entity_;
  std::unique_ptr<ManipulateEntity> y_negative_entity_;
  std::unique_ptr<ManipulateEntity> z_positive_entity_;
  std::unique_ptr<ManipulateEntity> z_negative_entity_;

  std::shared_ptr<ManipulatePickable> x_positive_pickable_;
  std::shared_ptr<ManipulatePickable> x_negative_pickable_;
  std::shared_ptr<ManipulatePickable> y_positive_pickable_;
  std::shared_ptr<ManipulatePickable> y_negative_pickable_;
  std::shared_ptr<ManipulatePickable> z_positive_pickable_;
  std::shared_ptr<ManipulatePickable> z_negative_pickable_;
};

}  // namespace qtuser_3d
