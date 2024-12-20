#ifndef _NULLSPACE_ROTATEOP_1589770383921_H
#define _NULLSPACE_ROTATEOP_1589770383921_H
#include "basickernelexport.h"
#include <QPointer>
#include <QQmlComponent>
#include "entity/rotate3dhelperentity.h"
#include "data/modeln.h"
#include "data/undochange.h"
#include "data/interface.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/module/manipulatecallback.h"

#include <QVector3D>
#include "moveoperatemode.h"
#include "scaleop.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API RotateOp : public MoveOperateMode
		, public ModelNSelectorTracer
		, public qtuser_3d::RotateCallback
		, public SpaceTracer
	{
		Q_OBJECT
	public:
		RotateOp(QObject* parent = nullptr);
		virtual ~RotateOp();

		void reset();
		QVector3D rotate();
		void setRotate(QVector3D rotate);
		//void startRotate();

		bool getShowPop();

	signals:
		void rotateChanged();
		void mouseLeftClicked();

	protected:
		void onAttach() override;
		void onDettach() override;
		void onLeftMouseButtonPress(QMouseEvent* event) override;
		void onLeftMouseButtonRelease(QMouseEvent* event) override;
		void onLeftMouseButtonMove(QMouseEvent* event) override;
		void onLeftMouseButtonClick(QMouseEvent* event) override;
		void onWheelEvent(QWheelEvent* event) override;

		void onHoverEnter(QHoverEvent* event);
		void onHoverLeave(QHoverEvent* event);
		void onHoverMove(QHoverEvent* event);

		void onSelectionsChanged() override;
		void onSceneChanged(const trimesh::dbox3& box) override;

		void onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds) override;

		void onStartRotate() override;
		void onRotate(QQuaternion q) override;
		void onEndRotate(QQuaternion q) override;
		void setRotateAngle(QVector3D axis, float angle) override;

		bool shouldMultipleSelect() override;

	protected:
		void setSelectedModel(QList<creative_kernel::ModelN*> models);

		void buildFromSelections();
		void rotateByAxis(const QList<creative_kernel::ModelN*>& models, QVector3D axis, float angle);

		void updateHelperEntity();

		void setTipObjectPos(const QPoint& pos);
		void setTipObjectVisible(bool visible);
		bool getTipObjectVisible();

	protected:
		void reset_models(QList<creative_kernel::ModelN*>& models);
		QVector3D rotate_models(QList<creative_kernel::ModelN*>& models);
		void setRotate_models(QVector3D rotate, QList<creative_kernel::ModelN*>& models);
		
		void reset_groups(QList<creative_kernel::ModelGroup*>& groups);
		QVector3D rotate_groups(QList<creative_kernel::ModelGroup*>& groups);
		void setRotate_groups(QVector3D rotate, QList<creative_kernel::ModelGroup*>& groups);
		

		void onStartRotate_models(QList<creative_kernel::ModelN*>& models);
		void onRotate_models(QQuaternion q, QList<creative_kernel::ModelN*>& models);
		void onEndRotate_models(QQuaternion q, QList<creative_kernel::ModelN*>& models);

		void onStartRotate_groups(QList<creative_kernel::ModelGroup*>& groups);
		void onRotate_groups(QQuaternion q, QList<creative_kernel::ModelGroup*>& groups);
		void onEndRotate_groups(QQuaternion q, QList<creative_kernel::ModelGroup*>& groups);

	private:

		qtuser_3d::Rotate3DHelperEntity* m_helperEntity;

		float m_saveAngle;

		bool m_isRoate;
		bool m_isMoving{ false };

		QList<creative_kernel::ModelN*> m_selectedModels;
		QVector3D m_displayRotate;
		bool m_bShowPop = false;

		QPointer<QQmlComponent> tip_component_;
		QPointer<QObject> tip_object_;

		trimesh::dvec3 m_rotateCenter;

		QList<GroupBoundingBox> m_lastGroupBoxes;
	};
}
#endif // _NULLSPACE_ROTATEOP_1589770383921_H
