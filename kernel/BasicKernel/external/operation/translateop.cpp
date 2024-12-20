#include "translateop.h"
#include <QQmlProperty>
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/trimesh2/conv.h"

#include <QMetaObject>
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/printerinterface.h"
#include "interface/modelgroupinterface.h"

#include "interface/eventinterface.h"
#include "interface/reuseableinterface.h"
#include "kernel/kernelui.h"
#include "data/spaceutils.h"

#include "internal/multi_printer/printer.h"

namespace creative_kernel
{
	TranslateOp::TranslateOp(QObject* parent)
		:MoveOperateMode(parent)
	{
		int types =
			HELPERTYPE_AXIS_X | \
			HELPERTYPE_AXIS_Y | \
			HELPERTYPE_AXIS_Z | \
			HELPERTYPE_PLANE_XY | \
			HELPERTYPE_PLANE_YZ | \
			HELPERTYPE_PLANE_ZX;

		m_type = qtuser_3d::SceneOperateMode::ReusableMode;
		m_helperEntity = new qtuser_3d::TranslateHelperEntity(nullptr, types);
		m_helperEntity->setCameraController(cameraController());
	}

	TranslateOp::~TranslateOp()
	{
	}

	/*void TranslateOp::setMessage(bool isRemove)
	{
		if (isRemove)
		{
			requestVisUpdate(true);
		}
	}

	bool TranslateOp::getMessage()
	{
		return false;
	}*/

	void TranslateOp::onAttach()
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
		addKeyEventHandler(this);

		traceSpace(this);

