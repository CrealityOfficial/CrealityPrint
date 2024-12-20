#include "splitop.h"

#include "splitjob.h"

#include "entity/alonepointentity.h"
#include "entity/lineexentity.h"
#include "entity/splitplane.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/trimesh2/conv.h"
#include "qtuser3d/math/space3d.h"

#include "cxkernel/interface/jobsinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/eventinterface.h"
#include "interface/spaceinterface.h"
#include "data/modeln.h"

#include "kernel/kernelui.h"
#include "interface/printerinterface.h"

using namespace creative_kernel;
using namespace qtuser_3d;
SplitOp::SplitOp(QObject* parent)
	:MoveOperateMode(parent)
	, m_rotateAngle(0, 0, 0)
	, m_lineEntity(nullptr)
	, m_pointEntity(nullptr)
	, m_splitPlane(nullptr)
	, m_operate_cut_flag(false)
	, m_selectPlaneByCursor(false)
	, m_offset_inner(0)
	, m_capture(false)
	, m_canSplitPlaneMove(false)
	, m_bIndicate(false)
	, m_planeMoveOnClick(false)
	, m_gap(0)
	, m_tempHidePlane(false)
{
	m_type = qtuser_3d::SceneOperateMode::FixedMode;
}

SplitOp::~SplitOp()
{

}

void SplitOp::onAttach()
{
	m_axisType = 2;
	m_plane = qtuser_3d::Plane();

	getKernelUI()->setAutoResetOperateMode(false);
	if (!m_splitPlane)
	{
		m_splitPlane = new SplitPlane();
		visHide(m_splitPlane);
	}

	tracePickable(m_splitPlane->pickable());

	if (!m_lineEntity)
	{
		m_lineEntity = new LineExEntity();
		m_lineEntity->setLineWidth(4);
		m_lineEntity->setColor(QVector4D(1.0f, 0.7529f, 0.0f, 1.0));
		visHide(m_lineEntity);
	}

	if (!m_pointEntity)
	{
		m_pointEntity = new AlonePointEntity();
		m_pointEntity->setColor(QVector4D(1.0f, 0.7529f, 0.0f, 1.0));
		m_pointEntity->setPointSize(6.0);
	}

	updateSelected();
	onSelectionsBoxChanged();

	prependLeftMouseEventHandler(this);
	// addLeftMouseEventHandler(this);
	addHoverEventHandler(this);
	addKeyEventHandler(this);

	//traceSpace(this);
	addModelNSelectorTracer(this);

	setAcitiveAxis(m_axisType);

	notifyOffsetChanged();

	m_bShowPop = true;
	m_tempHidePlane = false;
	m_ignoreSelectChangeEvent = false;

	if (m_onAxitTypeChanged)
	{
		m_offset_inner = 0.0f;
		m_onAxitTypeChanged = false;
	}
}

void SplitOp::onDettach()
{
	getKernelUI()->setAutoResetOperateMode(true);
	enableSelectPlaneByCursor(false);

	m_capture = false;
	visHide(m_splitPlane);
	unTracePickable(m_splitPlane->pickable());

	visHide(m_lineEntity);
	visHide(m_pointEntity);

	removeLeftMouseEventHandler(this);
	removeHoverEventHandler(this);
	removeKeyEventHandler(this);

	//unTraceSpace(this);
	removeModelNSelectorTracer(this);
	requestVisUpdate(true);

	m_bShowPop = false;
}

void SplitOp::setRotateAngle(QVector3D axis, float angle)
{
	for (int i = 0; i < 3; i++)
	{
		if (qAbs(axis[i]) < 0.01)
		{
			axis[i] = 0;
		}
		else
		{
			axis[i] = axis[i] > 0 ? 1 : -1;
		}
	}
	QVector3D v = axis * angle;
	m_rotateAngle = m_saveAngle + v;
	emit rotateAngleChanged();
}

void SplitOp::onStartRotate()
{
	m_saveNormal = m_plane.dir;
	m_saveAngle = m_rotateAngle;
}

