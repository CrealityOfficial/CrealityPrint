#include "eventinterface.h"

#include "kernel/kernel.h"
#include "qtuser3d/module/rendercenter.h"
#include "qtuser3d/event/eventsubdivide.h"

using namespace qtuser_3d;

#define IMPL_NULL_CHECK(x)  \
void prepend##x(x* handler){\
									EventSubdivide* divide = getKernel()->renderCenter()->eventSubdivide();\
		if(divide) divide->prepend##x(handler);} \
void add##x(x* handler){\
		EventSubdivide* divide = getKernel()->renderCenter()->eventSubdivide();\
									if(divide) divide->add##x(handler);} \
void remove##x(x* handler) { \
		EventSubdivide* divide = getKernel()->renderCenter()->eventSubdivide(); \
		if (divide) divide->remove##x(handler); }\
void close##x##s(x* handler) { \
		EventSubdivide* divide = getKernel()->renderCenter()->eventSubdivide(); \
		if (divide) divide->close##x##s(); }

namespace creative_kernel
{						
	IMPL_NULL_CHECK(ResizeEventHandler)
	IMPL_NULL_CHECK(HoverEventHandler)
	IMPL_NULL_CHECK(WheelEventHandler)
	IMPL_NULL_CHECK(RightMouseEventHandler)
	IMPL_NULL_CHECK(MidMouseEventHandler)
	IMPL_NULL_CHECK(LeftMouseEventHandler)
	IMPL_NULL_CHECK(KeyEventHandler)
}
