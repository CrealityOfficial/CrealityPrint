#ifndef QTUSER_3D_ALONEPOINTENTITY_1594872195512_H
#define QTUSER_3D_ALONEPOINTENTITY_1594872195512_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include <Qt3DRender/QPointSize>
namespace qtuser_3d
{
	class XRenderPass;
	class SHADER_ENTITY_API AlonePointEntity : public XEntity
	{
		Q_OBJECT
	public:
		AlonePointEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~AlonePointEntity();

		void updateGlobal(QVector3D& point);
		void updateLocal(QVector3D& point, const QMatrix4x4& parentMatrix);
		void setColor(const QVector4D& color);
		void setPointSize(float size);	
		void setShader(const QString& name);
		void setFilterType(const QString& filterType);
	
	protected:
		XRenderPass* m_pass;
		Qt3DRender::QParameter* m_colorParameter;
		Qt3DRender::QPointSize* m_pointSizeState;
	};
}
#endif // QTUSER_3D_ALONEPOINTENTITY_1594872195512_H