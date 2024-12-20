#ifndef _qtuser_3d_EVENTHANDLERS_1588745383269_H
#define _qtuser_3d_EVENTHANDLERS_1588745383269_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QHoverEvent>
#include <QtCore/QSize>

namespace qtuser_3d
{
	class ResizeEventHandler
	{
	public:
		virtual ~ResizeEventHandler() {}

		virtual void onResize(const QSize& size) = 0;

		bool m_resizeStatus = true;
	};

	class HoverEventHandler
	{
	public:
		virtual ~HoverEventHandler() {}

		virtual void onHoverEnter(QHoverEvent* event) = 0;
		virtual void onHoverLeave(QHoverEvent* event) = 0;
		virtual void onHoverMove(QHoverEvent* event) = 0;

		bool m_hoverEnterStatus = true;
		bool m_hoverLeaveStatus = true;
		bool m_hoverMoveStatus = true;
	};

	class WheelEventHandler
	{
	public:
		virtual ~WheelEventHandler() {}

		virtual void onWheelEvent(QWheelEvent* event) = 0;

		bool m_wheelStatus = true;
	};

	class LeftMouseEventHandler
	{
	public:
		virtual ~LeftMouseEventHandler() {}

		virtual void onLeftMouseButtonPress(QMouseEvent* event) = 0;
		virtual void onLeftMouseButtonRelease(QMouseEvent* event) = 0;
		virtual void onLeftMouseButtonMove(QMouseEvent* event) = 0;
		virtual void onLeftMouseButtonClick(QMouseEvent* event) = 0;

		bool m_leftPressStatus = true;
		bool m_leftReleaseStatus = true;
		bool m_leftMoveStatus = true;
		bool m_leftClickStatus = true;
	};

	class MidMouseEventHandler
	{
	public:
		virtual ~MidMouseEventHandler() {}

		virtual void onMidMouseButtonPress(QMouseEvent* event) = 0;
		virtual void onMidMouseButtonRelease(QMouseEvent* event) = 0;
		virtual void onMidMouseButtonMove(QMouseEvent* event) = 0;
		virtual void onMidMouseButtonClick(QMouseEvent* event) = 0;

		bool m_midPressStatus = true;
		bool m_midReleaseStatus = true;
		bool m_midMoveStatus = true;
		bool m_midClickStatus = true;
	};

	class RightMouseEventHandler
	{
	public:
		virtual ~RightMouseEventHandler() {}

		virtual void onRightMouseButtonPress(QMouseEvent* event) = 0;
		virtual void onRightMouseButtonRelease(QMouseEvent* event) = 0;
		virtual void onRightMouseButtonMove(QMouseEvent* event) = 0;
		virtual void onRightMouseButtonClick(QMouseEvent* event) = 0;

		bool m_rightPressStatus = true;
		bool m_rightReleaseStatus = true;
		bool m_rightMoveStatus = true;
		bool m_rightClickStatus = true;
	};

	class EventLogic : public HoverEventHandler
		, public WheelEventHandler
		, public RightMouseEventHandler
		, public MidMouseEventHandler
		, public LeftMouseEventHandler
		, public ResizeEventHandler
	{
	public:
		virtual ~EventLogic() {}
	};

	class KeyEventHandler
	{
	public:
		virtual ~KeyEventHandler() {}
		virtual void onKeyPress(QKeyEvent* event) = 0;
		virtual void onKeyRelease(QKeyEvent* event) = 0;

		bool m_keyPressStatus = true;
		bool m_keyRelaseStatus = true;
	};
	
	class QTUSER_3D_API EventCheckHandler
	{
	public:
		EventCheckHandler();
		virtual ~EventCheckHandler();

		virtual bool hoverMove(QHoverEvent* event);
		virtual bool leftMouseButtonPress(QMouseEvent* event);
		virtual void leftMouseButtonRelease(QMouseEvent* event);
		virtual void leftMouseButtonMove(QMouseEvent* event);
		virtual void leftMouseButtonClick(QMouseEvent* event);
		virtual bool midMouseButtonPress(QMouseEvent* event);
		virtual void midMouseButtonRelease(QMouseEvent* event);
		virtual void midMouseButtonMove(QMouseEvent* event);
		virtual void midMouseButtonClick(QMouseEvent* event);
		virtual bool rightMouseButtonPress(QMouseEvent* event);
		virtual void rightMouseButtonRelease(QMouseEvent* event);
		virtual void rightMouseButtonMove(QMouseEvent* event);
		virtual void rightMouseButtonClick(QMouseEvent* event);
		virtual bool keyPress(QKeyEvent* event);
		virtual void keyRelease(QKeyEvent* event);
		virtual bool wheelEvent(QWheelEvent* event);
	};
}
#endif // _qtuser_3d_EVENTHANDLERS_1588745383269_H
