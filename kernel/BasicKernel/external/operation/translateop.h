#ifndef _NULLSPACE_TRANSLATEOP_1589770383921_H
#define _NULLSPACE_TRANSLATEOP_1589770383921_H
#include "basickernelexport.h"
#include "qtuser3d/scene/sceneoperatemode.h"

#include <QVector3D>
#include "entity/translatehelperentity.h"
#include "qtuser3d/math/box3d.h"
#include "data/modeln.h"
#include "internal/utils/operationutil.h"
#include "moveoperatemode.h"
#include "scaleop.h"

#define PosMax 10000

namespace creative_kernel
{
	class Kernel;

	class BASIC_KERNEL_API TranslateOp : public MoveOperateMode
		, public ModelNSelectorTracer, public SpaceTracer
	{
		Q_OBJECT
	public:
		TranslateOp(QObject* parent = nullptr);
		virtual ~TranslateOp();

		/*void setMessage(bool isRemove);
		bool getMessage();*/

		void reset();
		void center();
		QVector3D position();
		void setPosition(QVector3D position);
		void setBottom();

		void notifyPositionChanged();

	signals:
		void positionChanged();
		void mouseLeftClicked();

	protected:
		void onAttach() override;
		void onDettach() override;
		void onLeftMouseButtonPress(QMouseEvent* event) override;
		void onLeftMouseButtonRelease(QMouseEvent* event) override;
		void onLeftMouseButtonMove(QMouseEvent* event) override;
		void onLeftMouseButtonClick(QMouseEvent* event) override;
		void onWheelEvent(QWheelEvent* event) override;
		bool shouldMultipleSelect() override;

		void onSelectionsChanged() override;
		void onSceneChanged(const trimesh::dbox3& box) override;

		void onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds) override;

	protected:

		void updateHelperEntity();

		void movePositionToinit(QList<creative_kernel::ModelN*>& selections);

		void onSelectModelsChanged(QList<creative_kernel::ModelN*>& models);

		void reset_models(QList<creative_kernel::ModelN*>& models);
		void center_models(QList<creative_kernel::ModelN*>& models);
		QVector3D position_models(QList<creative_kernel::ModelN*>& models);
		void setPosition_models(QVector3D position, QList<creative_kernel::ModelN*>& models);
		void setBottom_models(QList<creative_kernel::ModelN*>& models);

		void reset_groups(QList<ModelGroup*>& groups);
		void center_groups(QList<ModelGroup*>& groups);
		QVector3D position_groups(QList<ModelGroup*>& groups);
		void setPosition_groups(QVector3D position, QList<ModelGroup*>& groups);
		void setBottom_groups(QList<ModelGroup*>& groups);

		trimesh::dvec3 _position_groups(QList<ModelGroup*>& groups);

	private:
		qtuser_3d::TranslateHelperEntity* m_helperEntity;
		QList<GroupBoundingBox> m_lastGroupBoxes;
	};
}
#endif // _NULLSPACE_TRANSLATEOP_1589770383921_H
