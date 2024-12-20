//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef CX_COORD_T_H
#define CX_COORD_T_H


//Include Clipper to get the ClipperLib::IntPoint definition, which we reuse as Point definition.
#include "clipper3r/clipper.hpp"
namespace svg
{
	using coord_t = Clipper3r::cInt;
} // namespace cura

#endif // CX_UTILS_COORD_T_H
