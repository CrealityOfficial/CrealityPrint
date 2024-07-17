// Vector.cpp

#include "GEOM/Vector.hpp"
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace GEOM {

	// 构造函数
	Vector::Vector(const double& x, const double& y, const double& z) : x(x), y(y), z(z) {}

	// 计算向量长度
	double Vector::length() const {
		return std::sqrt(x * x + y * y + z * z);
	}

	// 单位化向量
	void Vector::normalize() {
		double len = length();
		if (len > 0.0) {
			x /= len;
			y /= len;
			z /= len;
		}
		else {
			std::cerr << "Cannot normalize a zero-length vector." << std::endl;
			x = 0;
			y = 0;
			z = 0;
		}
	}

	// 向量加法
	Vector Vector::operator+(const Vector & v) const {
		return Vector(x + v.x, y + v.y, z + v.z);
	}

	// 向量减法
	Vector Vector::operator-(const Vector & v) const {
		return Vector(x - v.x, y - v.y, z - v.z);
	}

	// 向量乘以标量
	Vector Vector::operator*(const double& lambda) const {
		return Vector(x * lambda, y * lambda, z * lambda);
	}

	// 相等操作符
	bool Vector::operator==(const Vector& v) const {
		// 计算相对误差并检查是否足够小
		return std::fabs(x - v.x) <= std::max(std::fabs(x), std::fabs(v.x)) * relativeError &&
			std::fabs(y - v.y) <= std::max(std::fabs(y), std::fabs(v.y)) * relativeError &&
			std::fabs(z - v.z) <= std::max(std::fabs(z), std::fabs(v.z)) * relativeError;

	}

	// 在类外部定义 << 操作符重载
	std::ostream& operator<<(std::ostream& os, const Vector& v) {
		os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
		return os;
	}

	// 计算点积
	double Vector::dot(const Vector & v) const {
		return x * v.x + y * v.y + z * v.z;
	}

	// 计算叉积
	Vector Vector::cross(const Vector & v) const {
		return Vector(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}

	// 计算夹角（弧度）
	double Vector::angle(const Vector & v) const {
		double lenProduct = length() * v.length();
		if (lenProduct > 0.0) {
			return std::acos(dot(v) / lenProduct);
		}
		else {
			throw std::invalid_argument("Cannot calculate angle for vectors with zero length.");
		}
	}

}
