#include "msbase/space/intersect.h"

namespace msbase
{
	bool rayIntersectTriangle(const trimesh::vec3& orig, const trimesh::vec3& dir,
		trimesh::vec3& v0, trimesh::vec3& v1, trimesh::vec3& v2, trimesh::vec3& baryPosition)
	{
		auto E1 = v1 - v0;
		auto E2 = v2 - v0;

		auto P = trimesh::cross(dir, E2);
		auto det = trimesh::dot(E1, P);
		auto s = orig - v0;

		if (det < 0)
		{
			det = -det;
			s = v0 - orig;
		}
		if (det < 0.0001f)
			return false;

		auto f = 1.0f / det;
		auto dSP = trimesh::dot(s, P);
		baryPosition.at(0) = f * dSP;
		if (baryPosition.at(0) < 0.0f)
			return false;
		if (baryPosition.at(0) > 1.0f)
			return false;
		auto q = trimesh::cross(s, E1);
		auto dDQ = trimesh::dot(dir, q);
		baryPosition.y = f * dDQ;
		if (baryPosition.at(1) < 0.0f)
			return false;
		if (baryPosition.at(1) + baryPosition.at(0) > 1.0f)
			return false;
		auto dEQ = trimesh::dot(E2, q);
		baryPosition.at(2) = f * dEQ;
		return baryPosition.at(2) >= 0.0f;
	}

	bool rayIntersectTriangle(const trimesh::vec3& orig, const trimesh::vec3& dir,
		trimesh::vec3& v0, trimesh::vec3& v1, trimesh::vec3& v2, float* t, float* u, float* v)
	{
		trimesh::vec3 E1 = v1 - v0;
		trimesh::vec3 E2 = v2 - v0;

		trimesh::vec3 P = trimesh::cross(dir, E2);
		float det = trimesh::dot(E1, P);

		// keep det > 0, modify T accordingly
		trimesh::vec3 T;
		if (det > 0)
		{
			T = orig - v0;
		}
		else
		{
			T = v0 - orig;
			det = -det;
		}

		// If determinant is near zero, ray lies in plane of triangle
		if (det < 0.0001f)
			return false;

		// Calculate u and make sure u <= 1
		*u = trimesh::dot(T, P);
		if (*u < 0.0f || *u > det)
			return false;

		trimesh::vec3 Q = trimesh::cross(T, E1);

		// Calculate v and make sure u + v <= 1
		*v = trimesh::dot(dir, Q);
		if (*v < 0.0f || *u + *v > det)
			return false;

		// Calculate t, scale parameters, ray intersects triangle
		*t = trimesh::dot(E2, Q);

		float fInvDet = 1.0f / det;
		*t *= fInvDet;
		*u *= fInvDet;
		*v *= fInvDet;

		return true;
	}

	bool rayIntersectPlane(const trimesh::vec3& orig, const trimesh::vec3& dir, trimesh::vec3& vertex, trimesh::vec3& normal, float& t)
	{
		float l = trimesh::dot(normal, dir);
		if (l == 0.0f) return false;

		t = trimesh::dot(normal, (vertex - orig)) / l;
		return true;
	}

	bool PointinTriangle(const trimesh::vec3& A, const trimesh::vec3& B, const trimesh::vec3& C, const trimesh::vec3& P)
	{
		trimesh::vec3 v0 = C - A;
		trimesh::vec3 v1 = B - A;
		trimesh::vec3 v2 = P - A;

		float dot00 = v0.dot(v0);
		float dot01 = v0.dot(v1);
		float dot02 = v0.dot(v2);
		float dot11 = v1.dot(v1);
		float dot12 = v1.dot(v2);

		float inverDeno = 1.0 / (dot00 * dot11 - dot01 * dot01);

		float u = (dot11 * dot02 - dot01 * dot12) * inverDeno;
		if (u < 0 || u > 1) // if u out of range, return directly
		{
			return false;
		}

		float v = (dot00 * dot12 - dot01 * dot02) * inverDeno;
		if (v < 0 || v > 1) // if v out of range, return directly
		{
			return false;
		}

		return u + v <= 1;
	}
}