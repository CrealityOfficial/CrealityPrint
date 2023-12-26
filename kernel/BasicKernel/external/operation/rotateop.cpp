#include "rotateop.h"

#include <QtCore/qmath.h>
#include <QQmlProperty>

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/modelinterface.h"
#include "interface/eventinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/spaceinterface.h"

#include "kernel/kernelui.h"

#include "qtuser3d/math/angles.h"
#include "qtuser3d/math/space3d.h"
#include "qtusercore/property/qmlpropertysetter.h"

#include "data/fdmsupportgroup.h"

#define MODEL_INIT_ROT_TAG "initRotation"

using namespace creative_kernel;
RotateOp::RotateOp(QObject* parent)
	: MoveOperateMode(parent)
	, m_mode(TMode::null)
	, m_saveAngle(0.0f)
	, tip_component_(nullptr)
{
	m_helperEntity = new qtuser_3d::Rotate3DHelperEntity();
	m_isRoate = false;

	m_originFovy = 25.0f;

	auto* ui_parent = getKernelUI()->glMainView();

	tip_component_ = new QQmlComponent{ qmlEngine(ui_parent),
		QUrl{ QStringLiteral("qrc:/CrealityUI/qml/RotateTip.qml") }, this };

	tip_object_ = tip_component_->create(qmlContext(ui_parent));
	if (tip_object_ != nullptr) {
		tip_object_->setParent(ui_parent);
		qtuser_qml::writeObjectProperty(tip_object_, QStringLiteral("parent"), ui_parent);
	}
	traceSpace(this);

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

void RotateOp::setMessage(bool isRemove)
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

bool RotateOp::getMessage()
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

void RotateOp::onAttach()
{
    QList<ModelN*> selectModels = selectionms();
	QList<qtuser_3d::Pickable*> pickableList = m_helperEntity->getPickables();
	for (qtuser_3d::Pickable* pickable : pickableList)
	{
		tracePickable(pickable);
	}

	for (int i = 0; i < selectModels.size(); i++)
	{
		creative_kernel::appendSelect(selectModels.at(i));
	}
	addSelectTracer(this);
	onSelectionsChanged();
	QList<qtuser_3d::LeftMouseEventHandler*> handlers = m_helperEntity->getLeftMouseEventHandlers();
	for (qtuser_3d::LeftMouseEventHandler* handler : handlers)
	{
		addLeftMouseEventHandler(handler);
	}
	addWheelEventHandler(this);
	addLeftMouseEventHandler(this);

	m_helperEntity->setPickSource(visPickSource());
	m_helperEntity->setScreenCamera(visCamera());
	m_helperEntity->setRotateCallback(this);

	requestVisUpdate(true);

	m_bShowPop = true;
}

void RotateOp::onDettach()
{
	QList<qtuser_3d::Pickable*> pickableList = m_helperEntity->getPickables();
	for (qtuser_3d::Pickable* pickable : pickableList)
	{
		unTracePickable(pickable);
	}

	removeSelectorTracer(this);
	visHide(m_helperEntity);
	m_selectedModels.clear();

	QList<qtuser_3d::LeftMouseEventHandler*> handlers = m_helperEntity->getLeftMouseEventHandlers();
	for (qtuser_3d::LeftMouseEventHandler* handler : handlers)
	{
		removeLeftMouseEventHandler(handler);
	}
	removeWheelEventHandler(this);
	removeLeftMouseEventHandler(this);

	m_helperEntity->setPickSource(nullptr);
	m_helperEntity->setScreenCamera(nullptr);
	m_helperEntity->setRotateCallback(nullptr);

	requestVisUpdate(true);

	m_bShowPop = false;
}

void RotateOp::onLeftMouseButtonPress(QMouseEvent* event)
{
	MoveOperateMode::onLeftMouseButtonPress(event);

	m_mode = TMode::null;
	m_spacePoints.clear();
	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		m_spacePoints.push_back(process(event->pos(), m_selectedModels[i]));

		m_saveAngle = 0;
		m_isRoate = true;

		//m_selectedModels[i]->setNeedCheckScope(0);
	}

	fillChangeStructs(m_changes, true);
}

void RotateOp::perform(const QPoint& point, bool reversible, bool needcheck)
{
	QVector3D axis;
	float angle = 0;
	if (m_selectedModels.size() > 0)
	{
		process(point, axis, angle);
		axis.normalize();
	}
	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		QQuaternion oldq = m_selectedModels[i]->localQuaternion();
		QQuaternion q = m_selectedModels[i]->rotateByStandardAngle(axis, angle);

		qtuser_3d::Box3D box = m_selectedModels[i]->globalSpaceBox();
		QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());

		QVector3D oldPosition = m_selectedModels[i]->localPosition();
		QVector3D newPosition = m_selectedModels[i]->localPosition();

		mixTRModel(m_selectedModels[i], oldPosition, newPosition, oldq, q, reversible);

	}

	if (m_selectedModels.size() > 0)
	{
		m_displayRotate = m_selectedModels.back()->localRotateAngle();
	}
	
	requestVisUpdate(true);
}

