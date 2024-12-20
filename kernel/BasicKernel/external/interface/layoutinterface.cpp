#include "layoutinterface.h"
#include "interface/printerinterface.h"
#include "kernel/kernel.h"
#include "internal/layout/layoutmanager.h"

namespace creative_kernel
{
	void extendFillModelGroup(int groupId, int curBinIndex)
	{
		return getKernel()->layoutManager()->extendFillModelGroup(groupId, curBinIndex);
	}

	void layoutModelGroups(QList<int> groupIds, int priorBinIndex)
	{
		return getKernel()->layoutManager()->layoutModelGroups(groupIds, priorBinIndex);
	}

	void getPositionBySimpleLayout(const QList<CONTOUR_PATH>& inOutlinePaths, QList<QVector3D>& outPos)
	{
		getKernel()->layoutManager()->getPositionBySimpleLayout(inOutlinePaths, outPos);
	}
	
}