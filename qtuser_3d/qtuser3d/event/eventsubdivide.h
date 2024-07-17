#ifndef _qtuser_3d_EVENTSUBDIVIDE_1588737572858_H
#define _qtuser_3d_EVENTSUBDIVIDE_1588737572858_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtCore/QList>
#include <QtGui/QMouseEvent>
#include <QtGui/QHoverEvent>
#include <QtGui/QWheelEvent>
#include <QTimer>

namespace qtuser_3d
{
	class ClickEventChecker;
	class HoverEventHandler;
	class WheelEventHandler;
	class RightMouseEventHandler;
	class MidMouseEventHandler;
	class LeftMouseEventHandler;
	class ResizeEventHandler;
	class KeyEventHandler;

	class QTUSER_3D_API EventSubdivide: public QObject
	{
	public:
		EventSubdivide(QObject* parent = nullptr);
		virtual ~EventSubdivide();

		void geometryChanged(const QSize& size);
		void mousePressEvent(QMouseEvent* event);
		void mouseMoveEvent(QMouseEvent* event);
		void mouseReleaseEvent(QMouseEvent* event);
		void wheelEvent(QWheelEvent* event);
		void hoverEnterEvent(QHoverEvent* event);
		void hoverMoveEvent(QHoverEvent* event);
		void hoverLeaveEvent(QHoverEvent* event);
		void keyPressEvent(QKeyEvent* event);
		void keyReleaseEvent(QKeyEvent* event);

		void prependResizeEventHandler(ResizeEventHandler* handler);
		void addResizeEventHandler(ResizeEventHandler* handler);
		void removeResizeEventHandler(ResizeEventHandler* handler);
		void closeResizeEventHandlers();

		void prependHoverEventHandler(HoverEventHandler* handler);
		void addHoverEventHandler(HoverEventHandler* handler);
		void removeHoverEventHandler(HoverEventHandler* handler);
		void closeHoverEventHandlers();

		void prependWheelEventHandler(WheelEventHandler* handler);
		void addWheelEventHandler(WheelEventHandler* handler);
		void removeWheelEventHandler(WheelEventHandler* handler);
		void closeWheelEventHandlers();

		void prependRightMouseEventHandler(RightMouseEventHandler* handler);
		void addRightMouseEventHandler(RightMouseEventHandler* handler);
		void removeRightMouseEventHandler(RightMouseEventHandler* handler);
		void closeRightMouseEventHandlers();

		void prependMidMouseEventHandler(MidMouseEventHandler* handler);
		void addMidMouseEventHandler(MidMouseEventHandler* handler);
		void removeMidMouseEventHandler(MidMouseEventHandler* handler);
		void closeMidMouseEventHandlers();

		void prependLeftMouseEventHandler(LeftMouseEventHandler* handler);
		void addLeftMouseEventHandler(LeftMouseEventHandler* handler);
		void removeLeftMouseEventHandler(LeftMouseEventHandler* handler);
		void closeLeftMouseEventHandlers();

		void prependKeyEventHandler(KeyEventHandler* handler);
		void addKeyEventHandler(KeyEventHandler* handler);
		void removeKeyEventHandler(KeyEventHandler* handler);
		void closeKeyEventHandlers();

		void closeHandlers();
	protected:
		ClickEventChecker* m_clickEventChecker;

		QList<ResizeEventHandler*> m_resizeEventHandlers;
		QList<HoverEventHandler*> m_hoverEventHandlers;
		QList<WheelEventHandler*> m_wheelEventHandlers;
		QList<RightMouseEventHandler*> m_rightMouseEventHandlers;
		QList<MidMouseEventHandler*> m_midMouseEventHandlers;
		QList<LeftMouseEventHandler*> m_leftMouseEventHandlers;
		QList<KeyEventHandler*> m_KeyEventHandlers;
		QSize m_size;

		QTimer m_releaseTimer;

        bool m_bLastMove { false };
		bool m_spaceModifier { false };

	};
}
#endif // _qtuser_3d_EVENTSUBDIVIDE_1588737572858_H
