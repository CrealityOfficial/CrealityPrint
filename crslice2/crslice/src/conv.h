#ifndef CRSLICE_CLIPPERUTIL_CONV1_1682319629911_H
#define CRSLICE_CLIPPERUTIL_CONV1_1682319629911_H
#include "libslic3r/ExPolygon.hpp"
#include "ccglobal/debugger.h"

namespace crslice2
{
	trimesh::vec3 convert(const Slic3r::Point& point, float z = 0.0f);
	trimesh::vec3 convert(const Slic3r::Vec3f& point);
	ccglobal::Polygon convert(const Slic3r::Polygon& in, float z = 0.0f);

	struct CovertParam
	{
		trimesh::vec4 out = trimesh::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		trimesh::vec4 in = trimesh::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		float z = 0.0f;
		bool to_lines = true;
		float width = 1.0f;
	};

	void convert(const Slic3r::ExPolygons& expolys, ccglobal::Polygon& lines, std::vector<trimesh::vec4>* colors, const CovertParam& param);
	void convert(const Slic3r::Polygons& polys, ccglobal::Polygon& lines, std::vector<trimesh::vec4>* colors, const CovertParam& param);
	void append(const Slic3r::ExPolygon& expoly, ccglobal::Polygon& lines, std::vector<trimesh::vec4>* colors, const CovertParam& param);
	void append(const Slic3r::Polygon& poly, ccglobal::Polygon& lines, std::vector<trimesh::vec4>* colors, const trimesh::vec4& use_color, float z = 0.0f,
		bool to_lines = true, bool loop = true);
}

#endif // CRSLICE_CLIPPERUTIL_CONV1_1682319629911_H