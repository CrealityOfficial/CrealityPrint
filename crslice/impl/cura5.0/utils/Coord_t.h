//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef UTILS_COORD_T_H
#define UTILS_COORD_T_H


//Include Clipper to get the ClipperLib::IntPoint definition, which we reuse as Point definition.
#include <polyclipping/clipper.hpp>

#define FDM_USE_MM
namespace cura52
{
	using coord_t = ClipperLib::cInt;

	static inline coord_t operator "" _mu(unsigned long long i) { return i; };

#define INT2MM(n) (static_cast<double>(n) / 1000.0)
#define INT2MM2(n) (static_cast<double>(n) / 1000000.0)
#define MM2INT(n) (static_cast<cura52::coord_t>((n) * 1000 + 0.5 * (((n) > 0) - ((n) < 0))))
#define MM2_2INT(n) (static_cast<cura52::coord_t>((n) * 1000000 + 0.5 * (((n) > 0) - ((n) < 0))))
#define MM3_2INT(n) (static_cast<cura52::coord_t>((n) * 1000000000 + 0.5 * (((n) > 0) - ((n) < 0))))

#define INT2MICRON(n) ((n) / 1)
#define MICRON2INT(n) ((n) * 1)

} // namespace cura52

#ifdef FDM_USE_MM
#define FDM_F2I(n) MM2INT(n)
#define FDM_I2F(n) INT2MM(n)
#else
#define FDM_F2I(n) MM2_2INT(n)
#define FDM_I2F(n) INT2MM2(n)
#endif
#endif // UTILS_COORD_T_H
