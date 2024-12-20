#include "rayTriangleIntersect.h"
#pragma once
#include "Eigen/Dense"


namespace topomesh
{
	bool rayTriangleIntersect_1(const point& v0, const point& v1, const point& v2, const point& ori, const point& dir, point& result)
	{
		point S = ori - v0;
		point E1 = v1 - v0;
		point E2 = v2 - v1;
		point S1 = dir % E2;
		point S2 = S % E1;

		float S1E1 = S1 ^ E1;
		float t = (S2 ^ E2)*1.f / S1E1*1.f;
		float b1 = (S1 ^ S) * 1.f / S1E1 * 1.f;
		float b2 = (S2 ^ dir) * 1.f / S1E1 * 1.f;

		if (t > 0.f && b1 >= 0.f && b2 >= 0.f && (1 - b1 - b2) >= 0.f)
		{
			result = ori + t * dir;
			return true;
		}
		return false;
	}

	//  be lowest of up
	bool rayTriangleIntersect_2(const point& t0, const point& t1, const point& t2, const point& v0, const point& v1, point& result)
	{
		Eigen::Matrix3f mat;
		mat << v1.x - v0.x, t0.x - t1.x, t0.x - t2.x,
			v1.y - v0.y, t0.y - t1.y, t0.y - t2.y,
			v1.z - v0.z, t0.z - t1.z, t0.z - t2.z;

		Eigen::Vector3f vec;
		vec << t0.x - v0.x, t0.y - v0.y, t0.z - v0.z;
		Eigen::Vector3f x = mat.fullPivLu().solve(vec);
		if (x.x() > 0.f &&x.x()<1.f&& x.y() >= 0.f && x.z() >= 0.f && (1 - x.y() - x.z()) >= 0.f)
		{
			result = v0 + x.x() * (v1 - v0);
			return true;
		}
		return false;
	}

	bool faceTriangleIntersect(const trimesh::point& va1, const trimesh::point& va2, const trimesh::point& va3,
		const trimesh::point& vb1, const trimesh::point& vb2, const trimesh::point& vb3,
		trimesh::point& cross1, trimesh::point& cross2)
	{
		int cross_num = 0;
		std::vector<trimesh::point> ori;
		std::vector<trimesh::point> dir;
		ori.push_back(vb1); ori.push_back(vb2); ori.push_back(vb3);
		dir.push_back(vb2 - vb1); dir.push_back(vb3 - vb2); dir.push_back(vb1 - vb3);
		std::vector<trimesh::point> result_array;
		for (int i = 0; i < 3; i++)
		{
			trimesh::point result;
			if (rayTriangleIntersect_1(va1, va2, va3, ori[i], dir[i], result))
			{
				cross_num++;
				result_array.push_back(result);
			}
			if (cross_num == 2)
			{
				cross1 = result_array[0]; cross2 = result_array[1];
				return true;
			}
		}
		ori.clear(); dir.clear();
		ori.push_back(va1); ori.push_back(va2); ori.push_back(va3);
		dir.push_back(va2 - va1); dir.push_back(va3 - va2); dir.push_back(va1 - va3);
		for (int i = 0; i < 3; i++)
		{
			trimesh::point result;
			if (rayTriangleIntersect_1(vb1, vb2, vb3, ori[i], dir[i], result))
			{
				cross_num++;
				result_array.push_back(result);
			}
			if (cross_num == 2)
			{
				cross1 = result_array[0]; cross2 = result_array[1];
				return true;
			}
		}
		if (cross_num == 1)
		{
			cross1 = result_array[0]; cross2 = result_array[0];
			return true;
		}else
			return false;
	}
}