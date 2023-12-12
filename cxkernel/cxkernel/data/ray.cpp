#include "ray.h"

namespace cxkernel
{
	Ray::Ray()
		: start(0.0f, 0.0f, 0.0f)
		, dir(0.0f, 0.0f, 1.0f)
	{

	}

	Ray::Ray(const trimesh::vec3& vstart)
		: start(vstart)
		, dir(0.0f, 0.0f, 1.0f)
	{

	}

	Ray::Ray(const trimesh::vec3& vstart, const trimesh::vec3& ndir)
		: start(vstart)
		, dir(ndir)
	{

	}

	Ray::~Ray()
	{
	}

	bool Ray::collidePlane(const trimesh::vec3& planeCenter, const trimesh::vec3& planeDir, trimesh::vec3& collide) const
	{
		float l = dir DOT planeDir;
		if (l == 0.0f) 
			return false;

		float t = (planeDir DOT(planeCenter - start)) / l;
		collide = start + dir * t;

		return true;
	}
}