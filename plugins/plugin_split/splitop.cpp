#include "splitop.h"

#include "splitplane.h"
#include "splitjob.h"
#include "splitpartsjob.h"

#include "entity/alonepointentity.h"
#include "entity/lineexentity.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"

#include "cxkernel/interface/jobsinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/eventinterface.h"
#include "interface/spaceinterface.h"
#include "data/modeln.h"

#include "data/fdmsupportgroup.h"
#include "kernel/kernelui.h"

using namespace creative_kernel;
using namespace qtuser_3d;
SplitOp::SplitOp(QObject* parent)
	:MoveOperateMode(parent)
	, m_rotateAngle(0, 0, 0)
	, m_lineEntity(nullptr)
	, m_pointEntity(nullptr)
	, m_selectPlaneByCursor(false)
	, m_offset(0)
{
	m_splitPlane = new SplitPlane();
	
	m_pointEntity = new AlonePointEntity();
	m_pointEntity->setColor(QVector4D(1.0f, 0.7529f, 0.0f, 1.0));
	m_pointEntity->setPointSize(6.0);

	m_lineEntity = new LineExEntity();
	m_lineEntity->setLineWidth(4);
	m_lineEntity->setColor(QVector4D(1.0f, 0.7529f, 0.0f, 1.0));
}

SplitOp::~SplitOp()
{

}

void SplitOp::onAttach()
{
	m_plane = qtuser_3d::Plane();
    QList<ModelN*> selectModels = selectionms();
	
    if(selectModels.length()>0)
    {
		ModelN* model = selectModels.at(0);
        creative_kernel::selectOne(model);
    }else
	{
        //creative_kernel::selectOne(selectModels.at(0));
    }
	visShow(m_splitPlane);

	prependLeftMouseEventHandler(this);
	addHoverEventHandler(this);
	addKeyEventHandler(this);

	//traceSpace(this);
	addSelectTracer(this);
	//onSelectionsChanged();

	updatePlanePosition();
	setAcitiveAxis(m_axisType);

	m_bShowPop = true;
}

