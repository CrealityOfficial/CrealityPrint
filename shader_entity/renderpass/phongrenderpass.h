#ifndef QTUSER_3D_PHONGRENDERPASS_1679990543678_H
#define QTUSER_3D_PHONGRENDERPASS_1679990543678_H
#include <QtGui/QVector4D>
#include <QtGui/QVector3D>
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xrenderpass.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API PhongRenderPass : public qtuser_3d::XRenderPass
	{
	public:
		PhongRenderPass(Qt3DCore::QNode* parent = nullptr);
		virtual ~PhongRenderPass();

		void setColor(const QVector4D& color);
		void setAmbient(const QVector4D& color);
		void setDiffuse(const QVector4D& color);
		void setSpecular(const QVector4D& color);
		void setSpecularPower(const float& power);

	protected:
		Qt3DRender::QParameter* m_color;
		Qt3DRender::QParameter* m_ambient;
		Qt3DRender::QParameter* m_diffuse;
		Qt3DRender::QParameter* m_specular;
		Qt3DRender::QParameter* m_specularPower;
		Qt3DRender::QParameter* m_lightDir;

	};
}

#endif // QTUSER_3D_PHONGRENDERPASS_1679990543678_H