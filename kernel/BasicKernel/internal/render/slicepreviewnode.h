#ifndef _NULLSPACE_SLICEPREVIEWNODE_1590320779367_H
#define _NULLSPACE_SLICEPREVIEWNODE_1590320779367_H
#include "cxgcode/render/gcodeviewentity.h"

namespace creative_kernel
{
	class SlicePreviewNode : public cxgcode::GCodeViewEntity
	{
	public:
		SlicePreviewNode(Qt3DCore::QNode* parent = nullptr);
		virtual ~SlicePreviewNode();

		void initialize();
	};
}
#endif // _NULLSPACE_SLICEPREVIEWNODE_1590320779367_H
