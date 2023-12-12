#ifndef _qtuser_3d_CLICKEVENTCHECKER_1588738592787_H
#define _qtuser_3d_CLICKEVENTCHECKER_1588738592787_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

namespace qtuser_3d
{
	class QTUSER_3D_API ClickEventChecker: public QObject
	{
	public:
		ClickEventChecker(QObject* parent = nullptr);
		virtual ~ClickEventChecker();

		bool CheckLeftButton(QMouseEvent* event);
		bool CheckRightButton(QMouseEvent* event);
		bool CheckMiddleButton(QMouseEvent* event);

		void mousePressEvent(QMouseEvent* event);
		void mouseMoveEvent(QMouseEvent* event);
		void wheelEvent(QWheelEvent* event);

		void SetXYDelta(int xyDelta);
		void SetTimeDelta(ulong timeDelta);
	private:
		bool NullCheck();
		bool ReleaseCheck(QMouseEvent* event);
	protected:
		Qt::MouseButtons m_buttons;
		int m_pickX;
		int m_pickY;
		ulong m_pickTime;

		int m_xyDelta;
        ulong m_timeDelta;
	};
}
#endif // _qtuser_3d_CLICKEVENTCHECKER_1588738592787_H
