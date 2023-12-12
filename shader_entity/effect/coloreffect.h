#ifndef CXKERNEL_COLOREFFECT_1681819876499_H
#define CXKERNEL_COLOREFFECT_1681819876499_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xeffect.h"
#include <QtGui/QVector4D>

namespace qtuser_3d
{
	class SHADER_ENTITY_API ColorEffect : public qtuser_3d::XEffect
	{
	public:
		ColorEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~ColorEffect();

	};
}

#endif // CXKERNEL_COLOREFFECT_1681819876499_H