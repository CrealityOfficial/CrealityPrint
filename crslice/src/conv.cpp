#include "conv.h"
#include "utils/Coord_t.h"

namespace crslice
{
	void convert(const ClipperLib::Paths& paths, std::vector<trimesh::vec3>& lines, float z, bool loop)
	{
		for (const ClipperLib::Path& path : paths)
			convert(path, lines, z, loop, true);
	}

	void convert(const ClipperLib::Path& path, std::vector<trimesh::vec3>& lines, float z, bool loop, bool append)
	{
		if (!append)
			lines.clear();

		size_t size = path.size();
		if (size <= 1)
			return;

		size_t end = loop ? size : size - 1;
		for (size_t i = 0; i < end; ++i)
		{
			const ClipperLib::IntPoint& point = path.at(i);
			const ClipperLib::IntPoint& point1 = path.at((i + 1) % size);
			lines.push_back(trimesh::vec3(FDM_I2F(point.X), FDM_I2F(point.Y), z));
			lines.push_back(trimesh::vec3(FDM_I2F(point1.X), FDM_I2F(point1.Y), z));
		}
	}

	trimesh::vec3 convert(const ClipperLib::IntPoint& point, float z)
	{
		return trimesh::vec3(FDM_I2F(point.X), FDM_I2F(point.Y), z);
	}

	trimesh::vec3 convert(const cura52::Point3& point)
	{
		return trimesh::vec3(FDM_I2F(point.x), FDM_I2F(point.y), FDM_I2F(point.z));
	}

	trimesh::vec3 convertScale(const ClipperLib::IntPoint& point, float z)
	{
		return trimesh::vec3(INT2MM2(point.X), INT2MM2(point.Y), z);
	}

	ClipperLib::IntPoint convert(const trimesh::vec3& point)
	{
		return ClipperLib::IntPoint(FDM_F2I(point.x), FDM_F2I(point.y));
	}

	ClipperLib::IntPoint convertScale(const trimesh::vec3& point)
	{
		return ClipperLib::IntPoint(MM2_2INT(point.x), MM2_2INT(point.y));
	}

	void convertRaw(const ClipperLib::Paths& paths, std::vector<std::vector<trimesh::vec3>>& polygons, float z)
	{
		int size = (int)paths.size();
		if (size == 0)
			return;

		polygons.resize(size);
		for (int i = 0; i < size; ++i)
			convertRaw(paths.at(i), polygons.at(i), z);
	}

	void convertRaw(const ClipperLib::Path& path, std::vector<trimesh::vec3>& lines, float z)
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

	void convertPolygonRaw(const cura52::Polygons& polys, std::vector<std::vector<trimesh::vec3>>& lines, float z)
	{
		convertRaw(polys.paths, lines, z);
	}

	void convertRaw(const std::vector<std::vector<trimesh::vec3>>& lines, ClipperLib::Paths& paths, bool scaleNm)
	{
		int size = (int)lines.size();
		if (size == 0)
			return;

		paths.resize(size);
		for (int i = 0; i < size; ++i)
			convertRaw(lines.at(i), paths.at(i), scaleNm);
	}

	void convertRaw(const std::vector<trimesh::vec3>& lines, ClipperLib::Path& path, bool scaleNm)
	{
		size_t size = lines.size();
		if (size <= 1)
			return;

		path.resize(size);
		for (size_t i = 0; i < size; ++i)
		{
			path.at(i) = scaleNm ? convertScale(lines.at(i)) : convert(lines.at(i));
		}
	}

	void convertPolygonRaw(const std::vector<std::vector<trimesh::vec3>>& lines, cura52::Polygons& polys, bool scaleNm)
	{
		convertRaw(lines, polys.paths, scaleNm);
	}
}