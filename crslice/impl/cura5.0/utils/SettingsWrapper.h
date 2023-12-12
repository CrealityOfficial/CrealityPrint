//Copyright (c) 2020 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef SETTINGS_WRAPPER_ZSEAMCONFIG_H
#define SETTINGS_WRAPPER_ZSEAMCONFIG_H
#include "utils/Simplify.h"
#include "utils/polygon.h"
#include "utils/ExtrusionLine.h"

namespace cura52
{
	class Settings;
	Polygons simplifyPolygon(const Polygons& inputPolygon, const Settings& settings);
	Polygons simplifyPolyline(const Polygons& inputPolygon, const Settings& settings);

	ExtrusionLine simplifyClosedExtrusionLine(const ExtrusionLine& line, const Settings& settings);
	ExtrusionLine simplifyExtrusionLine(const ExtrusionLine& line, const Settings& settings);
} //Cura namespace.

#endif //SETTINGS_WRAPPER_ZSEAMCONFIG_H