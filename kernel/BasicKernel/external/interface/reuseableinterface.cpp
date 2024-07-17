#include "reuseableinterface.h"
#include "external/kernel/kernel.h"
#include "external/kernel/reuseablecache.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/camera/screencamera.h"

#include "internal/render/printerentity.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"
#include "internal/parameter/parametermanager.h"
#include "entity/worldindicatorentity.h"

namespace creative_kernel
{
	void cacheReuseable(Qt3DCore::QNode* parent)
	{
		getKernel()->reuseableCache()->setParent(parent);
		getIndicatorEntity()->freshTextures();
	}

	Qt3DRender::QCamera* getCachedCameraEntity()
	{
		return getKernel()->reuseableCache()->mainScreenCamera()->camera();
	}

	qtuser_3d::XEffect* getCachedModelEffect()
	{
		return getKernel()->reuseableCache()->getCachedModelEffect();
	}

	qtuser_3d::XEffect* getCachedModelOffscreenEffect()
	{
		return getKernel()->reuseableCache()->getCachedModelOffscreenEffect();
	}

	qtuser_3d::XEffect* getCachedModelCombindEffect()
	{
		return getKernel()->reuseableCache()->getCachedModelCombindEffect();
	}

	qtuser_3d::XEffect* getCacheModelLayerEffect()
	{
		return getKernel()->reuseableCache()->getCacheModelLayerEffect();
	}

	qtuser_3d::XEffect* getCachedSupportEffect()
	{
		return getKernel()->reuseableCache()->getCachedSupportEffect();
	}

    void setModelEffectClipBottomZ(float bottomZ)
    {
        getKernel()->reuseableCache()->setVisibleBottomHeight(bottomZ);
		requestVisUpdate();
    }

	/*void setModelEffectBottomZ(float bottomZ)
	{
		getKernel()->reuseableCache()->setBottom(bottomZ);
	}*/
	
	void setModelEffectClipBottomZSceneBottom() {
		qtuser_3d::Box3D box = sceneBoundingBox();
		setModelEffectClipBottomZ(box.min.z());
	}

	void setNeedCheckScope(int checkscope)
	{
		getKernel()->reuseableCache()->setNeedCheckScope(checkscope);
	}

	void setModelEffectClipMaxZ(float z)
	{
		getKernel()->reuseableCache()->setVisibleTopHeight(z);
		requestVisUpdate();
	}
	
	void setModelEffectBox(const QVector3D& dmin, const QVector3D& dmax)
	{
		getKernel()->reuseableCache()->setSpaceBox(dmin, dmax);
	}
	
	void setModelEffectClipMaxZSceneTop() {
		qtuser_3d::Box3D box = sceneBoundingBox();
		setModelEffectClipMaxZ(box.max.z() + 0.5f);
	}

	void setPreviewEffectSpecificColorLayers(const QVariantList& specificColorLayers)
	{
		getKernel()->reuseableCache()->setSpecificColorLayers(specificColorLayers);
	}

	void setModelZProjectColor(const QVector4D& color)
	{
		getKernel()->reuseableCache()->setModelZProjectColor(color);
	}

	void setModelClearColor(const QVector4D& color)
	{
		getKernel()->reuseableCache()->setModelClearColor(color);
	}

	void setModelSection(const QVector3D &frontPos, const QVector3D &backPos, const QVector3D &normal)
	{
		getKernel()->reuseableCache()->setSection(frontPos, backPos, normal);
	}

	void resetModelSection()
	{
		getKernel()->reuseableCache()->resetSection();
	}

	void setModelOffset(const QVector2D& offset)
	{
		getKernel()->reuseableCache()->setOffset(offset);
	}

	void setPrinterVisible(bool visible)
	{
		getKernel()->reuseableCache()->setPrinterVisible(visible);
	}

	qtuser_3d::WorldIndicatorEntity* getIndicatorEntity()
	{
		return getKernel()->reuseableCache()->getIndicatorEntity();
	}

	void updatePrinterBox(const qtuser_3d::Box3D& box)
	{
		return getKernel()->reuseableCache()->updatePrinterBox(box);
	}

	void updatePrinterBox(float width, float depth, float height)
	{
		float _width = width > 1.0f ? width : 220;
		float _depth = depth > 1.0f ? depth : 220;
		float _height = height > 1.0f ? height : 250;

		qtuser_3d::Box3D box(QVector3D(0.0f, 0.0f, 0.0f), QVector3D(_width, _depth, _height));
		if (getKernel()->parameterManager()->currentMachineCenterIsZero())
		{
			float MAX_X = _width / 2.0;
			float MAX_Y = _depth / 2.0;
			box = qtuser_3d::Box3D(QVector3D(-MAX_X, -MAX_Y, 0), QVector3D(MAX_X, MAX_Y, _height));
		}
		return getKernel()->reuseableCache()->updatePrinterBox(box);
	}

	qtuser_3d::CacheTexture* getCacheTexture()
	{
		return getKernel()->reuseableCache()->getCacheTexture();
	}
}
