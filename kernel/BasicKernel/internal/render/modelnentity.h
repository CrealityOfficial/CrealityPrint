#ifndef QTUSER_3D_MODELNENTITY_1595161543232_H
#define QTUSER_3D_MODELNENTITY_1595161543232_H
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/math/box3d.h"
#include "trimesh2/Vec.h"
#include <Qt3DRender/QTexture>

namespace qtuser_3d 
{
	class BoxEntity;
	class NozzleRegionEntity;
	class PureEntity;
}

namespace creative_kernel
{
	class ModelNEntity : public qtuser_3d::XEntity
	{
		Q_OBJECT
	public:
		ModelNEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~ModelNEntity();

		
		void setBoxEnabled(bool enabled);
		void setBoxVisibility(bool visible);
		void updateBoxLocal(const qtuser_3d::Box3D& box, const QMatrix4x4& parentMatrix);
		void setBoxColor(QVector4D color);

		/*
		state:none hover clicked preview
		*/
		void setState(int state);
		int getState();

		void setVertexBase(QPoint vertexBase);
		void setSupportCos(float supportCos);
		void enterSupportStatus();
		void leaveSupportStatus();

		void setTransparency(float alpha);
		void setLightingEnable(bool enable);

		void setNozzle(int nozzle);

		void setTDiffuse(Qt3DRender::QTexture2D* aDiffuse);
		void setTAmbient(Qt3DRender::QTexture2D* aAmbient);
		void setTSpecular(Qt3DRender::QTexture2D* aSpecular);
		void setTNormal(Qt3DRender::QTexture2D* aNormal);

		void setAssociatePrinterBox(const qtuser_3d::Box3D& box);

		//print by object
		void setLinesPose(const QMatrix4x4& pose);
		void updateLines(const std::vector<trimesh::vec3>& lines);
		void setOuterLinesColor(const QVector4D& color);
		void setOuterLinesVisibility(bool visible);
		void setNozzleRegionVisibility(bool visible);

		void setLayerHeightProfile(const std::vector<double>& layers, float minLayerHeight, float maxLayerHeight, float uniqueLayerHeight);
		void setLayersTexture(Qt3DRender::QTexture2D* tex);

	protected:

		qtuser_3d::BoxEntity* m_boxEntity;
		qtuser_3d::NozzleRegionEntity* m_nozzleRegionEntity;
		qtuser_3d::PureEntity* m_lineEntity;

		bool m_nozzleRegionVisible{ false };
		bool m_lineEntityVisible{ false };

		Qt3DRender::QParameter* m_stateParameter;

		Qt3DRender::QParameter* m_vertexBaseParameter;
		
		Qt3DRender::QParameter* m_transparencyParameter;
		Qt3DRender::QParameter* m_lightingFlagParameter;

		Qt3DRender::QParameter* m_supportCosParameter;
		Qt3DRender::QParameter* m_hoverParameter;
		
		Qt3DRender::QParameter* m_nozzleParameter;

		Qt3DRender::QParameter* m_textureDiffuse;
		Qt3DRender::QParameter* m_textureAmbient;
		Qt3DRender::QParameter* m_textureSpecular;
		Qt3DRender::QParameter* m_textureNormal;

		//for valiable layer height
		Qt3DRender::QParameter* m_layersTextureParameter;
		Qt3DRender::QParameter* m_modelHeightParameter;
	};
}
#endif // QTUSER_3D_MODELNENTITY_1595161543232_H