void RotateOp::onLeftMouseButtonRelease(QMouseEvent* event)
{
	MoveOperateMode::onLeftMouseButtonRelease(event);
	m_isRoate = false;
	if (m_selectedModels.size() && (m_mode != TMode::null))
	{
		perform(event->pos(), false, true);

		for (size_t i = 0; i < m_selectedModels.size(); i++)
		{
			qtuser_3d::Box3D box = m_selectedModels[i]->globalSpaceBox();
			QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());
			QVector3D oldPosition = m_selectedModels[i]->localPosition();
			QVector3D newPosition = m_selectedModels[i]->localPosition() + zoffset;

			moveModel(m_selectedModels[i], oldPosition, newPosition, false);

			//m_selectedModels[i]->setNeedCheckScope(1);
		}
		emit rotateChanged();
        m_bShowPop = false;
	}
	/*QList<creative_kernel::ModelN*> alls = creative_kernel::modelns();
	for (size_t i = 0; i < alls.size(); i++)
	{
		alls[i]->setNeedCheckScope(1);
	}*/

	fillChangeStructs(m_changes, false);
	mixUnions(m_changes, true);
	m_changes.clear();
}

void RotateOp::rotateByAxis(QVector3D& axis,float & angle)
{
	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		QQuaternion oldQ = m_selectedModels[i]->localQuaternion();
		QQuaternion q = m_selectedModels[i]->rotateByStandardAngle(axis, angle);

		qtuser_3d::Box3D box = m_selectedModels[i]->globalSpaceBox();
		QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());
		QVector3D localPosition = m_selectedModels[i]->localPosition();
		QVector3D newPosition = localPosition + zoffset;
		//m_selectedModels[i]->setLocalPosition(newPosition);
		mixTRModel(m_selectedModels[i], localPosition, newPosition, oldQ, q, true);
		emit rotateChanged();
		if (axis.x() != 0 || axis.y() != 0)
			m_selectedModels[i]->resetNestRotation();
	}
}

void RotateOp::onLeftMouseButtonMove(QMouseEvent* event)
{
	MoveOperateMode::onLeftMouseButtonMove(event);
	if (m_selectedModels.size() && (m_mode != TMode::null))
	{
		perform(event->pos(), true, false);
        emit rotateChanged();
        if(!m_bShowPop)
        {
            m_bShowPop=true;
        }
	}

	if (tip_object_ && m_isRoate)
	{
		QPoint point = event->pos();
		QQmlProperty::write(tip_object_, "posX", QVariant::fromValue<int>(point.x()+10));
		QQmlProperty::write(tip_object_, "posY", QVariant::fromValue<int>(point.y()+10));
	}
}

void RotateOp::onLeftMouseButtonClick(QMouseEvent* event)
{
	
}

void RotateOp::onWheelEvent(QWheelEvent* event)
{
	Qt3DRender::QCamera* mainCamera = getCachedCameraEntity();
	float curFovy = mainCamera->fieldOfView();
	m_helperEntity->setScale(curFovy / m_originFovy);
}

void RotateOp::setSelectedModel(QList<creative_kernel::ModelN*> models)
{
	//trace selected node
	m_selectedModels = models;
    if(m_selectedModels.size())
        m_displayRotate = m_selectedModels.back()->localRotateAngle();
	qDebug() << "setSelectedModel";
	buildFromSelections();
}

void RotateOp::onSelectionsChanged()
{
	QList<creative_kernel::ModelN*> selections = selectionms();
	setSelectedModel(selections);

	emit mouseLeftClicked();
}

void RotateOp::selectChanged(qtuser_3d::Pickable* pickable)
{
	if (!m_isRoate)
	{
		updateHelperEntity();
	}
	
	if (m_selectedModels.size() > 0)
	{
		m_displayRotate = m_selectedModels.at(0)->localRotateAngle();
		emit rotateChanged();
	}
}

void RotateOp::onStartRotate()
{
	m_isRoate = true;
	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		m_selectedModels[i]->rotateByStandardAngle(QVector3D(0.0, 0.0, 0.0), 0.0, false);
	}

	setNeedCheckScope(0);
}

