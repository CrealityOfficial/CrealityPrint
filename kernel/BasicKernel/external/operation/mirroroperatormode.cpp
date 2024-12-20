#include "mirroroperatormode.h"

#include "interface/camerainterface.h"
#include "interface/eventinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"

namespace {

template<typename _Type>
_Type* WeakToRaw(std::weak_ptr<_Type> _value) noexcept {
  return _value.lock().get();
}

const auto X_MIRROR_HANDLER = std::bind(creative_kernel::mirrorXSelections, true);
const auto Y_MIRROR_HANDLER = std::bind(creative_kernel::mirrorYSelections, true);
const auto Z_MIRROR_HANDLER = std::bind(creative_kernel::mirrorZSelections, true);

} // namespace

namespace creative_kernel {

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

        

         connect(this, &MoveOperateMode::moving, this, [=] ()
         {
           updateEntity();
         });
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
                auto box = m->globalSpaceBox();
                minX = box.min.x() < minX ? box.min.x() : minX;
                minY = box.min.y() < minY ? box.min.y() : minY;
                minZ = box.min.z() < minZ ? box.min.z() : minZ;
                maxX = box.max.x() > maxX ? box.max.x() : maxX;
                maxY = box.max.y() > maxY ? box.max.y() : maxY;
                maxZ = box.max.z() > maxZ ? box.max.z() : maxZ;
            }
            qtuser_3d::Box3D box(QVector3D(minX, minY, minZ), QVector3D(maxX, maxY, maxZ));

            entity_->setSpaceBox(box);
            creative_kernel::visShow(entity_.get());
        }
        else {
            creative_kernel::visHide(entity_.get());
        }
        
    }

    void MirrorOperateMode::onAttach() {
        
        entity_->attach();

        creative_kernel::addModelNSelectorTracer(this);
        creative_kernel::addLeftMouseEventHandler(this);
        
        traceSpace(this);

        for (const auto& pair : pickable_callback_map_) {
            creative_kernel::tracePickable(pair.first);
        }

        updateEntity();
        creative_kernel::requestVisUpdate(true);
    }

    void MirrorOperateMode::onDettach() {

        for (const auto& pair : pickable_callback_map_) {
            creative_kernel::unTracePickable(pair.first);
        }

        entity_->detach();

        creative_kernel::removeModelNSelectorTracer(this);
        creative_kernel::removeLeftMouseEventHandler(this);
        
        unTraceSpace(this);

        if (entity_->parentEntity() != nullptr) {
            creative_kernel::visHide(entity_.get());
            creative_kernel::requestVisUpdate();
        }
    }

    void MirrorOperateMode::onLeftMouseButtonClick(QMouseEvent* event) {
        auto iter = pickable_callback_map_.find(creative_kernel::checkPickable(event->pos(), nullptr));
        if (iter == pickable_callback_map_.cend())
        {
            return;
        }

        iter->second();
        creative_kernel::requestVisPickUpdate(true);
    }

    void MirrorOperateMode::onSelectionsChanged() {
        updateEntity();
        creative_kernel::requestVisUpdate(true);
    }

    void MirrorOperateMode::onSceneChanged(const trimesh::dbox3& box)
    {
        updateEntity();
    }

    void MirrorOperateMode::onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds)
    {
        if (!_model_group->models().isEmpty())
        {
            onSelectionsChanged();
        }
    }
}