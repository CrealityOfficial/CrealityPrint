#include "displaywarninginterface.h"
#include "kernel/kernel.h"
#include "external/kernel/displaymessagemanager.h"

namespace creative_kernel
{
	bool checkGCodePathScope(const qtuser_3d::Box3D& gcodePathBox, Printer* printer)
	{
		return getKernel()->displayMessageManager()->checkGCodePathScope(gcodePathBox, printer);
	}

	bool checkSliceWarning(const QMap<QString, QPair<QString, int64_t> >& warningDetails)
	{
		return getKernel()->displayMessageManager()->checkSliceWarning(warningDetails);
	}

	void checkGroupHigherThanBottom()
	{
		return getKernel()->displayMessageManager()->checkGroupHigherThanBottom();
	}

	void checkSliceEngineError(const QString& sliceErrorStr)
	{
		return getKernel()->displayMessageManager()->checkSliceEngineError(sliceErrorStr);
	}
}