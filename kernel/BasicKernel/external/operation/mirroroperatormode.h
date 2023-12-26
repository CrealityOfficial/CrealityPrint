#pragma once

#include <map>
#include <memory>

#include "basickernelexport.h"
#include "data/modeln.h"
#include "qtuser3d/module/pickable.h"
#include "qtuser3d/module/pickableselecttracer.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "entity/mirrorentity.h"
#include <QTimer>
#include "operation/moveoperatemode.h"

class BASIC_KERNEL_API MirrorOperateMode : public MoveOperateMode
                                         , public qtuser_3d::SelectorTracer {
  Q_OBJECT;

public:
  explicit MirrorOperateMode(QObject* parent = nullptr);
	virtual ~MirrorOperateMode() = default;

private:
  void updateEntity();

protected:
  virtual void onAttach() override;
  virtual void onDettach() override;
  
  virtual void onWheelEvent(QWheelEvent* event) override;

  virtual void onLeftMouseButtonClick(QMouseEvent* event) override;

protected:
  virtual void onSelectionsChanged() override;
  virtual void selectChanged(qtuser_3d::Pickable* pickable) override;

private:
  QTimer m_operateTimer;
  bool m_canOperate { true };
  std::unique_ptr<qtuser_3d::MirrorEntity> entity_;
  std::map<QPointer<qtuser_3d::Pickable>, std::function<void(void)>> pickable_callback_map_;
};
