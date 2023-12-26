#ifndef CXKERNEL_SURFACEQUADENTITY_1682318036139_H
#define CXKERNEL_SURFACEQUADENTITY_1682318036139_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API SurfaceQuadEntity : public qtuser_3d::XEntity
	{
		Q_OBJECT
	public:
		SurfaceQuadEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~SurfaceQuadEntity();

		void setColor(const QVector4D& color);
	protected:
		Qt3DRender::QParameter* m_color;
	};
}

#endif // CXKERNEL_SURFACEQUADENTITY_1682318036139_H