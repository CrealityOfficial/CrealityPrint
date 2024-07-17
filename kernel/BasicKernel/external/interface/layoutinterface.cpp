#include "layoutinterface.h"
#include "external/kernel/kernelui.h"
#include "internal/tool/layouttoolcommand.h"
#include "interface/printerinterface.h"

namespace creative_kernel
{
	void layoutModels(QList<ModelN*> models, int priorBinIndex, bool needPrintersLayout, bool needAddToScene)
	{
		getKernelUI()->getLayoutCommand()->layoutModels(models, priorBinIndex, needPrintersLayout, needAddToScene);
	}

	void extendFillModel(ModelN* extendModel, int curBinIndex)
	{
		if (curBinIndex < 0)
			return;

		getKernelUI()->getLayoutCommand()->extendFillModel(extendModel, curBinIndex);
	}

	bool needCheckPrinterLock()
	{
		return getKernelUI()->getLayoutCommand()->checkPrinterLock();
	}
	
}