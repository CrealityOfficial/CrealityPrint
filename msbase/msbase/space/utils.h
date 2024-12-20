#ifndef MSBASE_UTIL_1695465928791_H
#define MSBASE_UTIL_1695465928791_H
#include "msbase/interface.h"
#include "trimesh2/Vec.h"
#include "trimesh2/XForm.h"
#include "trimesh2/Box.h"
#include <vector>

namespace msbase
{
	MSBASE_API trimesh::vec3 centerBox(const trimesh::vec3& center, const std::vector<trimesh::box3>& boxes, bool ignoreZ = true);

	MSBASE_API trimesh::fxform layMatrixFromPositionNormal(const trimesh::vec3& position, const trimesh::vec3& normal, const trimesh::vec3& scale, const trimesh::vec3& originNormal);
	MSBASE_API trimesh::fxform layArrowMatrix(const trimesh::vec3& position, const trimesh::vec3& normal, const trimesh::vec3& scale);
}

#endif // MSBASE_CLOUD_1695465928791_H