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

	qtuser_3d::XEffect* getCachedModelWireFrameEffect()
	{
		return getKernel()->reuseableCache()->getCachedModelWireFrameEffect();
	}

	qtuser_3d::XEffect* getCachedCaptureModelWireFrameEffect()
	{
		return getKernel()->reuseableCache()->getCachedCaptureModelWireFrameEffect();
	}

	qtuser_3d::XEffect* getCachedCaptureModelEffect()
	{
		return getKernel()->reuseableCache()->getCachedCaptureModelEffect();
	}

	qtuser_3d::XEffect* getCachedTransparentModelEffect()
	{
		return getKernel()->reuseableCache()->getCachedTransparentModelEffect();
	}

	qtuser_3d::XEffect* getCachedPreviewModelEffect()
	{
		return getKernel()->reuseableCache()->getCachedPreviewModelEffect();
	}

	qtuser_3d::XEffect* getCacheModelLayerEffect()
	{
		return getKernel()->reuseableCache()->getCacheModelLayerEffect();
	}

    void setModelEffectClipBottomZ(float bottomZ)
    {
        getKernel()->reuseableCache()->setVisibleBottomHeight(bottomZ);
		requestVisUpdate();
    }
	
	void setModelEffectClipBottomZSceneBottom() {
		trimesh::dbox3 box = sceneBoundingBox();
		setModelEffectClipBottomZ(box.min.z);
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
		trimesh::dbox3 box = sceneBoundingBox();
		setModelEffectClipMaxZ((float)box.max.z + 0.5f);
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

	void setModelOffset(const QVector3D& offset)
	{
		getKernel()->reuseableCache()->setOffset(offset);
	}

	void setZProjectEnabled(bool enabled)
	{
		getKernel()->reuseableCache()->setZProjectEnabled(enabled);
	}

	void setPrinterVisible(bool visible)
	{
		getKernel()->reuseableCache()->setPrinterVisible(visible);
	}

	qtuser_3d::WorldIndicatorEntity* getIndicatorEntity()
	{
		return getKernel()->reuseableCache()->getIndicatorEntity();
	}

	void updatePrinterBox(const trimesh::dbox3& box)
	{
		return getKernel()->reuseableCache()->updatePrinterBox(box);
	}

	qtuser_3d::CacheTexture* getCacheTexture()
	{
		return getKernel()->reuseableCache()->getCacheTexture();
	}

	void resetIndicatorAngle()
	{
		getKernel()->reuseableCache()->resetIndicatorAngle();
	}
}
