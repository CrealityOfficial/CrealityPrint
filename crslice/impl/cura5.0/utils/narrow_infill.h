#ifndef NARROW_INFILL274874
#define NARROW_INFILL274874

#include "utils/polygon.h"
#include "utils/ExtrusionLine.h"
#include "communication/sliceDataStorage.h"

namespace cura52
{

	bool result_is_narrow_infill_area(const cura52::Polygons& polygons);
	static	bool	is_narrow_infill_area(const cura52::Polygons& polygons);

	bool result_is_top_area(const cura52::Polygons& area, cura52::Polygons& polygons);
	static	bool	is_top_area(const cura52::Polygons& area, const cura52::Polygons& polygons);

	void processOverhang(const SliceLayerPart& part, const Polygons outlines_below, const int half_outer_wall_width, 
		std::vector<VariableWidthLines>& new_wall_toolpaths);
	void excludePoints(Polygons& polyin, Polygons& polyout);
}

#endif  