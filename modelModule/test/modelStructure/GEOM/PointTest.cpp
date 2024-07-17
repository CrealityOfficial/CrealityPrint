// 测试代码

#include <gtest/gtest.h>
#include "GEOM/Point.hpp"

const float floatEqualEpsilon = 1e-2;

namespace test_modelStructure_GEOM_PointTest
{
	TEST(GEOM_Point, operatorMinus) {
		GEOM::Point p1(3, 4, 5);
		GEOM::Point p2(5, 4, 3);
		GEOM::Vector vdiff = p2 - p1;
		GEOM::Vector v(2, 0, -2);
		ASSERT_EQ(v, vdiff);
	}

	TEST(GEOM_Point, hash) {
		GEOM::Point p1(3, 4, 5);
		GEOM::Point p2(5, 4, 3);
		ASSERT_NE(p1.hash(), p2.hash());
	}

	TEST(GEOM_Point, hash_negetive) {
		GEOM::Point p1(1, 1, 0);
		GEOM::Point p2(-1, -1, 0);
		ASSERT_NE(p1.hash(), p2.hash());
	}

}
