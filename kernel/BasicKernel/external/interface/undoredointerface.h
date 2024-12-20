#ifndef UNDO_REDO_INTERFACE_202404231607_H
#define UNDO_REDO_INTERFACE_202404231607_H
#include "basickernelexport.h"
#include "data/undochange.h"

namespace creative_kernel
{
	BASIC_KERNEL_API void layoutChangeScene(const LayoutChangeInfo& changeInfo);
}

#endif // UNDO_REDO_INTERFACE_202404231607_H
