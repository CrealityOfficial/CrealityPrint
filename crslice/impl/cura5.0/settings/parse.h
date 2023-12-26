// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef WRAPPER_SETTINGS_SETTINGS_PARSE_H
#define WRAPPER_SETTINGS_SETTINGS_PARSE_H
#include <string>
#include "utils/Point3.h"
#include "types/Angle.h"

namespace cura52
{
	Point3 parse_special_slope_slice_axis(const std::string& str);
	AngleDegrees parse_special_slope_slice_axis_degree(const std::string& str);
} //namespace cura52

#endif //WRAPPER_SETTINGS_SETTINGS_PARSE_H

