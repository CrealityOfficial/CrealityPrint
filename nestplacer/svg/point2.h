#ifndef SLICE_POINT2_1598707819216_H
#define SLICE_POINT2_1598707819216_H
#include "svg/coord_t.h"

namespace svg
{
	class Point2
	{
	public:
		coord_t x;
		coord_t y;

		Point2();
		Point2(coord_t _x, coord_t _y);
	};
}

#endif // SLICE_POINT2_1598707819216_H