void RotateOp::onRotate(QQuaternion q)
{
	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		QQuaternion initq;
		QVariant initTemp = m_selectedModels[i]->property(MODEL_INIT_ROT_TAG);
		if (!initTemp.isValid())
		{
			initq = m_selectedModels[i]->localQuaternion();
			m_selectedModels[i]->setProperty(MODEL_INIT_ROT_TAG, initq);
		}
		else
		{
			initq = initTemp.value<QQuaternion>();
		}

		QQuaternion oldq = m_selectedModels[i]->localQuaternion();
		QQuaternion newq = q * initq;
		m_selectedModels[i]->setLocalQuaternion(initq);
		m_selectedModels[i]->rotateByStandardAngle(m_helperEntity->getCurrentRotateAxis(), m_helperEntity->getCurrentRotAngle(), true);

		qtuser_3d::Box3D box = m_selectedModels[i]->globalSpaceBox();
		QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());

		QVector3D oldPosition = m_selectedModels[i]->localPosition();
		QVector3D newPosition = m_selectedModels[i]->localPosition();

		mixTRModel(m_selectedModels[i], oldPosition, newPosition, oldq, newq, false);
	}

	if (m_selectedModels.size() > 0)
	{
		m_displayRotate = m_selectedModels.back()->localRotateAngle();
	}

	requestVisUpdate(true);
}

void RotateOp::setRotateAngle(QVector3D axis, float angle)
{
	if (tip_object_ == nullptr)
		return;

	if (tip_object_)
	{
		QQmlProperty::write(tip_object_, "isVisible", QVariant::fromValue<bool>(true));
	}

	axis = m_helperEntity->getCurrentRotateAxis();
	angle = m_helperEntity->getCurrentRotAngle();

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
	setNeedCheckScope(1);

	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{		
		QVariant initTemp = m_selectedModels[i]->property(MODEL_INIT_ROT_TAG);
		QQuaternion oldq = initTemp.isValid() ? initTemp.value<QQuaternion>() : m_selectedModels[i]->localQuaternion();
		QQuaternion newq = q * oldq;
		float x, y, z, angle;
		q.getAxisAndAngle(&x, &y, &z, &angle);
		m_selectedModels[i]->setLocalQuaternion(oldq);
		m_selectedModels[i]->rotateByStandardAngle(m_helperEntity->getCurrentRotateAxis(), m_helperEntity->getCurrentRotAngle(), true);

		qtuser_3d::Box3D box = m_selectedModels[i]->globalSpaceBox();
		QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());

		QVector3D oldPosition = m_selectedModels[i]->localPosition();
		QVector3D newPosition = oldPosition + zoffset;

		mixTRModel(m_selectedModels[i], oldPosition, newPosition, oldq, newq, true);

		m_selectedModels[i]->setProperty(MODEL_INIT_ROT_TAG, QVariant());
		if (std::abs(q.x() - q.y()) > 0.00001)
		{
			m_selectedModels[i]->resetNestRotation();
		}
	}

	updateHelperEntity();

	if (m_selectedModels.size() > 0)
	{
		m_displayRotate = m_selectedModels.back()->localRotateAngle();
	}

	emit rotateChanged();
	m_bShowPop = false;
	m_isRoate = false;

	if (tip_object_)
	{
		QQmlProperty::write(tip_object_, "isVisible", QVariant::fromValue<bool>(false));
	}
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
	qDebug() << "rotate reset";
	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		QQuaternion oldQ = m_selectedModels[i]->localQuaternion();
		QQuaternion q = QQuaternion();
		//m_selectedModels[i]->setLocalQuaternion(q);
		m_selectedModels[i]->resetRotate();

		qtuser_3d::Box3D box = m_selectedModels[i]->globalSpaceBox();
		QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());

		QVector3D oldPosition = m_selectedModels[i]->localPosition();
		QVector3D newPosition = oldPosition + zoffset;

		mixTRModel(m_selectedModels[i], oldPosition, newPosition, oldQ, q, true);

		//moveModel(m_selectedModels[i], oldPosition, newPosition, true);		
		m_selectedModels[i]->setProperty(MODEL_INIT_ROT_TAG, QVariant());
		m_selectedModels[i]->resetNestRotation();
	}
    m_displayRotate = QVector3D();
	emit rotateChanged();
}

QVector3D RotateOp::rotate()
{
	if (m_selectedModels.size())
	{
        return m_displayRotate;
	}

	return QVector3D(0.0f, 0.0f, 0.0f);
}

