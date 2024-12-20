#include "msbase/data/quaterniond.h"

namespace msbase
{
	quaterniond::quaterniond()
		:wp(1.0), xp(0.0), yp(0.0), zp(0.0)
	{
	}
	quaterniond::~quaterniond()
	{
	}
	quaterniond::quaterniond(double w, double x, double y, double z)
	{
		wp = w;
		xp = x;
		yp = y;
		zp = z;
	}
	quaterniond::quaterniond(double w, const trimesh::dvec3& src)
	{
		wp = w;
		xp = src.at(0);
		yp = src.at(1);
		zp = src.at(2);
	}
	quaterniond quaterniond::fromRotationMatrix(double rot3x3[3][3])
	{
		// Algorithm from:
	// http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q55
		double scalar;
		double axis[3];
		const double trace = rot3x3[0][0] + rot3x3[1][1] + rot3x3[2][2];
		if (trace > 0.00000001) {
			const double s = 2.0 * std::sqrt(trace + 1.0);
			scalar = 0.25 * s;
			axis[0] = (rot3x3[2][1] - rot3x3[1][2]) / s;
			axis[1] = (rot3x3[0][2] - rot3x3[2][0]) / s;
			axis[2] = (rot3x3[1][0] - rot3x3[0][1]) / s;
		}
		else {
			static int s_next[3] = { 1, 2, 0 };
			int i = 0;
			if (rot3x3[1][1] > rot3x3[0][0])
				i = 1;
			if (rot3x3[2][2] > rot3x3[i][i])
				i = 2;
			int j = s_next[i];
			int k = s_next[j];
			const double s = 2.0 * std::sqrt(rot3x3[i][i] - rot3x3[j][j] - rot3x3[k][k] + 1.0);
			axis[i] = 0.25 * s;
			scalar = (rot3x3[k][j] - rot3x3[j][k]) / s;
			axis[j] = (rot3x3[j][i] + rot3x3[i][j]) / s;
			axis[k] = (rot3x3[k][i] + rot3x3[i][k]) / s;
		}
		return quaterniond(scalar, axis[0], axis[1], axis[2]);
	}

	quaterniond quaterniond::fromRotationMatrix(const trimesh::xform& rot3x3)
	{
		// Algorithm from:
	// http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q55
		double scalar;
		double axis[3];
		const double trace = rot3x3(0,0) + rot3x3(1,1) + rot3x3(2,2);
		if (trace > 0.00000001) {
			const double s = 2.0 * std::sqrt(trace + 1.0);
			scalar = 0.25 * s;
			axis[0] = (rot3x3(2,1) - rot3x3(1,2)) / s;
			axis[1] = (rot3x3(0,2) - rot3x3(2,0)) / s;
			axis[2] = (rot3x3(1,0) - rot3x3(0,1)) / s;
		}
		else {
			static int s_next[3] = { 1, 2, 0 };
			int i = 0;
			if (rot3x3(1,1) > rot3x3(0,0))
				i = 1;
			if (rot3x3(2,2) > rot3x3(i,i))
				i = 2;
			int j = s_next[i];
			int k = s_next[j];
			const double s = 2.0 * std::sqrt(rot3x3(i,i) - rot3x3(j,j) - rot3x3(k,k) + 1.0);
			axis[i] = 0.25 * s;
			scalar = (rot3x3(k,j) - rot3x3(j,k)) / s;
			axis[j] = (rot3x3(j,i) + rot3x3(i,j)) / s;
			axis[k] = (rot3x3(k,i) + rot3x3(i,k)) / s;
		}
		return quaterniond(scalar, axis[0], axis[1], axis[2]);
	}

