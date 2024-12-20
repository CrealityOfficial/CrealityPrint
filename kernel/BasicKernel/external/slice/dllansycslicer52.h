#ifndef _NULLSPACE_DLLANSYCSLICER52_1591781523824_H
#define _NULLSPACE_DLLANSYCSLICER52_1591781523824_H
#include "slice/ansycslicer.h"

namespace creative_kernel
{
	class DLLAnsycSlicer52 : public AnsycSlicer
	{
	public:
		DLLAnsycSlicer52(QObject* parent = nullptr);
		virtual ~DLLAnsycSlicer52();

		SliceResultPointer doSlice(SliceInput& input, qtuser_core::ProgressorTracer& tracer, SliceAttain* _fDebugger = nullptr) override;
	};
}
#endif // _NULLSPACE_DLLANSYCSLICER52_1591781523824_H
