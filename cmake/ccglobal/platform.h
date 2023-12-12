#ifndef __CCGLOBAL_PLATFORM_H
#define __CCGLOBAL_PLATFORM_H

#include <stdio.h>
#include <cmath>

#if CC_SYSTEM_WIN
#include <io.h>
#else
#include <unistd.h>
#endif
#define _cc_access access

#if CC_SYSTEM_WIN
	#define _cc_ftelli64 _ftelli64
#elif (CC_SYSTEM_ANDROID | CC_SYSTEM_IOS)
	#define _cc_ftelli64 ftell
#else
	#define _cc_ftelli64 ftell
#endif

#endif // __CCGLOBAL_PLATFORM_H
