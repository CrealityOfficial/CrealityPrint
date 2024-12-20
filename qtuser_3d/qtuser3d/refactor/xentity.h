#ifndef QTUSER_3D_XENTITY_1679973441774_H
#define QTUSER_3D_XENTITY_1679973441774_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QMaterial>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QRenderState>

namespace qtuser_3d
{
	class XEffect;
	class QTUSER_3D_API XEntity : public Qt3DCore::QEntity
	{
		Q_OBJECT
	public:
		XEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~XEntity();

		Qt3DRender::QParameter* setParameter(const QString& name, const QVariant& value);
		void setEffect(XEffect* effect);
		XEffect* xeffect();

		void setPose(const QMatrix4x4& matrix);
		void setModelMatrix(const QMatrix4x4& matrix);

		void setGeometry(Qt3DRender::QGeometry* geometry, Qt3DRender::QGeometryRenderer::PrimitiveType type = Qt3DRender::QGeometryRenderer::Triangles,
			bool deleteOldGeometry = true);
		void replaceGeometryRenderer(Qt3DRender::QGeometryRenderer* geometryRenderer);

		void addRenderState(int index, Qt3DRender::QRenderState* state);
		void addPassFilter(int index, const QString& filter);
		void removePassFilter(int passIndex, const QString& filterName, const QVariant& filterValue);

		Qt3DRender::QGeometry* geometry();
		void updateAttribute(int index, const QByteArray& bytes);

		QMatrix4x4 pose();
	protected:
		Qt3DRender::QMaterial* m_material;
		Qt3DCore::QTransform* m_transform;
		Qt3DRender::QGeometryRenderer* m_geometryRenderer;
	};
}

#endif // QTUSER_3D_XENTITY_1679973441774_H