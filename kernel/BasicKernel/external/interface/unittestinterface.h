#ifndef CREATIVE_KERNEL_LAYOUTINTERFACE_1592889033863_H
#define CREATIVE_KERNEL_LAYOUTINTERFACE_1592889033863_H
#include "basickernelexport.h"
#include "data/modeln.h"

namespace creative_kernel
{
	class UnitTestFlow;
	BASIC_KERNEL_API int slicerUnitType();
	BASIC_KERNEL_API QString slicerBaselinePath();
	BASIC_KERNEL_API QString slicerCompErrorPath();
	BASIC_KERNEL_API QString slicerCacheDir();
	BASIC_KERNEL_API QString slicerGCodeDir();
	BASIC_KERNEL_API bool onlySliceCacheEnabled();
	BASIC_KERNEL_API UnitTestFlow* unitTestFlow();


}
#endif // CREATIVE_KERNEL_LAYOUTINTERFACE_1592889033863_H