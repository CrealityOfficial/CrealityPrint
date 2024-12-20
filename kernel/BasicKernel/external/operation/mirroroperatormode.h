#pragma once

#include <map>
#include <memory>

#include "basickernelexport.h"
#include "data/modeln.h"
#include "qtuser3d/module/pickable.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "entity/mirrorentity.h"
#include <QTimer>
#include "operation/moveoperatemode.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API MirrorOperateMode : public MoveOperateMode
		, public ModelNSelectorTracer, public SpaceTracer {
		Q_OBJECT;

	public:
		explicit MirrorOperateMode(QObject* parent = nullptr);
		virtual ~MirrorOperateMode() = default;

	private:
		void updateEntity();

	protected:
		virtual void onAttach() override;
		virtual void onDettach() override;

		virtual void onLeftMouseButtonClick(QMouseEvent* event) override;

	protected:
		virtual void onSelectionsChanged() override;
		void onSceneChanged(const trimesh::dbox3& box) override;
		virtual void onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds) override;


	private:
		std::unique_ptr<qtuser_3d::MirrorEntity> entity_;
		std::map<QPointer<qtuser_3d::Pickable>, std::function<void(void)>> pickable_callback_map_;

	};
}