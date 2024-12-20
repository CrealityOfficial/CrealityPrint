#ifndef _PRINTERENTITY_USE_1588651416007_H
#define _PRINTERENTITY_USE_1588651416007_H
#include "data/header.h"

#include <Qt3DCore/QEntity>
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/math/box3d.h"
#include "entity/faceentity.h"
#include "entity/hotbedentity.h"

namespace qtuser_3d
{
	class BoxEntity;
	class AxisEntity;
}

namespace creative_kernel
{
	class Printer;
	class WipeTowerEntity;
	class PlateEntity;
	class SimplePlateEntity;

	struct PrinterColorConfig
	{
		QVector4D skirt;
		QVector4D bottom;
		QVector4D bottomLog;
		QVector4D gridLine;
		QVector4D gridX;
		QVector4D grideY;
		QVector4D box;
		QVector4D skirtInner;
		QVector4D skirtOuter;
		QVector4D skirtVerticalBottom;
	};

	class BASIC_KERNEL_API PrinterEntity : public qtuser_3d::XEntity
	{
	public:
		PrinterEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PrinterEntity();

		void setPrinter(Printer* printer);

		void updateBox(const qtuser_3d::Box3D& box);

		//void onModelChanged(qtuser_3d::Box3D basebox, bool hasModel, 
		//	bool bleft, bool bright, bool bfront, bool bback, bool bup, bool bdown);

		void showPrinterEntity(bool isShow);

		void enableSkirt(bool enable);
		void setSkirtHighlight(bool highlight);

		//void updateFace(qtuser_3d::Box3D& box, qtuser_3d::faceType type);
		//void setVisibility(int type, bool visibility);

		//void drawBedFaces(qtuser_3d::bedType _bedType);
		void updatePrinterColor(const PrinterColorConfig& config);
		void setTheme(int theme);

		void setIndex(int idx);

		//void onCheckBed(QList<qtuser_3d::Box3D>& boxes);

		void switchPrinterStyle(bool isSimple);

		PlateEntity* plateEntity();
		WipeTowerEntity* wipeTowerEntity();

	protected:
		qtuser_3d::BoxEntity* m_boxEntity;
		
		qtuser_3d::AxisEntity* m_axisEntity;
		//qtuser_3d::FaceEntity* m_faceEntity;
		
		bool m_bShowEntity;

		//qtuser_3d::HotbedEntity* m_hotbed;
		
		PlateEntity* m_plateEntity;
		SimplePlateEntity* m_simplePlateEntity;

		WipeTowerEntity* m_wipeTowerEntity;
	};

}

#endif // _PRINTERENTITY_USE_1588651416007_H
