#ifndef _QTUSER_3D_PIEFADEENTITY_1590036879870_H
#define _QTUSER_3D_PIEFADEENTITY_1590036879870_H

#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include <QtGui/QVector4D>

namespace qtuser_3d
{
	class XRenderPass;
	class SHADER_ENTITY_API PieFadeEntity : public XEntity
	{
	public:
		PieFadeEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PieFadeEntity();

		void setColor(const QVector4D& color);

		// 旋转交互相关参数，非旋转相关交互请保持默认值
		void setRotMode(int mode);
		void setRotRadians(float radians);
		void setRotCenter(QVector3D center);
		void setRotInitDir(QVector3D dir);
		void setRotAxis(QVector3D axis);

		void setLigthEnable(bool enable);
		void setPassBlend();
	protected:
		Qt3DRender::QParameter* m_colorParameter;

		Qt3DRender::QParameter* m_rotModeParameter;
		Qt3DRender::QParameter* m_rotRadiansParameter;
		Qt3DRender::QParameter* m_rotCenterParameter;
		Qt3DRender::QParameter* m_rotInitDirParameter;
		Qt3DRender::QParameter* m_rotAxisParameter;

		Qt3DRender::QParameter* m_lightEnableParameter;

		XRenderPass* m_viewPass;
	};
}
#endif // _QTUSER_3D_PIEFADEENTITY_1590036879870_H
