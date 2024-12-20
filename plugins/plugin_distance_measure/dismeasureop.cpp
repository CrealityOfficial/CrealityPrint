#include "dismeasureop.h"

#include "entity/lineentity.h"
#include "qtuser3d/geometry/basicshapecreatehelper.h"
#include "qtusercore/property/qmlpropertysetter.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"
#include "interface/eventinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/camerainterface.h"

#include <QtQml/QQmlProperty>

#include "data/modeln.h"
#include "kernel/kernelui.h"

#include "AbstractMeasureObj.h"
#include "MeasureObjFactory.h"

#define SINGLE_MEASURE   1

using namespace creative_kernel;
using namespace qtuser_3d;
using namespace qtuser_qml;

DistanceMeasureOp::DistanceMeasureOp(QObject* parent)
	:SceneOperateMode(parent)
{
	m_type = qtuser_3d::SceneOperateMode::FixedMode;

	reset();
	m_measrureType = 1;

	m_root = getKernelUI()->getUI("UIRoot");
	m_context = qmlContext(m_root);
	QQmlEngine* engine = qmlEngine(m_root);
	m_measureComponent = new QQmlComponent(engine, QUrl::fromLocalFile(":/distance_measure/distance_measure/info.qml"), this);
}

DistanceMeasureOp::~DistanceMeasureOp()
{
	clear();
}

void DistanceMeasureOp::reset()
{
	m_measrureType = 1;
}

void DistanceMeasureOp::clear()
{
	m_currentMeasureObj = nullptr;
	for (AbstractMeasureObj* obj : m_measureObjects)
	{
		if (obj)
		{
			delete obj;
		}
	}
	m_measureObjects.clear();
}

QObject* DistanceMeasureOp::createMeasureShowUI()
{
	QObject* measureShowUI = m_measureComponent->create(m_context);
	measureShowUI->setParent(m_root);
	qtuser_qml::writeObjectProperty(measureShowUI, "parent", m_root);

	return measureShowUI;
}

void DistanceMeasureOp::generateMeasureObj()
{
	m_currentMeasureObj = MeasureObjFactory::generateMeasureObj(m_measrureType, this);
	if (m_currentMeasureObj)
	{
		m_measureObjects.append(m_currentMeasureObj);
	}
}

void DistanceMeasureOp::onAttach()
{
	visCamera()->addCameraObserver(this);

	generateMeasureObj();
	
	reset();

	addLeftMouseEventHandler(this);
	addHoverEventHandler(this);
	addKeyEventHandler(this);
	traceSpace(this);

	m_bShowPop = true;
}

void DistanceMeasureOp::onDettach()
{
	visCamera()->removeCameraObserver(this);

	reset();
	clear();

	removeLeftMouseEventHandler(this);
	removeHoverEventHandler(this);
	removeKeyEventHandler(this);
	unTraceSpace(this);

	requestVisUpdate();

	m_bShowPop = false;
}

void DistanceMeasureOp::setMeasureType(int measure_type)
{
	m_measrureType = measure_type;
}

int DistanceMeasureOp::getMeasureType() const
{
	return m_measrureType;
}

void DistanceMeasureOp::onLeftMouseButtonClick(QMouseEvent* event)
{
	if (m_currentMeasureObj == nullptr)
	{
		return;
	}
	int faceID;
	QVector3D position, normal;
	ModelN* model = checkPickModel(event->pos(), position, normal, &faceID);
	if (model)
	{
		QMatrix4x4 m = model->globalMatrix();
		QVector3D vec3Pos = position;
		QVector3D vec3Normal = normal;
		int ret = m_currentMeasureObj->addPoint(vec3Pos, vec3Normal, m, AbstractMeasureObj::MeasureInputType::CLICK);
		if (ret == 1)
		{
			generateMeasureObj();
		}

#if SINGLE_MEASURE
		else if (ret == 0) {

			for (AbstractMeasureObj* obj : m_measureObjects)
			{
				if (obj != m_currentMeasureObj)
				{
					delete obj;
					m_measureObjects.removeOne(obj);
				}
			}
		}
#endif // SINGLE_MEASURE
	}
}

void DistanceMeasureOp::onLeftMouseButtonPress(QMouseEvent* event)
{
	//return true;
}

void DistanceMeasureOp::onHoverMove(QHoverEvent* event)
{
	if (m_currentMeasureObj == nullptr)
	{
		//return false;
		return;
	}
	int faceID;
	//return true;
	QVector3D position, normal;
	ModelN* model = checkPickModel(event->pos(), position, normal, &faceID);
	if (model)
	{
		QMatrix4x4 m = model->globalMatrix();
		QVector3D vec3Pos = position;
		QVector3D vec3Normal = normal;
		//QMatrix4x4 m = model->globalMatrix();
		m_currentMeasureObj->addPoint(vec3Pos, vec3Normal, m, AbstractMeasureObj::MeasureInputType::HOVER);
	}
	 //return true;

}

void DistanceMeasureOp::onCameraChanged(qtuser_3d::ScreenCamera* camera)
{
	for (AbstractMeasureObj* measure_obj : m_measureObjects)
	{
		measure_obj->updateShowUI();
	}
}

void DistanceMeasureOp::onModelToRemoved(creative_kernel::ModelN* model)
{
	clear();
}

bool DistanceMeasureOp::getShowPop()
{
	return m_bShowPop;
}

void DistanceMeasureOp::onKeyPress(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape)
	{
		if (m_currentMeasureObj != nullptr && !m_currentMeasureObj->complete())
		{
			m_currentMeasureObj->reset();
		}
	}
}

void DistanceMeasureOp::onKeyRelease(QKeyEvent* event)
{

}