void RotateOp::setRotate(QVector3D rotate)
{
	qDebug() << "rotate setRotate" << rotate;

	if (m_selectedModels.size() == 0)
	{
		return;
	}

	creative_kernel::ModelN* m = m_selectedModels.back();
	QVector3D currentAngles = m->localRotateAngle();
    m_displayRotate = rotate;
	rotate = rotate - currentAngles;
	
	{
		if (qAbs(rotate.x()) > 0.000001)
		{
			QVector3D axis = QVector3D(1.0f, 0.0f, 0.0f);
			float angle = rotate.x();			
			rotateByAxis(axis, angle);
		}
		if (qAbs(rotate.y()) > 0.000001)
		{
			QVector3D axis = QVector3D(0.0f, 1.0f, 0.0f);
			float angle = rotate.y();
			rotateByAxis(axis, angle);
		}
		if (qAbs(rotate.z()) > 0.000001)
		{
			QVector3D axis = QVector3D(0.0f, 0.0f, 1.0f);
			float angle = rotate.z();
			rotateByAxis(axis, angle);
		}

		updateHelperEntity();
		requestVisUpdate(true);
	}
}

void RotateOp::startRotate()
{
}

void RotateOp::process(const QPoint& point, QVector3D& axis, float& angle)
{
	//QVector3D p = process(point);

	axis = QVector3D(0.0f, 0.0f, 1.0f);
	QVector3D planeCenter = QVector3D(0.0f, 0.0f, 0.0f);
	
	//if (m_selectedModels.size())
	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		ModelN* model = m_selectedModels.at(i);
		if (model->hasFDMSupport())
		{
			emit supportMessage();
			break;
		}
	}
	
//	for (size_t i = 0; i < m_selectedModels.size(); i++)
	{
		int i = m_selectedModels.size() - 1;
		QVector3D p = process(point, m_selectedModels[i]);
		qtuser_3d::Box3D box = m_selectedModels[i]->globalSpaceBox();
		planeCenter = box.center();

		QVector3D delta;
		QVector3D oc0 = m_spacePoints[i] - planeCenter;
		QVector3D oc1 = p - planeCenter;
		if (oc0.length() != 0.0f && oc1.length() != 0.0f)
		{
			oc0.normalize();
			oc1.normalize();

			if (oc0 == oc1)
			{
				angle = 0.0f;
			}
			else if (oc0 == -oc1)
			{
				angle = 180.0f;
			}
			else
			{
				axis = QVector3D::crossProduct(oc0, oc1);
				axis.normalize();
				for (int j = 0; j < 3; j++)
				{
					if (qAbs(axis[j]) < 0.01)
					{
						axis[j] = 0;
					}
					else
					{
						axis[j] = axis[j] > 0 ? 1 : -1;
					}
				}
				angle = qAcos(QVector3D::dotProduct(oc0, oc1)) * 180.0f / 3.1415926f;
				float baseAngle = m_saveAngle;
				m_saveAngle = angle;
				angle -= baseAngle;
			}
		}
	}
	//angle = 0.0f;
}

QVector3D RotateOp::process(const QPoint& point, creative_kernel::ModelN* model)
{
	qtuser_3d::Ray ray = visRay(point);

	QVector3D planeCenter;
	QVector3D planeDir;
	getProperPlane(planeCenter, planeDir, ray, model);

	QVector3D c;
	qtuser_3d::lineCollidePlane(planeCenter, planeDir, ray, c);
	return c;
}

void RotateOp::getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray, creative_kernel::ModelN* model)
{
	planeCenter = QVector3D(0.0f, 0.0f, 0.0f);

	if (model)
	{
		qtuser_3d::Box3D box = model->globalSpaceBox();
		planeCenter = box.center();
	}

	if (m_mode == TMode::x)  // x
	{
		planeDir = QVector3D(1.0f, 0.0f, 0.0f);
	}
	else if (m_mode == TMode::y)
	{
		planeDir = QVector3D(0.0f, 1.0f, 0.0f);
	}
	else if (m_mode == TMode::z)
	{
		planeDir = QVector3D(0.0f, 0.0f, 1.0f);
	}
}

void RotateOp::updateHelperEntity()
{
	if (m_selectedModels.size() && (!m_isRoate || m_isMoving))
	{
		qtuser_3d::Box3D box = m_selectedModels[m_selectedModels.size() - 1]->globalSpaceBox();
		m_helperEntity->onBoxChanged(box);

		Qt3DRender::QCamera* mainCamera = getCachedCameraEntity();
		float curFovy = mainCamera->fieldOfView();
		m_helperEntity->setScale(curFovy / m_originFovy);
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

//��ӡ���ɴ�ӡ����
void RotateOp::onBoxChanged(const qtuser_3d::Box3D& box)
{
	const QVector3D size = box.size();
	float length = size.length();
	float s = fmin(length / 450.0, 1.6);
	m_helperEntity->setInitScale(s);
}

void RotateOp::onSceneChanged(const qtuser_3d::Box3D& box)
{

}