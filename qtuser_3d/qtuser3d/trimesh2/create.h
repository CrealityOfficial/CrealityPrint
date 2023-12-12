#ifndef QT3D_MMESH_CREATECUBE_1619866821989_H
#define QT3D_MMESH_CREATECUBE_1619866821989_H
#include "qtuser3d/qtuser3dexport.h"
#include "trimesh2/TriMesh.h"

namespace qtuser_3d
{
	QTUSER_3D_API trimesh::TriMesh* createCube(const trimesh::box3& box, float gap = 10.0);

	QTUSER_3D_API void boxLineIndices(const trimesh::box3& box, std::vector<trimesh::vec3>& corners, std::vector<int>& indices);
	QTUSER_3D_API void boxLines(const trimesh::box3& box, std::vector<trimesh::vec3>& lines);

	QTUSER_3D_API void offsetPoints(std::vector<trimesh::vec3>& points, const trimesh::vec3& offset);
}

#endif // MMESH_CREATECUBE_1619866821989_H