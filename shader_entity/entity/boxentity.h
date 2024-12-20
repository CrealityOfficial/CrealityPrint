#ifndef QTUSER_3D_BOXENTITY_1594866500149_H
#define QTUSER_3D_BOXENTITY_1594866500149_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/math/box3d.h"

namespace qtuser_3d
{	
	class XRenderPass;
	class SHADER_ENTITY_API BoxEntity : public XEntity
	{
		Q_OBJECT
	public:
		BoxEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~BoxEntity();

		void updateGlobal(const Box3D& box, bool need_bottom = true);
		void updateLocal(const Box3D& box, const QMatrix4x4& parentMatrix);
		void update(const Box3D& box, const QMatrix4x4& parentMatrix);
		void setColor(const QVector4D& color);
		void setLineWidth(float width);
	protected:
		Qt3DRender::QParameter* m_colorParameter;
		XRenderPass* m_viewPass;
	};
}
#endif // QTUSER_3D_BOXENTITY_1594866500149_H