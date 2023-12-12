#ifndef CREATIVE_KERNEL_REUSEABLECACHE_1594457868780_H
#define CREATIVE_KERNEL_REUSEABLECACHE_1594457868780_H
#include "basickernelexport.h"
#include <Qt3DCore/QEntity>
#include "qtuser3d/refactor/xeffect.h"
#include "data/kernelmacro.h"
#include "qtuser3d/math/box3d.h"

namespace qtuser_3d
{
	class ScreenCamera;
	class WorldIndicatorEntity;
}

namespace creative_kernel
{
	class PrinterEntity;
	class BASIC_KERNEL_API ReuseableCache : public Qt3DCore::QNode
	{
		Q_OBJECT
	public:
		ReuseableCache(Qt3DCore::QNode *parent = nullptr);
		virtual ~ReuseableCache();

		void setPrinterVisible(bool visible);

		PrinterEntity *getCachedPrinterEntity();
		qtuser_3d::WorldIndicatorEntity *getIndicatorEntity();

		qtuser_3d::ScreenCamera *mainScreenCamera();
		qtuser_3d::XEffect *getCachedModelEffect();
		qtuser_3d::XEffect *getCachedSupportEffect();

		void setModelZProjectColor(const QVector4D &color);
		void setModelClearColor(const QVector4D &color);
		void setSpaceBox(const QVector3D &minspace, const QVector3D &maxspace);
		void setBottom(float bottom);
		void setVisibleBottomHeight(float bottomHeight);
		void setVisibleTopHeight(float topHeight);
		void setNeedCheckScope(int checkscope);
		void resetSection();
		void setSection(const QVector3D &frontPos, const QVector3D &backPos, const QVector3D &normal);

		Q_INVOKABLE void setIndicatorScreenPos(const QPoint &p, float length = 100.0);
		Q_INVOKABLE void resetIndicatorAngle();

		void intialize();
		void blockRelation();

		void updatePrinterBox(const qtuser_3d::Box3D &box);

	protected:
		PrinterEntity *m_printerEntity;

		qtuser_3d::WorldIndicatorEntity *m_indicator;
		qtuser_3d::ScreenCamera *m_mainCamera;

		qtuser_3d::XEffect *m_modelEffect;
		qtuser_3d::XEffect *m_supportEffect;
	};
}
#endif // CREATIVE_KERNEL_REUSEABLECACHE_1594457868780_H