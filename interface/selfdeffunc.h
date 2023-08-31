#ifndef _NULLSPACE_SELFDEFFUNC_1591791524731_H
#define _NULLSPACE_SELFDEFFUNC_1591791524731_H
#include <functional>

typedef std::function<void(float)> selfProgressFunc;
typedef std::function<bool()> selfInterruptFunc;
typedef std::function<void(const char*)> selfFailedFunc;

#endif // _NULLSPACE_SELFDEFFUNC_1591791524731_H
