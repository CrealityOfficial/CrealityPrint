#ifndef PLATE_ENTITY_202311131528_H
#define PLATE_ENTITY_202311131528_H

#include "basickernelexport.h"
#include "qtuser3d/refactor/xentity.h"
#include "platetexturecomponententity.h"

namespace qtuser_3d {
	class PrinterGrid;
	class Box3D;
	class SimplePickable;
}

namespace creative_kernel {

	class PlatePureComponentEntity;
	class PlateTextureComponentEntity;
	class PlateIconComponentEntity;
	
	class BASIC_KERNEL_API PlateEntity : public qtuser_3d::XEntity
	{
	public:
		PlateEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PlateEntity();

		void setSize(const QSizeF size);

		void setTheme(int theme);

		void setGridColor(const QVector4D clr);
		void updateBounding(qtuser_3d::Box3D& box);

		void setHighlight(bool highlight);

		void setName(const std::string& str);
		void setModelNumber(const std::string& str);

		void setClickCallback(OnClickFunc call);
	protected:
		void setupGroundGeometry(const QSizeF size);
		void setupInterlayerGeometry(const QSizeF size);
		void setupUpperGeometry(const QSizeF size);

		void createTopRightEntities();
		void adjustTopRightEntities(const QSizeF size);

		void createHandleEntities();
		void adjustHandleEntities(const QSizeF size);

		void createTopLeftLabel();
		void adjustTopLeftLabel(const QSizeF size);

		void createTopRightLabel();
		void adjustTopRightLabel(const QSizeF size);

		void tracePickables();
		void untracePickables();


		QString qrcPathOfName(const std::string& name);

	protected:

		QSizeF m_size;

		PlatePureComponentEntity* m_groundEntity;

		PlatePureComponentEntity* m_interlayerEntity;

		qtuser_3d::PrinterGrid* m_printerGrid;

		PlateTextureComponentEntity* m_upperEntity;

		PlateIconComponentEntity* m_closeEntity;
		PlateIconComponentEntity* m_autoLayoutEntity;
		PlateIconComponentEntity* m_pickBottomEntity;
		PlateIconComponentEntity* m_lockEntity;
		PlateIconComponentEntity* m_settingEntity;


		PlateTextureComponentEntity* m_leftHandleEntity;
		PlateTextureComponentEntity* m_rightHandleEntity;

		PlateIconComponentEntity* m_topLeftLabel;
		QSizeF m_topLeftLabelSize;

		PlateTextureComponentEntity* m_topRightLabel;
		QSizeF m_topRightLabelSize;

		int m_theme;
		bool m_highlight;
	};
}



#endif // !PLATE_ENTITY_202311131528_H
