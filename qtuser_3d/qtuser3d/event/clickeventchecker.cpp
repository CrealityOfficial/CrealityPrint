#include "clickeventchecker.h"
#include <qdebug.h>

namespace qtuser_3d
{
	ClickEventChecker::ClickEventChecker(QObject* parent)
		:QObject(parent)
		, m_xyDelta(50)
		, m_timeDelta(2000)
	{
		m_buttons = Qt::NoButton;
		m_pickX = -1;
		m_pickY = -1;
		m_pickTime = 0;
	}

	ClickEventChecker::~ClickEventChecker()
	{
	}

	bool ClickEventChecker::CheckLeftButton(QMouseEvent* event)
	{
		if (!NullCheck()) return false;
		if (event->button() != Qt::LeftButton) return false;
		return ReleaseCheck(event);
	}

	bool ClickEventChecker::CheckRightButton(QMouseEvent* event)
	{
		if (!NullCheck()) return false;
		if (event->button() != Qt::RightButton) return false;
		return ReleaseCheck(event);
	}

	bool ClickEventChecker::CheckMiddleButton(QMouseEvent* event)
	{
		if (!NullCheck()) return false;
		if (event->button() != Qt::MiddleButton) return false;
		return ReleaseCheck(event);
	}

	bool ClickEventChecker::NullCheck()
	{
		if ((m_pickX >= 0) && (m_pickY >= 0) && (m_pickTime > 0) && (m_buttons > 0))
			return true;
		return false;
	}

	bool ClickEventChecker::ReleaseCheck(QMouseEvent* event)
	{
		int x = event->x();
		int y = event->y();
		ulong t = event->timestamp();

		int dx = abs(x - m_pickX);
		int dy = abs(y - m_pickY);
		ulong dt = abs((int)(t - m_pickTime));

		if (dx <= m_xyDelta && dy <= m_xyDelta && dt < m_timeDelta)
			return true;

		return false;
	}

	void ClickEventChecker::mousePressEvent(QMouseEvent* event)
	{
		m_pickX = event->x();
		m_pickY = event->y();
		m_pickTime = event->timestamp();
		m_buttons = event->buttons();
	}

	void ClickEventChecker::mouseMoveEvent(QMouseEvent* event)
	{
		//m_pickX = -1;
		//m_pickY = -1;
		//m_pickTime = 0;
		//m_buttons = Qt::NoButton;
	}

	void ClickEventChecker::wheelEvent(QWheelEvent* event)
	{
		m_pickX = -1;
		m_pickY = -1;
		m_pickTime = 0;
		m_buttons = Qt::NoButton;
	}

	void ClickEventChecker::SetXYDelta(int xyDelta)
	{
		m_xyDelta = xyDelta;
	}

	void ClickEventChecker::SetTimeDelta(ulong timeDelta)
	{
		m_timeDelta = timeDelta;
	}
}