		requestVisUpdate(true);

	}

	void TranslateOp::onDettach()
	{
		m_helperEntity->detach();

		unTraceSpace(this);

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
		removeKeyEventHandler(this);
	
		requestVisUpdate(true);
	}

	void TranslateOp::onLeftMouseButtonPress(QMouseEvent* event)
	{
		MoveOperateMode::onLeftMouseButtonPress(event);
		setNeedCheckScope(0);

		auto models = selectionms();
		if (models.isEmpty())
			return;

		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		if (pickable == m_helperEntity->xArrowPickable())
			setMode(TMode::x);
		else if (pickable == m_helperEntity->yArrowPickable())
			setMode(TMode::y);
		else if (pickable == m_helperEntity->zArrowPickable())
			setMode(TMode::z);
		else if (pickable == m_helperEntity->xyPlanePickable())
			setMode(TMode::xy);
		else if (pickable == m_helperEntity->yzPlanePickable())
			setMode(TMode::yz);
		else if (pickable == m_helperEntity->zxPlanePickable())
			setMode(TMode::zx);
		resetSpacePoint(event->pos());
	}

	void TranslateOp::onLeftMouseButtonRelease(QMouseEvent* event)
	{
		MoveOperateMode::onLeftMouseButtonRelease(event);
		notifySpaceChange(true);
		setNeedCheckScope(0);
	}

	void TranslateOp::onLeftMouseButtonMove(QMouseEvent* event)
	{
		MoveOperateMode::onLeftMouseButtonMove(event);
		updateHelperEntity();

		emit positionChanged();
		setNeedCheckScope(0);
	}

	void TranslateOp::onLeftMouseButtonClick(QMouseEvent* event)
	{
	}

	void TranslateOp::onWheelEvent(QWheelEvent* event)
	{
	}

	void TranslateOp::onSelectionsChanged()
	{
		QList<creative_kernel::ModelN*> selections = selectionms();
		onSelectModelsChanged(selections);

		creative_kernel::initSelectedGroupsBoudingBox(m_lastGroupBoxes);
	}

	void TranslateOp::onSceneChanged(const trimesh::dbox3& box)
	{
		updateHelperEntity();
		creative_kernel::checkSelectedGroupsBoudingBox(m_lastGroupBoxes);
	}

	void TranslateOp::onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds)
	{
		if (!_model_group->models().isEmpty())
		{
			onSelectionsChanged();
		}
	}

	void TranslateOp::center()
	{
		auto groups = selectedGroups();
		if (groups.size())
		{
			center_groups(groups);
		}
		else {
			QList<ModelN*> list = selectedParts();
			center_models(list);
		}
	}

	void TranslateOp::movePositionToinit(QList<creative_kernel::ModelN*>& selections)
	{
		if (selections.isEmpty())
		{
			return;
		}

		QList<ModelGroup*>gs;
		gs << selections.first()->getModelGroup();
		beginNodeSnap(gs, selections);

		for (size_t i = 0; i < selections.size(); i++)
		{
			//QVector3D oldLocalPosition = selections.at(i)->localPosition();
			QVector3D initPosition = selections.at(i)->GetInitPosition();
			initPosition.setZ(0.0f);
			QVector3D position = selections.at(i)->mapGlobal2Local(initPosition);
			position.setX(initPosition.x());
			position.setY(initPosition.y());

			//moveModel(selections.at(i), oldLocalPosition, position, true);
			trimesh::dvec3 tDelta(position.x(), position.y(), position.z());
			trimesh::xform xf = trimesh::xform::trans(tDelta);
			selections.at(i)->applyMatrix(xf);
			selections.at(i)->dirty();
		}

		updateSpaceNodeRender(gs, false);
		updateSpaceNodeRender(selections, true);
		endNodeSnap();
	}

	void TranslateOp::reset()
	{
	}

	QVector3D TranslateOp::position()
	{
		auto groups = selectedGroups();
		if (groups.size())
		{
			return position_groups(groups);
		}
		else {
			auto list = selectedParts();
			return position_models(list);
		}

		return QVector3D();
	}

	void TranslateOp::setPosition(QVector3D position)
	{
		auto groups = selectedGroups();
		if (groups.size())
		{
			setPosition_groups(position, groups);
		}
		else {
			auto list = selectedParts();
			setPosition_models(position, list);
		}

		setNeedCheckScope(0);
	}

	void TranslateOp::setBottom()
	{
		auto groups = selectedGroups();
		if (groups.size())
		{
			setBottom_groups(groups);
		}
		else {
			auto list = selectedParts();
			setBottom_models(list);
		}
	}

	void TranslateOp::updateHelperEntity()
	{
		auto models = selectionms();
		if (models.size())
		{
			trimesh::dbox3 box = creative_kernel::modelsBox(models);
			m_helperEntity->updateBox(qtuser_3d::triBox2Box3D(box));
		}
	}

	bool TranslateOp::shouldMultipleSelect()
	{
		return true;
	}

	void TranslateOp::onSelectModelsChanged(QList<creative_kernel::ModelN*>& models)
	{
		if (models.size())
		{
			updateHelperEntity();
			visShow(m_helperEntity);
		}
		else
		{
			visHide(m_helperEntity);
		}

		requestVisPickUpdate(true);
		
		if (models.size())
		{
			emit positionChanged();
		}
		emit mouseLeftClicked();
	}

	void TranslateOp::TranslateOp::reset_models(QList<creative_kernel::ModelN*>& models)
	{
	}

	void TranslateOp::TranslateOp::center_models(QList<creative_kernel::ModelN*>& models)
	{
		beginNodeSnap(QList<ModelGroup *>(), models);

		qtuser_3d::Box3D modelsbox;
		for (auto m : models)
		{
			qtuser_3d::Box3D modelbox = m->globalSpaceBox();
			modelsbox += modelbox;
		}
		QVector3D movePos;
		float newZ = 0;
		if (modelsbox.valid)
		{
			Printer* printer = getSelectedPrinter();
			qtuser_3d::Box3D box = printer->globalBox();
			QVector3D size = box.size();
			QVector3D center = box.center();
			newZ = center.z() - size.z() / 2.0f;

			movePos = box.center() - modelsbox.center();
			movePos.setZ(0);
		}

		for (auto m : models)
		{
			//QVector3D oldLocalPosition = m->localPosition();
			//QVector3D newPositon = oldLocalPosition + movePos;
			//moveModel(m, oldLocalPosition, newPositon, true);

			trimesh::dvec3 tDelta(movePos.x(), movePos.y(), movePos.z());
			trimesh::xform xf = trimesh::xform::trans(tDelta);
			m->applyMatrix(xf);
			m->dirty();
		}

		endNodeSnap();

		updateHelperEntity();
		requestVisUpdate(true);
		emit positionChanged();

	}

	QVector3D TranslateOp::TranslateOp::position_models(QList<creative_kernel::ModelN*>& models)
	{
		if (models.size() == 1)
		{
			creative_kernel::ModelN* theModel = models.at(0);
			qtuser_3d::Box3D box = theModel->globalSpaceBox();
			return QVector3D(box.center().x(), box.center().y(), box.min.z());
		}
		return QVector3D(0.0f, 0.0f, 0.0f);
	}

	void TranslateOp::TranslateOp::setPosition_models(QVector3D position, QList<creative_kernel::ModelN*>& models)
	{
		if (models.empty())
			return;

		beginNodeSnap(QList<ModelGroup *>(), models);

		QVector3D displayPos = position_models(models);
		if (!displayPos.isNull())
		{
			if (models.size() == 1)
			{
				creative_kernel::ModelN* theModel = models.at(0);
				qtuser_3d::Box3D box = theModel->globalSpaceBox();
				float newZ = position.z() - displayPos.z() + box.center().z();
				position.setZ(newZ);
			}
		}
		
		if (1 == models.size())
		{
			auto model = models.last();
			setModelToWorldPosition(model, position);
			model->dirty();
		}
		else
		{
			for (auto m : models)
			{
				trimesh::dvec3 src_w = m->globalBoundingBox().center();
				setModelToWorldPosition(m, QVector3D(src_w.x + position.x(), 
											src_w.y + position.y(),
											src_w.z + position.z()));
				m->dirty();
			}
			
		}
		endNodeSnap();

		updateSpaceNodeRender(models, true);

		updateHelperEntity();
		//requestVisUpdate(true);
		emit positionChanged();

		setNeedCheckScope(0);
	}

	void TranslateOp::TranslateOp::setBottom_models(QList<creative_kernel::ModelN*>& models)
	{
		if (models.isEmpty())
		{
			return;
		}

		beginNodeSnap(QList<ModelGroup *>(), models);

		for (auto m : models)
		{
			qtuser_3d::Box3D box = m->globalSpaceBox();
			QVector3D c = box.center();
			setModelToWorldPosition(m, QVector3D(c.x(), c.y(), box.size().z()/2.0));

			m->dirty();
		}
		
		endNodeSnap();
		updateHelperEntity();
		updateSpaceNodeRender(models, true);
		emit positionChanged();

	}

	void TranslateOp::reset_groups(QList<ModelGroup*>& groups)
	{
	}

	void TranslateOp::center_groups(QList<ModelGroup*>& groups)
	{
		if (groups.isEmpty())
		{
			return;
		}

		beginNodeSnap(groups, QList<ModelN*>());

		qtuser_3d::Box3D groupsbox;
		for (auto m : groups)
		{
			qtuser_3d::Box3D modelbox = m->globalSpaceBox();
			groupsbox += modelbox;
		}
		QVector3D movePos;
		float newZ = 0;
		if (groupsbox.valid)
		{
			Printer* printer = getSelectedPrinter();
			qtuser_3d::Box3D box = printer->globalBox();
			QVector3D size = box.size();
			QVector3D center = box.center();
			newZ = center.z() - size.z() / 2.0f;

			movePos = box.center() - groupsbox.center();
			movePos.setZ(0);
		}

		//QList<QVector3D> starts, ends;
		for (auto m : groups)
		{
			//QVector3D oldLocalPosition = m->localPosition();
			//QVector3D newPositon = oldLocalPosition + movePos;
			
			//m->setLocalPosition(newPositon);
			/*starts << oldLocalPosition;
			ends << newPositon;*/
			trimesh::dvec3 tDelta(movePos.x(), movePos.y(), movePos.z());
			trimesh::xform xf = trimesh::xform::trans(tDelta);
			m->applyMatrix(xf);
			m->dirty();
		}
		//moveModelGroups(groups, starts, ends, true);

		endNodeSnap();
		updateHelperEntity();
		requestVisUpdate(true);
		emit positionChanged();
	}

	QVector3D TranslateOp::position_groups(QList<ModelGroup*>& groups)
	{
		trimesh::dvec3 pos = _position_groups(groups);
		return QVector3D(pos.x, pos.y, pos.z);
	}

	trimesh::dvec3 TranslateOp::_position_groups(QList<ModelGroup*>& groups)
	{
		if (groups.size() == 1)
		{
			auto g = groups.at(0);
			trimesh::dbox3 box;
			for (ModelN* model : g->normalModels())
			{
				if (model->isVisible())
				{
					box += model->globalBoundingBox();
				}
			}
			return trimesh::dvec3(box.center().x, box.center().y, box.min.z);
		}
		return trimesh::dvec3(0.0, 0.0, 0.0);
	}

	void TranslateOp::setPosition_groups(QVector3D position, QList<ModelGroup*>& groups)
	{
		if (groups.empty())
			return;

		beginNodeSnap(groups, QList<ModelN*>());

		trimesh::dvec3 displayPos = _position_groups(groups);
		
		if (1 == groups.size())
		{
			auto group = groups.last();
			trimesh::dvec3 delta = trimesh::dvec3(position.x() - displayPos.x,
												  position.y() - displayPos.y, 
												  position.z() - displayPos.z);
			trimesh::xform xf = trimesh::xform::trans(delta);
			group->applyMatrix(xf);
			
			group->dirty();
		}
		else
		{
			for (auto& m : groups)
			{
				trimesh::dvec3 tDelta(position.x(), position.y(), position.z());
				trimesh::xform xf = trimesh::xform::trans(tDelta);
				m->applyMatrix(xf);
				m->dirty();
			}
		}

		endNodeSnap();
		updateHelperEntity();
		requestVisUpdate(true);
		emit positionChanged();
	}

	void TranslateOp::setBottom_groups(QList<ModelGroup*>& groups)
	{
		if (groups.isEmpty())
		{
			return;
		}
		beginNodeSnap(groups, QList<ModelN*>());

		for (auto m : groups)
		{
			m->layerBottom();
			m->dirty();
		}
		
		endNodeSnap();
		updateSpaceNodeRender(groups, true);
		updateHelperEntity();
		emit positionChanged();
	}

	void TranslateOp::notifyPositionChanged()
	{
		updateHelperEntity();
		requestVisUpdate(true);
		emit positionChanged();
	}
}