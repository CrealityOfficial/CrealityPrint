#ifndef QTUSER_3D_MODELNENTITY_1595161543232_H
#define QTUSER_3D_MODELNENTITY_1595161543232_H
#include "basickernelexport.h"
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/math/box3d.h"
#include "trimesh2/Vec.h"
#include <Qt3DRender/QTexture>

namespace creative_kernel
{
	class BASICKERNEL_API ModelNEntity : public qtuser_3d::XEntity
	{
		Q_OBJECT
	public:		
		enum RenderType 
		{
			NoRender = 0,
			ViewRender,
			TransparentViewRender,
			Layer,
			PreviewRender
		};

		enum EntityType 
		{
			NormalEntity = 0,
			SpecialEntity
		};

		enum Scene
		{
			Prepare = 0,
			Preview,
			GcodePreview	
		};

		ModelNEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~ModelNEntity();

		void updateBoxLocal(const qtuser_3d::Box3D& box, const QMatrix4x4& parentMatrix);

		/* render mode */
		void updateRender();
		void setOffscreenRenderEnabled(bool enabled, bool applyNow = true);
		void setEntityType(int type, bool applyNow = true);
		void setVisible(bool visible, bool applyNow = true);
		bool isVisible();
		void useCustomEffect(qtuser_3d::XEffect* effect); 

		/* render */
		void setPalette(const QVariantList& colors);

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

		void setLayerHeightProfile(const std::vector<double>& layers, float minLayerHeight, float maxLayerHeight, float uniqueLayerHeight);
		void setLayersTexture(Qt3DRender::QTexture2D* tex);

		void setLayersColor(const QVariantList& layersColor);
		void setOffscreenRenderOffset(const QVector3D& offset);

		void useVariableLayerHeightEffect();
		void useDefaultModelEffect();

	private:
		int phase();
		void applyModelEffect();

	protected:		
		int m_type { 0 };
		int m_entityType { 0 };
		bool m_prepareVisible { true };	
		bool m_previewVisible { false };
		bool m_offscreenRenderEnabled { false };
		bool m_usingLayer { false };

		qtuser_3d::XEffect* m_customEffect{NULL};

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

		Qt3DRender::QParameter* m_paletteParameter;

		qtuser_3d::XEffect* m_modelOffscreenEffect;
		qtuser_3d::XEffect* m_modelCombindEffect;
	};
}
#endif // QTUSER_3D_MODELNENTITY_1595161543232_H
