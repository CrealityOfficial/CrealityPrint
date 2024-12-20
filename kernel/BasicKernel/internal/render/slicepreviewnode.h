#ifndef _NULLSPACE_SLICEPREVIEWNODE_1590320779367_H
#define _NULLSPACE_SLICEPREVIEWNODE_1590320779367_H
#include "entity/gcodeviewentity.h"

namespace creative_kernel
{
	class SlicePreviewNode : public qtuser_3d::GCodeViewEntity
	{
	public:
		SlicePreviewNode(Qt3DCore::QNode* parent = nullptr);
		virtual ~SlicePreviewNode();

		void initialize();

		void setNozzleColorList(const QVariantList& list);
	};
}
#endif // _NULLSPACE_SLICEPREVIEWNODE_1590320779367_H
