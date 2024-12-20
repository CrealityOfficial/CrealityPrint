#ifndef QTUSER_3D_GRIDCREATEHELPER_1595036334895_H
#define QTUSER_3D_GRIDCREATEHELPER_1595036334895_H
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "qtuser3d/math/box3d.h"

namespace qtuser_3d
{
	class QTUSER_3D_API GridCreateHelper : public GeometryCreateHelper
	{
		Q_OBJECT
	public:
		GridCreateHelper(QObject* parent = nullptr);
		virtual ~GridCreateHelper();

		static Qt3DRender::QGeometry* create(Box3D& box, float gap = 10.0f, Qt3DCore::QNode * parent = nullptr);   //lines
		static Qt3DRender::QGeometry* createMid(Box3D& box, float gap = 10.0f, float offset = 5.0f, Qt3DCore::QNode* parent = nullptr);

		static Qt3DRender::QGeometry* createPlane(float width, float height, bool triangle = true, Qt3DCore::QNode* parent = nullptr);   //triangles
		static Qt3DRender::QGeometry* createTextureQuad(Box3D& box, bool flatZ, Qt3DCore::QNode* parent = nullptr);
	};
}
#endif // QTUSER_3D_GRIDCREATEHELPER_1595036334895_H