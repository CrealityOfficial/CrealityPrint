#include "conv.h"

namespace nestplacer
{
	trimesh::vec3 convert(const Clipper3r::IntPoint& point, float z)
	{
		return trimesh::vec3(INT2MM(point.X), INT2MM(point.Y), z);
	}

	Clipper3r::IntPoint convert(const trimesh::vec3& point)
	{
		return Clipper3r::IntPoint(MM2INT(point.x), MM2INT(point.y));
	}

	void convertRaw(const Clipper3r::Paths& paths, std::vector<std::vector<trimesh::vec3>>& polygons, float z)
	{
		int size = (int)paths.size();
		if (size == 0)
			return;

		polygons.resize(size);
		for (int i = 0; i < size; ++i)
			convertRaw(paths.at(i), polygons.at(i), z);
	}

	void convertRaw(const Clipper3r::Path& path, std::vector<trimesh::vec3>& lines, float z)
	{
		size_t size = path.size();
		if (size <= 1)
			return;

		lines.resize(size);
		for (size_t i = 0; i < size; ++i)
		{
			lines.at(i) = convert(path.at(i));
		}
	}

	void convertPolygon(const libnest2d::PolygonImpl& poly, DebugPolygon& dPoly)
	{
		convertRaw(poly.Contour, dPoly.outline);
		convertRaw(poly.Holes, dPoly.holes);
	}

	void convertPolygons(const std::vector<libnest2d::PolygonImpl>& polys,
		std::vector<DebugPolygon>& dPolys)
	{
		size_t size = polys.size();
		if (size > 0)
		{
			dPolys.resize(size);
			for (size_t i = 0; i < size; ++i)
				convertPolygon(polys.at(i), dPolys.at(i));
		}
	}

	libnest2d::_Box<libnest2d::PointImpl> convert(const trimesh::box& b)
	{
		Clipper3r::IntPoint minPoint(convert(b.min));
		Clipper3r::IntPoint maxPoint(convert(b.max));
		Clipper3r::IntPoint rect = maxPoint - minPoint;

		libnest2d::_Box<libnest2d::PointImpl> binBox 
			= libnest2d::_Box<libnest2d::PointImpl>(rect.X, rect.Y, convert(b.center()));
		return binBox;
	}
}