void SplitOp::onRotate(QQuaternion q)
{
	QVector3D n = q * m_saveNormal;
	setPlaneNormal(n);
}

void SplitOp::onEndRotate(QQuaternion q)
{
	QVector3D n = q * m_saveNormal;
	setPlaneNormal(n);
}
void SplitOp::enabledIndicate(bool enable)
{
	m_bIndicate = enable;
}
void SplitOp::setModelGap(float gap)
{
	m_gap = gap;
}

void SplitOp::setPlaneDir(const QVector3D& rotate)
{
	QList<ModelN*> selections = selectionms();
	m_rotateAngle = rotate;
	m_saveAngle = rotate;

	qDebug() << "setPlaneDir";

	if (selections.size())
	{
		QVector3D dir(0.0f, 0.0f, 1.0f);

		QVector3D axis = QVector3D(1.0f, 0.0f, 0.0f);
		float angle = rotate.x();
		QQuaternion q = QQuaternion::fromAxisAndAngle(axis, angle);
		QVector3D nx = q * dir;
		setPlaneNormal(nx);

        axis = QVector3D(0.0f, 1.0f, 0.0f);
        angle = rotate.y();
		q = QQuaternion::fromAxisAndAngle(axis, angle);
		QVector3D ny = q * m_plane.dir;
		setPlaneNormal(ny);
       
        axis = QVector3D(0.0f, 0.0f, 1.0f);
        angle = rotate.z();
		q = QQuaternion::fromAxisAndAngle(axis, angle);
		QVector3D nz = q * m_plane.dir;
		setPlaneNormal(nz);
   }
}

QVector3D& SplitOp::planeRotateAngle()
{
	return m_rotateAngle;
}

void SplitOp::onSelectionsBoxChanged()
{
	 if (!m_operateNodes.isEmpty())
	 	setPlanePosition(calculateNodesBox().center() + m_planePositionOffset);
	 updatePlaneEntity();
}

void SplitOp::onSelectionsChanged()
{
	if (m_ignoreSelectChangeEvent)
		return;

	if (!m_capture && !m_operateNodes.isEmpty())  // the splitPlane is selected
		return;

	auto nodesCache = m_operateNodes;

	updateSelected();

	if (!m_operateNodes.isEmpty())
	{
		if (nodesCache == m_operateNodes)
			setPlanePosition(calculateNodesBox().center() + m_planePositionOffset);
		else
		{
			setPlanePosition(calculateNodesBox().center());
			m_offset_inner = 0.0f;
		}
	}
	
	m_maxOffset = calMaxOffset();
	updatePlaneEntity();

	notifyOffsetChanged();
	requestVisPickUpdate(true);
	
}

bool SplitOp::shouldMultipleSelect()
{
	return false;
}

void SplitOp::setSelectedModel(creative_kernel::ModelN* model)
{
	if (model)
	{
		if (m_selectPlaneByCursor)
		{
			visHide(m_splitPlane);
		}
		else {
			visShow(m_splitPlane);
		}
		setAcitiveAxis(m_axisType);
	}
	else
	{
		visHide(m_lineEntity);
		visHide(m_splitPlane);
	}

	//m_splitPlane->setTargetModel(model);

	requestVisPickUpdate(true);
}

void SplitOp::onKeyPress(QKeyEvent* event)
{
#ifdef _DEBUG
	if (event->key() == Qt::Key_S)
	{
		split();
	}
#endif
}

void SplitOp::setCutToParts(bool cutflag)
{
	m_operate_cut_flag = cutflag;
}
bool SplitOp::getCutToParts()
{
	return m_operate_cut_flag;
}

//void SplitOp::onHoverMove(QHoverEvent* event)
//{
//	if (m_selectPlaneByCursor)
//		showCursorPoint(event->pos());
//}

