#include "conv1.h"

namespace topomesh
{
	trimesh::vec3 convert(const ClipperLib::IntPoint& point, float z)
	{
		return trimesh::vec3(INT2MM(point.X), INT2MM(point.Y), z);
	}

	ClipperLib::IntPoint convert(const trimesh::vec3& point)
	{
		return ClipperLib::IntPoint(ClipperLib::cInt(1000.0f * point.x), ClipperLib::cInt(1000.0f * point.y));
	}

	void convertRaw(const ClipperLib::Paths& paths, std::vector<std::vector<trimesh::vec3>>& polygons, float z)
	{
		polygons.clear();
		int size = (int)paths.size();
		if (size == 0)
			return;

		polygons.resize(size);
		for (int i = 0; i < size; ++i)
			convertRaw(paths.at(i), polygons.at(i), z);
	}

	void convertRaw(const ClipperLib::Path& path, std::vector<trimesh::vec3>& lines, float z)
	{
		lines.clear();
		int size = (int)path.size();
		if (size == 0)
			return;

		lines.resize(size);
		for (int i = 0; i < size; ++i)
		{
			const ClipperLib::IntPoint& point = path.at(i);
			lines[i] = convert(point, z);
		}
	}

	void convertRaw(const std::vector<std::vector<trimesh::vec3>>& lines, ClipperLib::Paths& paths)
	{
		paths.clear();
		int size = (int)lines.size();
		if (size == 0)
			return;

		paths.resize(size);
		for (int i = 0; i < size; ++i)
			convertRaw(lines.at(i), paths.at(i));
	}

	void convertRaw(const std::vector<trimesh::vec3>& lines, ClipperLib::Path& path)
	{
		path.clear();
		int size = (int)lines.size();
		if (size == 0)
			return;

		path.resize(size);
		for (int i = 0; i < size; ++i)
		{
			const trimesh::vec3& point = lines.at(i);
			path[i] = convert(point);
		}
	}
}