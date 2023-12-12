#ifndef CXKERNEL_MODELPHONGEFFECT_1681899751633_H
#define CXKERNEL_MODELPHONGEFFECT_1681899751633_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xeffect.h"
#include <QtGui/QVector4D>

namespace qtuser_3d
{
	enum class RenderEffectMode
	{
		rem_face = 1,
		rem_line,
		rem_face_line
	};

	class SHADER_ENTITY_API ModelPhongEffect : public qtuser_3d::XEffect
	{
	public:
		ModelPhongEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~ModelPhongEffect();

		void setRenderEffectMode(RenderEffectMode mode);
		void useColor(bool use);
		void setColor(const QVector4D& color);
	protected:
		Qt3DRender::QParameter* m_remParameter;
		Qt3DRender::QParameter* m_colorParameter;
	};
}

#endif // CXKERNEL_MODELPHONGEFFECT_1681899751633_H