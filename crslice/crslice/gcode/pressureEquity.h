
#ifndef CRCOMMON_PRESSUREEQUITY_1690769853657_H
#define CRCOMMON_PRESSUREEQUITY_1690769853657_H

#include "crslice/interface.h"
#include <string>
#include <vector>



namespace crslice
{
	CRSLICE_API void pressureE(std::vector<std::string>& inputGcodes, std::vector<std::string>& outputGcodes,double filament_diameter, float extrusion_rate, float segment_length, bool use_relative_e_distances);
}

#endif