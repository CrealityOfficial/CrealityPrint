#ifndef _NULLSPACE_DISTANCE_MEASURE_OP_1591235104278_H
#define _NULLSPACE_DISTANCE_MEASURE_OP_1591235104278_H
#include "qtuser3d/scene/sceneoperatemode.h"

#include "qtuser3d/math/plane.h"
#include "data/modeln.h"
#include <QtQml/QQmlComponent>
#include "qtuser3d/camera/screencamera.h"
#include "data/interface.h"

class AbstractMeasureObj;

class DistanceMeasureOp: public qtuser_3d::SceneOperateMode
	, public qtuser_3d::ScreenCameraObserver
	, public creative_kernel::SpaceTracer
{
	Q_OBJECT
public:
	DistanceMeasureOp(QObject* parent = nullptr);
	virtual ~DistanceMeasureOp();

	bool getShowPop();
	virtual bool isGlobalMode() { return true; }

protected:
	void onAttach() override;
	void onDettach() override;
	void onLeftMouseButtonClick(QMouseEvent* event) override;
	void onLeftMouseButtonPress(QMouseEvent* event) override;
	void onHoverMove(QHoverEvent* event) override;

	void onKeyPress(QKeyEvent* event) override;
	void onKeyRelease(QKeyEvent* event) override;

	void onCameraChanged(qtuser_3d::ScreenCamera* camera) override;
	void onModelToRemoved(creative_kernel::ModelN* model) override;
public:
	void setMeasureType(int measure_type);
	int getMeasureType() const;

	QObject* createMeasureShowUI();
	
private:
	void reset();
	void clear();
	void generateMeasureObj();

protected:
	QObject* m_root;
	QQmlContext* m_context;

protected:
	int m_measrureType;      // 0�� �㵽�㣻 1���㵽�棻 2 �浽�棨ֻ��ƽ�������Ч��

	QList<AbstractMeasureObj*> m_measureObjects;
	AbstractMeasureObj* m_currentMeasureObj;

	QQmlComponent* m_measureComponent;

	bool m_bShowPop = false;
};
#endif // _NULLSPACE_DISTANCE_MEASURE_OP_1591235104278_H
