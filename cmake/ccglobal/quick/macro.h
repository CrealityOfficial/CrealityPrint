#ifndef _CCGLOBAL_MACRO_1588900396612_H
#define _CCGLOBAL_MACRO_1588900396612_H
#include <QtQml/qqml.h>

#define QUICK_INTERFACE(x, scope, major, minor) qmlRegisterType<x>(scope, major, minor, #x)
#define QUICK_INTERFACE_U(x, scope, major, minor, reason)  qmlRegisterUncreatableType<x>(scope, major, minor, #x, reason);

#define QUICK_AUTO_TYPE_2(x, y, scope, major, minor) namespace qml_type {\
						class x##y \
						{                    \
						public:              \
						    x##y()    \
						    {                 \
						        QUICK_INTERFACE(x, scope, major, minor);   \
						    };                 \
						    ~x##y() {};  \
						}; }                        \
						qml_type::x##y x##init;  
#define QUICK_AUTO_TYPE(x, scope, major, minor) QUICK_AUTO_TYPE_2(x, Initialize, scope, major, minor)
#endif // _CCGLOBAL_MACRO_1588900396612_H