void SplitOp::split()
{
	if (cxkernel::jobExecutorAvaillable())
	{
		auto job = QSharedPointer<SplitJob>::create();
		connect(job.get(), &SplitJob::beforeModifyScene, this, &SplitOp::beforeModifyScene);
		connect(job.get(), &SplitJob::onSplitSuccess, this, &SplitOp::onSplitSuccessed);
		CutParam param;
		param.isCutGroup = true;

		QList<ModelN*> models;
		ModelGroup* modelGroup = NULL;
		QList<ModelGroup*> modelGroups;

		for (creative_kernel::QNode3D* node : m_operateNodes)
		{
			creative_kernel::ModelGroup* group = dynamic_cast<creative_kernel::ModelGroup*>(node);
			ModelN* model = dynamic_cast<ModelN*>(node);
			if (model)
			{
				param.isCutGroup = false;
				models << model;
				modelGroup = model->getModelGroup();
			}
			else if (group)
			{
				modelGroups << group;
			}
		}
		param.gap = m_gap;
		param.cut2Parts = m_operate_cut_flag;
		param.normal = trimesh::dvec3(m_plane.dir.x(), m_plane.dir.y(), m_plane.dir.z());
		param.position = trimesh::dvec3(m_plane.center.x(), m_plane.center.y(), m_plane.center.z());

		job->setCutParam(param);
		if (modelGroup)
			job->setParts(models, modelGroup);
		else 
			job->setModelGroups(modelGroups);
		//job->setModelGroup(modelGroup);
		job->prepareWork();
		cxkernel::executeJob(job);
	}

	// after splitJob success, go into selectionChanged event
	m_capture = true;
}

const Plane& SplitOp::plane()
{
	return m_plane;
}

void SplitOp::setPlanePosition(const QVector3D& position)
{
	m_planePositionOffset = position - calculateNodesBox().center();
	m_plane.center = position;

	emit posionChanged();
}

void SplitOp::setPlaneNormal(const QVector3D& normal)
{
	m_plane.dir = normal;
	if (normal.length() == 0.0f)
		m_plane.dir = QVector3D(0.0f, 0.0f, 1.0f);
	m_plane.dir.normalize();
	
}

void SplitOp::updatePlaneEntity()
{
	if (!m_splitPlane)
		return;

	if (m_tempHidePlane || m_operateNodes.isEmpty())
	{
		visHide(m_splitPlane);
		return;
	}

	// m_splitPlane->setEnabled(false);
	visHide(m_splitPlane);

	trimesh::box3 modlesBox = qtuser_3d::qBox32box3( calculateNodesBox() );
	
	m_splitPlane->updatePlanePosition(modlesBox);

	QMatrix4x4 m;
	QQuaternion q = QQuaternion::fromDirection(m_plane.dir, QVector3D(0.0f, 0.0f, 1.0f));
	m.translate(m_plane.center);
	m.rotate(q);
	m_splitPlane->setPose(m);

	 visShow(m_splitPlane);

	requestVisUpdate(true);

}

bool SplitOp::getShowPop()
{
	return m_bShowPop;
}

bool SplitOp::getMessage()
{
	return false;
}

void SplitOp::setMessage(bool isRemove)
{
	if (isRemove)
	{
		requestVisUpdate(true);
	}
}

void SplitOp::onLeftMouseButtonClick(QMouseEvent* event)
{
#ifdef _DEBUG
	printf("%s\n", __func__);
#endif

	if (m_bIndicate && !m_pressPoint.isNull())
	{
		planeMoveOnClickPos(m_pressPoint);
	}

	if (m_pickEntity)
		m_leftClickStatus = false;
}

