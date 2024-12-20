#ifndef CREATIVE_KERNEL_PROJECT_INTERFACE_1592732397175_H
#define CREATIVE_KERNEL_PROJECT_INTERFACE_1592732397175_H
#include "kernel/projectinfoui.h"

namespace creative_kernel
{
	BASIC_KERNEL_API ProjectInfoUI* projectUI();
	BASIC_KERNEL_API bool projectSpaceDirty();
	BASIC_KERNEL_API void setMenuMostRecentFile(const QString& file_name);

	BASIC_KERNEL_API void savePostProcess(bool result, const QString& msg="");
	BASIC_KERNEL_API void setDefaultPath(const QString& filePath);
	BASIC_KERNEL_API void triggerAutoSave(QList<creative_kernel::ModelGroup*> modelgroup,int type);
	BASIC_KERNEL_API QString projectNameNoSuffix();
	BASIC_KERNEL_API QString projectName();
	BASIC_KERNEL_API QString projectPath();
	BASIC_KERNEL_API void saveGcode3mf(const QString& file_name, QList<int> plate_ids);
}

#endif // CREATIVE_KERNEL_PROJECT_INTERFACE_1592732397175_H