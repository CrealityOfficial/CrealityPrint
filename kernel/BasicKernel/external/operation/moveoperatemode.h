#ifndef _NULLSPACE_MOVE_OPERATE_MODE_1589770383921_H
#define _NULLSPACE_MOVE_OPERATE_MODE_1589770383921_H
#include "basickernelexport.h"
#include "qtuser3d/scene/sceneoperatemode.h"
#include "data/interface.h"

#include <QVector3D>
#include "entity/translatehelperentity.h"
#include "qtuser3d/math/box3d.h"
#include "data/modeln.h"
#include "data/modelgroup.h"

#include "interface/spaceinterface.h"

#define PosMax 1000

namespace creative_kernel
{
	enum class TMode
	{
		null,
		x,
		y,
		z,
		xy,
		yz,
		zx,
		xyz,
		sp
	};

	/*
	 * 旋转和缩放
	 */
	BASIC_KERNEL_API void getTSProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray, qtuser_3d::Box3D box, TMode m);

	/*
	 * 旋转
	 */
	BASIC_KERNEL_API void getRProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray, qtuser_3d::Box3D box, TMode m);

	/*
	 * op_type:
	 *		= 0  getTSProperPlane
	 *		= 1  getRProperPlane
	 */
	BASIC_KERNEL_API QVector3D operationProcessCoord(const QPoint& point, const qtuser_3d::Box3D& box, int op_type, TMode m);

	class Kernel;
	class BASIC_KERNEL_API MoveOperateMode : public qtuser_3d::SceneOperateMode
	{
		Q_OBJECT
	public:
		MoveOperateMode(QObject* parent = nullptr);
		virtual ~MoveOperateMode();

		void setType(int type) { m_type = type; }

	signals:
		void moving();
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

		void setModelToWorldPosition(ModelN* m, const QVector3D& wps);

	protected:
		
		void prepareMove_models(QMouseEvent* event, const QList<ModelN*>& models);
		void onMove_models(QMouseEvent* event, const QList<ModelN*>& models);
		void finishMove_models(QMouseEvent* event, const QList<ModelN*>& models);

		void prepareMove_groups(QMouseEvent* event, const QList<ModelGroup*>& groups);
		void onMove_groups(QMouseEvent* event, const QList<ModelGroup*>& groups);
		void finishMove_groups(QMouseEvent* event, const QList<ModelGroup*>& groups);

	protected:
		QVector3D m_spacePoint;
		TMode m_mode;
		QVector3D m_tempLocal;
	};
}
#endif // _NULLSPACE_MOVE_OPERATE_MODE_1589770383921_H