void SplitOp::onLeftMouseButtonPress(QMouseEvent* event)
{
	m_onAxitTypeChanged = false;
	int pID = -1;
	m_capture = false;
	m_tempHidePlane = false;
	m_pressPoint = QPoint();
	QVector3D position, normal;

	qtuser_3d::Pickable* pickable = nullptr;
	pickable = checkPickable(event->pos(), &pID);
	if (pickable == m_splitPlane->pickable())  // splitPlane pickable
	{
		m_capture = false;
		m_savePoint = event->pos();
		m_canSplitPlaneMove = true;
		m_maxOffset = calMaxOffset();
		m_pickEntity = true;

		m_leftPressStatus = false;
		return;
	}

	ModelN* model = dynamic_cast<ModelN*>(pickable);
	if (!model)
	{
		m_pickEntity = false;
		m_capture = true;
		m_firstPosition = makeWorldPositionFromScreen(event->pos());
		return;
	}
	
	m_pickEntity = true;
	int oriFaceId = model->getFacet2Facets(pID);
	model->rayCheckEx(oriFaceId, visRay(event->pos()), position, normal);
	m_capture = true;
	m_firstPosition = position;

	bool alt = event->modifiers() & Qt::KeyboardModifier::AltModifier;
	bool ctrl = event->modifiers() & Qt::KeyboardModifier::ControlModifier;
	if (alt)
	{
		//unselectAll();
		selectModelN(model, ctrl);
		m_leftPressStatus = false;
	}
	else 
	{
		ModelGroup* modelGroup = model->getModelGroup();
		selectGroup(modelGroup, ctrl);
		m_leftPressStatus = false;
	}

	updateSelected();

	// if (m_bIndicate)
	// {
	// 	planeMoveOnClickPos(event->pos());
	// }
	
	// fix :[ID1028150] when in split operate mode, moving is NOT allowed
	//if (m_axisType != 3)
	//{
	//	MoveOperateMode::onLeftMouseButtonPress(event);
	//}
	//else
	//{
	//	m_generatePlane = false;
	//}

	m_generatePlane = false;
	m_pressPoint = event->pos();
}

void SplitOp::onLeftMouseButtonMove(QMouseEvent* event)
{
	// fix :[ID1028150] when in split operate mode, moving is NOT allowed
	//if (m_axisType != 3)
	//	MoveOperateMode::onLeftMouseButtonMove(event);

	if (!m_capture)
		processPlaneMove(event->pos());
	else
		processCursorMoveEvent(event->pos());

	//this call is very import in mac
	requestVisUpdate();
	
	if (m_pickEntity)
		m_leftMoveStatus = false;
}

void SplitOp::onLeftMouseButtonRelease(QMouseEvent* event)
{
	// fix :[ID1028150] when in split operate mode, moving is NOT allowed
	//if (m_axisType != 3)
	//	MoveOperateMode::onLeftMouseButtonRelease(event);

#ifdef _DEBUG
	printf("%s\n", __func__);
#endif

	if (!m_capture)
	{
		creative_kernel::requestVisPickUpdate(false);
	}
	else
	{
		showSplitPlaneOnRelease(event->pos());

		m_maxOffset = calMaxOffset();
		visHide(m_lineEntity);
	}
	m_canSplitPlaneMove = false;

	if (m_pickEntity)
		m_leftReleaseStatus = false;

}

void SplitOp::setAcitiveAxis(int axisType)
{
	m_axisType = axisType;
	m_offset_inner = 0.0;

	if (m_axisType < 3)
	{
		m_tempHidePlane = false;
		QVector3D axis;
		QVector3D size = calculateNodesBox().size();
		switch (m_axisType)
		{
		case 0:
            axis = QVector3D(1, 0, 0);
			 m_offset_inner = -size.x() / 2.0;
            break;
        case 1:
            axis = QVector3D(0, 1, 0);
			 m_offset_inner = -size.y() / 2.0;
            break;
        case 2:
            axis = QVector3D(0, 0, 1);
			 m_offset_inner = -size.z() / 2.0;
            break;
		}

		enableSelectPlaneByCursor(false);
		QVector3D center;
		if (getSelectedModelsCenter(&center))
		{
			m_plane.center = center;
		}
			

		setPlaneNormal(axis);
		updatePlaneEntity();
	} 
	else
	{
        enableSelectPlaneByCursor(true);
		m_tempHidePlane = true;
	}

	m_maxOffset = calMaxOffset();
	if(m_axisType > 2)
		m_offset_inner = 0.0f;

	if (!m_operateNodes.isEmpty())
	{
		qtuser_3d::Box3D box = calculateNodesBox();
		float xLen_half = box.size().x() / 2.0f;
		float yLen_half = box.size().y() / 2.0f;
		float zLen_half = box.size().z() / 2.0f;
		switch (m_axisType)
		{
		case 0:
			m_offset = xLen_half;
			break;
		case 1:
			m_offset = yLen_half;
			break;
		case 2:
			m_offset = zLen_half;
			break;

		default:
			m_offset = 0.0f;
			break;
		}
	}
	emit offsetChanged();

	m_onAxitTypeChanged = true;

}

