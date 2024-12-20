#ifndef CRSLICE_CLIPPERUTIL_CONV1_1682319629911_H
#define CRSLICE_CLIPPERUTIL_CONV1_1682319629911_H
#include "nestplacer/concaveplacer.h"
#include "libnest2d/backends/clipper/geometries.hpp"

#define INT2MM(n) (static_cast<double>(n) / 1000.0)
#define MM2INT(n) (static_cast<Clipper3r::cInt>((n) * 1000.0 + 0.5 * (((n) > 0) - ((n) < 0))))

namespace nestplacer
{
	trimesh::vec3 convert(const Clipper3r::IntPoint& point, float z = 0.0f);
	Clipper3r::IntPoint convert(const trimesh::vec3& point);

	void convertRaw(const Clipper3r::Paths& paths, std::vector<std::vector<trimesh::vec3>>& lines, float z = 0.0f);
	void convertRaw(const Clipper3r::Path& path, std::vector<trimesh::vec3>& lines, float z = 0.0f);

	void convertPolygon(const libnest2d::PolygonImpl& poly, DebugPolygon& dPoly);
	void convertPolygons(const std::vector<libnest2d::PolygonImpl>& polys,
		std::vector<DebugPolygon>& dPolys);

	libnest2d::_Box<libnest2d::PointImpl> convert(const trimesh::box& b);
}

#endif // CRSLICE_CLIPPERUTIL_CONV1_1682319629911_H