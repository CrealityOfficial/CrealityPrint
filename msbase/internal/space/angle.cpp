#include "msbase/space/angle.h"
#include <climits>

namespace msbase {

	float angleOfVector3D2(const trimesh::vec3& v1, const trimesh::vec3& v2)
	{
		float radian = radianOfVector3D2(v1, v2);
		return radian * 180.0f / (float)M_PI;
	}

	float radianOfVector3D2(const trimesh::vec3& v1, const trimesh::vec3& v2)
	{
		double denominator = sqrt((double)trimesh::len2(v1) * (double)trimesh::len2(v2));
		double cosinus = 0.0;

		if (denominator < INT_MIN)
			cosinus = 1.0; // cos (1)  = 0 degrees

		cosinus = (double)(v1 DOT v2) / denominator;
		cosinus = cosinus > 1.0 ? 1.0 : (cosinus < -1.0 ? -1.0 : cosinus);
		double angle = acos(cosinus);
#ifdef WIN32
		if (!_finite(angle) || angle > M_PI)
			angle = 0.0;
#elif __APPLE__
		if (!isfinite(angle) || angle > M_PI)
			angle = 0.0;
#elif defined(__ANDROID__)
		if (!isfinite(angle) || angle > M_PI)
			angle = 0.0;
#else
		if (!finite(angle) || angle > M_PI)
			angle = 0.0;
#endif
		return (float)angle;
	}

	trimesh::vec3 triangleNormal(const trimesh::vec3& v1, const trimesh::vec3& v2, const trimesh::vec3& v3)
	{
		trimesh::vec3 v01 = v2 - v1;
		trimesh::vec3 v02 = v3 - v1;
		trimesh::vec3 n = v01 TRICROSS v02;
		trimesh::normalize(n);
		return n;
	}
}