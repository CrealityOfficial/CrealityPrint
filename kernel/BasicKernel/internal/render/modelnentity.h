#ifndef QTUSER_3D_MODELNENTITY_1595161543232_H
#define QTUSER_3D_MODELNENTITY_1595161543232_H
#include "qtuser3d/refactor/xentity.h"
#include "entity/pureentity.h"
#include "qtuser3d/math/box3d.h"
#include "trimesh2/Vec.h"
#include <Qt3DRender/QTexture>

namespace qtuser_3d 
{
	class BoxEntity;
}

namespace creative_kernel
{
	class ModelNEntity : public qtuser_3d::XEntity
	{
		Q_OBJECT
	public:
		ModelNEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~ModelNEntity();

		void update();
		void setBoxVisibility(bool visible);
		void updateBoxLocal(const qtuser_3d::Box3D& box, const QMatrix4x4& parentMatrix);
		void updateLines(const std::vector<trimesh::vec3>& lines);
		void setBoxColor(QVector4D color);

		void setState(float state);
		float getState();
		void setErrorState(bool error);
		void setVertexBase(QPoint vertexBase);
		void setSupportCos(float supportCos);
		void enterSupportStatus();
		void leaveSupportStatus();
		void setFanZhuan(int fz);
		
		// 自定义颜色，当 state 值大于 5 时生效
		void setCustomColor(QColor color);
		QColor getCustomColor();

		void setTransparency(float alpha);
		void setLightingEnable(bool enable);

		void setNozzle(float nozzle);

		void setRenderMode(int mode);

		void setTDiffuse(Qt3DRender::QTexture2D* aDiffuse);
		void setTAmbient(Qt3DRender::QTexture2D* aAmbient);
		void setTSpecular(Qt3DRender::QTexture2D* aSpecular);
		void setTNormal(Qt3DRender::QTexture2D* aNormal);

		void setUseVertexColor(bool used);
	protected:
		qtuser_3d::BoxEntity* m_boxEntity;
		qtuser_3d::PureEntity* m_lineEntity;

		Qt3DRender::QParameter* m_stateParameter;
		Qt3DRender::QParameter* m_vertexBaseParameter;
		Qt3DRender::QParameter* m_errorParameter;

		Qt3DRender::QParameter* m_customColorParameter;
		Qt3DRender::QParameter* m_transparencyParameter;
		Qt3DRender::QParameter* m_lightingFlagParameter;

		Qt3DRender::QParameter* m_supportCosParameter;
		Qt3DRender::QParameter* m_hoverParameter;
		Qt3DRender::QParameter* m_fanzhuanParameter;

		Qt3DRender::QParameter* m_nozzleParameter;
		//Qt3DRender::QParameter* m_zlocal;

		Qt3DRender::QParameter* m_renderModeParameter;

		Qt3DRender::QParameter* m_textureDiffuse;
		Qt3DRender::QParameter* m_textureAmbient;
		Qt3DRender::QParameter* m_textureSpecular;
		Qt3DRender::QParameter* m_textureNormal;

		Qt3DRender::QParameter* m_useVertexColor;
	};
}
#endif // QTUSER_3D_MODELNENTITY_1595161543232_H