#include "qtuser3d/event/eventsubdivide.h"
#include "qtuser3d/event/eventhandlers.h"
#include "qtuser3d/event/clickeventchecker.h"
#include <qdebug.h>

namespace qtuser_3d
{
	EventSubdivide::EventSubdivide(QObject* parent)
		:QObject(parent)
	{
		m_clickEventChecker = new ClickEventChecker(this);
	}

	EventSubdivide::~EventSubdivide()
	{
	}

	void EventSubdivide::geometryChanged(const QSize& size)
	{
		m_size = size;
		for (ResizeEventHandler* handler : m_resizeEventHandlers)
		{
			handler->onResize(size);
			if (!handler->m_resizeStatus)
			{
				handler->m_resizeStatus = true;
				break;
			}
		}
	}

	void EventSubdivide::mousePressEvent(QMouseEvent* event)
	{
		if ((event->button() == Qt::LeftButton))
		{
			for (LeftMouseEventHandler* handler : m_leftMouseEventHandlers)
			{
				handler->onLeftMouseButtonPress(event);
				if (!handler->m_leftPressStatus)
				{
					handler->m_leftPressStatus = true;
					break;
				}
			}
		}
		if ((event->button() == Qt::MiddleButton))
		{
			for (MidMouseEventHandler* handler : m_midMouseEventHandlers)
			{
				handler->onMidMouseButtonPress(event);
				if (!handler->m_midPressStatus)
				{
					handler->m_midPressStatus = true;
					break;
				}
			}
		}
		if ((event->button() == Qt::RightButton))
		{
			for (RightMouseEventHandler* handler : m_rightMouseEventHandlers)
			{
				handler->onRightMouseButtonPress(event);
				if (!handler->m_rightPressStatus)
				{
					handler->m_rightPressStatus = true;
					break;
				}
			}
            m_bLastMove = false;
		}

		m_clickEventChecker->mousePressEvent(event);
	}

	void EventSubdivide::mouseMoveEvent(QMouseEvent* event)
	{
		if ((event->buttons() == Qt::RightButton))
		{
			for (RightMouseEventHandler* handler : m_rightMouseEventHandlers)
			{
				handler->onRightMouseButtonMove(event);
				if (!handler->m_rightMoveStatus)
				{
					handler->m_rightMoveStatus = true;
					break;
				}
			}
            m_bLastMove = true;
		}
		if ((event->buttons() == Qt::MiddleButton))
		{
			for (MidMouseEventHandler* handler : m_midMouseEventHandlers)
			{
				handler->onMidMouseButtonMove(event);
				if (!handler->m_midMoveStatus)
				{
					handler->m_midMoveStatus = true;
					break;
				}
			}
		}
		if ((event->buttons() == Qt::LeftButton))
		{
			for (LeftMouseEventHandler* handler : m_leftMouseEventHandlers)
			{
				handler->onLeftMouseButtonMove(event);
				if (!handler->m_leftMoveStatus)
				{
					handler->m_leftMoveStatus = true;
					break;
				}
			}
		}

		m_clickEventChecker->mouseMoveEvent(event);
	}

