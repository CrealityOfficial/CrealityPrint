#ifndef QTUSER_3D_LINEENTITY_1594892276547_H
#define QTUSER_3D_LINEENTITY_1594892276547_H

#include "shader_entity_export.h"
#include <QtCore/QVector>
#include <QtGui/QColor>
#include <QtGui/QVector3D>
#include <Qt3DRender/QLineWidth>

#include "qtuser3d/refactor/xentity.h"

namespace qtuser_3d
{
	class XRenderPass;
	class SHADER_ENTITY_API LineEntity : public XEntity
	{
		Q_OBJECT
	public:
		LineEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~LineEntity();

		void setLineWidth(float width);
		void updateGeometry(const QVector<QVector3D>& positions, const QVector<QVector4D>& colors, bool loop = false);
		void updateGeometry(const QVector<QVector3D>& positions, bool loop = false);
		void updateGeometry(int pointsNum, float* positions, float* color = nullptr, bool loop = false);
		void updateGeometry(Qt3DRender::QGeometry* geometry, bool color = false, bool loop = false);
		void setColor(const QVector4D& color);
	protected:
		Qt3DRender::QParameter* m_colorParameter;
		bool m_usePure;

		Qt3DRender::QLineWidth* m_lineWidth;

		XRenderPass* m_purePass;
		XRenderPass* m_colorPass;
	};
}
#endif // QTUSER_3D_LINEENTITY_1594892276547_H