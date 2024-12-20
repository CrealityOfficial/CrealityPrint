#ifndef MSBASE_QUATERNIOND_1695188680764_H
#define MSBASE_QUATERNIOND_1695188680764_H
#include "msbase/interface.h"
#include "trimesh2/Vec.h"
#include "trimesh2/XForm.h"

namespace msbase
{
	class quaterniond
	{
	public:
		quaterniond();
		~quaterniond();
		quaterniond(double w, double x, double y, double z);
		quaterniond(double w, const trimesh::dvec3& src);
		static quaterniond rotationTo(const trimesh::dvec3& from, const trimesh::dvec3& to);
		quaterniond QuaterNormalized() const;
		static quaterniond fromAxes(const trimesh::dvec3& xAxis, const trimesh::dvec3& yAxis, const trimesh::dvec3& zAxis);
		static quaterniond fromRotationMatrix(double rot3x3[3][3]);
		static quaterniond fromRotationMatrix(const trimesh::xform& xf);
		inline quaterniond conjugated() const;
		inline trimesh::dvec3 vector() const;
		trimesh::dvec3 rotatedVector(const trimesh::dvec3& vector3) const;
		inline float dotProduct(const quaterniond& q1, const quaterniond& q2);
		void normalize();
		quaterniond normalized() const;
		static quaterniond fromDirection(trimesh::dvec3 dir, const trimesh::dvec3& fixedValue);
		static quaterniond fromAxisAndAngle(const trimesh::dvec3& axis, double angle);
		static quaterniond fromEular(trimesh::dvec3 angles);
		static bool qFuzzyIsNull(double f);
	
	public:
		double wp, xp, yp, zp;
	};

	const quaterniond operator/(const quaterniond& quaternion, double divisor);
	trimesh::dvec3 operator*(const quaterniond& quaternion, const trimesh::dvec3& from);
	const quaterniond operator*(const quaterniond& q1, const quaterniond& q2);
	
	trimesh::xform fromQuaterian(const quaterniond& q);
}

#endif // MSBASE_QUATERNIOND_1695188680764_H