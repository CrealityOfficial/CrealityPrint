#ifndef QCXUTIL_RENDERPRIMITIVE_1681808843406_H
#define QCXUTIL_RENDERPRIMITIVE_1681808843406_H
#include "qtuser3d/qtuser3dexport.h"
#include "trimesh2/Box.h"
#include <vector>
#include <Qt3DRender/QGeometry>

namespace qtuser_3d
{
	QTUSER_3D_API Qt3DRender::QGeometry* createLinesGeometry(trimesh::vec3* lines, int num);
	QTUSER_3D_API Qt3DRender::QGeometry* createIndicesGeometry(trimesh::vec3* positions, int num, int* indices, int tnum);

	QTUSER_3D_API Qt3DRender::QGeometry* createLinesGeometry(const std::vector<trimesh::vec3>& lines);
	QTUSER_3D_API Qt3DRender::QGeometry* createColorLinesGeometry(const std::vector<trimesh::vec3>& lines, const std::vector<trimesh::vec4>& colors);

	struct CubeParameter
	{
		bool parts = false;
		float ratio = 1.0f;
	};

	QTUSER_3D_API Qt3DRender::QGeometry* createCubeLines(const trimesh::box3& box, const CubeParameter& parameter = CubeParameter());
	QTUSER_3D_API Qt3DRender::QGeometry* createCubeTriangles(const trimesh::box3& box);

	struct GridParameter
	{
		float delta = 20.0f;
		int xNum = 10;
		int yNum = 10;
		trimesh::vec3 offset = trimesh::vec3(0.0f, 0.0f, 0.0f);
	};

	QTUSER_3D_API Qt3DRender::QGeometry* createGridLines(const GridParameter& parameter = GridParameter());
	QTUSER_3D_API Qt3DRender::QGeometry* createMidGridLines(const trimesh::box3& box, float gap = 10.0f, float offset = 5.0f);

	QTUSER_3D_API Qt3DRender::QGeometry* createIdentityTriangle();
	QTUSER_3D_API Qt3DRender::QGeometry* createSimpleTriangle(const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3);

	QTUSER_3D_API Qt3DRender::QGeometry* createSimpleQuad();
	QTUSER_3D_API Qt3DRender::QGeometry* createTriangles(const std::vector<trimesh::vec3>& tris);
	QTUSER_3D_API Qt3DRender::QGeometry* createTrianglesWithNormals(const std::vector<trimesh::vec3>& tris, const std::vector<trimesh::vec3>& normals);

	QTUSER_3D_API Qt3DRender::QGeometry* createTrianglesWithColors(const std::vector<trimesh::vec3>& tris, const std::vector<trimesh::vec3>& colors);

	QTUSER_3D_API Qt3DRender::QGeometry* createTrianglesWithFlags(const std::vector<trimesh::vec3>& tris, const std::vector<float>& flags);

	QTUSER_3D_API Qt3DRender::QGeometry* createPoints(const std::vector<trimesh::vec3>& points);
}

#endif // QCXUTIL_RENDERPRIMITIVE_1681808843406_H