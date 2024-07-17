#include "Point2PointMeasureObj.h"
#include "interface/visualsceneinterface.h"
#include "interface/reuseableinterface.h"
#include "qtuser3d/geometry/basicshapecreatehelper.h"
#include "interface/camerainterface.h"

#include <QMatrix4x4>
#include <QEffect>
#include <QParameter>
#include <QObject>
#include <QtQml/QQmlProperty>
#include <qmath.h>
#include <Qt3DRender/QDepthTest>

#include "dismeasureop.h"
#include "qtuser3d/refactor/xeffect.h"

using namespace qtuser_3d;
using namespace creative_kernel;

Point2PointMeasureObj::Point2PointMeasureObj(QObject* object) 
	: AbstractMeasureObj(object)
	, m_showUI(nullptr)
{
	m_measureType = 1;

	m_totalPointNum = 2;
	m_currentPointPos = 0;

	Qt3DRender::QGeometry *geometry = qtuser_3d::BasicShapeCreateHelper::createBall(QVector3D(0, 0, 0), 0.5, 30);
	
	/*Qt3DRender::QDepthTest* state = new Qt3DRender::QDepthTest(geometry);
	state->setDepthFunction(Qt3DRender::QDepthTest::Always);*/

	for (int i = 0; i < 2; i++)
	{
		m_measureBall[i].xeffect()->removePassFilter(0, "view", 0);
		m_measureBall[i].xeffect()->addPassFilter(0, "overlay");
		m_measureBall[i].xeffect()->setPassDepthTest(0);
		m_measureBall[i].setGeometry(geometry);
		m_measureBall[i].setColor(QVector4D(0.2f, 0.2f, 0.8f, 1.0));
		//m_measureBall[i].addRenderState(0, state);

	}

	m_normal = QVector3D(0, 0, 1);
	m_measureLineEntity.setLineWidth(4);
	m_measureLineEntity.setColor(QVector4D(1.0f, 0.7529f, 0.0f, 1.0));
	m_measureLineEntity.changeLineStype(LineExEntity::LINE_TYPE::PIN_ARROW_LINE);
	reset();
}

Point2PointMeasureObj::~Point2PointMeasureObj()
{
	visHide(&m_measureBall[0]);
	visHide(&m_measureBall[1]);
	visHide(&m_measureLineEntity);

	if (m_showUI)
	{
		m_showUI->setParent(nullptr);
		delete m_showUI;
	}
	m_showUI = nullptr;
}

int Point2PointMeasureObj::reset()
{
	visHide(&m_measureBall[0]);
	visHide(&m_measureBall[1]);
	visHide(&m_measureLineEntity);
	m_point[0] = QVector3D(0, 0, 0);
	m_point[1] = QVector3D(0, 0, 0);

	m_currentPointPos = 0;

	if (m_showUI)
	{
		m_showUI->setParent(nullptr);
		delete m_showUI;
		m_showUI = nullptr;
	}

	return 0;
}


int Point2PointMeasureObj::addPoint(const QVector3D& position, const QVector3D& normal, const QMatrix4x4& m, MeasureInputType type)
{
	if (type == MeasureInputType::HOVER && m_currentPointPos == 0)
	{
		return -1;
	}
	if (m_currentPointPos >= m_totalPointNum)
	{
		return -2;
	}
	QVector3D new_pos = position;    // m* position;
	QVector3D new_normal = normal;  //  m* normal;

	int ret = setPoint(new_pos, new_normal);
	if (ret >= 0)
	{
		if (type == MeasureInputType::CLICK)
		{
			m_currentPointPos++;
			if (m_currentPointPos == m_totalPointNum)
			{
				executeMeasure(MeasureResultType::FINAL);
				return 1;
			}
		}
	}

	return 0;
}

int Point2PointMeasureObj::executeMeasure(MeasureResultType result_type)
{
	if (m_currentPointPos > 0)
	{
		QVector3D t = m_point[1] - m_point[0];
		float v = t.length();
		m_measureValue = QString::number(v, 'f', 2) + "mm";

		createShowUI(m_measureValue);

		return 0;
	}
	return -1;

}

int Point2PointMeasureObj::setPoint(const QVector3D& position, const QVector3D& normal)
{
	QMatrix4x4 m;
	m.translate(position);
	m_measureBall[m_currentPointPos].setPose(m);
	visShow(&m_measureBall[m_currentPointPos]);
	requestVisUpdate();

	if (m_currentPointPos == 0)
	{
		m_normal = normal;
	}

	m_point[m_currentPointPos] = position;

	if (m_currentPointPos == 1)
	{
		m_measureLineEntity.setPoint(m_point[0], m_point[1], m_normal);
		m_measureLineEntity.genGeometry();
		visShow(&m_measureLineEntity);
		requestVisUpdate();
		
		executeMeasure(MeasureResultType::MID);
	}

	return 0;
}

int Point2PointMeasureObj::createShowUI(QString s)
{
	if (m_attachUI == nullptr)
	{
		return -1;
	}
	if (m_showUI == nullptr)
	{
		DistanceMeasureOp* measureOp = qobject_cast<DistanceMeasureOp*> (m_attachUI);
		m_showUI = measureOp->createMeasureShowUI();
	}
	updateShowUI();
	
	return 0;
}

int Point2PointMeasureObj::updateShowUI()
{
	if (m_showUI)
	{
		QQmlProperty::write(m_showUI, "text_show", m_measureValue);

		QVector3D mid = (m_point[0] + m_point[1]) / 2;
		QVector2D r = cameraSpace2WordSpace(mid);
		QQmlProperty::write(m_showUI, "x_value", qRound(r.x()));
		QQmlProperty::write(m_showUI, "y_value", qRound(r.y()));
		return 1;
	}
	return 0;
}


QVector2D Point2PointMeasureObj::cameraSpace2WordSpace(const QVector3D& p)
{
	qtuser_3d::ScreenCamera* camera = visCamera();
	QMatrix4x4 camera_m = camera->viewMatrix();
	QMatrix4x4 project_m = camera->projectionMatrix();

	QSize size = camera->size();
	int w = size.width();
	int h = size.height();

	QVector3D final_p = project_m * camera_m * p;

	int s_w = (final_p.x() + 1) * w / 2;
	int s_h = h - (final_p.y() + 1) * h / 2;

	QVector2D r(s_w, s_h);
	return r;
}


