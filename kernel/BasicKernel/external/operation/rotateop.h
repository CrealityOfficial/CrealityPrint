#ifndef _NULLSPACE_ROTATEOP_1589770383921_H
#define _NULLSPACE_ROTATEOP_1589770383921_H
#include "basickernelexport.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/module/pickableselecttracer.h"
#include "qtuser3d/module/manipulatecallback.h"

#include <QVector3D>
#include <QPointer>
#include <QQmlComponent>
#include "entity/rotate3dhelperentity.h"
#include "data/modeln.h"
#include "data/undochange.h"
#include "moveoperatemode.h"

class BASIC_KERNEL_API RotateOp : public MoveOperateMode
	, public qtuser_3d::SelectorTracer, public qtuser_3d::RotateCallback
{
	Q_OBJECT
public:
	RotateOp(QObject* parent = nullptr);
	virtual ~RotateOp();

	void setMessage(bool isRemove);
	bool getMessage();

	void reset();
	QVector3D rotate();
	void setRotate(QVector3D rotate);
    void startRotate();
	bool getShowPop();
	

signals:
	void rotateChanged();
	void supportMessage();
	void mouseLeftClicked();
protected:
	void onAttach() override;
	void onDettach() override;
	void onLeftMouseButtonPress(QMouseEvent* event) override;
	void onLeftMouseButtonRelease(QMouseEvent* event) override;
	void onLeftMouseButtonMove(QMouseEvent* event) override;
	void onLeftMouseButtonClick(QMouseEvent* event) override;
	void onWheelEvent(QWheelEvent* event) override;
	void onSelectionsChanged() override;
	void selectChanged(qtuser_3d::Pickable* pickable) override;
	void onStartRotate() override;
	void onRotate(QQuaternion q) override;
	void onEndRotate(QQuaternion q) override;
	void setRotateAngle(QVector3D axis, float angle) override;

	bool shouldMultipleSelect() override;

protected:
	void setSelectedModel(QList<creative_kernel::ModelN*> models);

	void buildFromSelections();
	QVector3D process(const QPoint& point, creative_kernel::ModelN* model);
	void process(const QPoint& point, QVector3D& axis, float& angle);
	void getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray, creative_kernel::ModelN* model);
    void rotateByAxis(QVector3D& axis,float & angle);

	void updateHelperEntity();

	void perform(const QPoint& point, bool reversible, bool needcheck);

private:
	
	qtuser_3d::Rotate3DHelperEntity* m_helperEntity;

	QVector3D m_spacePoint;
	QList<QVector3D> m_spacePoints;
	TMode m_mode;

	float m_saveAngle;

	bool m_isRoate;
	bool m_isMoving{ false };

	QList<creative_kernel::ModelN*> m_selectedModels;
    QVector3D m_displayRotate;
    bool m_bShowPop=false;

	QList<creative_kernel::NUnionChangedStruct> m_changes;

	QPointer<QQmlComponent> tip_component_;
	QPointer<QObject> tip_object_;
};
#endif // _NULLSPACE_ROTATEOP_1589770383921_H
