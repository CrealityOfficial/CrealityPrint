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

	inline void checkVec3Range(trimesh::dvec3& v, const trimesh::dvec3& minscope, const trimesh::dvec3& maxscope)
	{
		for (int i = 0; i < 3; i++)
		{
			if (v[i] > maxscope[i])
			{
				v[i] = maxscope[i];
			}
			if (v[i] < minscope[i])
			{
				v[i] = minscope[i];
			}
		}

	}

	MSBASE_API trimesh::dvec3 transFromXform(const trimesh::xform& xf);

	MSBASE_API bool checkValid(const trimesh::xform& xf);
}

#endif // MSBASE_GET_CONV_1695188680764_H