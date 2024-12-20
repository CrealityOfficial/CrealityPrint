#ifndef DISPLAY_WARNING_INTERFACE_202405271546_H
#define DISPLAY_WARNING_INTERFACE_202405271546_H
#include "basickernelexport.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{
	BASIC_KERNEL_API bool checkGCodePathScope(const qtuser_3d::Box3D& gcodePathBox, Printer* printer);
	BASIC_KERNEL_API bool checkSliceWarning(const QMap<QString, QPair<QString, int64_t> >& warningDetails);

	BASIC_KERNEL_API void checkGroupHigherThanBottom();
	BASIC_KERNEL_API void checkSliceEngineError(const QString& sliceErrorStr);
}
#endif // DISPLAY_WARNING_INTERFACE_202405271546_H