#pragma once

#include "trimesh2/Vec.h"
#include "trimesh2/TriMesh.h"

namespace CUT
{
	void stackSave(const char* name, std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points);
	void stackLoad(const char* name, std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points);
}
