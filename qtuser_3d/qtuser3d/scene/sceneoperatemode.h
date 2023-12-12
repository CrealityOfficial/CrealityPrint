#ifndef QTUSER_3D_SCENEOPERATEMODE_1592203699385_H
#define QTUSER_3D_SCENEOPERATEMODE_1592203699385_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/event/eventhandlers.h"

namespace qtuser_3d
{
	class QTUSER_3D_API SceneOperateMode : public QObject
		, public qtuser_3d::EventCheckHandler
		, public qtuser_3d::HoverEventHandler
		, public qtuser_3d::WheelEventHandler
		, public qtuser_3d::RightMouseEventHandler
		, public qtuser_3d::MidMouseEventHandler
		, public qtuser_3d::LeftMouseEventHandler
		, public qtuser_3d::KeyEventHandler
	{
		Q_OBJECT
	public:
		SceneOperateMode(QObject* parent = nullptr);
		virtual ~SceneOperateMode();

		virtual void onAttach() = 0;
		virtual void onDettach() = 0;

		virtual bool shouldMultipleSelect();

		void onHoverEnter(QHoverEvent* event) override;
		void onHoverLeave(QHoverEvent* event) override;
		void onHoverMove(QHoverEvent* event) override;

		void onLeftMouseButtonPress(QMouseEvent* event) override;
		void onLeftMouseButtonRelease(QMouseEvent* event) override;
		void onLeftMouseButtonMove(QMouseEvent* event) override;
		void onLeftMouseButtonClick(QMouseEvent* event) override;

		void onMidMouseButtonPress(QMouseEvent* event) override;
		void onMidMouseButtonRelease(QMouseEvent* event) override;
		void onMidMouseButtonMove(QMouseEvent* event) override;
		void onMidMouseButtonClick(QMouseEvent* event) override;

		void onWheelEvent(QWheelEvent* event) override;

		void onRightMouseButtonPress(QMouseEvent* event) override;
		void onRightMouseButtonRelease(QMouseEvent* event) override;
		void onRightMouseButtonMove(QMouseEvent* event) override;
		void onRightMouseButtonClick(QMouseEvent* event) override;


		void onKeyPress(QKeyEvent* event) override;
		void onKeyRelease(QKeyEvent* event) override;
	};
}
#endif // QTUSER_3D_SCENEOPERATEMODE_1592203699385_H