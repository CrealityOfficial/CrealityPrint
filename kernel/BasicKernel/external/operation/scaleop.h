#ifndef _NULLSPACE_SCALEOP_1589770383921_H
#define _NULLSPACE_SCALEOP_1589770383921_H
#include "basickernelexport.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/module/pickableselecttracer.h"

#include <QVector3D>
#include "entity/translatehelperentity.h"
#include "data/modeln.h"
#include "data/undochange.h"

#include "../utils/operationutil.h"
#include "interface/spaceinterface.h"
#include "moveoperatemode.h"

namespace creative_kernel
{
	class Kernel;
}

class BASIC_KERNEL_API ScaleOp : public MoveOperateMode
	, public qtuser_3d::SelectorTracer , public creative_kernel::SpaceTracer
{
public:
	Q_OBJECT

public:
	ScaleOp(QObject* parent = nullptr);
	virtual ~ScaleOp();

	void setMessage(bool isRemove);
	bool getMessage();

	void reset();
	QVector3D scale();
    QVector3D box();
	QVector3D globalbox();
	void setScale(QVector3D scale);
	void setScaleLock(bool on);

	bool uniformCheck();
	void setUniformCheck(bool check);

	bool getShowPop();

signals:
	void scaleChanged();
	void sizeChanged();
	void checkChanged();
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
	bool shouldMultipleSelect() override;
protected:
	void setSelectedModel(creative_kernel::ModelN* model);

	void buildFromSelections();
	QVector3D process(const QPoint& point);
	void getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray);
	QVector3D getScale(const QVector3D& p);

	void updateHelperEntity();

	void perform(const QPoint& point, bool reversible);

protected:
	void onBoxChanged(const qtuser_3d::Box3D& box) override;
	void onSceneChanged(const qtuser_3d::Box3D& box) override;
	void onModelDestroyed();

private:
	qtuser_3d::TranslateHelperEntity* m_helperEntity;

	QVector3D m_spacePoint;
	TMode m_mode;

	QVector3D m_saveScale;
	QVector3D m_savePosition;

	creative_kernel::ModelN* m_selectedModel;
    bool m_bShowPop=false;

	bool m_uniformCheck=true;

	double m_originFovy;

	QList<creative_kernel::NUnionChangedStruct> m_changes;

	QMap<creative_kernel::ModelN*, bool> m_modelsScaleLock;

};
#endif // _NULLSPACE_SCALEOP_1589770383921_H