float  SplitOp::toOffsetInner(float off)
{
	if (!m_operateNodes.isEmpty())
	{
		qtuser_3d::Box3D box = calculateNodesBox();
		float xLen_half = box.size().x() / 2.0f;
		float yLen_half = box.size().y() / 2.0f;
		float zLen_half = box.size().z() / 2.0f;
		switch (m_axisType)
		{
		case 0:
			off -= xLen_half;
			break;
		case 1:
			off -= yLen_half;
			break;
		case 2:
			off -= zLen_half;
			break;

		default:
			break;
		}
	}

	return off;
}

void SplitOp::setOffset(float off)
{
	float tmpOffset = off;
	if (qAbs(tmpOffset) > m_maxOffset)
	{
		if (tmpOffset < 0.0f)
			tmpOffset = -m_maxOffset;
		else
			tmpOffset = m_maxOffset;
	}

	QVector3D newCenter = m_plane.center + m_plane.dir * (tmpOffset - m_offset_inner);
	setPlanePosition(newCenter);
	m_offset_inner = tmpOffset;

	updatePlaneEntity();

	notifyOffsetChanged();

	if (m_onAxitTypeChanged)
	{
		m_offset_inner = 0.0f;
		m_onAxitTypeChanged = false;
	}
}

float SplitOp::getOffset()
{
	m_onAxitTypeChanged = false;
	return m_offset;
}

float SplitOp::getGap()
{
	return m_gap;
}

void SplitOp::tryCollectMouseClickEvent(QMouseEvent* event)
{
	QList<creative_kernel::ModelN*> selections = selectionms();
	if (selections.size() == 0)
		return;

	if (!m_selectPlaneByCursor)
		return;

	if (m_selectedPosition.size() >= 2)
	{
		m_selectedPosition.clear();
	}

	QVector3D pos = makeWorldPositionFromScreen(event->pos());
	m_selectedPosition.push_back(pos);

	if (m_selectedPosition.size() >= 2) {
		setCustomPlanePosition();
	}
	else {
		visHide(m_splitPlane);
	}
}


void SplitOp::processCursorMoveEvent(const QPoint& pos)
{
	if (!m_selectPlaneByCursor || !m_capture)
		return;

	m_offset_inner = 0.0f;
	visHide(m_lineEntity);

	QVector3D qPosition, qNormal;
	ModelN* model = checkPickModelEx(pos, qPosition, qNormal);

	trimesh::vec3 position = qtuser_3d::qVector3D2Vec3(qPosition);
	trimesh::vec3 normal = qtuser_3d::qVector3D2Vec3(qNormal);

	QVector<QVector3D> positions;
	positions.push_back(m_firstPosition);

	QVector3D secondPosition;
	if (model)
	{
		secondPosition = qtuser_3d::vec2qvector(position);
	}
	else
	{
		secondPosition = makeWorldPositionFromScreen(pos);
	}
	positions.push_back(secondPosition);
	m_generatePlane = (secondPosition.distanceToPoint(m_firstPosition) > 1);	//防抖动

	m_lineEntity->updateGeometry(positions);
	visShow(m_lineEntity);
}

bool SplitOp::getSelectedModelsCenter(QVector3D* outCenter)
{
	bool valid = false;
	if (!m_operateNodes.isEmpty())
	{
		qtuser_3d::Box3D box = calculateNodesBox();
		QVector3D qcenter = box.center();
		*outCenter = qcenter;
		valid = true;
	}

	return valid;
}

void SplitOp::enableSelectPlaneByCursor(bool enable)
{
	m_selectPlaneByCursor = enable;
	m_selectedPosition.clear();

	visHide(m_pointEntity);
	visHide(m_lineEntity);

	if (enable)
	{
		visHide(m_splitPlane);
	}
	else {
		if (!m_operateNodes.isEmpty())
			visShow(m_splitPlane);
	}
	requestVisUpdate(false);
}

