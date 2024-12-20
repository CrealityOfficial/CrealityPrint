#ifndef MMESH_TRIANGLESPLIT_1631523909475_H
#define MMESH_TRIANGLESPLIT_1631523909475_H
#include "trimesh2/Vec.h"
#include <vector>
#include "ccglobal/tracer.h"

namespace msbase
{
	struct TriSegment
	{
		trimesh::dvec3 v1;
		trimesh::dvec3 v2;
		bool topPositive;

		trimesh::ivec2 index; // -1 not edge, 1 edge
	};

	bool splitTriangle(const trimesh::dvec3& v0, const trimesh::dvec3& v1, const trimesh::dvec3& v2,
		const std::vector<TriSegment>& tri, bool positive, std::vector<trimesh::vec3>& tris,std::vector<bool>& isInner,ccglobal::Tracer* tracer);

	struct SplitTriangleCache
	{
		trimesh::vec3 v0;
		trimesh::vec3 v1;
		trimesh::vec3 v2;
		std::vector<TriSegment> segments;
	};
}

#endif // MMESH_TRIANGLESPLIT_1631523909475_H