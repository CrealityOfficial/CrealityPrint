#ifndef CREATIVE_KERNEL_REUSEABLEINTERFACE_1594457885812_H
#define CREATIVE_KERNEL_REUSEABLEINTERFACE_1594457885812_H
#include "basickernelexport.h"
#include "data/kernelenum.h"
#include <Qt3DRender/QCamera>
#include "qtuser3d/math/box3d.h"
#include "qtuser3d/trimesh2/conv.h"

#include "data/kernelmacro.h"
namespace qtuser_3d
{
	class XEffect;
	class WorldIndicatorEntity;
	class CacheTexture;
}

namespace creative_kernel
{
	qtuser_3d::WorldIndicatorEntity* getIndicatorEntity();

	BASIC_KERNEL_API void cacheReuseable(Qt3DCore::QNode* parent);
	BASIC_KERNEL_API Qt3DRender::QCamera* getCachedCameraEntity();
	BASIC_KERNEL_API qtuser_3d::XEffect* getCachedModelEffect();
	BASIC_KERNEL_API qtuser_3d::XEffect* getCachedModelWireFrameEffect();
	BASIC_KERNEL_API qtuser_3d::XEffect* getCachedCaptureModelWireFrameEffect();
	BASIC_KERNEL_API qtuser_3d::XEffect* getCachedCaptureModelEffect();
	BASIC_KERNEL_API qtuser_3d::XEffect* getCachedTransparentModelEffect();
	BASIC_KERNEL_API qtuser_3d::XEffect* getCachedPreviewModelEffect();
	BASIC_KERNEL_API qtuser_3d::XEffect* getCacheModelLayerEffect();

    BASIC_KERNEL_API void setModelEffectClipBottomZ(float bottomZ = 0.0f);
	BASIC_KERNEL_API void setModelEffectClipBottomZSceneBottom();
	BASIC_KERNEL_API void setNeedCheckScope(int checkscope);

	BASIC_KERNEL_API void setModelEffectClipMaxZ(float z = 100000.0f);
	BASIC_KERNEL_API void setModelEffectBox(const QVector3D& dmin, const QVector3D& dmax);
	BASIC_KERNEL_API void setModelEffectClipMaxZSceneTop();

	BASIC_KERNEL_API void setModelZProjectColor(const QVector4D& color);
	BASIC_KERNEL_API void setModelClearColor(const QVector4D& color);

	BASIC_KERNEL_API void setModelSection(const QVector3D &frontPos, const QVector3D &backPos, const QVector3D &normal);
	BASIC_KERNEL_API void resetModelSection();

	BASIC_KERNEL_API void setModelOffset(const QVector3D& offset);

	BASIC_KERNEL_API void setZProjectEnabled(bool enabled);

	// printer entity
	BASIC_KERNEL_API void setPrinterVisible(bool visible);

	BASIC_KERNEL_API void updatePrinterBox(const trimesh::dbox3& box);

	BASIC_KERNEL_API qtuser_3d::CacheTexture* getCacheTexture();

	BASIC_KERNEL_API void resetIndicatorAngle();
}
#endif // CREATIVE_KERNEL_REUSEABLEINTERFACE_1594457885812_H
