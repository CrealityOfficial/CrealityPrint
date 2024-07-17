#pragma once

#include "ccglobal/export.h"

#if USE_CUT_DLL
	#define CUT_API CC_DECLARE_IMPORT
#elif USE_CUT_STATIC
	#define CUT_API CC_DECLARE_STATIC
#else
	#if CUT_DLL
		#define CUT_API CC_DECLARE_EXPORT
	#else
		#define CUT_API CC_DECLARE_STATIC
	#endif
#endif
