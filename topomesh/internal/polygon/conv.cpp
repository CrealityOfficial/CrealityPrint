#include "conv.h"

namespace topomesh
{
	void convertRaw(const ClipperLibXYZ::Paths& paths, std::vector<std::vector<trimesh::vec3>>& polygons)
	{
		int size = (int)paths.size();
		if (size == 0)
			return;

		polygons.resize(size);
		for (int i = 0; i < size; ++i)
			convertRaw(paths.at(i), polygons.at(i));
	}

	void convertRaw(const ClipperLibXYZ::Path& path, std::vector<trimesh::vec3>& lines)
	{
		size_t size = path.size();
		if (size < 1)
			return;

		lines.resize(size);
		for (size_t i = 0; i < size; ++i)
		{
			const ClipperLibXYZ::IntPoint& point = path.at(i);
			lines.at(i) = trimesh::vec3(INT2MM(point.X), INT2MM(point.Y), INT2MM(point.Z));
		}
	}
}