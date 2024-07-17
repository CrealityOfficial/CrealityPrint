#ifndef _NULLSPACE_SPLITOP_1591235104278_H
#define _NULLSPACE_SPLITOP_1591235104278_H
#include "qtuser3d/scene/sceneoperatemode.h"

#include "qtuser3d/module/pickableselecttracer.h"
#include "qtuser3d/module/manipulatecallback.h"
#include "qtuser3d/math/plane.h"
#include "data/modeln.h"
#include "operation/moveoperatemode.h"

namespace qtuser_3d
{
	class LineExEntity;
	class AlonePointEntity;
	class SplitPlane;
}

class SplitOp: public MoveOperateMode
	, public qtuser_3d::SelectorTracer
	, public qtuser_3d::RotateCallback
{
	Q_OBJECT
public:
	SplitOp(QObject* parent = nullptr);
	virtual ~SplitOp();

	void split();
	void splitParts();

	const qtuser_3d::Plane& plane();
	void setPlanePosition(const QVector3D& position);
	void setPlaneNormal(const QVector3D& normal);
	void setPlaneDir(const QVector3D& rotate);

	void setAcitiveAxis(int axisType);
	void setOffset(float offset);
	float getOffset();

	QVector3D& planeRotateAngle();
	bool getShowPop();
	bool getMessage();
	void setMessage(bool isRemove);
	void enabledIndicate(bool enable);
	void setModelGap(float gap);
	void enableSelectPlaneByCursor(bool enable);

protected:
	void onAttach() override;
	void onDettach() override;
	void onKeyPress(QKeyEvent* event) override;
	//void onHoverMove(QHoverEvent* event) override;

	void onLeftMouseButtonPress(QMouseEvent* event) override;
	void onLeftMouseButtonMove(QMouseEvent* event) override;
	void onLeftMouseButtonRelease(QMouseEvent* event) override;
	void onLeftMouseButtonClick(QMouseEvent* event) override;

	void setRotateAngle(QVector3D axis, float angle) override;

	void onStartRotate() override;
	void onRotate(QQuaternion q) override;
	void onEndRotate(QQuaternion q) override;

	void selectChanged(qtuser_3d::Pickable* pickable) override;
	virtual bool shouldMultipleSelect();

	void onSelectionsChanged() override;
	void setSelectedModel(creative_kernel::ModelN* model);

	void updatePlaneEntity();

	void tryCollectMouseClickEvent(QMouseEvent *event);
	void processCursorMoveEvent(const QPoint& pos);

	bool getSelectedModelsCenter(QVector3D* center);

	void updatePlanePosition();

	QVector3D makeWorldPositionFromScreen(const QPoint& pos);

	void setCustomPlanePosition();

	void processPlaneMove(const QPoint& pos);
	void planeMoveOnClickPos(const QPoint& pos);
	float calMaxOffset();
	void updateOffset(const QVector3D& position);

	void showSplitPlaneOnRelease(const QPoint& releasePos);
	void showCursorPoint(const QPoint& pos);
	void notifyOffsetChanged();

protected:
	qtuser_3d::LineExEntity* m_lineEntity;
	qtuser_3d::AlonePointEntity* m_pointEntity;
	qtuser_3d::SplitPlane* m_splitPlane;

	creative_kernel::ModelN* m_operateModel;

	qtuser_3d::Plane m_plane;

	QVector3D m_saveNormal;
	QVector3D m_saveAngle;

	QVector3D m_rotateAngle;    // rotate euler

	QVector3D m_firstPosition;

	bool m_bShowPop = false;
	
    int m_axisType = 2;     // 0 : x ;1 : y ; z:2

	bool m_selectPlaneByCursor;
	QVector<QVector3D> m_selectedPosition;
	float m_offset;
	bool m_capture;
	bool m_canSplitPlaneMove;
	float m_maxOffset;
	QPoint m_savePoint;
	bool m_bIndicate;
	bool m_planeMoveOnClick;
	int m_gap = 0;

	QPoint m_pressPoint;	//indicate
	bool m_generatePlane { false }; // axis 3
	bool m_tempHidePlane;

	bool m_pickEntity { false };

signals:
	void posionChanged();
	void rotateAngleChanged();
	void mouseLeftClicked();
	void offsetChanged();
};
#endif // _NULLSPACE_SPLITOP_1591235104278_H
