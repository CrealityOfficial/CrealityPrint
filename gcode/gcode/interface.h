#pragma once

#ifndef GCODE_INTERFACE_HPP_
#define GCODE_INTERFACE_HPP_

#include "ccglobal/export.h"

#if USE_GCODE_DLL
	#define GCODE_API CC_DECLARE_IMPORT
#elif USE_GCODE_STATIC
	#define GCODE_API CC_DECLARE_STATIC
#else
	#if GCODE_DLL
		#define GCODE_API CC_DECLARE_EXPORT
	#else
		#define GCODE_API CC_DECLARE_STATIC
	#endif
#endif

#endif // GCODE_INTERFACE_HPP_
