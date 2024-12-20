#ifndef CXKERNEL_SURFACEQUADEFFECT_1682318036140_H
#define CXKERNEL_SURFACEQUADEFFECT_1682318036140_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xeffect.h"
#include <QtGui/QVector4D>

namespace qtuser_3d
{
	class SHADER_ENTITY_API SurfaceQuadEffect : public qtuser_3d::XEffect
	{
		
	public:
		SurfaceQuadEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~SurfaceQuadEffect();

		void setColor(const QVector4D& color);
	protected:
		Qt3DRender::QParameter* m_color;
	};
}
#endif // CXKERNEL_SURFACEQUADEFFECT_1682318036140_H