#ifndef CLIPPERUTIL_TOPOMESH_CONV1_1682319629911_H
#define CLIPPERUTIL_TOPOMESH_CONV1_1682319629911_H
#include "trimesh2/Vec.h"
#include "clipper/clipper.hpp"

namespace topomesh
{
	trimesh::vec3 convert(const ClipperLib::IntPoint& point, float z = 0.0f);
	ClipperLib::IntPoint convert(const trimesh::vec3& point);

	void convertRaw(const ClipperLib::Paths& paths, std::vector<std::vector<trimesh::vec3>>& lines, float z = 0.0f);
	void convertRaw(const ClipperLib::Path& path, std::vector<trimesh::vec3>& lines, float z = 0.0f);

	void convertRaw(const std::vector<std::vector<trimesh::vec3>>& lines, ClipperLib::Paths& paths);
	void convertRaw(const std::vector<trimesh::vec3>& lines, ClipperLib::Path& path);
}

#endif // CLIPPERUTIL_TOPOMESH_CONV1_1682319629911_H