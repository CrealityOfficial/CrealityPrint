#ifndef _NULLSPACE_DLLANSYCSLICER52CHENGFEI_1591781523824_H
#define _NULLSPACE_DLLANSYCSLICER52CHENGFEI_1591781523824_H
#include "slice/ansycslicer.h"
#include "chengfeislice.h"

namespace creative_kernel
{
	class DLLAnsycSlicer52CF : public AnsycSlicer
	{
	public:
		DLLAnsycSlicer52CF(const chengfeiSplit& chengfeiSplit,QObject* parent = nullptr);
		virtual ~DLLAnsycSlicer52CF();

		SliceResultPointer doSlice(SliceInput& input,qtuser_core::ProgressorTracer& tracer) override;

	private:
		chengfeiSplit m_chengfeiSplit;
	};
}
#endif // _NULLSPACE_DLLANSYCSLICER52CHENGFEI_1591781523824_H
