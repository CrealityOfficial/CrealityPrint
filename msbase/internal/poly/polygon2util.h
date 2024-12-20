#ifndef _NULLSPACE_POLYGON2UTIL_1592616606700_H
#define _NULLSPACE_POLYGON2UTIL_1592616606700_H
#include "trimesh2/Vec.h"
#include <vector>

namespace msbase
{
	double crossValue(const trimesh::dvec2& v10, const trimesh::dvec2& v12);
	double dotValue(const trimesh::dvec2& v10, const trimesh::dvec2& v12);
	double angle(const trimesh::dvec2& v10, const trimesh::dvec2& v12);

	bool insideTriangle(const trimesh::dvec2& va, const trimesh::dvec2& vb, const trimesh::dvec2& vc,
		const trimesh::dvec2& vp);
	
	bool insideTriangleEx(const trimesh::dvec2& va, const trimesh::dvec2& vb, const trimesh::dvec2& vc,
		const trimesh::dvec2& vp);   // not in edges
	
	double polygonArea(std::vector<int>& polygon, std::vector<trimesh::dvec2>& points);
}
#endif // _NULLSPACE_POLYGON2UTIL_1592616606700_H
