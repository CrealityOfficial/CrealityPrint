#include "parse.h"

namespace cura52
{
	Point3 parse_special_slope_slice_axis(const std::string& str)
	{
		Point3 axis = Point3(0, 1, 0);
		if (str == "X") axis = Point3(1, 0, 0);
		return axis;
	}

	AngleDegrees parse_special_slope_slice_axis_degree(const std::string& str)
	{
		if (str == "X")
			return 0.0;
		else
			return 90.0;
	}
} //namespace cura52


