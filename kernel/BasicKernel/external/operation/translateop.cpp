#include "translateop.h"
#include <QQmlProperty>
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/camera/cameracontroller.h"
#include <QMetaObject>
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/modelinterface.h"

#include "interface/eventinterface.h"
#include "interface/reuseableinterface.h"
#include "kernel/kernelui.h"

#include "data/fdmsupportgroup.h"


using namespace creative_kernel;
TranslateOp::TranslateOp(QObject* parent)
	:SceneOperateMode(parent)
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
	m_helperEntity = new qtuser_3d::TranslateHelperEntity(nullptr, types);
	m_helperEntity->setCameraController(cameraController());
	moveEnable = false;

	m_originFovy = 60.0;
	traceSpace(this);
}

TranslateOp::~TranslateOp()
{
	//unTraceSpace(this);
}

void TranslateOp::setMessage(bool isRemove)
{
	if (isRemove)
	{
		for (size_t i = 0; i < m_selectedModels.size(); i++)
		{
			ModelN* model = m_selectedModels.at(i);
			if (model->hasFDMSupport())
			{
				FDMSupportGroup* p_support = m_selectedModels.at(i)->fdmSupport();
				p_support->clearSupports();
			}
		}
		requestVisUpdate(true);
	}
}

bool TranslateOp::getMessage()
{
	//if (m_selectedModels.size())
	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		ModelN* model = m_selectedModels.at(i);
		if (model->hasFDMSupport())
		{
			FDMSupportGroup* p_support = m_selectedModels.at(i)->fdmSupport();
			if (p_support->fdmSupportNum())
			{
				return true;
			}
		}
	}
	return false;
}

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

    //QList<ModelN*> selectModels = selectionms();
    //if(selectModels.length()>0)
    //{
    //    creative_kernel::selectOne(selectModels.at(0));
    //}

	addSelectTracer(this);
	onSelectionsChanged();

	addLeftMouseEventHandler(this);
	addWheelEventHandler(this);
	addKeyEventHandler(this);

	requestVisUpdate(true);

}

void TranslateOp::onDettach()
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
	//setSelectedModel(nullptr);
	m_selectedModels.clear();
	

	removeLeftMouseEventHandler(this);
	removeWheelEventHandler(this);
	removeKeyEventHandler(this);

	requestVisUpdate(true);
}

void TranslateOp::onLeftMouseButtonPress(QMouseEvent* event)
{
	moveEnable = false;
	m_mode = TMode::null;
	m_saveLocalPositions.clear();
	for (size_t i = 0; i < m_selectedModels.size(); i++)
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

		if (pickable == m_selectedModels[i] || (m_mode != TMode::sp && m_mode != TMode::null))
		{
			moveEnable = true;
		}


		m_spacePoint = process(event->pos());
		m_saveLocalPositions.push_back(m_selectedModels[i]->localPosition());
	}

	//if (m_selectedModel)
	//{
	//	Pickable* pickable = checkPickable(event->pos(), nullptr);
	//	if (pickable == m_helperEntity->xPickable()) m_mode = TMode::x;
	//	if (pickable == m_helperEntity->yPickable()) m_mode = TMode::y;
	//	if (pickable == m_helperEntity->zPickable()) m_mode = TMode::z;
	//	
	//	ModelN* model = qobject_cast<ModelN*>(pickable);
	//	if (model == m_selectedModel)
	//	{
	//		m_mode = TMode::sp;
	//	}

	//	m_spacePoint = process(event->pos());
	//	m_saveLocalPosition = m_selectedModel->localPosition();
	//}
}

void TranslateOp::onLeftMouseButtonRelease(QMouseEvent* event)
{
	if (!moveEnable)
	{
		return;
	}

	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		if (m_mode != TMode::null)
		{	//
			QVector3D p = process(event->pos());
			QVector3D delta = clip(p - m_spacePoint);
			QVector3D newLocalPosition = m_saveLocalPositions[i] + delta;
			moveModel(m_selectedModels[i], m_saveLocalPositions[i], newLocalPosition, true);
			requestVisUpdate(true);
			emit positionChanged();
		}
		//m_saveLocalPositions[i] = m_selectedModels[i]->localPosition();
	}
}

void TranslateOp::onLeftMouseButtonMove(QMouseEvent* event)
{
	if (!moveEnable)
	{
		return;
	}

	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		if (m_mode != TMode::null)
		{
			QVector3D p = process(event->pos());
			QVector3D delta = clip(p - m_spacePoint);
			QVector3D newLocalPosition = m_saveLocalPositions[i] + delta;
			moveModel(m_selectedModels[i], m_saveLocalPositions[i], newLocalPosition);

			requestVisUpdate(true);
		}
	}
    emit positionChanged();
}

void TranslateOp::onLeftMouseButtonClick(QMouseEvent* event)
{
}

void TranslateOp::onWheelEvent(QWheelEvent* event)
{
	Qt3DRender::QCamera* mainCamera = getCachedCameraEntity();
	float curFovy = mainCamera->fieldOfView();
	m_helperEntity->setScale(curFovy / m_originFovy);
}

