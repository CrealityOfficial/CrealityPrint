#include "scaleop.h"

#include "qtuser3d/math/space3d.h"
#include "qtuser3d/math/angles.h"
#include "qtuser3d/trimesh2/conv.h"

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/eventinterface.h"
#include "interface/spaceinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/modelgroupinterface.h"

#include "kernel/kernelui.h"
#include <QQmlProperty>
#include "data/spaceutils.h"

namespace creative_kernel
{
	ScaleOp::ScaleOp(QObject* parent)
		:MoveOperateMode(parent)
		, m_mode(TMode::null)
	{
		int types =
			HELPERTYPE_AXIS_X | \
			HELPERTYPE_AXIS_Y | \
			HELPERTYPE_AXIS_Z | \
			HELPERTYPE_PLANE_XY | \
			HELPERTYPE_PLANE_YZ | \
			HELPERTYPE_PLANE_ZX;

		m_type = qtuser_3d::SceneOperateMode::ReusableMode;
		m_helperEntity = new qtuser_3d::TranslateHelperEntity(nullptr, types, qtuser_3d::TranslateHelperEntity::IndicatorType::CUBE);
		m_helperEntity->setCameraController(cameraController());
	
		connect(this, &MoveOperateMode::moving, this, [=]()
			{	
				updateHelperEntity();	
			});
	}

	ScaleOp::~ScaleOp()
	{
	}

	void ScaleOp::onAttach()
	{
		m_helperEntity->attach();

		tracePickable(m_helperEntity->xArrowPickable());
		tracePickable(m_helperEntity->yArrowPickable());
		tracePickable(m_helperEntity->zArrowPickable());
		tracePickable(m_helperEntity->xyPlanePickable());
		tracePickable(m_helperEntity->yzPlanePickable());
		tracePickable(m_helperEntity->zxPlanePickable());
		tracePickable(m_helperEntity->xyzCubePickable());

		addModelNSelectorTracer(this);
		onSelectionsChanged();

		addLeftMouseEventHandler(this);
		addWheelEventHandler(this);

		traceSpace(this);
		requestVisUpdate(true);

		m_bShowPop = true;
	}

	void ScaleOp::onDettach()
	{
		m_helperEntity->detach();

		unTracePickable(m_helperEntity->xArrowPickable());
		unTracePickable(m_helperEntity->yArrowPickable());
		unTracePickable(m_helperEntity->zArrowPickable());
		unTracePickable(m_helperEntity->xyPlanePickable());
		unTracePickable(m_helperEntity->yzPlanePickable());
		unTracePickable(m_helperEntity->zxPlanePickable());
		unTracePickable(m_helperEntity->xyzCubePickable());

		visHide(m_helperEntity);

		removeModelNSelectorTracer(this);
		removeLeftMouseEventHandler(this);
		removeWheelEventHandler(this);

		unTraceSpace(this);

		requestVisUpdate(true);

		m_bShowPop = false;
	}

	void ScaleOp::onLeftMouseButtonPress(QMouseEvent* event)
	{
		MoveOperateMode::onLeftMouseButtonPress(event);
		m_mode = TMode::null;

		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		if (pickable == m_helperEntity->xArrowPickable()) m_mode = TMode::x;
		else if (pickable == m_helperEntity->yArrowPickable()) m_mode = TMode::y;
		else if (pickable == m_helperEntity->zArrowPickable()) m_mode = TMode::z;
		else if (pickable == m_helperEntity->xyPlanePickable()) m_mode = TMode::xy;
		else if (pickable == m_helperEntity->yzPlanePickable()) m_mode = TMode::yz;
		else if (pickable == m_helperEntity->zxPlanePickable()) m_mode = TMode::zx;
		else if (pickable != nullptr) m_mode = TMode::sp;

		trimesh::dbox3 box;
		QList<creative_kernel::ModelGroup*> groups = getHandleGroups();
		if (groups.size() > 0)
		{
			beginNodeSnap(groups, QList<ModelN*>());

			for (size_t i = 0; i < groups.size(); i++)
			{
				groups[i]->snapMatrix();

				//Hide possible conflict states 
				groups[i]->setNozzleRegionVisibility(false);
				groups[i]->setOuterLinesVisibility(false);
			}

			box = groupsGlobalBoundingBox(groups);
		}
		else {
			QList<creative_kernel::ModelN*> selections = selectedParts();
			if (selections.size() > 0)
			{
				QList<creative_kernel::ModelGroup*> groups = findRelativeGroups(selections);
				beginNodeSnap(groups, selections);

				for (size_t i = 0; i < selections.size(); i++)
				{
					selections[i]->snapMatrix();
				}

				//Hide possible conflict states
				for (auto group : groups)
				{
					group->setNozzleRegionVisibility(false);
					group->setOuterLinesVisibility(false);
				}
			}

			box = modelsBox(selections);
		}

		m_scaleCenter = box.center();
		m_spacePoint = operationProcessCoord(event->pos(), qtuser_3d::triBox2Box3D(box), 0, m_mode);
			
		setNeedCheckScope(0);
	}

