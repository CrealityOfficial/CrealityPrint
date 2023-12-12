#ifndef MSBASE_CLOUD_1695465928791_H
#define MSBASE_CLOUD_1695465928791_H
#include "msbase/interface.h"
#include "trimesh2/Vec.h"
#include "trimesh2/XForm.h"
#include "trimesh2/Box.h"
#include <vector>

namespace msbase
{
	MSBASE_API void offsetPoints(std::vector<trimesh::vec3>& points, const trimesh::vec3& offset);
	MSBASE_API void applyMatrix2Points(std::vector<trimesh::vec3>& points, const trimesh::fxform& xf);
}

#endif // MSBASE_CLOUD_1695465928791_H