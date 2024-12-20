#ifndef CXKERNEL_MODELENTITY_1681899751632_H
#define CXKERNEL_MODELENTITY_1681899751632_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/pxentity.h"
#include "effect/modelphongeffect.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API ModelEntity : public qtuser_3d::PickXEntity
	{
	public:
		ModelEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~ModelEntity();

		ModelPhongEffect* mEffect();
	protected:
		ModelPhongEffect* m_effect;
	};
}

#endif // CXKERNEL_MODELENTITY_1681899751632_H