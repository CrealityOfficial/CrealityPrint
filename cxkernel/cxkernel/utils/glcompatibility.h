#ifndef CXKERNEL_QTUSER_CORE_GLCOMPATIBILITY_1654748123930_H
#define CXKERNEL_QTUSER_CORE_GLCOMPATIBILITY_1654748123930_H
#include "cxkernel/cxkernelinterface.h"
#include <QtCore/QCoreApplication>

namespace cxkernel
{
	void setBeforeApplication();
	void setDefaultAfterApp();

	CXKERNEL_API bool isOpenGLVaild();
	CXKERNEL_API bool isGles();
	CXKERNEL_API bool isSoftwareGL();
}

#endif // QTUSER_CORE_GLCOMPATIBILITY_1654748123930_H