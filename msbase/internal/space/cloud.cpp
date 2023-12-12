#include "msbase/space/cloud.h"

namespace msbase {

	void offsetPoints(std::vector<trimesh::vec3>& points, const trimesh::vec3& offset)
	{
		for (trimesh::vec3& point : points)
			point += offset;
	}

	void applyMatrix2Points(std::vector<trimesh::vec3>& points, const trimesh::fxform& xf)
	{
		for (trimesh::vec3& point : points)
			point = xf * point;
	}
}