#ifndef MSBASE_GET_CONV_1695188680764_H
#define MSBASE_GET_CONV_1695188680764_H
#include "msbase/interface.h"
#include "trimesh2/quaternion.h"
#include "trimesh2/XForm.h"

namespace msbase
{
	//quaterian
	MSBASE_API trimesh::fxform fromQuaterian(const trimesh::quaternion& q);

	inline trimesh::dvec3 vec32dvec3(const trimesh::vec3& v)
	{
		return trimesh::dvec3(v.x, v.y, v.z);
	}
}

#endif // MSBASE_GET_CONV_1695188680764_H