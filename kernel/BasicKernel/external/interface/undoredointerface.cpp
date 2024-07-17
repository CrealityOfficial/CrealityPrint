#include "undoredointerface.h"
#include "data/modelspaceundo.h"
#include "kernel/kernel.h"
#include "interface/printerinterface.h"
#include "interface/modelinterface.h"

namespace creative_kernel
{
	void layoutChangeScene(const LayoutChangeInfo& changeInfo, bool reversible)
	{
		if (reversible)
		{
			creative_kernel::getKernel()->modelSpaceUndo()->layoutChangeScene(changeInfo);
		}
		else
		{
			if(changeInfo.needPrinterLayout || changeInfo.nowPrintersCount > changeInfo.prevPrintersCount)
				resizePrinters(changeInfo.nowPrintersCount);

			QList<ModelN*> models = getModelnsBySerialName(changeInfo.moveModelsSerialNames);
			mixTRModels(models, changeInfo.startPoses, changeInfo.endPoses, changeInfo.startQuaternions, changeInfo.endQuaternions, false);
		}
		
	}
}