QVector3D SplitOp::makeWorldPositionFromScreen(const QPoint& pos)
{
	qtuser_3d::ScreenCamera* screenCamera = visCamera();
	QSize size = screenCamera->size();
	QMatrix4x4 p = screenCamera->projectionMatrix();
	QMatrix4x4 v = screenCamera->viewMatrix();

	int x = pos.x();
	int y = pos.y();

	float X = float(x) / size.width() * 2.0 - 1.0;
	float Y = (1.0 - float(y) / size.height()) * 2.0 - 1.0;

	QVector4D ndcP(X, Y, 0.0, 1.0);

	QMatrix4x4 inv_vp = (p * v).inverted();

	QVector4D worldP = inv_vp * ndcP;
	worldP /= worldP.w();
	return QVector3D(worldP);
}

void SplitOp::setCustomPlanePosition()
{
	if (m_selectedPosition.size() < 2)
		return; 

	QVector3D start = m_selectedPosition.at(0);
	QVector3D end = m_selectedPosition.at(1);
	QVector3D v = end - start;

	QVector3D viewPos = cameraController()->getViewPosition();

	QVector3D viewDir = viewPos - end;
	QVector3D planeDir = QVector3D::crossProduct(v , viewDir);
	planeDir.normalize();

	QVector3D planeNormal = planeDir;
	setPlaneNormal(planeNormal);

	QVector3D boxCenter;
	if (getSelectedModelsCenter(&boxCenter))
	{
		float dis = boxCenter.distanceToPlane(start, planeNormal);
		QVector3D planeCenter = boxCenter - planeNormal * dis;
		setPlanePosition(planeCenter);

		visHide(m_lineEntity);
	}
}

void SplitOp::processPlaneMove(const QPoint& pos)
{
	if (m_operateNodes.isEmpty())
		return;

	QVector3D viewCenter = visCamera()->camera()->viewCenter();
	QVector3D cameraPosition = visCamera()->camera()->position();

	QVector3D dir = viewCenter - cameraPosition;
	dir.normalize();

	qtuser_3d::Ray ray0 = visCamera()->screenRay(m_savePoint);
	qtuser_3d::Ray ray = visCamera()->screenRay(pos);

	QVector3D c0, c;
	qtuser_3d::lineCollidePlane(viewCenter, dir, ray0, c0);
	qtuser_3d::lineCollidePlane(viewCenter, dir, ray, c);

	QVector3D delta = c - c0;

	float dotVal = QVector3D::dotProduct(delta, m_plane.dir);

	//trimesh::vec3 position = m_position + m_dir * dotVal;
	QVector3D position = m_plane.center + m_plane.dir * dotVal;

	updateOffset(position);

	if (!m_canSplitPlaneMove)
	{
		m_savePoint = pos;
		return;
	}

	visHide(m_splitPlane);

	QMatrix4x4 m;
	QQuaternion q = QQuaternion::fromDirection(m_plane.dir, QVector3D(0.0f, 0.0f, 1.0f));
	m.translate(position);
	m.rotate(q);
	m_splitPlane->setPose(m);

	visShow(m_splitPlane);

	m_savePoint = pos;

	notifyOffsetChanged();
}

void SplitOp::planeMoveOnClickPos(const QPoint& pos)
{
	if (m_operateNodes.isEmpty())
		return;

	if (!m_bIndicate)
		return;

	QVector3D delta = m_firstPosition - m_plane.center;

	visHide(m_splitPlane);

	float dotVal = QVector3D::dotProduct(delta, m_plane.dir);

	QVector3D position = m_plane.center + m_plane.dir * dotVal;

	updateOffset(position);

	if (!m_canSplitPlaneMove)
		return;

	QMatrix4x4 m;
	QQuaternion q = QQuaternion::fromDirection(m_plane.dir, QVector3D(0.0f, 0.0f, 1.0f));
	m.translate(m_plane.center);
	m.rotate(q);
	m_splitPlane->setPose(m);

	visShow(m_splitPlane);

	requestVisPickUpdate(true);
	// to prevent updating plane when "onLeftMouseButtonRelease" happens
	// m_planeMoveOnClick = true;

}

