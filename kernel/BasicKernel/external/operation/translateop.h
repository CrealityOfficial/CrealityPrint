#ifndef _NULLSPACE_TRANSLATEOP_1589770383921_H
#define _NULLSPACE_TRANSLATEOP_1589770383921_H
#include "basickernelexport.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/module/pickableselecttracer.h"

#include <QVector3D>
#include "qtuser3d/entity/translatehelperentity.h"
#include "qtuser3d/math/box3d.h"
#include "data/modeln.h"
#include "../utils/operationutil.h"
#include "interface/spaceinterface.h"

#define PosMax 1000

namespace creative_kernel
{
	class Kernel;
}

class BASIC_KERNEL_API TranslateOp : public qtuser_3d::SceneOperateMode
	, public qtuser_3d::SelectorTracer , public creative_kernel::SpaceTracer
{
	Q_OBJECT
public:
	TranslateOp(QObject* parent = nullptr);
	virtual ~TranslateOp();

	void setMessage(bool isRemove);
	bool getMessage();

	void reset();
    void center();
	QVector3D position();
	void setPosition(QVector3D position);
	void setBottom();
signals:
	void positionChanged();
	void mouseLeftClicked();

protected:
	void onAttach() override;
	void onDettach() override;
	void onLeftMouseButtonPress(QMouseEvent* event) override;
	void onLeftMouseButtonRelease(QMouseEvent* event) override;
	void onLeftMouseButtonMove(QMouseEvent* event) override;
	void onLeftMouseButtonClick(QMouseEvent* event) override;
	void onWheelEvent(QWheelEvent* event) override;
	void onKeyPress(QKeyEvent* event) override;
	void onKeyRelease(QKeyEvent* event) override;
	bool shouldMultipleSelect() override;
	void onSelectionsChanged() override;
	void selectChanged(qtuser_3d::Pickable* pickable) override;
protected:
	void setSelectedModel(creative_kernel::ModelN* model);
	void setSelectedModel(QList<creative_kernel::ModelN*> models);

	void buildFromSelections();
	QVector3D process(const QPoint& point);
	void getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray);
	QVector3D clip(const QVector3D& delta);

	void updateHelperEntity();

    void movePositionToinit(QList<creative_kernel::ModelN*>& selections);

protected:
	void onBoxChanged(const qtuser_3d::Box3D& box) override;
	void onSceneChanged(const qtuser_3d::Box3D& box) override;

private:
	qtuser_3d::TranslateHelperEntity* m_helperEntity;

	QVector3D m_spacePoint;

	TMode m_mode;

	QVector3D m_saveLocalPosition;
	QList<QVector3D> m_saveLocalPositions;

	creative_kernel::ModelN* m_selectedModel;
	QList<creative_kernel::ModelN*> m_selectedModels;
	bool moveEnable;
	double m_originFovy;
};
#endif // _NULLSPACE_TRANSLATEOP_1589770383921_H
