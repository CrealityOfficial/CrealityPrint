#pragma once

#include <QVector3D>

#include "trimesh2/TriMesh.h"

#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/refactor/xentity.h"
#include "data/modeln.h"
#include "qtuser3d/module/pickableselecttracer.h"

class DrillOperateMode : public qtuser_3d::SceneOperateMode, public qtuser_3d::SelectorTracer {
  Q_OBJECT;

public:
  explicit DrillOperateMode(QObject* parent = nullptr);
  virtual ~DrillOperateMode() = default;

public:
  bool isOneLayerOnly() const;
  void setOneLayerOnly(bool one_layer_only);

public:
  float getRadius() const;
  void setRadius(float radius);

public:
  float getDepth() const;
  void setDepth(float depth);

public:
  enum class Shape : int {
    CIRCLE   = 0,
    TRIANGLE = 1,
    SQUARE   = 2
  };

  auto getShape() const -> Shape;
  void setShape(Shape shape);

public:
  enum class Direction : int {
    NORMAL_DIRECTION        = 0,
    PARALLEL_TO_PLATFORM    = 1,
    PERPENDICULAR_TO_SCREEN = 2,
  };

  auto getDirection() const -> Direction;
  void setDirection(Direction direction);

protected:
  virtual void onAttach() override;
  virtual void onDettach() override;

  virtual void onHoverMove(QHoverEvent* event) override;

  virtual void onLeftMouseButtonClick(QMouseEvent* event) override;

  virtual void onSelectionsChanged();
  virtual void selectChanged(qtuser_3d::Pickable* pickable) {};

private:
  bool m_dirlling{ false };
  creative_kernel::ModelN* m_model { NULL };

  bool one_layer_only_;
  float radius_;
  float depth_;
  Shape shape_;
  Direction direction_;

  std::unique_ptr<qtuser_3d::XEntity> entity_;
  std::shared_ptr<trimesh::TriMesh> mesh_;
};
