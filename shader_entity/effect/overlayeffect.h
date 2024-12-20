#ifndef CXKERNEL_OVERLAYEFFECT_1681819876499_H
#define CXKERNEL_OVERLAYEFFECT_1681819876499_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xeffect.h"
#include <QtGui/QVector4D>

namespace qtuser_3d
{
	class SHADER_ENTITY_API OverlayEffect : public qtuser_3d::XEffect
	{
	public:
		OverlayEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~OverlayEffect();

	};
}

#endif // CXKERNEL_SUPPORTPOSEFFECT_1681819876499_H