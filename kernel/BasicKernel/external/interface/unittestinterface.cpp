#include "unittest/unittestflow.h"
#include "kernel/kernel.h"

namespace creative_kernel
{
	int slicerUnitType()
	{
		return getKernel()->unitTest()->slicerUnitType();
	}
	QString slicerBaselinePath(){
		return getKernel()->unitTest()->slicerBaseLinePath();
	}
	QString slicerCompErrorPath() {
		return getKernel()->unitTest()->compareErrorPath();
	}
	UnitTestFlow* unitTestFlow()
	{
		return getKernel()->unitTest();
	}
}