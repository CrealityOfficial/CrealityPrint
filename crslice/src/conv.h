#ifndef CRSLICE_CLIPPERUTIL_CONV1_1682319629911_H
#define CRSLICE_CLIPPERUTIL_CONV1_1682319629911_H
#include "trimesh2/Vec.h"
#include "polyclipping/clipper.hpp"
#include "utils/polygon.h"
#include "utils/Point3.h"

namespace crslice
{
	void convert(const ClipperLib::Paths& paths, std::vector<trimesh::vec3>& lines,
		float z = 0.0f, bool loop = true);

	void convert(const ClipperLib::Path& path, std::vector<trimesh::vec3>& lines,
		float z = 0.0f, bool loop = true, bool append = false);

	trimesh::vec3 convert(const ClipperLib::IntPoint& point, float z = 0.0f);
	trimesh::vec3 convert(const cura52::Point3& point);

	trimesh::vec3 convertScale(const ClipperLib::IntPoint& point, float z = 0.0f);

	ClipperLib::IntPoint convert(const trimesh::vec3& point);
	ClipperLib::IntPoint convertScale(const trimesh::vec3& point);

	void convertRaw(const ClipperLib::Paths& paths, std::vector<std::vector<trimesh::vec3>>& lines, float z = 0.0f);
	void convertRaw(const ClipperLib::Path& path, std::vector<trimesh::vec3>& lines, float z = 0.0f);
	void convertPolygonRaw(const cura52::Polygons& polys, std::vector<std::vector<trimesh::vec3>>& lines, float z = 0.0f);

	void convertRaw(const std::vector<std::vector<trimesh::vec3>>& lines, ClipperLib::Paths& paths, bool scaleNm = false);
	void convertRaw(const std::vector<trimesh::vec3>& lines, ClipperLib::Path& path, bool scaleNm = false);
	void convertPolygonRaw(const std::vector<std::vector<trimesh::vec3>>& lines, cura52::Polygons& polys, bool scaleNm = false);
}

#endif // CRSLICE_CLIPPERUTIL_CONV1_1682319629911_H