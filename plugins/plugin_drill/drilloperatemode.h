#pragma once
#include "data/interface.h"
#include <QVector3D>

#include "trimesh2/TriMesh.h"

#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/refactor/xentity.h"
#include "data/modeln.h"

class DrillOperateMode : public qtuser_3d::SceneOperateMode
    , public creative_kernel::ModelNSelectorTracer {
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

  virtual bool needHoldConfigPanel() { return true;  }

  virtual void onHoverMove(QHoverEvent* event) override;

  virtual void onLeftMouseButtonClick(QMouseEvent* event) override;

  virtual void onSelectionsChanged();

private:
  bool m_dirlling{ false };

  bool one_layer_only_;
  float radius_;
  float depth_;
  Shape shape_;
  Direction direction_;
  float m_bottomOffset = 12.0f;  // fix bug : [ID1027639] 打洞功能-选择方向为垂直屏幕时，圆形的指示器显示有缺失

  std::unique_ptr<qtuser_3d::XEntity> entity_;
};