	void EventSubdivide::mouseReleaseEvent(QMouseEvent* event)
	{
		if ((event->button() == Qt::RightButton))
		{
			for (RightMouseEventHandler* handler : m_rightMouseEventHandlers)
			{
				handler->onRightMouseButtonRelease(event);
				if (!handler->m_rightReleaseStatus)
				{
					handler->m_rightReleaseStatus = true;
					break;
				}
			}
		}
		if ((event->button() == Qt::MiddleButton))
		{
			for (MidMouseEventHandler* handler : m_midMouseEventHandlers)
			{
				handler->onMidMouseButtonRelease(event);
				if (!handler->m_midReleaseStatus)
				{
					handler->m_midReleaseStatus = true;
					break;
				}
			}
		}
		if ((event->button() == Qt::LeftButton))
		{
			for (LeftMouseEventHandler* handler : m_leftMouseEventHandlers)
			{
				handler->onLeftMouseButtonRelease(event);
				if (!handler->m_leftReleaseStatus)
				{
					handler->m_leftReleaseStatus = true;
					break;
				}
			}
		}

		if (m_clickEventChecker->CheckRightButton(event))
		{
            if(m_bLastMove)return;
			for (RightMouseEventHandler* handler : m_rightMouseEventHandlers)
			{
				handler->onRightMouseButtonClick(event);
				if (!handler->m_rightClickStatus)
				{
					handler->m_rightClickStatus = true;
					break;
				}
			}
		}
		if (m_clickEventChecker->CheckMiddleButton(event))
		{
			for (MidMouseEventHandler* handler : m_midMouseEventHandlers)
			{
				handler->onMidMouseButtonClick(event);
				if (!handler->m_midClickStatus)
				{
					handler->m_midClickStatus = true;
					break;
				}
			}
		}
		if (m_clickEventChecker->CheckLeftButton(event))
		{
			for (LeftMouseEventHandler* handler : m_leftMouseEventHandlers)
			{
				handler->onLeftMouseButtonClick(event);
				if (!handler->m_leftClickStatus)
				{
					handler->m_leftClickStatus = true;
					break;
				}
			}
		}
	}

	void EventSubdivide::wheelEvent(QWheelEvent* event)
	{
		for (WheelEventHandler* handler : m_wheelEventHandlers)
		{
			handler->onWheelEvent(event);
			if (!handler->m_wheelStatus)
			{
				handler->m_wheelStatus = true;
				break;
			}
		}
	}

	void EventSubdivide::hoverEnterEvent(QHoverEvent* event)
	{
		for (HoverEventHandler* handler : m_hoverEventHandlers)
		{
			handler->onHoverEnter(event);
			if (!handler->m_hoverEnterStatus)
			{
				handler->m_hoverEnterStatus = true;
				break;
			}
		}
	}

	void EventSubdivide::hoverMoveEvent(QHoverEvent* event)
	{
		for (HoverEventHandler* handler : m_hoverEventHandlers)
		{
			handler->onHoverMove(event);
			if (!handler->m_hoverMoveStatus)
			{
				handler->m_hoverMoveStatus = true;
				break;
			}
		}
	}

	void EventSubdivide::hoverLeaveEvent(QHoverEvent* event)
	{
		for (HoverEventHandler* handler : m_hoverEventHandlers)
		{
			handler->onHoverLeave(event);
			if (!handler->m_hoverLeaveStatus)
			{
				handler->m_hoverLeaveStatus = true;
				break;
			}
		}
	}

	void EventSubdivide::keyPressEvent(QKeyEvent* event)
	{
		for (KeyEventHandler* handler : m_KeyEventHandlers)
		{
			handler->onKeyPress(event);
			{
				if (!handler->m_keyPressStatus)
				{
					handler->m_keyPressStatus = true;
					break;
				}
			}
		}
	}

	void EventSubdivide::keyReleaseEvent(QKeyEvent* event)
	{
		for (KeyEventHandler* handler : m_KeyEventHandlers)
		{
			handler->onKeyRelease(event);
			if (!handler->m_keyRelaseStatus)
			{
				handler->m_keyRelaseStatus = true;
				break;
			}
		}
	}

	void EventSubdivide::closeHandlers()
	{
		closeLeftMouseEventHandlers();
		closeRightMouseEventHandlers();
		closeWheelEventHandlers();
		closeHoverEventHandlers();
		closeResizeEventHandlers();
		closeKeyEventHandlers();
	}

// ResizeEventHandler
	void EventSubdivide::prependResizeEventHandler(ResizeEventHandler* handler)
	{
		if (handler)
		{
			handler->onResize(m_size);
			m_resizeEventHandlers.prepend(handler);
		}
	}

	void EventSubdivide::addResizeEventHandler(ResizeEventHandler* handler)
	{
		if (handler)
		{
			handler->onResize(m_size);
			m_resizeEventHandlers.push_back(handler);
		}
	}

	void EventSubdivide::removeResizeEventHandler(ResizeEventHandler* handler)
	{
		m_resizeEventHandlers.removeOne(handler);
	}

