#ifndef _NULLSPACE_SLICEPREVIEWSCENE_1589876502240_H
#define _NULLSPACE_SLICEPREVIEWSCENE_1589876502240_H
#include "data/kernelenum.h"
#include "crslice/gcode/header.h"
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QParameter>

#include "trimesh2/Vec.h"
#include <unordered_map>

namespace qtuser_3d
{
	class PureColorEntity;
	class AlonePointEntity;
	class NozzleEntity;
}

namespace creative_kernel
{
	class SlicePreviewNode;
	class SlicePreviewScene : public Qt3DCore::QEntity
	{
		Q_OBJECT
	public:
		SlicePreviewScene(Qt3DCore::QNode* parent = nullptr);
		virtual ~SlicePreviewScene();

		void initialize();
		void clear();

		void setOnlyLayer(int layer);
		void unsetOnlyLayer();

		void setIndicatorVisible(bool isShow);
		void showGCodeType(int type, bool isShow, bool isCuraProducer);
		void setGCodeVisualType(gcode::GCodeVisualType type);

		void setRetractionVisible();
		void setZSeamsVisible();
		void setUnretractVisible();

		void setZseamRetractDis();

		SlicePreviewNode* previewNode();

		//fdm
		void setGeometry(Qt3DRender::QGeometry* geometry, Qt3DRender::QGeometryRenderer::PrimitiveType type, int vCountPerPatch = -1);
		void setRetractionGeometry(Qt3DRender::QGeometry* geometry, Qt3DRender::QGeometryRenderer::PrimitiveType type, int vCountPerPatch = -1);
		void setZSeamsGeometry(Qt3DRender::QGeometry* geometry, Qt3DRender::QGeometryRenderer::PrimitiveType type, int vCountPerPatch = -1);
		void setUnretractGeometry(Qt3DRender::QGeometry* geometry, Qt3DRender::QGeometryRenderer::PrimitiveType type, int vCountPerPatch = -1);


		void setRetractionGeometryRenderer(Qt3DRender::QGeometryRenderer* renderer);
		void setZSeamsGeometryRenderer(Qt3DRender::QGeometryRenderer *renderer);

		void setPreviewNodeModelMatrix(const QMatrix4x4& matrix);

		void visual();
		void setClipValue(const QVector4D& clipValue);
		void setIndicatorPos(const QVector3D& position);

		void setAnimation(int animation);
	
		void setLayerHeight(float height);
		void setLineWidth(float width);

	protected:
		float caculateAdaptivePointSize();
		void updatePointEntitySize();

	protected Q_SLOTS:
		void cameraProjectionMatrixChanged(const QMatrix4x4&);

	protected:
		SlicePreviewNode* m_slicePreviewNode;
		qtuser_3d::NozzleEntity* m_fdmIndicator;
#if 0
		qtuser_3d::PureColorEntity* m_retractionPoints;
		qtuser_3d::PureColorEntity* m_zSeamsPoints;
#else
		qtuser_3d::AlonePointEntity* m_retractionPoints;
		qtuser_3d::AlonePointEntity* m_zSeamsPoints;
		qtuser_3d::AlonePointEntity* m_unretractPoints;
#endif // 0

		bool m_showZseam;
		bool m_showRetraction;
		bool m_showUnretract;

		float m_layerHeight;
		float m_lineWidth;

		gcode::GCodeVisualType m_gcodeVisulaType;
	};
}

#endif // _NULLSPACE_SLICEPREVIEWSCENE_1589876502240_H

