#ifndef CXKERNEL_MODELSIMPLEEFFECT_1681819876499_H
#define CXKERNEL_MODELSIMPLEEFFECT_1681819876499_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xeffect.h"
#include <QtGui/QVector4D>

namespace qtuser_3d
{
	class SHADER_ENTITY_API ModelSimpleEffect : public qtuser_3d::XEffect
	{
	public:
		ModelSimpleEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~ModelSimpleEffect();

	protected:

	};
}

#endif // CXKERNEL_MODELSIMPLEEFFECT_1681819876499_H