// Vector.hpp

#pragma once  // Include guard to prevent multiple inclusion

#include <iostream>

namespace GEOM {

	const double relativeError = 1e-6;  // 相对误差

	class Vector {
	public:
		double x;
		double y;
		double z;

		// 构造函数
		Vector(const double& x, const double& y, const double& z);

		// 计算向量长度
		double length() const;

		// 单位化向量
		void normalize();

		// 向量加法
		Vector operator+(const Vector& v) const;

		// 向量减法
		Vector operator-(const Vector& v) const;

		// 向量乘以标量
		Vector operator*(const double& lambda) const;

		// 相等操作符
		bool operator==(const Vector& v) const;

		// 友元函数，重载 << 操作符
		friend std::ostream& operator<<(std::ostream& os, const Vector& v);

		// 计算点积
		double dot(const Vector& v) const;

		// 计算叉积
		Vector cross(const Vector& v) const;

		// 计算夹角（弧度）
		double angle(const Vector& v) const;
	};

}