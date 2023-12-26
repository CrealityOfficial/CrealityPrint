#include "scaleop.h"

#include "qtuser3d/math/space3d.h"
#include "qtuser3d/math/angles.h"

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/modelinterface.h"
#include "interface/eventinterface.h"
#include "interface/spaceinterface.h"
#include "interface/reuseableinterface.h"
#include "kernel/kernelui.h"
#include <QQmlProperty>

#include "data/fdmsupportgroup.h"

using namespace creative_kernel;
ScaleOp::ScaleOp(QObject* parent)
	:MoveOperateMode(parent)
	, m_mode(TMode::null)
	, m_selectedModel(nullptr)
{
	int types =
		HELPERTYPE_AXIS_X | \
		HELPERTYPE_AXIS_Y | \
		HELPERTYPE_AXIS_Z | \
		HELPERTYPE_PLANE_XY | \
		HELPERTYPE_PLANE_YZ | \
		HELPERTYPE_PLANE_ZX;

	m_helperEntity = new qtuser_3d::TranslateHelperEntity(nullptr, types, qtuser_3d::TranslateHelperEntity::IndicatorType::CUBE);
	m_helperEntity->setCameraController(cameraController());

	m_originFovy = 60.0;
	traceSpace(this);
}

ScaleOp::~ScaleOp()
{
	unTraceSpace(this);
}

void ScaleOp::setMessage(bool isRemove)
{
	if (isRemove)
	{
		QList<creative_kernel::ModelN*> selections = selectionms();
		foreach(creative_kernel::ModelN * model, selections)
		{
			if (model->hasFDMSupport())
			{
				FDMSupportGroup* p_support = model->fdmSupport();
				p_support->clearSupports();
			}
		}
		requestVisUpdate(true);
	}
}

bool ScaleOp::getMessage()
{
	//if (!m_bShowPop)
	//{
	//	return false;
	//}
	QList<creative_kernel::ModelN*> selections = selectionms();
	foreach(creative_kernel::ModelN * model, selections)
	{
		if (model->hasFDMSupport())
		{
			FDMSupportGroup* p_support = model->fdmSupport();
			if (p_support->fdmSupportNum())
			{
				return true;
			}
		}
	}
	return false;
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

   /* QList<ModelN*> selectModels = selectionms();
    if(selectModels.length()>0)
    {
        creative_kernel::selectOne(selectModels.at(0));
    }*/

	addSelectTracer(this);
	onSelectionsChanged();

	addLeftMouseEventHandler(this);
	addWheelEventHandler(this);

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

	removeSelectorTracer(this);
	setSelectedModel(nullptr);

	removeLeftMouseEventHandler(this);
	removeWheelEventHandler(this);

	requestVisUpdate(true);

	m_bShowPop = false;
}

void ScaleOp::onLeftMouseButtonPress(QMouseEvent* event)
{
	MoveOperateMode::onLeftMouseButtonPress(event);

	m_mode = TMode::null;

	if (m_selectedModel)
	{
		qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
		if (pickable == m_helperEntity->xArrowPickable()) m_mode = TMode::x;
		else if (pickable == m_helperEntity->yArrowPickable()) m_mode = TMode::y;
		else if (pickable == m_helperEntity->zArrowPickable()) m_mode = TMode::z;
		else if (pickable == m_helperEntity->xyPlanePickable()) m_mode = TMode::xy;
		else if (pickable == m_helperEntity->yzPlanePickable()) m_mode = TMode::yz;
		else if (pickable == m_helperEntity->zxPlanePickable()) m_mode = TMode::zx;
		//else if (pickable == m_helperEntity->xyzCubePickable()) m_mode = TMode::xyz;
		else if (pickable != nullptr) m_mode = TMode::sp;

		m_saveScale = m_selectedModel->localScale();
		m_savePosition = m_selectedModel->localPosition();
		m_spacePoint = operationProcessCoord(event->pos(), m_selectedModel, 0, m_mode);


		//m_selectedModel->setNeedCheckScope(0);
	}
	setNeedCheckScope(0);

	if(m_mode != TMode::sp && m_mode != TMode::null)
		fillChangeStructs(m_changes, true);
}

