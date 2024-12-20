#include "undointerface.h"
#include "qtusercore/util/undoproxy.h"
#include "cxkernel/kernel/cxkernel.h"

namespace cxkernel
{
	void setUndoStack(QUndoStack* undoStack)
	{
		cxKernel->undoProxy()->setUndoStack(undoStack);
	}

	void undo()
	{
		cxKernel->undoProxy()->undo();
	}

	void redo()
	{
		cxKernel->undoProxy()->redo();
	}

	void addUndoCallback(qtuser_core::UndoCallback* callback)
	{
		cxKernel->undoProxy()->addUndoCallback(callback);
	}

	void removeUndoCallback(qtuser_core::UndoCallback* callback)
	{
		cxKernel->undoProxy()->removeUndoCallback(callback);
	}
}