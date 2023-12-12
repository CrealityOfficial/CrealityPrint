#ifndef QTUSER_3D_GLCOMPATIBILITY_1654748123930_H
#define QTUSER_3D_GLCOMPATIBILITY_1654748123930_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtCore/QCoreApplication>

namespace qtuser_3d
{
	QTUSER_3D_API bool isGles();
	QTUSER_3D_API bool isSoftwareGL();
	QTUSER_3D_API bool checkMRTSupported();

	QTUSER_3D_API void checkVendor();
}

#endif // QTUSER_CORE_GLCOMPATIBILITY_1654748123930_H