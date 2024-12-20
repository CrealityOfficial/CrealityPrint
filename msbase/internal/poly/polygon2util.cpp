#include "polygon2util.h"

namespace msbase
{

	double crossValue(const trimesh::dvec2& v10, const trimesh::dvec2& v12)
	{
		return v10.x * v12.y - v10.y * v12.x;
	}

	double dotValue(const trimesh::dvec2& v10, const trimesh::dvec2& v12)
	{
		trimesh::dvec2 nv10 = trimesh::normalized(v10);
		trimesh::dvec2 nv12 = trimesh::normalized(v12);
		return trimesh::dot(nv10, nv12);
	}

	double angle(const trimesh::dvec2& v10, const trimesh::dvec2& v12)
	{
		double D = dotValue(v10, v12);
		double C = crossValue(v10, v12);
		double A = std::acos(D);
		if (C < 0.0)
			A = 2.0 * M_PI - A;
		return A;
	}

	bool insideTriangle(const trimesh::dvec2& va, const trimesh::dvec2& vb, const trimesh::dvec2& vc,
		const trimesh::dvec2& vp)
	{
		double ax = vc.x - vb.x; double ay = vc.y - vb.y;
		double bx = va.x - vc.x; double by = va.y - vc.y;
		double cx = vb.x - va.x; double cy = vb.y - va.y;
		double apx = vp.x - va.x; double apy = vp.y - va.y;
		double bpx = vp.x - vb.x; double bpy = vp.y - vb.y;
		double cpx = vp.x - vc.x; double cpy = vp.y - vc.y;

		double aCbp = ax * bpy - ay * bpx;
		double cCap = cx * apy - cy * apx;
		double bCcp = bx * cpy - by * cpx;
		return ((aCbp >= 0.0) && (bCcp >= 0.0) && (cCap >= 0.0));
	}

	bool insideTriangleEx(const trimesh::dvec2& va, const trimesh::dvec2& vb, const trimesh::dvec2& vc,
		const trimesh::dvec2& vp)
	{
		double ax = vc.x - vb.x; double ay = vc.y - vb.y;
		double bx = va.x - vc.x; double by = va.y - vc.y;
		double cx = vb.x - va.x; double cy = vb.y - va.y;
		double apx = vp.x - va.x; double apy = vp.y - va.y;
		double bpx = vp.x - vb.x; double bpy = vp.y - vb.y;
		double cpx = vp.x - vc.x; double cpy = vp.y - vc.y;

		double aCbp = ax * bpy - ay * bpx;
		double cCap = cx * apy - cy * apx;
		double bCcp = bx * cpy - by * cpx;
		return ((aCbp > 0.0) && (bCcp > 0.0) && (cCap > 0.0));
	}

	double polygonArea(std::vector<int>& polygon, std::vector<trimesh::dvec2>& points)
	{
		double area = 0.0;
		size_t size = polygon.size();
		for (size_t i = 1; i < size; ++i)
		{
			trimesh::dvec2& v1 = points.at(polygon.at(i));
			trimesh::dvec2& v2 = points.at(polygon.at(i - 1));
			area += 0.5 * crossValue(v1, v2);
		}
		return area;
	}

}