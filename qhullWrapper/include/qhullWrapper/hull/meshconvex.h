#ifndef QHULLWRAPPER_MESHCONVEX_1627200674710_H
#define QHULLWRAPPER_MESHCONVEX_1627200674710_H
#include "qhullWrapper/interface.h"
#include "trimesh2/XForm.h"
#include <functional>
#include <vector>

namespace trimesh
{
	class TriMesh;
}

namespace ccglobal
{
	class Tracer;
}

namespace qhullWrapper
{
	QHULLWRAPPER_API void calculateConvex(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer);

	QHULLWRAPPER_API trimesh::TriMesh* convex_hull_3d(trimesh::TriMesh* inMesh);
	QHULLWRAPPER_API trimesh::TriMesh* convex_hull_2d(trimesh::TriMesh* inMesh);

	QHULLWRAPPER_API trimesh::TriMesh* convex2DPolygon(const float* vertex, int count);
	QHULLWRAPPER_API trimesh::TriMesh* convex2DPolygon(trimesh::TriMesh* mesh, const trimesh::fxform& xf, ccglobal::Tracer* tracer);
}

#endif // QHULLWRAPPER_MESHCONVEX_1627200674710_H