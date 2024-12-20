#ifndef _NULLSPACE_SCALEOP_1589770383921_H
#define _NULLSPACE_SCALEOP_1589770383921_H
#include "basickernelexport.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/module/pickableselecttracer.h"

#include <QVector3D>
#include "entity/translatehelperentity.h"
#include "data/modeln.h"
#include "data/undochange.h"
//#include "data/interface.h"

#include "internal/utils/operationutil.h"
#include "moveoperatemode.h"

namespace creative_kernel
{
	class Kernel;

	class GroupBoundingBox
	{
	public:
		GroupBoundingBox(ModelGroup* g, trimesh::dbox3 box)
			: group(QPointer<ModelGroup>(g))
			, bbox(box) 
		{
		}
		QPointer<ModelGroup> group;
		trimesh::dbox3 bbox;
	};

	class BASIC_KERNEL_API ScaleOp : public MoveOperateMode
		, public ModelNSelectorTracer
		, public SpaceTracer
	{
	public:
		Q_OBJECT

	public:
		ScaleOp(QObject* parent = nullptr);
		virtual ~ScaleOp();

		void reset();

		QVector3D scale();
		void setScale(QVector3D scale);

		QVector3D box();
		QVector3D globalbox();
		
		bool uniformCheck();
		void setUniformCheck(bool check);

		bool getShowPop();
		float deltaXY(float modelVal, float pressVal, float moveVal);

	signals:
		void scaleChanged();
		void sizeChanged();
		void checkChanged();
		void mouseLeftClicked();
	protected:
		void onAttach() override;
		void onDettach() override;
		void onLeftMouseButtonPress(QMouseEvent* event) override;
		void onLeftMouseButtonRelease(QMouseEvent* event) override;
		void onLeftMouseButtonMove(QMouseEvent* event) override;
		void onLeftMouseButtonClick(QMouseEvent* event) override;
		void onWheelEvent(QWheelEvent* event) override;
		void onSelectionsChanged() override;
		void onSceneChanged(const trimesh::dbox3& box) override;

		void onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds) override;

		bool shouldMultipleSelect() override;
	protected:
		void buildFromSelections();
		void updateHelperEntity();

		QVector3D getScale(const QVector3D& p);

		void perform(const QPoint& point, bool reversible);
		void reset_models(QList<creative_kernel::ModelN*>& models);

		QVector3D box_models(QList<creative_kernel::ModelN*>& models);
		QVector3D globalbox_models(QList<creative_kernel::ModelN*>& models);

		QVector3D scale_models(QList<creative_kernel::ModelN*>& models);
		void setScale_models(QVector3D scale, QList<creative_kernel::ModelN*>& models);


		void reset_groups(QList<creative_kernel::ModelGroup*>& groups);

		QVector3D box_groups(QList<creative_kernel::ModelGroup*>& groups);
		QVector3D globalbox_groups(QList<creative_kernel::ModelGroup*>& groups);

		trimesh::dbox3 _selectedBox();

		QVector3D scale_groups(QList<creative_kernel::ModelGroup*>& groups);
		void setScale_groups(QVector3D scale, QList<creative_kernel::ModelGroup*>& groups);

	protected:
		QVector3D _getLocalScale(QNode3D* node);

		creative_kernel::ModelGroup* isModelsEqualAGroup(QList<creative_kernel::ModelN*>& models);
		QList<creative_kernel::ModelGroup*> getHandleGroups();


		//for display multi models/groups scaling factor
		void resetScaleOfMulti();
		QVector3D getScaleOfMulti();
		void setScaleOfMulti(QVector3D newScale);

	private:
		qtuser_3d::TranslateHelperEntity* m_helperEntity;

		QVector3D m_spacePoint;
		TMode m_mode;
		QVector3D m_displayScaleOfMulti;

		bool m_bShowPop = false;
		bool m_uniformCheck = true;

		trimesh::dvec3 m_scaleCenter;

		QList<GroupBoundingBox> m_lastGroupBoxes;
	};

	void initSelectedGroupsBoudingBox(QList<GroupBoundingBox>& groupBoxes);
	void checkSelectedGroupsBoudingBox(QList<GroupBoundingBox>& groupBoxes);
}
#endif // _NULLSPACE_SCALEOP_1589770383921_H