void ScaleOp::perform(const QPoint& point, bool reversible)
{
	QVector3D p = operationProcessCoord(point, m_selectedModel, 0, m_mode);
	QVector3D delta = getScale(p);
	QVector3D newScale = m_saveScale * delta;

	// TODO
	//lisugui 2021-05-27 
	QList<creative_kernel::ModelN*> selections = selectionms();
	for(creative_kernel::ModelN * model : selections)
	{
		model->setLocalScale(newScale);

		qtuser_3d::Box3D box = model->globalSpaceBox();
		QVector3D zoffset = QVector3D(0.0f, 0.0f, 0.0f);  // QVector3D(0.0f, 0.0f, -box.min.z());
		QVector3D localPosition = model->localPosition();
		QVector3D newPosition = localPosition + zoffset;

		mixTSModel(model, m_savePosition, newPosition, m_saveScale, newScale, reversible);
	}
}

void ScaleOp::onLeftMouseButtonRelease(QMouseEvent* event)
{
	MoveOperateMode::onLeftMouseButtonRelease(event);
	setNeedCheckScope(1);

	if (m_selectedModel && m_mode != TMode::null && m_mode != TMode::sp)
	{
		perform(event->pos(), false);

		//lisugui 2021-05-27 
		QList<creative_kernel::ModelN*> selections = selectionms();
		foreach(creative_kernel::ModelN * model, selections)
		{
			qtuser_3d::Box3D box = model->globalSpaceBox();
			QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());
			QVector3D oldPosition = model->localPosition();
			QVector3D newPosition = model->localPosition() + zoffset;
			moveModel(model, oldPosition, newPosition, false);

			//m_selectedModel->setNeedCheckScope(1);
		}

		emit scaleChanged();
		emit sizeChanged();

        m_bShowPop = false;

		/*QList<creative_kernel::ModelN*> alls = creative_kernel::modelns();
		for (size_t i = 0; i < alls.size(); i++)
		{
			alls[i]->setNeedCheckScope(1);
		}*/
		

		fillChangeStructs(m_changes, false);
		mixUnions(m_changes, true);
		m_changes.clear();
	}
}

void ScaleOp::onLeftMouseButtonMove(QMouseEvent* event)
{
	MoveOperateMode::onLeftMouseButtonMove(event);
	if (m_selectedModel && m_mode != TMode::null && m_mode != TMode::sp)
	{
		perform(event->pos(), false);
        emit scaleChanged();
		emit sizeChanged();
        if(!m_bShowPop)
        {
            m_bShowPop=true;
        }
	}
}

void ScaleOp::onLeftMouseButtonClick(QMouseEvent* event)
{
	
}

void ScaleOp::onWheelEvent(QWheelEvent* event)
{
	Qt3DRender::QCamera* mainCamera = getCachedCameraEntity();
	float curFovy = mainCamera->fieldOfView();
	m_helperEntity->setScale(curFovy / m_originFovy);
}

void ScaleOp::onSelectionsChanged()
{
	QList<creative_kernel::ModelN*> selections = selectionms();
	setSelectedModel(selections.size() > 0 ? selections.at(0) : nullptr);

	emit mouseLeftClicked();
}

void ScaleOp::selectChanged(qtuser_3d::Pickable* pickable)
{

	if (pickable == m_selectedModel)
	{
		updateHelperEntity();
		emit scaleChanged();
	}
}

void ScaleOp::setSelectedModel(creative_kernel::ModelN* model)
{
	//trace selected node
	m_selectedModel = model;

	buildFromSelections();

	m_uniformCheck = true;
	if (m_selectedModel)
	{
		if (m_modelsScaleLock.contains(m_selectedModel))
		{
			m_uniformCheck = m_modelsScaleLock[m_selectedModel];
		}
		else 
		{
			m_modelsScaleLock[m_selectedModel] = true;
			connect(m_selectedModel, &QObject::destroyed, this, &ScaleOp::onModelDestroyed);
		}
		emit checkChanged();
	}
	emit scaleChanged();
	emit sizeChanged();

}

void ScaleOp::buildFromSelections()
{
	if (m_selectedModel)
	{
		updateHelperEntity();
		visShow(m_helperEntity);
	}
	else
	{
		visHide(m_helperEntity);
	}

	requestVisPickUpdate(true);
	emit scaleChanged();
	emit sizeChanged();
}

