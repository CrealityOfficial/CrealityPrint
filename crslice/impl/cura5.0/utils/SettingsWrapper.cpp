//Copyright (c) 2022 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#include "SettingsWrapper.h"
#include "Simplify.h"
#include "settings/Settings.h"

namespace cura52
{
	Polygons simplifyPolygon(const Polygons& inputPolygon, const Settings& settings)
	{
		coord_t max_resolution = settings.get<coord_t>("meshfix_maximum_resolution");
		coord_t max_deviation = settings.get<coord_t>("meshfix_maximum_deviation");
		coord_t max_area_deviation = settings.get<coord_t>("meshfix_maximum_extrusion_area_deviation");

		Simplify simple(max_resolution, max_deviation, max_area_deviation);

		return simple.polygon(inputPolygon);
	}

	Polygons simplifyPolyline(const Polygons& inputPolygon, const Settings& settings)
	{
		coord_t max_resolution = settings.get<coord_t>("meshfix_maximum_resolution");
		coord_t max_deviation = settings.get<coord_t>("meshfix_maximum_deviation");
		coord_t max_area_deviation = settings.get<coord_t>("meshfix_maximum_extrusion_area_deviation");

		Simplify simple(max_resolution, max_deviation, max_area_deviation);

		return simple.polyline(inputPolygon);
	}

	ExtrusionLine simplifyClosedExtrusionLine(const ExtrusionLine& line, const Settings& settings)
	{
		coord_t max_resolution = settings.get<coord_t>("meshfix_maximum_resolution");
		coord_t max_deviation = settings.get<coord_t>("meshfix_maximum_deviation");
		coord_t max_area_deviation = settings.get<coord_t>("meshfix_maximum_extrusion_area_deviation");

		Simplify simple(max_resolution, max_deviation, max_area_deviation);

		return simple.polygon(line);
	}

	ExtrusionLine simplifyExtrusionLine(const ExtrusionLine& line, const Settings& settings)
	{
		coord_t max_resolution = settings.get<coord_t>("meshfix_maximum_resolution");
		coord_t max_deviation = settings.get<coord_t>("meshfix_maximum_deviation");
		coord_t max_area_deviation = settings.get<coord_t>("meshfix_maximum_extrusion_area_deviation");

		Simplify simple(max_resolution, max_deviation, max_area_deviation);

		return simple.polyline(line);
	}
} //Cura namespace.