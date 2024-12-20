#ifndef QTUSER_3D_PRIMITIVESHAPECACHE_1594861761536_H
#define QTUSER_3D_PRIMITIVESHAPECACHE_1594861761536_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DRender/QGeometry>

namespace qtuser_3d
{
	class QTUSER_3D_API PrimitiveShapeCache : public QObject
	{
		Q_OBJECT
	public:
		static Qt3DRender::QGeometry* create(const QString& name);
	};
}

#define PRIMITIVESHAPE(x) qtuser_3d::PrimitiveShapeCache::create(x)

namespace qtuser_3d
{
	QTUSER_3D_API Qt3DRender::QGeometry* createLinesPrimitive(const QString& name);
	QTUSER_3D_API Qt3DRender::QGeometry* createPointsPrimitive(const QString& name);
	QTUSER_3D_API Qt3DRender::QGeometry* createTrianglesPrimitive(const QString& name);
	QTUSER_3D_API Qt3DRender::QGeometry* createTrianglesPrimitiveDLP(const QString& name);
}
#endif // QTUSER_3D_PRIMITIVESHAPECACHE_1594861761536_H