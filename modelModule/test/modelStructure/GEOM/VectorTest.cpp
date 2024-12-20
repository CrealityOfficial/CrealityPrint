// 测试代码

#include <gtest/gtest.h>
#include "GEOM/Vector.hpp"

const double angleEsplion = 1e-6;

namespace test_modelStructure_GEOM_VectorTest
{
	TEST(GEOM_Vector, length) {
		GEOM::Vector vx(3, 4, 5);
		ASSERT_EQ(std::sqrt(50), vx.length());
	}

	TEST(GEOM_Vector, normalize) {
		GEOM::Vector vx(3, 4, 5);
		double total = vx.length();
		vx.normalize();
		GEOM::Vector vy(3 / total, 4 / total, 5 / total);
		ASSERT_EQ(vy, vx);
	}

	TEST(GEOM_Vector, cross) {
		GEOM::Vector vx(1, 0, 0);
		GEOM::Vector vy(0, 1, 0);
		GEOM::Vector vz(0, 0, 1);
		ASSERT_EQ(vz, vx.cross(vy));
	}

	TEST(GEOM_Vector, angle) {
		GEOM::Vector vx(1, 0, 0);
		GEOM::Vector vy(0, 1, 0);

		EXPECT_NEAR(3.1415926 / 2, vx.angle(vy), angleEsplion);
	}
}