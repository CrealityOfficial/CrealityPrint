// Point.hpp

#pragma once  // Include guard to prevent multiple inclusion

#include "GEOM/Vector.hpp"
#include <array>
#include <iostream>

namespace GEOM {

	using GeomNumType = long long int;
	using HashNumType = size_t;

	class Point {
	public:
		GeomNumType x;  // x坐标，单位是微米
		GeomNumType y;  // y坐标，单位是微米
		GeomNumType z;  // z坐标，单位是微米

		Point(const GeomNumType& x, const GeomNumType& y, const GeomNumType& z);
		
		friend std::ostream& operator<<(std::ostream& os, const Point& v); // 友元函数，重载 << 操作符
		bool operator==(const Point& p) const;

		Vector operator-(const Point& a) const;  // 重载减法运算符，实现点相减得到向量
		HashNumType hash() const;
		
	};
}

// 哈希函数特化，使其可以在std::unordered_set中使用
namespace std {
	template <>
	struct hash<GEOM::Point> {
		GEOM::HashNumType operator()(const GEOM::Point& p) const {
			return p.hash();
		}
	};
}