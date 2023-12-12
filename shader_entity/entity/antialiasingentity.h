#ifndef CXKERNEL_ANTIALIASING_ENTITY_1681992284923_H
#define CXKERNEL_ANTIALIASING_ENTITY_1681992284923_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include <QVector2D>
#include <Qt3DRender/QTexture>

namespace qtuser_3d
{
	class SHADER_ENTITY_API AntiAliasingEntity : public qtuser_3d::XEntity
	{
	public:
		AntiAliasingEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~AntiAliasingEntity();

		void setTextureSize(const QVector2D& theSize);
		void setTextureId(quint16 texId);
		void setShareTexture(Qt3DRender::QSharedGLTexture* shareTexture);
		void setAntiFlag(int iFlag);
	protected:
		Qt3DRender::QParameter* m_textureSize;
		Qt3DRender::QParameter* m_textureId;
		Qt3DRender::QParameter* m_antiFlag;
	};
}

#endif // CXKERNEL_ANTIALIASING_ENTITY_1681992284923_H