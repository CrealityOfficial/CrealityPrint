#include "rotateop.h"

#include <QtCore/qmath.h>
#include <QQmlProperty>

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/eventinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/spaceinterface.h"
#include "interface/modelgroupinterface.h"

#include "kernel/kernelui.h"
#include "data/spaceutils.h"

#include "qtuser3d/math/angles.h"
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/trimesh2/conv.h"
#include "qtusercore/property/qmlpropertysetter.h"

namespace creative_kernel
{
	RotateOp::RotateOp(QObject* parent)
		: MoveOperateMode(parent)
		, tip_component_(nullptr)
	{
		m_type = qtuser_3d::SceneOperateMode::ReusableMode;
		m_helperEntity = new qtuser_3d::Rotate3DHelperEntity();
		m_helperEntity->setCameraController(cameraController());
		m_isRoate = false;

		auto* ui_parent = getKernelUI()->glMainView();

		tip_component_ = new QQmlComponent{ qmlEngine(ui_parent),
			QUrl{ QStringLiteral("qrc:/CrealityUI/qml/RotateTip.qml") }, this };

		tip_object_ = tip_component_->create(qmlContext(ui_parent));
		if (tip_object_ != nullptr) {
			tip_object_->setParent(ui_parent);
			qtuser_qml::writeObjectProperty(tip_object_, QStringLiteral("parent"), ui_parent);
		}

		connect(this, &MoveOperateMode::moving, this, [=]()
			{
				m_isMoving = true;
				updateHelperEntity();
				m_isMoving = false;
			});
	}

	RotateOp::~RotateOp()
	{
	}

	void RotateOp::onAttach()
	{
		m_helperEntity->attach();

		QList<qtuser_3d::Pickable*> pickableList = m_helperEntity->getPickables();
		for (qtuser_3d::Pickable* pickable : pickableList)
		{
			tracePickable(pickable);
		}

		addModelNSelectorTracer(this);
		onSelectionsChanged();
		QList<qtuser_3d::LeftMouseEventHandler*> handlers = m_helperEntity->getLeftMouseEventHandlers();
		for (qtuser_3d::LeftMouseEventHandler* handler : handlers)
		{
			addLeftMouseEventHandler(handler);
		}
		addWheelEventHandler(this);
		addLeftMouseEventHandler(this);
		addHoverEventHandler(this);
		traceSpace(this);

		m_helperEntity->setPickSource(visPickSource());
		m_helperEntity->setScreenCamera(visCamera());
		m_helperEntity->setRotateCallback(this);

		requestVisUpdate(true);

		m_bShowPop = true;
	}

	void RotateOp::onDettach()
	{
		m_helperEntity->detach();

		QList<qtuser_3d::Pickable*> pickableList = m_helperEntity->getPickables();
		for (qtuser_3d::Pickable* pickable : pickableList)
		{
			unTracePickable(pickable);
		}

		removeModelNSelectorTracer(this);
		visHide(m_helperEntity);
		m_selectedModels.clear();

		QList<qtuser_3d::LeftMouseEventHandler*> handlers = m_helperEntity->getLeftMouseEventHandlers();
		for (qtuser_3d::LeftMouseEventHandler* handler : handlers)
		{
			removeLeftMouseEventHandler(handler);
		}
		removeWheelEventHandler(this);
		removeLeftMouseEventHandler(this);
		removeHoverEventHandler(this);
		unTraceSpace(this);

		m_helperEntity->setPickSource(nullptr);
		m_helperEntity->setScreenCamera(nullptr);
		m_helperEntity->setRotateCallback(nullptr);

		requestVisUpdate(true);

		m_bShowPop = false;
	}

	void RotateOp::onLeftMouseButtonPress(QMouseEvent* event)
	{
		MoveOperateMode::onLeftMouseButtonPress(event);
	}

	void RotateOp::onLeftMouseButtonRelease(QMouseEvent* event)
	{
		MoveOperateMode::onLeftMouseButtonRelease(event);
	}

