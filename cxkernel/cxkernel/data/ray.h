#ifndef CXND_RAY_1638866681924_H
#define CXND_RAY_1638866681924_H
#include "cxkernel/cxkernelinterface.h"
#include "trimesh2/Vec.h"

namespace cxkernel
{
	class CXKERNEL_API Ray
	{
	public:
		Ray();
		Ray(const trimesh::vec3& vstart);
		Ray(const trimesh::vec3& vstart, const trimesh::vec3& ndir);

		~Ray();

		bool collidePlane(const trimesh::vec3& planeCenter, const trimesh::vec3& planeDir, trimesh::vec3& collide) const;

		trimesh::vec3 start;
		trimesh::vec3 dir;
	};
}

#endif // CXND_RAY_1638866681924_H