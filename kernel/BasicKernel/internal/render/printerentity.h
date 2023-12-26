#ifndef _PRINTERENTITY_USE_1588651416007_H
#define _PRINTERENTITY_USE_1588651416007_H
#include "data/header.h"

#include <Qt3DCore/QEntity>
#include "qtuser3d/math/box3d.h"
#include "entity/boxentity.h"
#include "entity/printerskirtentity.h"
//#include "entity/printergrid.h"
#include "entity/printertext.h"
#include "entity/axisentity.h"
#include "entity/faceentity.h"
#include "entity/texfaces.h"
#include "entity/hotbedentity.h"
#include "plateentity.h"

namespace creative_kernel
{
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

	class BASIC_KERNEL_API PrinterEntity : public Qt3DCore::QEntity
	{
	public:
		PrinterEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PrinterEntity();

		void updateBox(const qtuser_3d::Box3D& box);

		void onModelChanged(qtuser_3d::Box3D basebox, bool hasModel, 
			bool bleft, bool bright, bool bfront, bool bback, bool bup, bool bdown);

		void showPrinterEntity(bool isShow);

		void enableSkirt(bool enable);
		void setSkirtHighlight(bool highlight);

		void updateFace(qtuser_3d::Box3D& box, qtuser_3d::faceType type);
		void setVisibility(int type, bool visibility);

		void drawBedFaces(qtuser_3d::bedType _bedType);
		void updatePrinterColor(const PrinterColorConfig& config);
		void setTheme(int theme);

		void onCheckBed(QList<qtuser_3d::Box3D>& boxes);
	protected:
		qtuser_3d::BoxEntity* m_boxEntity;
		//qtuser_3d::PrinterGrid* m_printerGrid;
		//qtuser_3d::PrinterSkirtEntity* m_printerSkirt;
		qtuser_3d::PrinterText* m_printerText;
		qtuser_3d::AxisEntity* m_axisEntity;
		qtuser_3d::FaceEntity* m_faceEntity;
		qtuser_3d::TexFaces* m_bottom;
		bool m_bShowEntity;

		qtuser_3d::HotbedEntity* m_hotbed;
		//ImageEntity* m_imageEntity;
		PlateEntity* m_plateEntity;
	};

}

#endif // _PRINTERENTITY_USE_1588651416007_H
