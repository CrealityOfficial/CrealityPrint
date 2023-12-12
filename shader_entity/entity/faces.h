#ifndef QTUSER_3D_FACES_1595583144387_H
#define QTUSER_3D_FACES_1595583144387_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
namespace qtuser_3d
{	
	class XRenderPass;
	class SHADER_ENTITY_API Faces : public XEntity
	{
		Q_OBJECT
	public:
		Faces(Qt3DCore::QNode* parent = nullptr);
		virtual ~Faces();
		void setColor(const QVector4D& color);
	protected:
		Qt3DRender::QParameter* m_colorParameter;
		XRenderPass* m_renderPass;
	};
}
#endif // QTUSER_3D_FACES_1595583144387_H