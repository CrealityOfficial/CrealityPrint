#pragma once
#include <vector>
#include "trimesh2/TriMesh.h"

namespace topomesh {
	bool calculateCrossPoint4Dim2(const std::pair<trimesh::vec2,trimesh::vec2>& line1,const std::pair<trimesh::vec2,trimesh::vec2>& line2,trimesh::vec2& result);
}