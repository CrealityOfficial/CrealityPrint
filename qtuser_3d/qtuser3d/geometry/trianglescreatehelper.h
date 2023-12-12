#ifndef QTUSER_3D_TRIANGLESCREATEHELPER_1594889735714_H
#define QTUSER_3D_TRIANGLESCREATEHELPER_1594889735714_H
#include "qtuser3d/geometry/geometrycreatehelper.h"

namespace qtuser_3d
{
	class QTUSER_3D_API TrianglesCreateHelper : public GeometryCreateHelper
	{
		Q_OBJECT
	public:
		TrianglesCreateHelper(QObject* parent = nullptr);
		virtual ~TrianglesCreateHelper();

		static Qt3DRender::QGeometry* create(int vertexNum, float* position, float* normals, float* colors, int triangleNum, unsigned* indices
			, Qt3DCore::QNode* parent = nullptr);

		static Qt3DRender::QGeometry* createWithTex(int vertexNum, float* position, float* normals, float* texcoords, int triangleNum, unsigned* indices
			, Qt3DCore::QNode* parent = nullptr);

		static Qt3DRender::QGeometry* createFromVertex(int vertexNum, float* position, Qt3DCore::QNode* parent = nullptr);
	};
}
#endif // QTUSER_3D_TRIANGLESCREATEHELPER_1594889735714_H