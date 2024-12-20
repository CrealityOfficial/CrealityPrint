#ifndef _POINT_POINT_MEASURE_OBJ_H__
#define _POINT_POINT_MEASURE_OBJ_H__

#include "AbstractMeasureObj.h"
#include "entity/lineexentity.h"
#include "entity/purecolorentity.h"

class Point2PointMeasureObj : public AbstractMeasureObj
{
public:
	Point2PointMeasureObj(QObject* object);
	~Point2PointMeasureObj();

	int reset() override;

	int addPoint(const QVector3D& position, const QVector3D& normal, const QMatrix4x4& m, MeasureInputType type) override;

	int updateShowUI() override;

	bool complete() override {return m_currentPointPos >= m_totalPointNum;}
protected:
	int executeMeasure(MeasureResultType result_type);
	int setPoint(const QVector3D& position, const QVector3D& normal);

	int createShowUI(QString s);
	
	QVector2D cameraSpace2WordSpace(const QVector3D& p);
protected:
	QVector3D m_point[2];
	int m_currentPointPos;      // 当前点下标
	int m_totalPointNum;        // 最终接受几个点

protected:
	qtuser_3d::PureColorEntity m_measureBall[2];

	QVector3D m_normal;

	qtuser_3d::LineExEntity m_measureLineEntity;

	Qt3DRender::QParameter* m_stateColorsParam;

	QObject* m_showUI;

};




#endif
