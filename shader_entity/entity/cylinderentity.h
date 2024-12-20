#ifndef CXKERNEL_CYLINDERENTITY_1681992284923_H
#define CXKERNEL_CYLINDERENTITY_1681992284923_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API CylinderEntity : public qtuser_3d::XEntity
	{
	public:
		CylinderEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~CylinderEntity();

	};
}

#endif // CXKERNEL_CYLINDERENTITY_1681992284923_H