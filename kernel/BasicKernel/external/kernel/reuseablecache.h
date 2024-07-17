#ifndef CREATIVE_KERNEL_REUSEABLECACHE_1594457868780_H
#define CREATIVE_KERNEL_REUSEABLECACHE_1594457868780_H
#include "basickernelexport.h"
#include <Qt3DCore/QEntity>
#include "qtuser3d/refactor/xeffect.h"
#include "data/kernelmacro.h"
#include "data/kernelenum.h"
#include "qtuser3d/math/box3d.h"

namespace qtuser_3d
{
	class ScreenCamera;
	class WorldIndicatorEntity;
	class CacheTexture;
}

namespace creative_kernel
{
	class PrinterEntity;
	class PrinterMangager;

	class BASIC_KERNEL_API ReuseableCache : public Qt3DCore::QNode
	{
		Q_OBJECT;
		Q_PROPERTY(QObject* printerManager READ getPrinterManagerObject CONSTANT);

	public:
		ReuseableCache(Qt3DCore::QNode *parent = nullptr);
		virtual ~ReuseableCache();

		void setPrinterVisible(bool visible);

		qtuser_3d::WorldIndicatorEntity *getIndicatorEntity();

		qtuser_3d::ScreenCamera *mainScreenCamera();
		qtuser_3d::XEffect *getCachedModelEffect();
		qtuser_3d::XEffect* getCachedModelOffscreenEffect();
		qtuser_3d::XEffect* getCachedModelCombindEffect();
		qtuser_3d::XEffect *getCachedSupportEffect();
		qtuser_3d::XEffect* getCacheModelLayerEffect();

		Qt3DCore::QNode* getCachedPrinterEntitiesRoot();
		PrinterMangager* getPrinterMangager();
		qtuser_3d::CacheTexture* getCacheTexture();

		void setModelZProjectColor(const QVector4D &color);
		void setModelClearColor(const QVector4D &color);
		void setSpaceBox(const QVector3D &minspace, const QVector3D &maxspace);
		void setBottom(float bottom);
		void setVisibleBottomHeight(float bottomHeight);
		void setVisibleTopHeight(float topHeight);
		void setNeedCheckScope(int checkscope);
		void resetSection();
		void setSection(const QVector3D &frontPos, const QVector3D &backPos, const QVector3D &normal);
		void setOffset(const QVector2D& offset);
		void setSpecificColorLayers(const QVariantList& specificColorLayers);

		Q_INVOKABLE void setIndicatorScreenPos(const QPoint &p, float length = 100.0);
		Q_INVOKABLE void resetIndicatorAngle();
		Q_INVOKABLE void setTopCameraAngle();
		Q_INVOKABLE void setBottomCameraAngle();
		Q_INVOKABLE void setLeftCameraAngle();
		Q_INVOKABLE void setRightCameraAngle();
		Q_INVOKABLE void setFrontCameraAngle();
		Q_INVOKABLE void setBackCameraAngle();
		Q_INVOKABLE void setCameraZoom(float scale);

		void intialize();
		void blockRelation();

		void updatePrinterBox(const qtuser_3d::Box3D &box);

		// void setOffscreenRenderEnabled(bool enabled);
		// void setHiddenRenderEnabled(bool enabled);

		int getModelVisualMode();
		void setModelVisualMode(ModelVisualMode mode);
		Qt3DRender::QParameter* renderModeParameter();
	private:
		QObject* getPrinterManagerObject();

	protected:
		Qt3DCore::QNode* m_printerEntitisRoot;
		PrinterEntity *m_printerEntity;
		PrinterMangager* m_printerMangager;

		qtuser_3d::WorldIndicatorEntity *m_indicator;
		qtuser_3d::ScreenCamera *m_mainCamera;

		qtuser_3d::XRenderPass* m_viewPass { NULL };
		qtuser_3d::XRenderPass* m_rtPass { NULL };

		qtuser_3d::XEffect *m_modelEffect;
		qtuser_3d::XEffect *m_modelOffscreenEffect;
		qtuser_3d::XEffect *m_modelCombindEffect;

		qtuser_3d::XEffect *m_supportEffect;
		qtuser_3d::XEffect* m_modelLayerEffect;

		Qt3DRender::QParameter* m_renderModeParameter;

		qtuser_3d::CacheTexture* m_cacheTexture;
	};
}
#endif // CREATIVE_KERNEL_REUSEABLECACHE_1594457868780_H
