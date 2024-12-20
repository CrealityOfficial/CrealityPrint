#ifndef _NULLSPACE_DLLANSYCSLICERORCA_1591781523824_H
#define _NULLSPACE_DLLANSYCSLICERORCA_1591781523824_H
#include "slice/ansycslicer.h"

namespace creative_kernel
{
	class DLLAnsycSlicerOrca : public AnsycSlicer
	{
	public:
		DLLAnsycSlicerOrca(QObject* parent = nullptr);
		virtual ~DLLAnsycSlicerOrca();

		SliceResultPointer doSlice(SliceInput& input, qtuser_core::ProgressorTracer& tracer, SliceAttain* _fDebugger = nullptr) override;
	};
}
#endif // _NULLSPACE_DLLANSYCSLICERORCA_1591781523824_H