void SplitOp::onDettach()
{
	enableSelectPlaneByCursor(false);

	visHide(m_splitPlane);
	visHide(m_lineEntity);
	visHide(m_pointEntity);

	removeLeftMouseEventHandler(this);
	removeHoverEventHandler(this);
	removeKeyEventHandler(this);

	//unTraceSpace(this);
	removeSelectorTracer(this);
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

void SplitOp::onSelectionsChanged()
{
	QList<creative_kernel::ModelN*> selections = selectionms();
	setSelectedModel(selections.size() > 0 ? selections.at(0) : nullptr);
	
}

void SplitOp::selectChanged(qtuser_3d::Pickable* pickable)
{
	updatePlanePosition();
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

	m_splitPlane->setTargetModel(model);

	requestVisPickUpdate(true);
}

void SplitOp::onLeftMouseButtonClick(QMouseEvent* event)
{
#ifdef _DEBUG
	printf("%s\n", __func__);
#endif
}

void SplitOp::onKeyPress(QKeyEvent* event)
{
#ifdef _DEBUG
	if (event->key() == Qt::Key_S)
	{
		split();
	}
	else if (event->key() == Qt::Key_P)
	{
		splitParts();
	}
#endif
}

void SplitOp::onHoverMove(QHoverEvent* event)
{
#ifdef _DEBUG
	// printf("%s\n", __func__);
#endif
	if (m_selectPlaneByCursor)
		processCursorMoveEvent(event->pos());
}

void SplitOp::split()
{
	// 平面显示时才可以切割
	if (!m_splitPlane->isEnabled())	
		return;

	QList<ModelN*> selections = selectionms();

	if (selections.size() > 0)
	{
		SplitJob* job = new SplitJob();

		job->setModel(selections.at(0));
		job->setPlane(m_plane);
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}
}

void SplitOp::splitParts()
{
	QList<ModelN*> selections = selectionms();

	if (selections.size() == 1)
	{
		SplitPartsJob* job = new SplitPartsJob();
		
		job->setModel(selections.at(0));
		cxkernel::executeJob(qtuser_core::JobPtr(job));
	}
}

const Plane& SplitOp::plane()
{
	return m_plane;
}

void SplitOp::setPlanePosition(const QVector3D& position)
{
	m_plane.center = position;
	updatePlaneEntity(true);
	emit posionChanged();
}

void SplitOp::setPlaneNormal(const QVector3D& normal)
{
	m_plane.dir = normal;
	if (normal.length() == 0.0f)
		m_plane.dir = QVector3D(0.0f, 0.0f, 1.0f);
	m_plane.dir.normalize();

	m_splitPlane->setGlobalNormal(m_plane.dir);

	updatePlaneEntity(true);
	
}

void SplitOp::updatePlaneEntity(bool request)
{
	QList<ModelN*> selections = selectionms();
	
	if (selections.isEmpty())
	{
		visHide(m_splitPlane);
	}
	else {
		QMatrix4x4 m;
		QQuaternion q = QQuaternion::fromDirection(m_plane.dir, QVector3D(0.0f, 0.0f, 1.0f));
		m.translate(m_plane.center);
		m.rotate(q);
		m_splitPlane->setPose(m);

		visShow(m_splitPlane);
	}

	requestVisUpdate(false);
}

bool SplitOp::getShowPop()
{
	return m_bShowPop;
}

bool SplitOp::getMessage()
{
	//if (m_selectedModels.size())
	QList<ModelN*> selections = selectionms();
	for (size_t i = 0; i < selections.size(); i++)
	{
		ModelN* model = selections.at(i);
		if (model->hasFDMSupport())
		{
			FDMSupportGroup* p_support = selections.at(i)->fdmSupport();
			if (p_support->fdmSupportNum())
			{
				return true;
			}
		}
	}
	return false;
}

void SplitOp::setMessage(bool isRemove)
{
	if (isRemove)
	{
		QList<ModelN*> selections = selectionms();
		for (size_t i = 0; i < selections.size(); i++)
		{
			ModelN* model = selections.at(i);
			if (model->hasFDMSupport())
			{
				FDMSupportGroup* p_support = selections.at(i)->fdmSupport();
				p_support->clearSupports();
			}
		}
		requestVisUpdate(true);
	}
}

void SplitOp::onLeftMouseButtonPress(QMouseEvent* event)
{

#ifdef _DEBUG
	printf("%s\n", __func__);
#endif

	m_leftPressStatus = false;
	if (m_selectPlaneByCursor)
	{
		tryCollectMouseClickEvent(event);
	}
	else
	{
		selectVis(event->pos());
		MoveOperateMode::onLeftMouseButtonPress(event);
	}
}

void SplitOp::onLeftMouseButtonMove(QMouseEvent* event)
{
#ifdef _DEBUG
	printf("%s\n", __func__);
#endif
	if (m_selectPlaneByCursor)
		processCursorMoveEvent(event->pos());
	else
		MoveOperateMode::onLeftMouseButtonMove(event);
}

void SplitOp::onLeftMouseButtonRelease(QMouseEvent* event)
{
	MoveOperateMode::onLeftMouseButtonRelease(event);
#ifdef _DEBUG
	printf("%s\n", __func__);
#endif

	//判断有没有move
	bool move = false;

	QVector3D position = makeWorldPositionFromScreen(event->pos());
	//QVector3D position, normal;
	//ModelN* model = checkPickModel(event->pos(), position, normal);
	//if (model)
	{
		if (m_selectedPosition.size() > 0)
		{
			const QVector3D& first = m_selectedPosition.at(0);
			float length = position.distanceToPoint(first);
			if (length > 0.1)
			{
				move = true;
			}
		}
	}

	if (move)
	{
		tryCollectMouseClickEvent(event);
	}
	else
	{
		m_selectedPosition.clear();

		if (m_selectPlaneByCursor)
		{	/* select */
			selectVis(event->pos());
		}
	}
}

void SplitOp::setAcitiveAxis(int axisType)
{
	m_axisType = axisType;

	if (m_axisType < 3)
	{
		QVector3D axis;
		switch (m_axisType)
		{
		case 0:
            axis = QVector3D(1, 0, 0);
            break;
        case 1:
            axis = QVector3D(0, 1, 0);
            break;
        case 2:
            axis = QVector3D(0, 0, 1);
            break;
		}
		enableSelectPlaneByCursor(false);
		QVector3D center;
		if (getSelectedModelsCenter(&center))
		{
			m_offset = 0.0f;

			m_plane.center = center;
			setPlaneNormal(axis);
		}
	} 
	else
	{
        enableSelectPlaneByCursor(true);
	}
}

void SplitOp::setOffset(float off)
{
	QVector3D newCenter = m_plane.center + m_plane.dir * (off - m_offset);
	setPlanePosition(newCenter);
	m_offset = off;
}

float SplitOp::getOffset()
{
	return m_offset;
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


bool SplitOp::processCursorMoveEvent(const QPoint& pos)
{
	visHide(m_lineEntity);
	visHide(m_pointEntity);

	QList<creative_kernel::ModelN*> selections = selectionms();
	if (selections.size() < 1)
		return false;

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

	return true;
}

bool SplitOp::getSelectedModelsCenter(QVector3D* outCenter)
{
	QList<ModelN*> selections = selectionms();
	bool valid = !selections.isEmpty();
	if (valid)
	{
		ModelN* ModelNEnd = selections.at(0);
		qtuser_3d::Box3D box = ModelNEnd->globalSpaceBox();
		QVector3D qcenter = box.center();

		if (center != nullptr)
		{
			*outCenter = qcenter;
		}
	}

	return valid;
}

void SplitOp::enableSelectPlaneByCursor(bool enable)
{
	m_selectPlaneByCursor = enable;
	m_selectedPosition.clear();
	m_offset = 0.0f;

	visHide(m_pointEntity);
	visHide(m_lineEntity);

	if (enable)
	{
		visHide(m_splitPlane);
	}
	else {
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

void SplitOp::updatePlanePosition()
{
	QList<ModelN*> selections = selectionms();
	if (!selections.isEmpty())
	{
		ModelN* ModelNEnd = selections.at(0);
		qtuser_3d::Box3D box = ModelNEnd->globalSpaceBox();
		m_plane.center = box.center();
		m_splitPlane->updateBox(box);
		updatePlaneEntity(true);
	}
}