void TranslateOp::onKeyPress(QKeyEvent* event)
{
	QList<creative_kernel::ModelN*> selections = selectionms();
	if (selections.size() == 0)
		return;

	QVector3D delta;
	bool needUpdate = false;

	if (event->key() == Qt::Key_Right)
	{
		delta = QVector3D(1.0f, 0.0f, 0.0f);
		needUpdate = true;
	}
	//���û�����<--��ʱ
	else if (event->key() == Qt::Key_Left)
	{
		delta = QVector3D(-1.0f, 0.0f, 0.0f);
		needUpdate = true;
	}
	//���û�����up��ʱ
	else if (event->key() == Qt::Key_Up)
	{
		delta = QVector3D(0.0f, 1.0f, 0.0f);
		needUpdate = true;
	}
	//���û�����Down��ʱ
	else if (event->key() == Qt::Key_Down)
	{
		delta = QVector3D(0.0f, -1.0f, 0.0f);
		needUpdate = true;
	}

	if (needUpdate)
	{
		for (size_t i = 0; i < selections.size(); i++)
		{
			QVector3D localPosition = selections.at(i)->localPosition();
			moveModel(selections.at(i), localPosition, localPosition + delta);
		}
		requestVisUpdate(true);
		emit positionChanged();
	}
}

void TranslateOp::onKeyRelease(QKeyEvent* event)
{
}

void TranslateOp::onSelectionsChanged()
{
	QList<creative_kernel::ModelN*> selections = selectionms();
	setSelectedModel(selections);
	emit mouseLeftClicked();
}

void TranslateOp::selectChanged(qtuser_3d::Pickable* pickable)
{
	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		if (pickable == m_selectedModels[i])
			updateHelperEntity();
	}
	if (m_selectedModels.size() > 0)
	{
		emit positionChanged();
	}
	//if (pickable == m_selectedModel)
	//	updateHelperEntity();
}

void TranslateOp::setSelectedModel(creative_kernel::ModelN* model)
{
	//trace selected node
	m_selectedModel = model;

#if 0
	TranslateMode* mode = qobject_cast<TranslateMode*>(parent());
	if (!m_selectedModel)
	{
		mode->setSource("qrc:/kernel/qml/NoSelect.qml");
	}
	else {
		mode->setSource("qrc:/kernel/qml/MovePanel.qml");
	}

	qtuser_qml::ToolCommandCenter* left = getUILCenter();
	left->changeCommand(mode);
#endif
	buildFromSelections();
}

void TranslateOp::setSelectedModel(QList<creative_kernel::ModelN*> models)
{
	m_selectedModels = models;
	m_saveLocalPositions.clear();
	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		m_saveLocalPositions.push_back(m_selectedModels[i]->localPosition());

	}
	buildFromSelections();
}

void TranslateOp::buildFromSelections()
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

	//if (m_selectedModel)
	//{
	//	updateHelperEntity();
	//	visShow(m_helperEntity);
	//}
	//else
	//{
	//	visHide(m_helperEntity);
	//}

	requestVisUpdate(true);
	if (m_selectedModels.size())
	{
		emit positionChanged();
	}
		
}
void TranslateOp::center()
{
	qtuser_3d::Box3D modelsbox;
	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		if (m_selectedModels[i])
		{
			qtuser_3d::Box3D modelbox = m_selectedModels[i]->globalSpaceBox();
			modelsbox += modelbox;
		}
	}
	QVector3D movePos;
	float newZ = 0;
	if (modelsbox.valid)
	{
		qtuser_3d::Box3D box = creative_kernel::baseBoundingBox();
		QVector3D size = box.size();
		QVector3D center = box.center();
		newZ = center.z() - size.z() / 2.0f;

		movePos = box.center() - modelsbox.center();
		movePos.setZ(0);
	}

	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		if (m_selectedModels[i])
		{
			QVector3D oldLocalPosition = m_selectedModels[i]->localPosition();
			QVector3D newPositon = oldLocalPosition + movePos;
			//newPositon.setZ(newZ);

			moveModel(m_selectedModels[i], oldLocalPosition, newPositon/*m_selectedModels[i]->mapGlobal2Local(newPositon)*/, true);

			updateHelperEntity();
			requestVisUpdate(true);
		}
	}

	emit positionChanged();

  //  if(m_selectedModel)
  //  {
  //      QVector3D oldLocalPosition = m_selectedModel->localPosition();
  //      qtuser_3d::Box3D box = creative_kernel::baseBoundingBox();
  //      QVector3D size = box.size();
  //      QVector3D center = box.center();
		//center.setZ(center.z() - size.z() / 2.0f);

  //      moveModel(m_selectedModel, oldLocalPosition, m_selectedModel->mapGlobal2Local(center), true);

  //      updateHelperEntity();
		//requestVisUpdate(true);

  //      emit positionChanged();
  //  }
}

