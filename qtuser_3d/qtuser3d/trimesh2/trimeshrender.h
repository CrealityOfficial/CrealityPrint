#ifndef QTUSER_3D_MMESH_TRIMESHRENDER_1688522129048_H
#define QTUSER_3D_MMESH_TRIMESHRENDER_1688522129048_H
#include "qtuser3d/qtuser3dexport.h"
#include "trimesh2/TriMesh.h"

namespace qtuser_3d
{
	QTUSER_3D_API void traitTriangles(trimesh::TriMesh* mesh, const std::vector<int>& indices, std::vector<trimesh::vec3>& tris);
	QTUSER_3D_API void traitTriangles(trimesh::TriMesh* mesh, std::vector<trimesh::vec3>& tris);
	QTUSER_3D_API void traitTrianglesFromMeshes(const std::vector<trimesh::TriMesh*>& meshes, std::vector<trimesh::vec3>& tris);

	QTUSER_3D_API void loopPolygons2Lines(const std::vector<std::vector<trimesh::vec3>>& polygons, std::vector<trimesh::vec3>& lines, bool loop = true);
	QTUSER_3D_API void loopPolygon2Lines(const std::vector<trimesh::vec3>& polygon, std::vector<trimesh::vec3>& lines, bool loop = true, bool append = false);

	QTUSER_3D_API void traitLines(trimesh::TriMesh* mesh, const std::vector<int>& indices, std::vector<trimesh::vec3>& lines);

	QTUSER_3D_API void box2Lines(const trimesh::box2& box, std::vector<trimesh::vec3>& lines);
	QTUSER_3D_API void box3esLines(const std::vector<trimesh::box3>& boxes, std::vector<trimesh::vec3>& lines);
	QTUSER_3D_API void box3Lines(const trimesh::box3& box, std::vector<trimesh::vec3>::iterator begin);  //reserved
	QTUSER_3D_API void box3Lines(const trimesh::box3& box, std::vector<trimesh::vec3>& lines);
}

#endif // QTUSER_3D_MMESH_TRIMESHRENDER_1688522129048_H