	void RotateOp::rotateByAxis(const QList<creative_kernel::ModelN*>& models, QVector3D axis, float angle)
	{
		if (models.isEmpty())
			return;

		trimesh::dvec3 rotateCenter = modelsBox(models).center();

		QList<ModelGroup*> gs = findRelativeGroups(models);
		beginNodeSnap(gs, models);

		for (size_t i = 0; i < models.size(); i++)
		{
			ModelN* model = models.at(i);
			model->rotateByStandardAngle(axis, angle);

			ModelGroup* group = model->getModelGroup();
			trimesh::xform gMatrix = group->getMatrix();
			trimesh::xform gMatrix_inv = trimesh::inv(gMatrix);

			trimesh::dvec3 c = gMatrix_inv * rotateCenter;
			trimesh::dvec3 v0 = gMatrix_inv * trimesh::dvec3(0.0);
			trimesh::dvec3 v1 = gMatrix_inv * trimesh::dvec3(axis.x(), axis.y(), axis.z());
			trimesh::dvec3 v = v1 - v0;
			trimesh::xform rot = trimesh::xform::rot(angle * M_PI_180, v);

			trimesh::xform snap_matrix = model->getMatrix();
			trimesh::xform xf = trimesh::xform::trans(c) * rot * trimesh::xform::trans(-c) * snap_matrix;
			model->setMatrix(xf);

			if (axis.x() != 0 || axis.y() != 0)
				model->resetNestRotation();
		}

		for (size_t i = 0; i < gs.size(); i++)
		{
			ModelGroup* g = gs.at(i);
			g->layerBottom();
			g->updateSweepAreaPath();
		}

		endNodeSnap();
	}

	void RotateOp::onLeftMouseButtonMove(QMouseEvent* event)
	{
		
		if (tip_object_ && m_isRoate)
		{
			QPoint point = event->pos();
			setTipObjectPos(point + QPoint(10, 10));
		}
		else {
			MoveOperateMode::onLeftMouseButtonMove(event);
		}
	}

	void RotateOp::onLeftMouseButtonClick(QMouseEvent* event)
	{

	}

	void RotateOp::onWheelEvent(QWheelEvent* event)
	{
	}

	void RotateOp::onHoverEnter(QHoverEvent* event)
	{
	}

	void RotateOp::onHoverLeave(QHoverEvent* event)
	{

	}

	void RotateOp::onHoverMove(QHoverEvent* event)
	{
		if (m_isRoate)
			return;

		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		
		if (pickable == m_helperEntity->xPickable())
		{
			setRotateAngle(QVector3D(1.0, 0.0, 0.0), 0.0f);
			QPoint point = event->pos();
			setTipObjectPos(point + QPoint(10, 10));
		}
		else if (pickable == m_helperEntity->yPickable())
		{
			setRotateAngle(QVector3D(0.0, 1.0, 0.0), 0.0f);
			QPoint point = event->pos();
			setTipObjectPos(point + QPoint(10, 10));
		} 
		else if (pickable == m_helperEntity->zPickable())
		{
			setRotateAngle(QVector3D(0.0, 0.0, 1.0), 0.0f);
			QPoint point = event->pos();
			setTipObjectPos(point + QPoint(10, 10));
		}
		else {
			setTipObjectVisible(false);
		}
	}

	void RotateOp::setSelectedModel(QList<creative_kernel::ModelN*> models)
	{
		//trace selected node
		m_selectedModels = models;
		qDebug() << "setSelectedModel";
		buildFromSelections();
	}

	void RotateOp::onSelectionsChanged()
	{
		QList<creative_kernel::ModelN*> selections = selectionms();
		setSelectedModel(selections);

		creative_kernel::initSelectedGroupsBoudingBox(m_lastGroupBoxes);
		
		emit mouseLeftClicked();
	}

	void RotateOp::onSceneChanged(const trimesh::dbox3& box)
	{
		updateHelperEntity();

		creative_kernel::checkSelectedGroupsBoudingBox(m_lastGroupBoxes);
	}