float SplitOp::calMaxOffset()
{
	if (m_operateNodes.isEmpty())
		return 0.0f;

	float offsetValue = 0.002f;
	qtuser_3d::Box3D modelSpaceBox = calculateNodesBox();
	
	QVector3D boxCenter = modelSpaceBox.center();

	QVector3D boxMin = modelSpaceBox.min;
	QVector3D boxMax = modelSpaceBox.max;

	float xLen = modelSpaceBox.size().x();
	float yLen = modelSpaceBox.size().y();
	float zLen = modelSpaceBox.size().z();

	trimesh::vec3 p0 = qtuser_3d::qVector3D2Vec3(boxMin);
	trimesh::vec3 p1;
	p1.x = p0.x;
	p1.y = p0.y + yLen;
	p1.z = p0.z;

	trimesh::vec3 p2;
	p2.x = p0.x + xLen;
	p2.y = p0.y;
	p2.z = p0.z;

	trimesh::vec3 p3;
	p3.x = p0.x + xLen;
	p3.y = p0.y + yLen;
	p3.z = p0.z;

	trimesh::vec3 p7 = qtuser_3d::qVector3D2Vec3(boxMax);

	trimesh::vec3 p4;
	p4.x = p7.x;
	p4.y = p7.y - yLen;
	p4.z = p7.z;

	trimesh::vec3 p5;
	p5.x = p7.x - xLen;
	p5.y = p7.y;
	p5.z = p7.z;

	trimesh::vec3 p6;
	p6.x = p7.x - xLen;
	p6.y = p7.y - yLen;
	p6.z = p7.z;

	float disArr[8];

	QVector3D qp0 = qtuser_3d::vec2qvector(p0);
	disArr[0] = qAbs(qp0.distanceToPlane(boxCenter, m_plane.dir));

	QVector3D qp1 = qtuser_3d::vec2qvector(p1);
	disArr[1] = qAbs(qp1.distanceToPlane(boxCenter, m_plane.dir));

	QVector3D qp2 = qtuser_3d::vec2qvector(p2);
	disArr[2] = qAbs(qp2.distanceToPlane(boxCenter, m_plane.dir));

	QVector3D qp3 = qtuser_3d::vec2qvector(p3);
	disArr[3] = qAbs(qp3.distanceToPlane(boxCenter, m_plane.dir));

	QVector3D qp4 = qtuser_3d::vec2qvector(p4);
	disArr[4] = qAbs(qp4.distanceToPlane(boxCenter, m_plane.dir));

	QVector3D qp5 = qtuser_3d::vec2qvector(p5);
	disArr[5] = qAbs(qp5.distanceToPlane(boxCenter, m_plane.dir));

	QVector3D qp6 = qtuser_3d::vec2qvector(p6);
	disArr[6] = qAbs(qp6.distanceToPlane(boxCenter, m_plane.dir));

	QVector3D qp7 = qtuser_3d::vec2qvector(p7);
	disArr[7] = qAbs(qp7.distanceToPlane(boxCenter, m_plane.dir));

	float maxDis = 0.0f;
	for (int i = 0; i < 8; i++)
	{
		if (disArr[i] >= maxDis)
			maxDis = disArr[i];
	}

	return maxDis + offsetValue;
}

void SplitOp::updateOffset(const QVector3D& position)
{
	if (m_operateNodes.isEmpty())
		return;

	qtuser_3d::Box3D modelSpaceBox = calculateNodesBox();

	QVector3D deltaOffset;
	deltaOffset = position - modelSpaceBox.center();

	m_offset_inner = QVector3D::dotProduct(deltaOffset, m_plane.dir);

	if (qAbs(m_offset_inner) > m_maxOffset)
	{
		m_canSplitPlaneMove = false;
	}
	else
	{
		m_canSplitPlaneMove = true;
		setPlanePosition(position);
	}

	notifyOffsetChanged();

}