void ScaleOp::reset()
{
	QList<creative_kernel::ModelN*> selections = selectionms();
	foreach(creative_kernel::ModelN * model, selections)
	{
		QVector3D oldLocalScale = model->localScale();
		QVector3D newLocalScale = QVector3D(1.0f, 1.0f, 1.0f);

		model->setLocalScale(newLocalScale);

		qtuser_3d::Box3D box = model->globalSpaceBox();
		QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());
		QVector3D oldLocalPosition = model->localPosition();
		QVector3D newLocalPosition = oldLocalPosition + zoffset;

		mixTSModel(model, oldLocalPosition, newLocalPosition, oldLocalScale, newLocalScale, true);

		//emit scaleChanged();
	}
}

QVector3D ScaleOp::scale()
{
	if (m_selectedModel)
	{
		QVector3D globalBoxSize = m_selectedModel->globalSpaceBox().size();
		QVector3D localBoxSize = m_selectedModel->globalSpaceBoxNoScale().size();
		return QVector3D(globalBoxSize.x() / localBoxSize.x(), globalBoxSize.y() / localBoxSize.y(), globalBoxSize.z() / localBoxSize.z());
		//return m_selectedModel->localScale();
	}

    return QVector3D(0.0f, 0.0f, 0.0f);
}

QVector3D ScaleOp::box()
{
    if (m_selectedModel)
    {
         qtuser_3d::Box3D box3d = m_selectedModel->globalSpaceBoxNoScale();
         return box3d.size();
    }

    return QVector3D(0.0f, 0.0f, 0.0f);
}

QVector3D ScaleOp::globalbox()
{
	if (m_selectedModel)
	{
		qtuser_3d::Box3D box3d = m_selectedModel->globalSpaceBox();
		return box3d.size();
	}

	return QVector3D(0.0f, 0.0f, 0.0f);
}

void ScaleOp::setScale(QVector3D scale)
{
    //if (scale.x() <= 0.01f)
    //{
    //    scale.setX(0.01f);
    //}
    //else if (scale.y() <= 0.01f)
    //{
    //    scale.setY(0.01f);
    //}
    //else if (scale.z() <= 0.01f)
    //{
    //    scale.setZ(0.01f);
    //}

	//lisugui 2021-05-27 
	QList<creative_kernel::ModelN*> selections = selectionms();
	foreach(creative_kernel::ModelN * model ,selections)
	{
		QVector3D oldLocalScale = model->localScale();
		QVector3D newLocalScale = scale;

		model->setLocalScale(newLocalScale);

		qtuser_3d::Box3D box = model->boxWithSup();
		QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());
		QVector3D oldLocalPosition = model->localPosition();
		QVector3D newLocalPosition = oldLocalPosition + zoffset;

		mixTSModel(model, oldLocalPosition, newLocalPosition, oldLocalScale, newLocalScale, true);

		//emit scaleChanged();
	 }
	creative_kernel::checkModelRange();
	creative_kernel::checkBedRange();
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
	m_modelsScaleLock[m] = check;
	m_uniformCheck = check;

	if (m_uniformCheck)
	{
		m_helperEntity->setRotation(m_selectedModel->localQuaternion());
	}
	/*else
	{
		QList<creative_kernel::ModelN*> selections = selectionms();
		foreach(creative_kernel::ModelN * model, selections)
		{
			QMatrix4x4 srMatrix = model->embedScaleNRot();
			model->updateMatrix();

			//qtuser_3d::Box3D box = model->globalSpaceBox();
			//QVector3D zoffset = QVector3D(0.0f, 0.0f, 0.0f);  // QVector3D(0.0f, 0.0f, -box.min.z());
			//QVector3D localPosition = model->localPosition();
			//QVector3D newPosition = localPosition + zoffset;

			//mixTSModel(model, m_savePosition, newPosition, m_saveScale, newScale, reversible);
		}

		m_helperEntity->setRotation(QQuaternion());

		buildFromSelections();
	}*/

	emit checkChanged();
}

QVector3D ScaleOp::process(const QPoint& point)
{
	qtuser_3d::Ray ray = visRay(point);

	QVector3D planeCenter;
	QVector3D planeDir;
	getProperPlane(planeCenter, planeDir, ray);

	QVector3D c;
	qtuser_3d::lineCollidePlane(planeCenter, planeDir, ray, c);
	return c;
}

