#ifndef CREATIVE_KERNEL_LAYOUTINTERFACE_1592889033863_H
#define CREATIVE_KERNEL_LAYOUTINTERFACE_1592889033863_H
#include "basickernelexport.h"
#include "data/modeln.h"

namespace creative_kernel
{
	BASIC_KERNEL_API void layoutModels(QList<ModelN*> models, int priorBinIndex, bool needPrintersLayout=true, bool needAddToScene=true);

	BASIC_KERNEL_API void extendFillModel(ModelN* extendModel, int curBinIndex);

	BASIC_KERNEL_API bool needCheckPrinterLock();

}
#endif // CREATIVE_KERNEL_LAYOUTINTERFACE_1592889033863_H