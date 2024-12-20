#ifndef CREATIVE_KERNEL_EVENTINTERFACE_1594436854496_H
#define CREATIVE_KERNEL_EVENTINTERFACE_1594436854496_H
#include "basickernelexport.h"
#include "qtuser3d/event/eventhandlers.h"

namespace creative_kernel
{
	BASIC_KERNEL_API void prependResizeEventHandler(qtuser_3d::ResizeEventHandler* handler);
	BASIC_KERNEL_API void addResizeEventHandler(qtuser_3d::ResizeEventHandler* handler);
	BASIC_KERNEL_API void removeREsizeEventHandler(qtuser_3d::ResizeEventHandler* handler);
	BASIC_KERNEL_API void closeResizeEventHandlers();

	BASIC_KERNEL_API void prependHoverEventHandler(qtuser_3d::HoverEventHandler* handler);
	BASIC_KERNEL_API void addHoverEventHandler(qtuser_3d::HoverEventHandler* handler);
	BASIC_KERNEL_API void removeHoverEventHandler(qtuser_3d::HoverEventHandler* handler);
	BASIC_KERNEL_API void closeHoverEventHandlers();

	BASIC_KERNEL_API void prependWheelEventHandler(qtuser_3d::WheelEventHandler* handler);
	BASIC_KERNEL_API void addWheelEventHandler(qtuser_3d::WheelEventHandler* handler);
	BASIC_KERNEL_API void removeWheelEventHandler(qtuser_3d::WheelEventHandler* handler);
	BASIC_KERNEL_API void closeWheelEventHandlers();
	
	BASIC_KERNEL_API void prependRightMouseEventHandler(qtuser_3d::RightMouseEventHandler* handler);
	BASIC_KERNEL_API void addRightMouseEventHandler(qtuser_3d::RightMouseEventHandler* handler);
	BASIC_KERNEL_API void removeRightMouseEventHandler(qtuser_3d::RightMouseEventHandler* handler);
	BASIC_KERNEL_API void closeRightMouseEventHandlers();

	BASIC_KERNEL_API void prependMidMouseEventHandler(qtuser_3d::MidMouseEventHandler* handler);
	BASIC_KERNEL_API void addMidMouseEventHandler(qtuser_3d::MidMouseEventHandler* handler);
	BASIC_KERNEL_API void removeMidMouseEventHandler(qtuser_3d::MidMouseEventHandler* handler);
	BASIC_KERNEL_API void closeMidMouseEventHandlers();

	BASIC_KERNEL_API void prependLeftMouseEventHandler(qtuser_3d::LeftMouseEventHandler* handler);
	BASIC_KERNEL_API void addLeftMouseEventHandler(qtuser_3d::LeftMouseEventHandler* handler);
	BASIC_KERNEL_API void removeLeftMouseEventHandler(qtuser_3d::LeftMouseEventHandler* handler);
	BASIC_KERNEL_API void closeLeftMouseEventHandlers();

	BASIC_KERNEL_API void prependKeyEventHandler(qtuser_3d::KeyEventHandler* handler);
	BASIC_KERNEL_API void addKeyEventHandler(qtuser_3d::KeyEventHandler* handler);
	BASIC_KERNEL_API void removeKeyEventHandler(qtuser_3d::KeyEventHandler* handler);
	BASIC_KERNEL_API void closeKeyEventHandlers();

}
#endif // CREATIVE_KERNEL_EVENTINTERFACE_1594436854496_H
