#include "reuseableinterface.h"
#include "external/kernel/kernel.h"
#include "external/kernel/reuseablecache.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/camera/screencamera.h"

#include "internal/render/printerentity.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"
#include "internal/parameter/parametermanager.h"

namespace creative_kernel
{
	PrinterEntity* getCachedPrinterEntity()
	{
		return getKernel()->reuseableCache()->getCachedPrinterEntity();
	}

	void cacheReuseable(Qt3DCore::QNode* parent)
	{
		getKernel()->reuseableCache()->setParent(parent);
	}

	Qt3DRender::QCamera* getCachedCameraEntity()
	{
		return getKernel()->reuseableCache()->mainScreenCamera()->camera();
	}

	qtuser_3d::XEffect* getCachedModelEffect()
	{
		return getKernel()->reuseableCache()->getCachedModelEffect();
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

	void setModelEffectBottomZ(float bottomZ)
	{
		getKernel()->reuseableCache()->setBottom(bottomZ);
	}
	
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
		setModelEffectClipMaxZ(box.max.z());
	}

	void setModelZProjectColor(const QVector4D& color)
	{
		getKernel()->reuseableCache()->setModelZProjectColor(color);
	}

	void setModelClearColor(const QVector4D& color)
	{
		getKernel()->reuseableCache()->setModelClearColor(color);
	}

	void setPrinterVisible(bool visible)
	{
		getKernel()->reuseableCache()->setPrinterVisible(visible);
	}

	cxkernel::WorldIndicatorEntity* getIndicatorEntity()
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
}
