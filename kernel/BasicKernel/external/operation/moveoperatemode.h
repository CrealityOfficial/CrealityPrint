#ifndef _NULLSPACE_MOVE_OPERATE_MODE_1589770383921_H
#define _NULLSPACE_MOVE_OPERATE_MODE_1589770383921_H
#include "basickernelexport.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "qtuser3d/module/pickableselecttracer.h"

#include <QVector3D>
#include "entity/translatehelperentity.h"
#include "qtuser3d/math/box3d.h"
#include "data/modeln.h"
#include "../utils/operationutil.h"
#include "interface/spaceinterface.h"

#define PosMax 1000

namespace creative_kernel
{
	class Kernel;
}

class BASIC_KERNEL_API MoveOperateMode : public qtuser_3d::SceneOperateMode
{
	Q_OBJECT
public:
	MoveOperateMode(QObject* parent = nullptr);
	virtual ~MoveOperateMode();
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
	bool shouldMultipleSelect() override;

	QVector3D process(const QPoint& point);
	void getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray);
	QVector3D clip(const QVector3D& delta);
	void setMode(TMode mode);
	void resetSpacePoint(QPoint pos);

private:
	QVector3D m_spacePoint;
	TMode m_mode;
	QVector3D m_tempLocal;

};
#endif // _NULLSPACE_MOVE_OPERATE_MODE_1589770383921_H