void SplitOp::showSplitPlaneOnRelease(const QPoint& releasePos)
{
	if (!m_selectPlaneByCursor)
		return;

	if (m_operateNodes.isEmpty())
		return;

	if (!m_generatePlane)
		return;

	if (1)
	{
		QVector3D position = makeWorldPositionFromScreen(releasePos);
		QVector3D v = position - m_firstPosition;

		float d = v.length();
		if (d < 0.1f)
			return;

		QVector3D viewPos = cameraPosition();

		QVector3D viewDir = viewPos - position;
		QVector3D planeDir = QVector3D::crossProduct(v, viewDir);
		planeDir.normalize();

		setPlaneNormal(planeDir);

		QVector3D boxCenter = calculateNodesBox().center();
		float dis = boxCenter.distanceToPlane(m_firstPosition, planeDir);
		QVector3D planeCenter = boxCenter - planeDir * dis;
		setPlanePosition(planeCenter);

		updatePlaneEntity();

		updateOffset(planeCenter);

		m_offset_inner = 0;
		notifyOffsetChanged();
		m_planeMoveOnClick = false;
		m_pressPoint = QPoint();
		m_generatePlane = false;
	}
	
}

void SplitOp::showCursorPoint(const QPoint& pos)
{
	visHide(m_pointEntity);

	QList<creative_kernel::ModelN*> selections = selectionms();
	if (selections.size() < 1)
		return ;

	QVector3D ve3d_position = makeWorldPositionFromScreen(pos);
	{
		size_t size = m_selectedPosition.size();
		switch (size)
		{
		case 0:
		case 2:
		{
			QMatrix4x4 matrix;
			matrix.setToIdentity();
			matrix.translate(ve3d_position);
			m_pointEntity->setPose(matrix);
			visShow(m_pointEntity);

			requestVisUpdate(false);
		}
		break;

		case 1:
		{
			QVector<QVector3D> positions;
			positions.push_back(m_selectedPosition.at(0));
			positions.push_back(ve3d_position);
			m_lineEntity->updateGeometry(positions);
			visShow(m_lineEntity);
			requestVisUpdate(false);
		}
		break;

		default:
			break;
		}
	}

}

void SplitOp::notifyOffsetChanged()
{

	if (qAbs(m_offset_inner) > m_maxOffset)
		return;

	if (m_onAxitTypeChanged)
		return;

	m_offset = m_offset_inner;
	
	if (!m_operateNodes.isEmpty())
	{
		qtuser_3d::Box3D box = calculateNodesBox();
		float xLen_half = box.size().x()/2.0f;
		float yLen_half = box.size().y() / 2.0f;
		float zLen_half = box.size().z() / 2.0f;
		switch (m_axisType)
		{
		case 0:
			m_offset += xLen_half;
			break;
		case 1 :
			m_offset += yLen_half;
			break;
		case 2:
			m_offset += zLen_half;
			break;

		default:
			break;
		}
	}
	emit offsetChanged();
}

void SplitOp::updateSelected()
{
	m_operateNodes.clear();

	QList<ModelGroup*> groups = selectedGroups();
	if (groups.size() != 0)
	{
		for (ModelGroup* group : groups)
		{
			m_operateNodes << group;
		}
	}
	else 
	{
		QList<ModelN*> msels = selectionms();
		if (msels.size() != 0)
		{
			for (ModelN* model : msels)
			{
				m_operateNodes << model;
			}
		}
	}

	cancelRedundantSelected();
}

void SplitOp::cancelRedundantSelected()
{

}

qtuser_3d::Box3D SplitOp::calculateNodesBox()
{
	trimesh::dbox3 box;
	for (creative_kernel::QNode3D* node : m_operateNodes)
		box += node->globalBoundingBox();
	
	return qtuser_3d::triBox2Box3D(box);
}

void SplitOp::beforeModifyScene()
{
	m_ignoreSelectChangeEvent = true;
}

void SplitOp::onSplitSuccessed()
{
	m_ignoreSelectChangeEvent = false;
	unselectAll();
}