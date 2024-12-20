#include "conv.h"

namespace crslice2
{
	trimesh::vec3 convert(const Slic3r::Point& point, float z)
	{
		return trimesh::vec3(unscale_(point.x()), unscale_(point.y()), z);
	}

	trimesh::vec3 convert(const Slic3r::Vec3f& point)
	{
		return trimesh::vec3(point.x(), point.y(), point.z());
	}

	ccglobal::Polygon convert(const Slic3r::Polygon& in, float z)
	{
		ccglobal::Polygon poly;
		for (const Slic3r::Point& point : in.points)
			poly.emplace_back(convert(point, z));
		return poly;
	}

	void convert(const Slic3r::ExPolygons& expolys, ccglobal::Polygon& lines, std::vector<trimesh::vec4>* colors, const CovertParam& param)
	{
		lines.clear();
		if(colors)
			colors->clear();
		for (const Slic3r::ExPolygon& poly : expolys)
			append(poly, lines, colors, param);
	}

	void convert(const Slic3r::Polygons& polys, ccglobal::Polygon& lines, std::vector<trimesh::vec4>* colors, const CovertParam& param)
	{
		lines.clear();
		if (colors)
			colors->clear();
		for (const Slic3r::Polygon& poly : polys)
			append(poly, lines, colors, param.out, param.z);
	}

	void append(const Slic3r::ExPolygon& expoly, ccglobal::Polygon& lines, std::vector<trimesh::vec4>* colors, const CovertParam& param)
	{
		append(expoly.contour, lines, colors, param.out, param.z);
		for (const Slic3r::Polygon& poly : expoly.holes)
			append(poly, lines, colors, param.in, param.z);
	}

	void append(const Slic3r::Polygon& poly, ccglobal::Polygon& lines, std::vector<trimesh::vec4>* colors, const trimesh::vec4& use_color, float z, bool to_lines, bool loop)
	{
		if (to_lines)
		{
			size_t size = poly.size();
			if (size <= 1)
				return;

			int end = loop ? size : size - 1;
			for (size_t i = 0; i < end; ++i)
			{
				lines.emplace_back(convert(poly.points.at(i), z));
				lines.emplace_back(convert(poly.points.at((i + 1) % size), z));
				if (colors)
				{
					colors->emplace_back(use_color);
					colors->emplace_back(use_color);
				}
			}
		}
		else {
			for (const Slic3r::Point& point : poly.points)
			{
				lines.emplace_back(convert(point, z));
				if (colors)
					colors->emplace_back(use_color);
			}
		}
	}
}