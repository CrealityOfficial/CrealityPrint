#include "undoredointerface.h"
#include "data/modelspaceundo.h"
#include "kernel/kernel.h"
#include "interface/printerinterface.h"
#include "interface/modelgroupinterface.h"

namespace creative_kernel
{
	void layoutChangeScene(const LayoutChangeInfo& changeInfo)
	{
		creative_kernel::getKernel()->modelSpaceUndo()->layoutChangeScene(changeInfo);
	}
}