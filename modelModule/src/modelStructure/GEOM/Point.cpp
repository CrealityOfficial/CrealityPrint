// Point.cpp

#include "GEOM/Point.hpp"
#include <functional>

namespace GEOM {

	// 构造函数
	Point::Point(const GeomNumType& x, const GeomNumType& y, const GeomNumType& z) : x(x), y(y), z(z) {}

	// 重载减法运算符，实现点相减得到向量
	Vector Point::operator-(const Point& a) const {
		return Vector(x - a.x, y - a.y, z - a.z);
	}

	// 在类外部定义 << 操作符重载
	std::ostream& operator<<(std::ostream & os, const Point & p) {
		os << "(" << p.x << ", " << p.y << ", " << p.z << ")";
		return os;
	}

	bool Point::operator==(const Point& p) const
	{
		return x == p.x && y == p.y && z == p.z;
	}

	HashNumType Point::hash() const
	{
		return std::hash<GeomNumType>()(x * 99971) ^ std::hash<GeomNumType>()(y * 99989) ^ std::hash<GeomNumType>()(z* 99991);
	}

}