void TranslateOp::movePositionToinit(QList<creative_kernel::ModelN*>& selections)
{
    for (size_t i = 0; i < selections.size(); i++)
    {
        QVector3D oldLocalPosition = selections.at(i)->localPosition();
        QVector3D initPosition = selections.at(i)->GetInitPosition();
        initPosition.setZ(0.0f);
        QVector3D position = selections.at(i)->mapGlobal2Local(initPosition);
        position.setX(initPosition.x());
        position.setY(initPosition.y());

        moveModel(selections.at(i), oldLocalPosition, position, true);
    }
}

void TranslateOp::reset()
{
	QList<ModelN*> selections = selectionms();
	if(selections.size())
	{
        movePositionToinit(selections);
	}
	else
	{
		ModelSpace* space = getModelSpace();
		QList<ModelN*> models = space->modelns();

        movePositionToinit(models);
	}
	updateHelperEntity();
	requestVisUpdate(true);
	emit positionChanged();
}

QVector3D TranslateOp::position()
{
	if (m_selectedModels.size() > 0)
	{
		qtuser_3d::Box3D box = m_selectedModels[m_selectedModels.size() - 1]->globalSpaceBox();
		QVector3D size = box.size();
		QVector3D center = box.center();
		center.setZ(center.z() - size.z() / 2.0f);
		//end
		return center;
	}
    return QVector3D(0.0f, 0.0f, 0.0f);
}

void TranslateOp::setPosition(QVector3D position)
{
	if (m_selectedModels.empty())
		return;

	auto model = m_selectedModels.last(); 
	QVector3D src = model->localPosition();
	QVector3D dst = model->mapGlobal2Local(position);

	if (1 == m_selectedModels.size())
	{
		moveModel(model, src, dst, true);
		updateHelperEntity();
		requestVisUpdate(true);
		emit positionChanged();
	}
	else
	{
		QVector3D moveOffset = dst - src;
		for (size_t i = 0; i < m_selectedModels.size(); i++)
		{
			QVector3D pos = m_selectedModels[i]->localPosition();
            moveModel(m_selectedModels[i], pos, pos + moveOffset, true);
		}

		updateHelperEntity();
		requestVisUpdate(true);
		emit positionChanged();
	}
}

void TranslateOp::setBottom()
{
    for (size_t i = 0; i < m_selectedModels.size(); i++)
    {
        qtuser_3d::Box3D box = m_selectedModels[i]->globalSpaceBox();

		float fOffset = -box.min.z();

        QVector3D zoffset = QVector3D(0.0f, 0.0f, fOffset);
        QVector3D localPosition = m_selectedModels[i]->localPosition();

        moveModel(m_selectedModels[i], localPosition, localPosition + zoffset, true);
    }
    if(m_selectedModels.size())
    {
        updateHelperEntity();
        requestVisUpdate(true);
        emit positionChanged();
    }
}

QVector3D TranslateOp::process(const QPoint& point)
{
	qtuser_3d::Ray ray = visRay(point);

	QVector3D planeCenter;
	QVector3D planeDir;
	getProperPlane(planeCenter, planeDir, ray);

	QVector3D c;
	qtuser_3d::lineCollidePlane(planeCenter, planeDir, ray, c);
    if(c.x() > PosMax) { c.setX(PosMax); }
    if(c.y() > PosMax) { c.setY(PosMax);}
    if(c.z() > PosMax) { c.setZ(PosMax);}
	return c;
}

void TranslateOp::getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray)
{
	planeCenter = QVector3D(0.0f, 0.0f, 0.0f);
	qtuser_3d::Box3D box = m_selectedModels[m_selectedModels.size()-1]->globalSpaceBox();
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
	else if (m_mode == TMode::sp)
	{
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
		dirs << QVector3D(0.0f, 0.0f, 1.0f);
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

QVector3D TranslateOp::clip(const QVector3D& delta)
{
	QVector3D clipDelta = delta;
	if (m_mode == TMode::x)  // x
	{
		clipDelta.setY(0.0f);
		clipDelta.setZ(0.0f);
	}
	else if (m_mode == TMode::y)
	{
		clipDelta.setX(0.0f);
		clipDelta.setZ(0.0f);
	}
	else if (m_mode == TMode::z)
	{
		clipDelta.setY(0.0f);
		clipDelta.setX(0.0f);
	}

	return clipDelta;
}

void TranslateOp::updateHelperEntity()
{
	if (m_selectedModels.size())
	{
		qtuser_3d::Box3D box = m_selectedModels[m_selectedModels.size()-1]->globalSpaceBox();
		m_helperEntity->updateBox(box);

		Qt3DRender::QCamera* mainCamera = getCachedCameraEntity();
		float curFovy = mainCamera->fieldOfView();
		m_helperEntity->setScale(curFovy / m_originFovy);
	}
}

bool TranslateOp::shouldMultipleSelect()
{
	return true;
}

void TranslateOp::onBoxChanged(const qtuser_3d::Box3D& box)
{
	const QVector3D size = box.size();
	float length = size.length();
	float s = fmin(length / 6.0, 200.0);
	m_helperEntity->setEntitiesLocalScale(s);
}

void TranslateOp::onSceneChanged(const qtuser_3d::Box3D& box)
{
}