#ifndef SLICE_POINT2_1598707819216_H
#define SLICE_POINT2_1598707819216_H
#include "internal/polygon/coord_t.h"

namespace topomesh
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