void ScaleOp::getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray)
{
	planeCenter = QVector3D(0.0f, 0.0f, 0.0f);
	qtuser_3d::Box3D box = m_selectedModel->globalSpaceBox();
	planeCenter = box.center();

	QList<QVector3D> dirs;
	if (m_mode == TMode::x)  // x
	{
		dirs << QVector3D(0.0f, 1.0f, 0.0f);
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
	}
	else if (m_mode == TMode::y)
	{
		dirs << QVector3D(1.0f, 0.0f, 0.0f);
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
	}
	else if (m_mode == TMode::z)
	{
		dirs << QVector3D(1.0f, 0.0f, 0.0f);
		dirs << QVector3D(0.0f, 1.0f, 0.0f);
	}
	else if (m_mode == TMode::xy)
	{
		planeDir = QVector3D(0.0, 0.0, 1.0);
	}
	else if (m_mode == TMode::yz)
	{
		planeDir = QVector3D(1.0, 0.0, 0.0);
	}
	else if (m_mode == TMode::zx)
	{
		planeDir = QVector3D(0.0, 1.0, 0.0);
	}
	else if (m_mode == TMode::xyz)
	{
		planeDir = ray.dir;
	}

	float d = -1.0f;
	for (QVector3D& n : dirs)
	{
		float dd = QVector3D::dotProduct(n, ray.dir);
		if (qAbs(dd) > d)
		{
			planeDir = n;
		}
	}
}

QVector3D ScaleOp::getScale(const QVector3D& c)
{
	QVector3D scaleDelta = QVector3D(1.0f, 1.0f, 1.0f);
	scaleDelta.setX(m_spacePoint.x() == 0.0 ? 1.0 : abs(c.x() / m_spacePoint.x()));
	scaleDelta.setY(m_spacePoint.y() == 0.0 ? 1.0 : abs(c.y() / m_spacePoint.y()));
	scaleDelta.setZ(m_spacePoint.z() == 0.0 ? 1.0 : abs(c.z() / m_spacePoint.z()));

	if (m_mode == TMode::x)  // x
	{
		scaleDelta = m_uniformCheck ? QVector3D(scaleDelta.x(), scaleDelta.x(), scaleDelta.x()) : QVector3D(scaleDelta.x(), 1.0, 1.0);
	}
	else if (m_mode == TMode::y)
	{
		scaleDelta = m_uniformCheck ? QVector3D(scaleDelta.y(), scaleDelta.y(), scaleDelta.y()) : QVector3D(1.0, scaleDelta.x(), 1.0);
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

void ScaleOp::updateHelperEntity()
{
	if (m_selectedModel)
	{
		qtuser_3d::Box3D box = m_selectedModel->globalSpaceBox();
		/*
		m_helperEntity->setDirColor(QVector4D(1.0f, 0.0f, 0.0f, 0.8f), 0);
		m_helperEntity->setDirColor(QVector4D(0.0f, 1.0f, 0.0f, 0.8f), 1);
		m_helperEntity->setDirColor(QVector4D(0.0f, 0.0f, 1.0f, 0.8f), 2);
		*/
		m_helperEntity->updateBox(box);
		
		Qt3DRender::QCamera* mainCamera = getCachedCameraEntity();
		float curFovy = mainCamera->fieldOfView();
		m_helperEntity->setScale(curFovy / m_originFovy);

		m_helperEntity->setRotation(m_selectedModel->localQuaternion());
	}
}

bool ScaleOp::getShowPop()
{
	return m_bShowPop;
}

bool ScaleOp::shouldMultipleSelect()
{
	return true;
}

void ScaleOp::onBoxChanged(const qtuser_3d::Box3D& box)
{
	const QVector3D size = box.size();
	float length = size.length();
	float s = fmin(length / 6.0, 200.0);
	m_helperEntity->setEntitiesLocalScale(s);
}

void ScaleOp::onSceneChanged(const qtuser_3d::Box3D& box)
{

}

void ScaleOp::onModelDestroyed()
{
	ModelN* destroyedModel = qobject_cast<ModelN*>(sender());
	m_modelsScaleLock.remove(destroyedModel);
}