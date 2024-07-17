#ifndef CREATIVE_KERNEL_PROJECT_INTERFACE_1592732397175_H
#define CREATIVE_KERNEL_PROJECT_INTERFACE_1592732397175_H
#include "kernel/projectinfoui.h"

namespace creative_kernel
{
	BASIC_KERNEL_API ProjectInfoUI* projectUI();
	BASIC_KERNEL_API void dirtyProjectSpace();
	BASIC_KERNEL_API void clearProjectSpaceDrity();
	BASIC_KERNEL_API bool projectSpaceDirty();
	BASIC_KERNEL_API void savePostProcess();
	BASIC_KERNEL_API void triggerAutoSave();

	BASIC_KERNEL_API QString projectName();
	BASIC_KERNEL_API QString projectPath();
}

#endif // CREATIVE_KERNEL_PROJECT_INTERFACE_1592732397175_H