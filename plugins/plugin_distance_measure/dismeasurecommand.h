#pragma once

#include <QPointer>
#include <QVector3D>

#include "qtusercore/plugin/toolcommand.h"
#include "dismeasureop.h"

class DistanceMeasureCommand : public ToolCommand {
  Q_OBJECT;

public:
  explicit DistanceMeasureCommand(QObject* parent = nullptr);
  virtual ~DistanceMeasureCommand();

public:
  Q_INVOKABLE void execute();

protected:
  QPointer<DistanceMeasureOp> operate_mode_;
};
