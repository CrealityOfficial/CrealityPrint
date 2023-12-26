#ifndef _NULLSPACE_TRANSLATEOP_1589770383921_H
#define _NULLSPACE_TRANSLATEOP_1589770383921_H
#include "basickernelexport.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/module/pickableselecttracer.h"

#include <QVector3D>
#include "entity/translatehelperentity.h"
#include "qtuser3d/math/box3d.h"
#include "data/modeln.h"
#include "../utils/operationutil.h"
#include "interface/spaceinterface.h"
#include "moveoperatemode.h"

#define PosMax 1000

namespace creative_kernel
{
	class Kernel;
}

class BASIC_KERNEL_API TranslateOp : public MoveOperateMode
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

	void updateHelperEntity();

    void movePositionToinit(QList<creative_kernel::ModelN*>& selections);

protected:
	void onBoxChanged(const qtuser_3d::Box3D& box) override;
	void onSceneChanged(const qtuser_3d::Box3D& box) override;

private:
	qtuser_3d::TranslateHelperEntity* m_helperEntity;
	double m_originFovy;
};
#endif // _NULLSPACE_TRANSLATEOP_1589770383921_H
