#pragma once

#include <QPointer>

#include "qtusercore/plugin/toolcommand.h"

#include "drilloperatemode.h"

class DrillCommand : public ToolCommand {
  Q_OBJECT;

public:
  explicit DrillCommand(QObject* parent = nullptr);
  virtual ~DrillCommand();

public:
  Q_INVOKABLE void execute();

public:
  Q_PROPERTY(bool oneLayerOnly READ isOneLayerOnly WRITE setOneLayerOnly NOTIFY oneLayerOnlyChanged);
  bool isOneLayerOnly() const;
  void setOneLayerOnly(bool one_layer_only);
  Q_SIGNAL void oneLayerOnlyChanged();

public:
  Q_PROPERTY(float radius READ getRadius WRITE setRadius NOTIFY radiusChanged);
  float getRadius() const;
  void setRadius(float radius);
  Q_SIGNAL void radiusChanged();

public:
  Q_PROPERTY(float depth READ getDepth WRITE setDepth NOTIFY depthChanged);
  float getDepth() const;
  void setDepth(float depth);
  Q_SIGNAL void depthChanged();

public:
  Q_PROPERTY(int shape READ getShape WRITE setShape NOTIFY shapeChanged);
  int getShape() const;
  void setShape(int shape);
  Q_SIGNAL void shapeChanged();

public:
  Q_PROPERTY(int direction READ getDirection WRITE setDirection NOTIFY directionChanged);
  int getDirection() const;
  void setDirection(int direction);
  Q_SIGNAL void directionChanged();

private:
  QPointer<DrillOperateMode> operate_mode_;
};
