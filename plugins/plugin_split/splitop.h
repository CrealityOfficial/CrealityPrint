#ifndef _NULLSPACE_SPLITOP_1591235104278_H
#define _NULLSPACE_SPLITOP_1591235104278_H
#include "qtuser3d/scene/sceneoperatemode.h"

#include "qtuser3d/module/pickableselecttracer.h"
#include "qtuser3d/module/manipulatecallback.h"
#include "qtuser3d/math/plane.h"
#include "data/modeln.h"

namespace qtuser_3d
{
	class LineExEntity;
	class AlonePointEntity;
}

class SplitPlane;
class SplitOp: public qtuser_3d::SceneOperateMode
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

	void setAcitiveAxis(const QVector3D& axis);
	void setOffset(float offset);
	float getOffset();

	QVector3D& planeRotateAngle();
	bool getShowPop();
	bool getMessage();
	void setMessage(bool isRemove);


	void enableSelectPlaneByCursor(bool enable);

protected:
	void onAttach() override;
	void onDettach() override;
	void onLeftMouseButtonClick(QMouseEvent* event) override;
	void onKeyPress(QKeyEvent* event) override;
	void onHoverMove(QHoverEvent* event) override;

	void onLeftMouseButtonPress(QMouseEvent* event) override;
	void onLeftMouseButtonMove(QMouseEvent* event) override;
	void onLeftMouseButtonRelease(QMouseEvent* event) override;

	void setRotateAngle(QVector3D axis, float angle) override;

	void onStartRotate() override;
	void onRotate(QQuaternion q) override;
	void onEndRotate(QQuaternion q) override;

	void selectChanged(qtuser_3d::Pickable* pickable) override;

	void onSelectionsChanged() override;
	void setSelectedModel(creative_kernel::ModelN* model);

	void updatePlaneEntity(bool request);

	void tryCollectMouseClickEvent(QMouseEvent *event);
	bool processCursorMoveEvent(const QPoint& pos);

	bool getSelectedModelsCenter(QVector3D* center);

	void updatePlanePosition();

	QVector3D makeWorldPositionFromScreen(const QPoint& pos);

protected:
	qtuser_3d::LineExEntity* m_lineEntity;
	qtuser_3d::AlonePointEntity* m_pointEntity;
	SplitPlane* m_splitPlane;

	qtuser_3d::Plane m_plane;

	QVector3D m_saveNormal;
	QVector3D m_saveAngle;

	QVector3D m_rotateAngle;    // rotate euler

	bool m_bShowPop = false;

	bool m_selectPlaneByCursor;
	QVector<QVector3D> m_selectedPosition;
	float m_offset;
signals:
	void posionChanged();
	void rotateAngleChanged();
	void mouseLeftClicked();
};
#endif // _NULLSPACE_SPLITOP_1591235104278_H
