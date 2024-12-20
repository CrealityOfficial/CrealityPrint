#ifndef GLOBALPARAM_1600139246992_H
#define GLOBALPARAM_1600139246992_H
#include "coord_t.h"

namespace topomesh
{
	constexpr int largest_neglected_gap_first_phase = MM2INT(0.01); //!< distance between two line segments regarded as connected
	constexpr int largest_neglected_gap_second_phase = MM2INT(0.02); //!< distance between two line segments regarded as connected
	constexpr int max_stitch1 = MM2INT(10.0); //!< maximal distance stitched between open polylines to form polygons

	constexpr double one_over_sqrt_2 = 0.7071067811865475244008443621048490392848359376884740; //!< 1.0 / sqrt(2.0)
}

#endif // GLOBALPARAM_1600139246992_H