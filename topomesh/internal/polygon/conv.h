#ifndef CLIPPERUTIL_TOPOMESH_CONV_1682319629911_H
#define CLIPPERUTIL_TOPOMESH_CONV_1682319629911_H
#include "trimesh2/Vec.h"
#include "clipperxyz/clipper.hpp"

namespace topomesh
{
	void convertRaw(const ClipperLibXYZ::Paths& paths, std::vector<std::vector<trimesh::vec3>>& lines);
	void convertRaw(const ClipperLibXYZ::Path& path, std::vector<trimesh::vec3>& lines);
}

#endif // CLIPPERUTIL_TOPOMESH_CONV_1682319629911_H