	void ScaleOp::perform(const QPoint& point, bool reversible)
	{
		trimesh::dbox3 b = _selectedBox();

		QVector3D p = operationProcessCoord(point, qtuser_3d::triBox2Box3D(b), 0, m_mode);
		QVector3D _delta = getScale(p);
		trimesh::dvec3 delta(_delta.x(), _delta.y(), _delta.z());

		QList<ModelGroup*> groups = getHandleGroups();
		if (groups.size() > 0)
		{
			for (int i = 0; i < groups.size(); ++i)
			{
				trimesh::dvec3 c = m_scaleCenter;

				groups[i]->setMatrix(trimesh::xform::trans(c) *
					trimesh::xform::scale(delta.x, delta.y, delta.z) *
					trimesh::xform::trans(-c) * groups[i]->getSnapMatrix());
			}

			updateSpaceNodeRender(groups, reversible);
		}
		else {
			
			//lisugui 2021-05-27 
			QList<creative_kernel::ModelN*> selections = selectedParts();
			for (int i = 0; i < selections.size(); ++i)
			{
				ModelN* model = selections.at(i);
				ModelGroup* group = model->getModelGroup();

				trimesh::dvec3 c = trimesh::inv(group->getMatrix())* m_scaleCenter;

				selections[i]->setMatrix(trimesh::xform::trans(c) *
					trimesh::xform::scale(delta.x, delta.y, delta.z) *
					trimesh::xform::trans(-c) * model->getSnapMatrix());

				selections[i]->dirtyNestData();
			}

			updateSpaceNodeRender(selections, reversible);
		}
	}

	void ScaleOp::onLeftMouseButtonRelease(QMouseEvent* event)
	{
		MoveOperateMode::onLeftMouseButtonRelease(event);

		if (m_mode != TMode::null && m_mode != TMode::sp)
		{
			QList<creative_kernel::ModelGroup*> groups = getHandleGroups();
			if (groups.size() > 0)
			{
				for (creative_kernel::ModelGroup* g : groups)
				{
					g->layerBottom();
					g->updateSweepAreaPath();
				}

				endNodeSnap();
			}
			else {

				QList<creative_kernel::ModelN*> models = selectedParts();
				if (!models.isEmpty())
				{
					QList<ModelGroup*> relative_groups = findRelativeGroups(models);
					for (ModelGroup* model_group : relative_groups)
					{
						model_group->layerBottom();
						model_group->updateSweepAreaPath();
					}

					endNodeSnap();
				}
			}

			updateHelperEntity();

			emit scaleChanged();
			emit sizeChanged();

			m_bShowPop = false;
		}
	}

	void ScaleOp::onLeftMouseButtonMove(QMouseEvent* event)
	{
		if (m_mode != TMode::null && m_mode != TMode::sp)
		{
			perform(event->pos(), false);
			emit scaleChanged();
			emit sizeChanged();
			if (!m_bShowPop)
			{
				m_bShowPop = true;
			}
		}
		else {
			MoveOperateMode::onLeftMouseButtonMove(event);
		}
		setNeedCheckScope(0);
	}

	void ScaleOp::onLeftMouseButtonClick(QMouseEvent* event)
	{

	}

	void ScaleOp::onWheelEvent(QWheelEvent* event)
	{
	}

