#ifndef CREATIVE_KERNEL_LAYOUTINTERFACE_1592889033863_H
#define CREATIVE_KERNEL_LAYOUTINTERFACE_1592889033863_H
#include "basickernelexport.h"
#include "internal/layout/layoutmanager.h"

namespace creative_kernel
{
	BASIC_KERNEL_API void layoutModelGroups(QList<int> groupIds, int priorBinIndex);

	BASIC_KERNEL_API void extendFillModelGroup(int groupId, int curBinIndex);

	BASIC_KERNEL_API void getPositionBySimpleLayout(const QList<CONTOUR_PATH>& inOutlinePaths, QList<QVector3D>& outPos);

}
#endif // CREATIVE_KERNEL_LAYOUTINTERFACE_1592889033863_H