	inline quaterniond quaterniond::conjugated() const
	{
		return quaterniond(wp, -xp, -yp, -zp);
	}
	inline trimesh::dvec3 quaterniond::vector() const
	{
		return trimesh::dvec3(xp, yp, zp);
	}
	trimesh::dvec3 quaterniond::rotatedVector(const trimesh::dvec3& vector3) const
	{
		return (*this * quaterniond(0, vector3) * conjugated()).vector();
	}
	inline float quaterniond::dotProduct(const quaterniond& q1, const quaterniond& q2)
	{
		return q1.wp * q2.wp + q1.xp * q2.xp + q1.yp * q2.yp + q1.zp * q2.zp;
	}
	quaterniond quaterniond::fromAxes(const trimesh::dvec3& xAxis, const trimesh::dvec3& yAxis, const trimesh::dvec3& zAxis)
	{
		double rot3x3[3][3] = { 0 };
		rot3x3[0][0] = xAxis.at(0);
		rot3x3[1][0] = xAxis.at(1);
		rot3x3[2][0] = xAxis.at(2);
		rot3x3[0][1] = yAxis.at(0);
		rot3x3[1][1] = yAxis.at(1);
		rot3x3[2][1] = yAxis.at(2);
		rot3x3[0][2] = zAxis.at(0);
		rot3x3[1][2] = zAxis.at(1);
		rot3x3[2][2] = zAxis.at(2);
		return fromRotationMatrix(rot3x3);
	}
	quaterniond quaterniond::fromDirection(trimesh::dvec3 dir, const trimesh::dvec3& fixedValue)
	{
		if (qFuzzyIsNull(dir.at(0)) && qFuzzyIsNull(dir.at(1)) && qFuzzyIsNull(dir.at(2)))
			return quaterniond();
		const trimesh::dvec3 zAxis(trimesh::normalized(dir));
		trimesh::dvec3 xAxis(fixedValue TRICROSS zAxis);
		if (qFuzzyIsNull(len2(xAxis)))
		{
			// collinear or invalid up vector; derive shortest arc to new direction
			return quaterniond::rotationTo(trimesh::dvec3(0.0f, 0.0f, 1.0f), zAxis);
		}
		trimesh::normalize(xAxis);
		const trimesh::dvec3 yAxis((zAxis TRICROSS xAxis));
		return quaterniond::fromAxes(xAxis, yAxis, zAxis);
	}
	quaterniond quaterniond::fromAxisAndAngle(const trimesh::dvec3& axis, double angle)
	{
		// Algorithm from:
		// http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q56
		// We normalize the result just in case the values are close
		// to zero, as suggested in the above FAQ.
		double a = angle * M_PI_2 / 180.0;
		double s = std::sin(a);
		double c = std::cos(a);
		trimesh::dvec3 ax = trimesh::normalized(axis);
		return quaterniond(c, ax.x * s, ax.y * s, ax.z * s).normalized();
	}
	quaterniond quaterniond::fromEular(trimesh::dvec3 angles)
	{
		double yaw = angles.z * M_PI / 180.0;
		double pitch = angles.y * M_PI / 180;
		double roll = angles.x * M_PI / 180;
		double cy = cos(yaw * 0.5);
		double sy = sin(yaw * 0.5);
		double cp = cos(pitch * 0.5);
		double sp = sin(pitch * 0.5);
		double cr = cos(roll * 0.5);
		double sr = sin(roll * 0.5);
		quaterniond q;
		q.wp = cy * cp * cr + sy * sp * sr;
		q.xp = cy * cp * sr - sy * sp * cr;
		q.yp = sy * cp * sr + cy * sp * cr;
		q.zp = sy * cp * cr - cy * sp * sr;
		return q.normalized();
	}
	void quaterniond::normalize()
	{
		// Need some extra precision if the length is very small.
		double len = double(xp) * double(xp) +
			double(yp) * double(yp) +
			double(zp) * double(zp) +
			double(wp) * double(wp);
		if (qFuzzyIsNull(len - 1.0) || qFuzzyIsNull(len))
			return;
		len = std::sqrt(len);
		xp /= len;
		yp /= len;
		zp /= len;
		wp /= len;
	}
	quaterniond quaterniond::normalized() const
	{
		// Need some extra precision if the length is very small.
		double len = double(xp) * double(xp) +
			double(yp) * double(yp) +
			double(zp) * double(zp) +
			double(wp) * double(wp);
		if (qFuzzyIsNull(len - 1.0))
			return *this;
		else if (!qFuzzyIsNull(len))
			return *this / std::sqrt(len);
		else
			return quaterniond(0.0, 0.0, 0.0, 0.0);
	}
	quaterniond quaterniond::rotationTo(const trimesh::dvec3& from, const trimesh::dvec3& to)
	{
		// Based on Stan Melax's article in Game Programming Gems
		const trimesh::dvec3 v0(trimesh::normalized(from));
		const trimesh::dvec3 v1(trimesh::normalized(to));
		//float d = dotProduct(v0, v1) + 1.0f;
		double d = (v0 DOT v1) + 1.0;
		// if dest vector is close to the inverse of source vector, ANY axis of rotation is valid
		if (qFuzzyIsNull(d))
		{
			trimesh::dvec3 axis = (trimesh::dvec3(1.0, 0.0, 0.0) TRICROSS v0);
			if (qFuzzyIsNull(len2(axis)))
				axis = (trimesh::dvec3(0.0, 1.0, 0.0) TRICROSS v0);
			trimesh::normalize(axis);
			return quaterniond(0.0, axis.at(0), axis.at(1), axis.at(2));
		}
		d = std::sqrt(2.0 * d);
		const trimesh::dvec3 axis((v0 TRICROSS v1) / d);
		return quaterniond(d * 0.5, axis).QuaterNormalized();
	}
	quaterniond quaterniond::QuaterNormalized() const
	{
		// Need some extra precision if the length is very small.
		double len = double(xp) * double(xp) +
			double(yp) * double(yp) +
			double(zp) * double(zp) +
			double(wp) * double(wp);
		if (qFuzzyIsNull(len - 1.0f))
			return *this;
		else if (!qFuzzyIsNull(len))
			return *this / std::sqrt(len);
		else
			return quaterniond(0.0, 0.0, 0.0, 0.0);
	}
	const quaterniond operator/(const quaterniond& q, double divisor)
	{
		return quaterniond(q.wp / divisor, q.xp / divisor, q.yp / divisor, q.zp / divisor);
	}
	trimesh::dvec3 operator*(const quaterniond& q, const trimesh::dvec3& from)
	{
		return q.rotatedVector(from);
	}
	const quaterniond operator*(const quaterniond& q1, const quaterniond& q2)
	{
		double yy = (q1.wp - q1.yp) * (q2.wp + q2.zp);
		double zz = (q1.wp + q1.yp) * (q2.wp - q2.zp);
		double ww = (q1.zp + q1.xp) * (q2.xp + q2.yp);
		double xx = ww + yy + zz;
		double qq = 0.5 * (xx + (q1.zp - q1.xp) * (q2.xp - q2.yp));
		double w = qq - ww + (q1.zp - q1.yp) * (q2.yp - q2.zp);
		double x = qq - xx + (q1.xp + q1.wp) * (q2.xp + q2.wp);
		double y = qq - yy + (q1.wp - q1.xp) * (q2.yp + q2.zp);
		double z = qq - zz + (q1.zp + q1.yp) * (q2.wp - q2.xp);
		return quaterniond(w, x, y, z);
	}
	bool quaterniond::qFuzzyIsNull(double f)
	{
		return fabsf(f) <= 0.00001;
	}

	trimesh::xform fromQuaterian(const quaterniond& q)
	{
		double x2 = q.xp * q.xp;
		double y2 = q.yp * q.yp;
		double z2 = q.zp * q.zp;
		double xy = q.xp * q.yp;
		double xz = q.xp * q.zp;
		double yz = q.yp * q.zp;
		double wx = q.wp * q.xp;
		double wy = q.wp * q.yp;
		double wz = q.wp * q.zp;
		// This calculation would be a lot more complicated for non-unit length quaternions
		// Note: The constructor of Matrix4 expects the Matrix in column-major format like expected by
		//   OpenGL
		trimesh::xform m = trimesh::xform(1.0 - 2.0 * (y2 + z2), 2.0 * (xy - wz), 2.0 * (xz + wy), 0.0,
			2.0 * (xy + wz), 1.0 - 2.0 * (x2 + z2), 2.0 * (yz - wx), 0.0,
			2.0 * (xz - wy), 2.0 * (yz + wx), 1.0 - 2.0 * (x2 + y2), 0.0,
			0.0, 0.0, 0.0, 1.0);

		trimesh::transpose(m);
		return m;
	}
}