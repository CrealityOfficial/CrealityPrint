#include "mirroroperatormode.h"

#include "external/interface/modelinterface.h"
#include "interface/camerainterface.h"
#include "interface/eventinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "kernel/kernel.h"
#include "kernel/kernelui.h"
#include "qtuser3d/module/pickable.h"
#include "external/kernel/modelnselector.h"
#include "interface/visualsceneinterface.h"
#include "cxkernel/data/header.h"
#include "qtusercore/module/job.h"
#include "trimesh2/TriMesh.h"

namespace {

float CaculateCurrentScaleFactor() {
  static constexpr auto ORIGIN_FIELD_OF_VIEW{ 60.0f };
  return creative_kernel::getCachedCameraEntity()->fieldOfView() / ORIGIN_FIELD_OF_VIEW;
}

template<typename _Type>
_Type* WeakToRaw(std::weak_ptr<_Type> _value) noexcept {
  return _value.lock().get();
}

const auto X_MIRROR_HANDLER = std::bind(creative_kernel::mirrorXSelections, true);
const auto Y_MIRROR_HANDLER = std::bind(creative_kernel::mirrorYSelections, true);
const auto Z_MIRROR_HANDLER = std::bind(creative_kernel::mirrorZSelections, true);

} // namespace

MirrorOperateMode::MirrorOperateMode(QObject* parent)
    : MoveOperateMode(parent)
    , entity_(std::make_unique<qtuser_3d::MirrorEntity>(creative_kernel::cameraController()))
    , pickable_callback_map_({
      { WeakToRaw(entity_->xPositivePickable()), X_MIRROR_HANDLER },
      { WeakToRaw(entity_->xNegativePickable()), X_MIRROR_HANDLER },
      { WeakToRaw(entity_->yPositivePickable()), Y_MIRROR_HANDLER },
      { WeakToRaw(entity_->yNegativePickable()), Y_MIRROR_HANDLER },
      { WeakToRaw(entity_->zPositivePickable()), Z_MIRROR_HANDLER },
      { WeakToRaw(entity_->zNegativePickable()), Z_MIRROR_HANDLER },
    }) {
  
	    m_type = qtuser_3d::SceneOperateMode::FixedMode;

  for (const auto& pair : pickable_callback_map_) {
    creative_kernel::tracePickable(pair.first);
  }

  // connect(this, &MoveOperateMode::moving, this, [=] ()
  // {
  //   updateEntity();
  // });
}

void MirrorOperateMode::updateEntity()
{
  const auto selected_model_list = creative_kernel::selectionms();
  if (!selected_model_list.empty()) {
      float minX, minY, minZ, maxX, maxY, maxZ;
      minX = minY = minZ = 100000;
      maxX = maxY = maxZ = -100000;
      for (auto m : selected_model_list)
      {
        auto box = m->boxWithSup();
        minX = box.min.x() < minX ? box.min.x() : minX;
        minY = box.min.y() < minY ? box.min.y() : minY;
        minZ = box.min.z() < minZ ? box.min.z() : minZ;
        maxX = box.max.x() > maxX ? box.max.x() : maxX;
        maxY = box.max.y() > maxY ? box.max.y() : maxY;
        maxZ = box.max.z() > maxZ ? box.max.z() : maxZ;
      }
      qtuser_3d::Box3D box(QVector3D(minX, minY, minZ), QVector3D(maxX, maxY, maxZ));

      // entity_->setSpaceBox(selected_model_list.last()->globalSpaceBox());
      entity_->setSpaceBox(box);
      entity_->setScaleFactor(CaculateCurrentScaleFactor());
      creative_kernel::visShow(entity_.get());
  }
  else {
      creative_kernel::visHide(entity_.get());
  }
  creative_kernel::requestVisUpdate(true);
}

void MirrorOperateMode::onAttach() {
  entity_->attach();
  
  creative_kernel::addSelectTracer(this);
  creative_kernel::addWheelEventHandler(this);
  creative_kernel::addLeftMouseEventHandler(this);

  updateEntity();
}

void MirrorOperateMode::onDettach() {
  entity_->detach();
  
  creative_kernel::removeSelectorTracer(this);
  creative_kernel::removeWheelEventHandler(this);
  creative_kernel::removeLeftMouseEventHandler(this);

  if (entity_->parentEntity() != nullptr) {
    creative_kernel::visHide(entity_.get());
    creative_kernel::requestVisUpdate();
  }
}

void MirrorOperateMode::onWheelEvent(QWheelEvent* event) {
  std::ignore = event;

  entity_->setScaleFactor(CaculateCurrentScaleFactor());
}

void MirrorOperateMode::onLeftMouseButtonClick(QMouseEvent* event) {
  auto iter = pickable_callback_map_.find(creative_kernel::checkPickable(event->pos(), nullptr));
  if (iter == pickable_callback_map_.cend()) 
  { 
    return; 
  }

  iter->second();

}

void MirrorOperateMode::onSelectionsChanged() {
  auto selections = creative_kernel::selectionms();
  updateEntity();
  creative_kernel::requestVisPickUpdate(true);
}

void MirrorOperateMode::selectChanged(qtuser_3d::Pickable* pickable) {
  std::ignore = pickable;

  updateEntity();
}