	void ScaleOp::onSelectionsChanged()
	{
		resetScaleOfMulti();
		buildFromSelections();

		creative_kernel::initSelectedGroupsBoudingBox(m_lastGroupBoxes);

		emit mouseLeftClicked();
	}

	void ScaleOp::onSceneChanged(const trimesh::dbox3& box)
	{
		updateHelperEntity();

		creative_kernel::checkSelectedGroupsBoudingBox(m_lastGroupBoxes);
	}

	void ScaleOp::onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds)
	{
		if (!_model_group->models().isEmpty())
		{
			onSelectionsChanged();
		}
	}

	void ScaleOp::updateHelperEntity()
	{
		trimesh::dbox3 b = _selectedBox();
		if (b.valid)
		{
			m_helperEntity->updateBox(qtuser_3d::triBox2Box3D(b));
			visShow(m_helperEntity);
		}
		else
		{
			visHide(m_helperEntity);
		}
	}

	void ScaleOp::buildFromSelections()
	{
		updateHelperEntity();
		requestVisPickUpdate(true);
		emit scaleChanged();
		emit sizeChanged();
	}

	void ScaleOp::reset()
	{
		auto groups = getHandleGroups();
		if (groups.size() > 0)
		{
			reset_groups(groups);
		}
		else {
			auto list = selectedParts();
			reset_models(list);
		}
	}

	QVector3D ScaleOp::scale()
	{	
		auto groups = getHandleGroups();
		if (groups.size() > 0)
		{
			return scale_groups(groups);
		}
		else {
			auto list = selectedParts();
			return scale_models(list);
		}
	}

	QVector3D ScaleOp::box()
	{
		auto groups = getHandleGroups();
		if (groups.size() > 0)
		{
			return box_groups(groups);
		}
		else {
			auto list = selectedParts();
			return box_models(list);
		}
	}

	QVector3D ScaleOp::globalbox()
	{
		auto groups = getHandleGroups();
		if (groups.size() > 0)
		{
			return globalbox_groups(groups);
		}
		else {
			auto list = selectedParts();
			return globalbox_models(list);
		}
	}

	void ScaleOp::setScale(QVector3D scale)
	{
		auto groups = getHandleGroups();
		if (groups.size() > 0)
		{
			setScale_groups(scale, groups);
		}
		else {
			auto list = selectedParts();
			setScale_models(scale, list);
		}
	}

	bool ScaleOp::uniformCheck()
	{
		return m_uniformCheck;
	}

	void ScaleOp::setUniformCheck(bool check)
	{
		QList<creative_kernel::ModelN*> selections = selectionms();
		if (selections.isEmpty())
			return;

		ModelN* m = selections.first();
		m_uniformCheck = check;

		if (m_uniformCheck)
		{
		}

		emit checkChanged();
	}

	float ScaleOp::deltaXY(float centerVal, float pressVal, float moveVal)
	{
		float delta = 1.0f;
		//if (pressVal == 0.0 || moveVal == 0.0)
		//{
		//	return 1.0f;
		//}
		//1. ����ģ�͵��������
		if (abs(pressVal - centerVal) == 0.0)return 1.0f;
		if (pressVal > centerVal && moveVal > centerVal)
		{
			delta = abs(((moveVal - pressVal) + abs(pressVal - centerVal)) / abs(pressVal - centerVal)); // moveVal > pressVal ? abs(moveVal / pressVal) : abs(pressVal / moveVal);
		}
		//2.����ĵ���ģ��������࣬�ƶ��㵽�˸������
		if (pressVal > centerVal && moveVal < centerVal)
		{
			//if (centerVal == 0.0)return 1.0f;
			//delta = abs(moveVal / centerVal);
		}

		//3.���µĵ����ƶ�����ģ�͵ĸ�����
		if (pressVal < centerVal && moveVal < centerVal)
		{
			delta = abs(((pressVal - moveVal) + abs(pressVal - centerVal)) / abs(pressVal - centerVal));
			//delta = abs(((pressVal - moveVal) + abs(pressVal)) / pressVal);
		}

		//2.����ĵ���ģ��������࣬�ƶ��㵽�˸������
		if (pressVal < centerVal && moveVal > centerVal)
		{
			//if (centerVal == 0.0)return 1.0f;
			//delta = abs(moveVal / centerVal);
		}


		return delta;
	}

	QVector3D ScaleOp::getScale(const QVector3D& c)
	{
		QVector3D scaleDelta = QVector3D(1.0f, 1.0f, 1.0f);
		
		qtuser_3d::Box3D qbox;

		auto groups = getHandleGroups();
		if (groups.size() > 0)
		{
			qbox = convert(groupsGlobalBoundingBox(groups));
		}
		else {			
			qbox = selectionsBoundingbox();
		}

		scaleDelta.setX(deltaXY(qbox.center().x(), m_spacePoint.x(), c.x()));
		scaleDelta.setY(deltaXY(qbox.center().y(), m_spacePoint.y(), c.y()));
		scaleDelta.setZ(deltaXY(qbox.center().z(), m_spacePoint.z(), c.z()));


		if (m_mode == TMode::x)  // x
		{
			scaleDelta = m_uniformCheck ? QVector3D(scaleDelta.x(), scaleDelta.x(), scaleDelta.x()) : QVector3D(scaleDelta.x(), 1.0, 1.0);
		}
		else if (m_mode == TMode::y)
		{
			scaleDelta = m_uniformCheck ? QVector3D(scaleDelta.y(), scaleDelta.y(), scaleDelta.y()) : QVector3D(1.0, scaleDelta.y(), 1.0);
		}
		else if (m_mode == TMode::z)
		{
			scaleDelta = m_uniformCheck ? QVector3D(scaleDelta.z(), scaleDelta.z(), scaleDelta.z()) : QVector3D(1.0, 1.0, scaleDelta.z());
		}
		else if (m_mode == TMode::xy)
		{
			float scale = (scaleDelta.x() + scaleDelta.y()) / 2.0;
			scaleDelta = m_uniformCheck ? QVector3D(scaleDelta.x(), scaleDelta.x(), scaleDelta.x()) : QVector3D(scale, scale, 1.0);
		}
		else if (m_mode == TMode::yz)
		{
			float scale = (scaleDelta.y() + scaleDelta.z()) / 2.0;
			scaleDelta = m_uniformCheck ? QVector3D(scaleDelta.y(), scaleDelta.y(), scaleDelta.y()) : QVector3D(1.0, scale, scale);
		}
		else if (m_mode == TMode::zx)
		{
			float scale = (scaleDelta.z() + scaleDelta.x()) / 2.0;
			scaleDelta = m_uniformCheck ? QVector3D(scaleDelta.z(), scaleDelta.z(), scaleDelta.z()) : QVector3D(scale, 1.0, scale);
		}
		else if (m_mode == TMode::xyz)
		{
			float scale = (scaleDelta.x() + scaleDelta.y() + scaleDelta.z()) / 3.0;
			scaleDelta = m_uniformCheck ? QVector3D(scaleDelta.x(), scaleDelta.x(), scaleDelta.x()) : QVector3D(scale, scale, scale);
		}

		return scaleDelta;
	}

	bool ScaleOp::getShowPop()
	{
		return m_bShowPop;
	}

	bool ScaleOp::shouldMultipleSelect()
	{
		return true;
	}

	void ScaleOp::reset_models(QList<creative_kernel::ModelN*>& models)
	{
		//
		if (models.isEmpty())
		{
			return;
		}
		trimesh::dvec3 scale = trimesh::dvec3(1.0, 1.0, 1.0);
		QList<ModelGroup*>list = findRelativeGroups(models);
		beginNodeSnap(list, models);

		for (creative_kernel::ModelN* model : models)
		{
			model->setNodeScalingFactor(scale);
			model->dirtyNestData();
		}

		for (ModelGroup* g : list)
		{
			g->layerBottom();
			g->updateSweepAreaPath();
		}

		if (models.size() > 1)
		{
			setScaleOfMulti(QVector3D(scale.x, scale.y, scale.z));
		}

		endNodeSnap();

		emit scaleChanged();
		emit sizeChanged();
	}

	QVector3D ScaleOp::box_models(QList<creative_kernel::ModelN*>& models)
	{
		if (models.isEmpty())
		{
			return QVector3D(0.0f, 0.0f, 0.0f);
		}

		trimesh::dvec3 sz = modelsBox(models).size();
		return QVector3D(sz.x, sz.y, sz.z);
	}

	QVector3D ScaleOp::globalbox_models(QList<creative_kernel::ModelN*>& models)
	{
		qtuser_3d::Box3D box3d;
		for (auto m : models)
		{
			box3d += m->globalSpaceBox();
		}
		return box3d.size();
	}

	QVector3D ScaleOp::scale_models(QList<creative_kernel::ModelN*>& models)
	{
		if (models.isEmpty())
		{
			return QVector3D(1.0f, 1.0f, 1.0f);
		}

		if (models.size() == 1)
		{
			return _getLocalScale(models.first());
		}

		//return QVector3D(1.0f, 1.0f, 1.0f);
		return getScaleOfMulti();
	}
	
	void ScaleOp::setScale_models(QVector3D scale, QList<creative_kernel::ModelN*>& models)
	{
		if (models.isEmpty())
		{
			return;
		}

		QList<ModelGroup*>list = findRelativeGroups(models);
		beginNodeSnap(list, models);

		for(creative_kernel::ModelN * model : models)
		{
			QVector3D delta = scale / this->scale();
			ModelGroup* group = model->getModelGroup();

			trimesh::dvec3 c = model->globalBoundingBox().center();
			c = trimesh::inv(group->getMatrix()) * c;

			model->setMatrix(trimesh::xform::trans(c) *
				trimesh::xform::scale(delta.x(), delta.y(), delta.z()) *
				trimesh::xform::trans(-c) * model->getMatrix());

			model->dirtyNestData();
		}

		for (ModelGroup* g : list)
		{
			g->layerBottom();
			g->updateSweepAreaPath();
		}
		
		if (models.size() > 1)
		{
			setScaleOfMulti(scale);
		}

		updateSpaceNodeRender(list, false);
		updateSpaceNodeRender(models, true);
		endNodeSnap();

	}

	void ScaleOp::reset_groups(QList<creative_kernel::ModelGroup*>& groups)
	{
		if (groups.isEmpty())
		{
			return;
		}
		trimesh::dvec3 scale = trimesh::dvec3(1.0, 1.0, 1.0);
		beginNodeSnap(groups, QList<creative_kernel::ModelN*>());

		for (ModelGroup* group : groups)
		{
			group->setNodeScalingFactor(scale);
			group->layerBottom();
			group->updateSweepAreaPath();
		}

		if (groups.size() > 1)
		{
			setScaleOfMulti(QVector3D(scale.x, scale.y, scale.z));
		}

		endNodeSnap();

		emit scaleChanged();
		emit sizeChanged();
	}

	QVector3D ScaleOp::box_groups(QList<creative_kernel::ModelGroup*>& groups)
	{
		if (groups.isEmpty())
		{
			return QVector3D(0.0f, 0.0f, 0.0f);
		}
		return convert(groupsGlobalBoundingBox(groups)).size();
	}

	QVector3D ScaleOp::globalbox_groups(QList<creative_kernel::ModelGroup*>& groups)
	{
		qtuser_3d::Box3D box = convert(groupsGlobalBoundingBox(groups));
		return box.size();
	}

	trimesh::dbox3 ScaleOp::_selectedBox()
	{
		trimesh::dbox3 box;
		QList<creative_kernel::ModelGroup*> groups = getHandleGroups();
		if (groups.size() > 0)
		{
			box = groupsGlobalBoundingBox(groups);
		}
		else {
			QList<creative_kernel::ModelN*> selections = selectedParts();
			box = modelsBox(selections);
		}
		return box;
	}

	QVector3D ScaleOp::scale_groups(QList<creative_kernel::ModelGroup*>& groups)
	{
		if (groups.size() == 1)
		{
			return _getLocalScale( groups.first() );
		}

		//return QVector3D(1.0f, 1.0f, 1.0f);
		return getScaleOfMulti();
	}

	void ScaleOp::setScale_groups(QVector3D scale, QList<creative_kernel::ModelGroup*>& groups)
	{
		if (groups.isEmpty())
		{
			return;
		}

		beginNodeSnap(groups, QList<creative_kernel::ModelN*>());

		for(ModelGroup * group : groups)
		{
			QVector3D delta = scale / this->scale();

			trimesh::dvec3 c = group->globalBoundingBox().center();

			group->setMatrix(trimesh::xform::trans(c) *
				trimesh::xform::scale(delta.x(), delta.y(), delta.z()) *
				trimesh::xform::trans(-c) * group->getMatrix());

			group->layerBottom();
			group->updateSweepAreaPath();
		}

		if (groups.size() > 1)
		{
			setScaleOfMulti(scale);
		}

		endNodeSnap();
		updateSpaceNodeRender(groups, true);

		emit scaleChanged();
		emit sizeChanged();
	}


	QVector3D ScaleOp::_getLocalScale(QNode3D* node)
	{
		/*trimesh::xform xMatrix = node->getMatrix();
		QMatrix4x4 gMatrix = qtuser_3d::xform2QMatrix(trimesh::fxform(xMatrix));
		QVector3D scale;
		scale.setX(QVector3D(gMatrix(0, 0), gMatrix(1, 0), gMatrix(2, 0)).length());
		scale.setY(QVector3D(gMatrix(0, 1), gMatrix(1, 1), gMatrix(2, 1)).length());
		scale.setZ(QVector3D(gMatrix(0, 2), gMatrix(1, 2), gMatrix(2, 2)).length());*/
		if (node == nullptr)
		{
			return QVector3D(0, 0, 0);
		}
		trimesh::dvec3 s = node->getNodeScalingFactor();
		return QVector3D(s.x, s.y, s.z);
	}



	creative_kernel::ModelGroup* ScaleOp::isModelsEqualAGroup(QList<creative_kernel::ModelN*>& models)
	{
		QList<ModelGroup*>groups = findRelativeGroups(models);
		if (groups.size() == 1)
		{
			ModelGroup* theGroup = groups.first();
			if (theGroup && theGroup->models().size() == models.size())
			{
				return theGroup;
			}
		}
		return nullptr;
	}

	//如果选中的多个模型刚好是该组里面的所有模型，则视为选中该组
	QList<creative_kernel::ModelGroup*> ScaleOp::getHandleGroups()
	{
		QList<creative_kernel::ModelGroup*> groups = selectedGroups();
		if (!groups.isEmpty())
		{
			return groups;
		}

		QList<creative_kernel::ModelN*> parts = selectedParts();
		ModelGroup* aGroup = isModelsEqualAGroup(parts);
		if (aGroup != nullptr)
		{
			groups << aGroup;
			return groups;
		}

		return groups;
	}

	void ScaleOp::resetScaleOfMulti()
	{
		m_displayScaleOfMulti = QVector3D(1.0f, 1.0f, 1.0f);
	}

	QVector3D ScaleOp::getScaleOfMulti()
	{
		return m_displayScaleOfMulti;
	}

	void ScaleOp::setScaleOfMulti(QVector3D newScale)
	{
		m_displayScaleOfMulti = newScale;
	}

	void creative_kernel::initSelectedGroupsBoudingBox(QList<GroupBoundingBox>& groupBoxes)
	{
		groupBoxes.clear();
		QList<ModelGroup*> groups = selectedGroups();
		if (groups.isEmpty())
		{
			groups = findRelativeGroups(selectedParts());
		}
		for (size_t i = 0; i < groups.size(); i++)
		{
			ModelGroup* g = groups.at(i);
			trimesh::dbox3 box = g->globalBoundingBox();
			groupBoxes.push_back({ g, box });
		}
	}

	void creative_kernel::checkSelectedGroupsBoudingBox(QList<GroupBoundingBox>& groupBoxes)
	{
		if (groupBoxes.isEmpty())
			return;

		for (size_t i = 0; i < groupBoxes.size(); i++)
		{
			auto pair = groupBoxes.at(i);
			QPointer<ModelGroup> g = pair.group;
			if (g != nullptr)
			{
				trimesh::dbox3 oldbox = pair.bbox;
				trimesh::dbox3 newbox = g->globalBoundingBox();
				if (abs(oldbox.max.z - newbox.max.z) > 0.01)
				{
					g->setLayerHeightProfile({});
				}
			}
		}
	}
}