#ifndef QTUSER_3D_TEXTURECREATOR_1620889259012_H
#define QTUSER_3D_TEXTURECREATOR_1620889259012_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DRender/QTexture>

namespace qtuser_3d
{
	QTUSER_3D_API Qt3DRender::QTexture2D* createFromSource(const QUrl& url);
	QTUSER_3D_API Qt3DRender::QTexture2D* createFromImage(const QImage& image);
}

#endif // QTUSER_3D_TEXTURECREATOR_1620889259012_H