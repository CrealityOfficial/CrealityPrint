#ifndef CRSLICE_SLICE_GCODE_H_2
#define CRSLICE_SLICE_GCODE_H_2
#include "crslice2/interface.h"
#include "crslice2/header.h"
#include <string>

namespace crslice2
{
	CRSLICE2_API std::string writeVarialLines(double start_width, int n, double delta);
}

#endif // CRSLICE_SLICE_GCODE_H_2