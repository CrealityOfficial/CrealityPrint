#ifndef CXGCODE_GCODEVIEWEFFECT_1683805686161_H
#define CXGCODE_GCODEVIEWEFFECT_1683805686161_H

#include "shader_entity_export.h"
#include "qtuser3d/refactor/xeffect.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API GCodeViewEffect : public XEffect
	{
	public:
		GCodeViewEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~GCodeViewEffect();

	protected:

	};
}

#endif // CXGCODE_GCODEVIEWEFFECT_1683805686161_H