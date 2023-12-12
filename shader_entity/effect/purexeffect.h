#ifndef CXKERNEL_PUREXEFFECT_1681819876499_H
#define CXKERNEL_PUREXEFFECT_1681819876499_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xeffect.h"
#include <QtGui/QVector4D>

namespace qtuser_3d
{
	class SHADER_ENTITY_API PureXEffect : public qtuser_3d::XEffect
	{
	public:
		PureXEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~PureXEffect();

		void setColor(const QVector4D& color);
	protected:
		Qt3DRender::QParameter* m_color;
	};
}

#endif // CXKERNEL_PUREXEFFECT_1681819876499_H