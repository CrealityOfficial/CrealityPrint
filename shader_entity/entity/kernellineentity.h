#ifndef CXKERNEL_LINEENTITY_1594892276547_H
#define CXKERNEL_LINEENTITY_1594892276547_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include <QtCore/QVector>
#include <QtGui/QColor>
#include <QtGui/QVector3D>
#include <Qt3DRender/QLineWidth>

namespace qtuser_3d
{
	class ColorEffect;
	class PureXEffect;

	class SHADER_ENTITY_API KernelLineEntity : public qtuser_3d::XEntity
	{
		Q_OBJECT
	public:
		KernelLineEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~KernelLineEntity();

		void setLineWidth(float width);
		void updateGeometry(const QVector<QVector3D>& positions, const QVector<QVector4D>& colors, bool loop = false);
		void updateGeometry(const QVector<QVector3D>& positions, bool loop = false);
		void updateGeometry(int pointsNum, float* positions, float* color = nullptr, bool loop = false);
		void updateGeometry(Qt3DRender::QGeometry* geometry, bool color = false, bool loop = false);
		void setColor(const QVector4D& color);
	protected:
		Qt3DRender::QParameter* m_colorParameter;
		bool m_usePure;
		ColorEffect* m_colorEffect;
		PureXEffect* m_pureEffect;

		Qt3DRender::QLineWidth* m_lineWidth;
	};
}
#endif // CXKERNEL_LINEENTITY_1594892276547_H