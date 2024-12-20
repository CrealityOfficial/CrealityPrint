//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef CX_UTILS_COORD_T_H
#define CX_UTILS_COORD_T_H


//Include Clipper to get the ClipperLib::IntPoint definition, which we reuse as Point definition.
#include "clipper/clipper.hpp"
namespace topomesh
{
	using coord_t = ClipperLib::cInt;
} // namespace cura
#ifdef DLP_USE_UM
#define DLP_MM2_S(n) MM2INT(n)
#define DLP_S_2MM(n) (double(n) / 1000.0f)
#define DLP_S_UM(n) (double(n)/1.0f) 
#else
#define DLP_MM2_S(n) MM2_2INT(n)
#define DLP_S_2MM(n) INT2MM2(n)
#define DLP_S_UM(n) (double(n)/1000.0f) 

#endif
#define INT2MM2(n) (double(n) / 1000000.0)
#define MM2INT(n) (topomesh::coord_t((n) * 1000 + 0.5 * (((n) > 0) - ((n) < 0))))
#define MM2_2INT(n) (topomesh::coord_t((n) * 1000000 + 0.5 * (((n) > 0) - ((n) < 0))))
#define MM3_2INT(n) (topomesh::coord_t((n) * 1000000000 + 0.5 * (((n) > 0) - ((n) < 0))))

#define INT2MICRON(n) ((n) / 1)
#define MICRON2INT(n) ((n) * 1)

#endif // CX_UTILS_COORD_T_H
