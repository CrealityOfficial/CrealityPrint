#ifndef MMESH_SAVEPOLYGONSTACK_1614761622645_H
#define MMESH_SAVEPOLYGONSTACK_1614761622645_H
#include "trimesh2/Vec.h"
#include "trimesh2/TriMesh.h"

namespace msbase
{
	void stackSave(const char* name, std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points);
	void stackLoad(const char* name, std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points);
}

#endif // MMESH_SAVEPOLYGONSTACK_1614761622645_H