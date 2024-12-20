#ifndef CXKERNEL_SUPPORTPOSEFFECT_1681819876499_H
#define CXKERNEL_SUPPORTPOSEFFECT_1681819876499_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xeffect.h"
#include <QtGui/QVector4D>

namespace qtuser_3d
{
	class SHADER_ENTITY_API SupportPosEffect : public qtuser_3d::XEffect
	{
	public:
		SupportPosEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~SupportPosEffect();

	};
}

#endif // CXKERNEL_SUPPORTPOSEFFECT_1681819876499_H