	void EventSubdivide::closeResizeEventHandlers()
	{
		m_resizeEventHandlers.clear();
	}

// HoverEventHandler
	void EventSubdivide::prependHoverEventHandler(HoverEventHandler* handler)
	{
		if (handler) m_hoverEventHandlers.prepend(handler);
	}

	void EventSubdivide::addHoverEventHandler(HoverEventHandler* handler)
	{
		if (handler) m_hoverEventHandlers.push_back(handler);
	}

	void EventSubdivide::removeHoverEventHandler(HoverEventHandler* handler)
	{
		m_hoverEventHandlers.removeOne(handler);
	}

	void EventSubdivide::closeHoverEventHandlers()
	{
		m_hoverEventHandlers.clear();
	}

// WheelEventHandler
	void EventSubdivide::prependWheelEventHandler(WheelEventHandler* handler)
	{
		if (handler) m_wheelEventHandlers.prepend(handler);
	}

	void EventSubdivide::addWheelEventHandler(WheelEventHandler* handler)
	{
		if (handler) m_wheelEventHandlers.push_back(handler);
	}

	void EventSubdivide::removeWheelEventHandler(WheelEventHandler* handler)
	{
		m_wheelEventHandlers.removeOne(handler);
	}

	void EventSubdivide::closeWheelEventHandlers()
	{
		m_wheelEventHandlers.clear();
	}

// RightMouseEventHandler
	void EventSubdivide::prependRightMouseEventHandler(RightMouseEventHandler* handler)
	{
		if (handler) m_rightMouseEventHandlers.prepend(handler);
	}

	void EventSubdivide::addRightMouseEventHandler(RightMouseEventHandler* handler)
	{
		if (handler) m_rightMouseEventHandlers.push_back(handler);
	}

	void EventSubdivide::removeRightMouseEventHandler(RightMouseEventHandler* handler)
	{
		m_rightMouseEventHandlers.removeOne(handler);
	}

	void EventSubdivide::closeRightMouseEventHandlers()
	{
		m_rightMouseEventHandlers.clear();
	}

// MidMouseEventHandler
	void EventSubdivide::prependMidMouseEventHandler(MidMouseEventHandler* handler)
	{
		if (handler) m_midMouseEventHandlers.prepend(handler);
	}

	void EventSubdivide::addMidMouseEventHandler(MidMouseEventHandler* handler)
	{
		if (handler) m_midMouseEventHandlers.push_back(handler);
	}

	void EventSubdivide::removeMidMouseEventHandler(MidMouseEventHandler* handler)
	{
		m_midMouseEventHandlers.removeOne(handler);
	}

	void EventSubdivide::closeMidMouseEventHandlers()
	{
		m_midMouseEventHandlers.clear();
	}

// LeftMouseEventHandler
	void EventSubdivide::prependLeftMouseEventHandler(LeftMouseEventHandler* handler)
	{
		if (handler) m_leftMouseEventHandlers.prepend(handler);
	}

	void EventSubdivide::addLeftMouseEventHandler(LeftMouseEventHandler* handler)
	{
		if (handler) m_leftMouseEventHandlers.push_back(handler);
	}

	void EventSubdivide::removeLeftMouseEventHandler(LeftMouseEventHandler* handler)
	{
		m_leftMouseEventHandlers.removeOne(handler);
	}

	void EventSubdivide::closeLeftMouseEventHandlers()
	{
		m_leftMouseEventHandlers.clear();
	}

// KeyEventHandler
	void EventSubdivide::prependKeyEventHandler(KeyEventHandler* handler)
	{
		if (handler) m_KeyEventHandlers.prepend(handler);
	}

	void EventSubdivide::addKeyEventHandler(KeyEventHandler* handler)
	{
		if (handler) m_KeyEventHandlers.push_back(handler);
	}

	void EventSubdivide::removeKeyEventHandler(KeyEventHandler* handler)
	{
		m_KeyEventHandlers.removeOne(handler);
	}

	void EventSubdivide::closeKeyEventHandlers()
	{
		m_KeyEventHandlers.clear();
	}
}