	void RotateOp::onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds)
	{
		if (!_model_group->models().isEmpty())
		{
			onSelectionsChanged();
		}
	}

	void RotateOp::onStartRotate()
	{
		auto groups = selectedGroups();
		auto models = selectedParts();

		if (groups.isEmpty() && models.isEmpty())
		{
			return;
		}

		if (groups.size() > 0)
		{
			onStartRotate_groups(groups);
		}
		else if (models.size() > 0) {
			onStartRotate_models(models);
		}

		creative_kernel::forceHideBox(true);
	}

	void RotateOp::onRotate(QQuaternion q)
	{
		auto groups = selectedGroups();
		if (groups.size() > 0)
		{
			onRotate_groups(q, groups);
			emit rotateChanged();
		}
		else {
			auto list = selectedParts();
			onRotate_models(q, list);
			emit rotateChanged();
		}
	}

	void RotateOp::setRotateAngle(QVector3D axis, float angle)
	{
		if (tip_object_ == nullptr)
			return;

		setTipObjectVisible(true);

		//axis = m_helperEntity->getCurrentRotateAxis();
		//angle = m_helperEntity->getCurrentRotAngle();

		if (axis == QVector3D(1.0, 0.0, 0.0))
		{
			QQmlProperty::write(tip_object_, "rotateAxis", QVariant::fromValue<QString>("X"));
		}
		else if (axis == QVector3D(0.0, 1.0, 0.0)) {

			QQmlProperty::write(tip_object_, "rotateAxis", QVariant::fromValue<QString>("Y"));
		}
		else if (axis == QVector3D(0.0, 0.0, 1.0))
		{
			QQmlProperty::write(tip_object_, "rotateAxis", QVariant::fromValue<QString>("Z"));
		}

		QQmlProperty::write(tip_object_, "rotateValue", QVariant::fromValue<float>(angle));
	}

	void RotateOp::onEndRotate(QQuaternion q)
	{
		auto groups = selectedGroups();
		if (groups.size() > 0)
		{
			onEndRotate_groups(q, groups);
		}
		else {
			auto list = selectedParts();
			onEndRotate_models(q, list);
		}
		creative_kernel::forceHideBox(false);
		endNodeSnap();
		requestVisUpdate(true);
	}

	void RotateOp::buildFromSelections()
	{
		if (m_selectedModels.size())
		{
			updateHelperEntity();
			visShow(m_helperEntity);
		}
		else
		{
			visHide(m_helperEntity);
		}

		requestVisPickUpdate(true);
		emit rotateChanged();
	}

	void RotateOp::reset()
	{
		auto groups = selectedGroups();
		if (groups.size() > 0)
		{
			reset_groups(groups);
		}
		else {
			auto list = selectedParts();
			reset_models(list);
		}
	}

	QVector3D RotateOp::rotate()
	{
		auto groups = selectedGroups();
		if (groups.size() > 0)
		{
			return rotate_groups(groups);
		}
		else {
			auto list = selectedParts();
			return rotate_models(list);
		}
	}

	void RotateOp::setRotate(QVector3D rotate)
	{
		auto groups = selectedGroups();
		if (groups.size() > 0)
		{
			setRotate_groups(rotate, groups);
		}
		else {
			auto list = selectedParts();
			setRotate_models(rotate, list);
		}
	}

	void RotateOp::updateHelperEntity()
	{
		if (m_selectedModels.size() && (!m_isRoate || m_isMoving))
		{
			trimesh::dbox3 box = creative_kernel::modelsBox(m_selectedModels);
			m_helperEntity->onBoxChanged(qtuser_3d::triBox2Box3D(box));
		}
	}

	bool RotateOp::getShowPop()
	{
		return m_bShowPop;
	}

	bool RotateOp::shouldMultipleSelect()
	{
		return true;
	}

	void RotateOp::reset_models(QList<creative_kernel::ModelN*>& models)
	{
		if (models.isEmpty())
		{
			return;
		}

		QList<ModelGroup*> groups = findRelativeGroups(models);
		beginNodeSnap(groups, models);

		qDebug() << "rotate reset";
		for (size_t i = 0; i < models.size(); i++)
		{
			models[i]->resetRotate();

			{
				trimesh::xform xMatrix = models[i]->getMatrix();
				QMatrix4x4 gMatrix = qtuser_3d::xform2QMatrix(trimesh::fxform(xMatrix));
				QVector3D trans = gMatrix.column(3).toVector3D();

				QVector3D scale;
				scale.setX(QVector3D(gMatrix(0, 0), gMatrix(1, 0), gMatrix(2, 0)).length());
				scale.setY(QVector3D(gMatrix(0, 1), gMatrix(1, 1), gMatrix(2, 1)).length());
				scale.setZ(QVector3D(gMatrix(0, 2), gMatrix(1, 2), gMatrix(2, 2)).length());

				QVector3D actualTrans = trans / scale;

				trimesh::xform newMatrix = trimesh::xform::scale(scale.x(), scale.y(), scale.z()) * trimesh::xform::trans(actualTrans);

				models[i]->setMatrix(newMatrix);
				models[i]->dirty();
			}
		}

		for (size_t i = 0; i < groups.size(); i++)
		{
			ModelGroup* g = groups.at(i);
			g->layerBottom();
			g->updateSweepAreaPath();
		}

		endNodeSnap();

		updateSpaceNodeRender(models, true);

		updateHelperEntity();

		emit rotateChanged();

	}

	QVector3D RotateOp::rotate_models(QList<creative_kernel::ModelN*>& models)
	{
		if (models.size() == 1)
		{
			return models.at(0)->localRotateAngle();
		}
		return QVector3D(0.0f, 0.0f, 0.0f);
	}

	void RotateOp::setRotate_models(QVector3D rotate, QList<creative_kernel::ModelN*>& models)
	{
		qDebug() << "rotate setRotate" << rotate;

		if (models.isEmpty())
		{
			return;
		}
		QVector3D currentAngles = this->rotate();
		rotate = rotate - currentAngles;

		QVector3D axis(0.0f, 0.0f, 0.0f);
		float angle = 0.0f;
		if (qAbs(rotate.x()) > 0.000001)
		{
			axis = QVector3D(1.0f, 0.0f, 0.0f);
			angle = rotate.x();
		}
		if (qAbs(rotate.y()) > 0.000001)
		{
			axis = QVector3D(0.0f, 1.0f, 0.0f);
			angle = rotate.y();
		}
		if (qAbs(rotate.z()) > 0.000001)
		{
			axis = QVector3D(0.0f, 0.0f, 1.0f);
			angle = rotate.z();
		}

		rotateByAxis(models, axis, angle);

		emit rotateChanged();
		updateHelperEntity();
		requestVisUpdate(true);

	}

	void RotateOp::onStartRotate_models(QList<creative_kernel::ModelN*>& models)
	{
		if (models.isEmpty())
		{
			return;
		}

		m_isRoate = true;

		m_rotateCenter = modelsBox(models).center();
		
		for (size_t i = 0; i < models.size(); i++)
		{
			models[i]->rotateByStandardAngle(QVector3D(0.0, 0.0, 0.0), 0.0, false);
			models[i]->snapMatrix();
		}

		QList<ModelGroup*>gs = findRelativeGroups(models);
		beginNodeSnap(gs, models);
		setNeedCheckScope(0);

		//Hide possible conflict states
		for (ModelGroup* g : gs)
		{
			g->setNozzleRegionVisibility(false);
			g->setOuterLinesVisibility(false);
		}

	}

	void RotateOp::onRotate_models(QQuaternion q, QList<creative_kernel::ModelN*>& models)
	{
		QVector3D axis; float angle;
		q.getAxisAndAngle(&axis, &angle);

		for (size_t i = 0; i < models.size(); i++)
		{
			ModelN* model = models.at(i);
			model->rotateByStandardAngle(m_helperEntity->getCurrentRotateAxis(), m_helperEntity->getCurrentRotAngle(), true);
			
			ModelGroup* group = model->getModelGroup();
			trimesh::xform gMatrix = group->getMatrix();
			trimesh::xform gMatrix_inv = trimesh::inv(gMatrix);

			trimesh::dvec3 c = gMatrix_inv * m_rotateCenter;
			trimesh::dvec3 v0 = gMatrix_inv * trimesh::dvec3(0.0);
			trimesh::dvec3 v1 = gMatrix_inv * trimesh::dvec3(axis.x(), axis.y(), axis.z());
			trimesh::dvec3 v = v1 - v0;
			trimesh::xform rot = trimesh::xform::rot(angle * M_PI_180, v);

			trimesh::xform snap_matrix = model->getSnapMatrix();
			trimesh::xform xf = trimesh::xform::trans(c) * rot * trimesh::xform::trans(-c) * snap_matrix;
			model->setMatrix(xf);
		}

		updateSpaceNodeRender(models, false);

		setNeedCheckScope(0);

		requestVisUpdate(false);
	}

	void RotateOp::onEndRotate_models(QQuaternion q, QList<creative_kernel::ModelN*>& models)
	{
		if (models.isEmpty())
		{
			return;
		}

		for (size_t i = 0; i < models.size(); i++)
		{
			models[i]->rotateByStandardAngle(m_helperEntity->getCurrentRotateAxis(), m_helperEntity->getCurrentRotAngle(), true);

			if (std::abs(q.x() - q.y()) > 0.00001)
			{
				models[i]->resetNestRotation();
			}
		}

		QList<ModelGroup* > groups = findRelativeGroups(models);
		for (ModelGroup* group : groups)
		{
			group->layerBottom();
			group->updateSweepAreaPath();
		}
		
		updateSpaceNodeRender(groups, false);
		updateSpaceNodeRender(models, false);
		updateHelperEntity();

		emit rotateChanged();
		m_bShowPop = false;
		m_isRoate = false;

		setTipObjectVisible(false);
	}

	void RotateOp::onStartRotate_groups(QList<creative_kernel::ModelGroup*>& groups)
	{
		beginNodeSnap(groups, QList<ModelN*>());
		m_isRoate = true;

		m_rotateCenter = groupsGlobalBoundingBox(groups).center();

		for (size_t i = 0; i < groups.size(); i++)
		{
			groups[i]->rotateByStandardAngle(QVector3D(0.0, 0.0, 0.0), 0.0, false);
			groups[i]->snapMatrix();

			//Hide possible conflict states
			groups[i]->setNozzleRegionVisibility(false);
			groups[i]->setOuterLinesVisibility(false);
		}

		setNeedCheckScope(0);
	}

	void RotateOp::onRotate_groups(QQuaternion q, QList<creative_kernel::ModelGroup*>& groups)
	{
		QMatrix4x4 qm;
		qm.rotate(q);
		trimesh::xform qxf = trimesh::xform(qtuser_3d::qMatrix2Xform(qm));

		for (size_t i = 0; i < groups.size(); i++)
		{
			groups[i]->rotateByStandardAngle(m_helperEntity->getCurrentRotateAxis(), m_helperEntity->getCurrentRotAngle(), true);

			trimesh::dvec3 c = m_rotateCenter;
			trimesh::xform snap_matrix = groups[i]->getSnapMatrix();
			trimesh::xform xf = trimesh::xform::trans(c) * qxf * trimesh::xform::trans(- c) * snap_matrix;
			groups[i]->setMatrix(xf);
		}

		updateSpaceNodeRender(groups, false);

		setNeedCheckScope(0);
		requestVisUpdate(true);
	}

	void RotateOp::onEndRotate_groups(QQuaternion q, QList<creative_kernel::ModelGroup*>& groups)
	{
		if (groups.isEmpty())
		{
			return;
		}

		for (size_t i = 0; i < groups.size(); i++)
		{
			groups[i]->rotateByStandardAngle(m_helperEntity->getCurrentRotateAxis(), m_helperEntity->getCurrentRotAngle(), true);
			groups[i]->layerBottom();
			groups[i]->updateSweepAreaPath();
		}

		updateHelperEntity();

		emit rotateChanged();
		m_bShowPop = false;
		m_isRoate = false;

		setTipObjectVisible(false);
	}

	void RotateOp::reset_groups(QList<creative_kernel::ModelGroup*>& groups)
	{
		qDebug() << __FUNCTION__;
		if (groups.isEmpty())
		{
			return;
		}

		beginNodeSnap(groups, QList<ModelN*>());

		for (creative_kernel::ModelGroup* g : groups)
		{
			g->resetRotate();

			QMatrix4x4 initMatrix;
			trimesh::xform xMatrix = g->getMatrix();
			QMatrix4x4 gMatrix = qtuser_3d::xform2QMatrix(trimesh::fxform(xMatrix));
			QVector3D trans = gMatrix.column(3).toVector3D();

			QVector3D scale;
			scale.setX(QVector3D(gMatrix(0, 0), gMatrix(1, 0), gMatrix(2, 0)).length());
			scale.setY(QVector3D(gMatrix(0, 1), gMatrix(1, 1), gMatrix(2, 1)).length());
			scale.setZ(QVector3D(gMatrix(0, 2), gMatrix(1, 2), gMatrix(2, 2)).length());

			//QQuaternion qrot = initq;

			initMatrix.translate(trans);
			//initMatrix.rotate(qrot);
			initMatrix.scale(scale);

			g->setMatrix(trimesh::xform(qtuser_3d::qMatrix2Xform(initMatrix)));
			g->layerBottom();
			g->updateSweepAreaPath();
			g->dirty();
		}
		
		updateSpaceNodeRender(groups, true);
		endNodeSnap();

		updateHelperEntity();

		requestVisPickUpdate(true);
		emit rotateChanged();
	}

	QVector3D RotateOp::rotate_groups(QList<creative_kernel::ModelGroup*>& groups)
	{
		if (groups.size() == 1)
		{
			return groups.at(0)->localRotateAngle();
		}
		return QVector3D(0.0f, 0.0f, 0.0f);
	}

	void RotateOp::setRotate_groups(QVector3D rotate, QList<creative_kernel::ModelGroup*>& groups)
	{
		qDebug() << __FUNCTION__;

		if (groups.isEmpty()) return;

		beginNodeSnap(groups, QList<ModelN *>());

		QVector3D currentAngles = this->rotate();
		rotate = rotate - currentAngles;

		QVector3D axis = QVector3D(0.0f, 0.0f, 0.0f);
		float angle = 0.0f;

		if (qAbs(rotate.x()) > 0.000001)
		{
			axis = QVector3D(1.0f, 0.0f, 0.0f);
			angle = rotate.x();
		}
		if (qAbs(rotate.y()) > 0.000001)
		{
			axis = QVector3D(0.0f, 1.0f, 0.0f);
			angle = rotate.y();
		}
		if (qAbs(rotate.z()) > 0.000001)
		{
			axis = QVector3D(0.0f, 0.0f, 1.0f);
			angle = rotate.z();
		}

		for (creative_kernel::ModelGroup* g : groups)
		{
			g->rotateByStandardAngle(axis, angle);

			trimesh::xform rot = trimesh::xform::rot(angle * M_PI_180, axis);
			
			trimesh::xform m = g->getMatrix();
			trimesh::dvec3 c = g->globalBoundingBox().center();
			
			trimesh::xform xf = trimesh::xform::trans(c) * rot * trimesh::xform::trans(-c) * m;
			g->setMatrix(xf);

			g->layerBottom();
			g->updateSweepAreaPath();
		}
		
		endNodeSnap();

		requestVisPickUpdate(true);
		emit rotateChanged();
	}

	void RotateOp::setTipObjectPos(const QPoint& point)
	{
		if (tip_object_)
		{
			QQmlProperty::write(tip_object_, "posX", QVariant::fromValue<int>(point.x()));
			QQmlProperty::write(tip_object_, "posY", QVariant::fromValue<int>(point.y()));
		}
	}

	void RotateOp::setTipObjectVisible(bool visible)
	{
		if (tip_object_)
		{
			QQmlProperty::write(tip_object_, "isVisible", QVariant::fromValue<bool>(visible));
		}
	}
	
	bool RotateOp::getTipObjectVisible()
	{
		if (tip_object_)
		{
			QVariant var = QQmlProperty::read(tip_object_, "isVisible");
			return var.toBool();
		}

		return false;
	}
}