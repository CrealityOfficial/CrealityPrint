#ifndef WIPE_TOWER_HANDLE_202401091033_H
#define WIPE_TOWER_HANDLE_202401091033_H

#include <QtCore/QPointer>
#include "basickernelexport.h"
#include "qtuser3d/event/eventhandlers.h"
#include "operation/moveoperatemode.h"

namespace creative_kernel {
	class WipeTowerEntity;

	class WipeTowerHandler : public QObject, public qtuser_3d::LeftMouseEventHandler
	{
		Q_OBJECT
	public:
		WipeTowerHandler(QObject* parent = nullptr);
		~WipeTowerHandler();

	protected:
		virtual void onLeftMouseButtonPress(QMouseEvent* event) override;
		virtual void onLeftMouseButtonRelease(QMouseEvent* event) override;
		virtual void onLeftMouseButtonMove(QMouseEvent* event) override;
		virtual void onLeftMouseButtonClick(QMouseEvent* event) override;

	protected:
		QVector3D process(const QPoint& point);
		void getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray);
		QVector3D clip(const QVector3D& delta);

	private:
		TMode m_mode;
		QPointer<WipeTowerEntity> m_processEntity;
		
		QVector3D m_startPressPos;
		QVector3D m_startEntityPos;
		QVector3D m_startEntityPosOfLeftBottom;
	};
}

#endif // !WIPE_TOWER_HANDLE_202401091033_H
