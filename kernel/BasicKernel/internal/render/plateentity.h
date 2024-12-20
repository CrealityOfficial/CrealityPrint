#ifndef PLATE_ENTITY_202311131528_H
#define PLATE_ENTITY_202311131528_H

#include "basickernelexport.h"
#include "qtuser3d/refactor/xentity.h"
#include "platetexturecomponententity.h"
#include <Qt3DRender/QTexture>

namespace qtuser_3d {
	class PrinterGrid;
	class Box3D;
}

namespace creative_kernel {

	class Printer;
	class PlatePureComponentEntity;
	class PlateTextureComponentEntity;
	class PlateIconComponentEntity;

	enum class PlateViewMode {
		EDIT = 0,
		PREVIEW = 1,
	};

	enum class PlateStyle {
		Custom_Plate = 0,
		Textured_PEI_Plate,
		Smooth_PEI_Plate,
		Epoxy_Resin_Plate,
	};

	class BASIC_KERNEL_API PlateEntity : public qtuser_3d::XEntity
	{
		Q_OBJECT
	public:
		PlateEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PlateEntity();

		void setPrinter(Printer* printer);

		int theme();
		void setTheme(int theme);

		//void setGridColor(const QVector4D clr);
		void updateBounding(qtuser_3d::Box3D& box);

		bool selected();
		void setSelected(bool selected);

		const QString& name();
		void setName(const QString& str);
		void setupNameTextureAndGeometry();
		bool exitTopLeftLabel(); //label to show name

		void setModelNumber(const QString& str);

		void setOrder(int order);
		int order();
		int bitsOfOrderToShow(int order);

		bool lock();
		void setLock(bool lock);

		void setClickCallback(OnClickFunc call);

		PlateViewMode viewModel();
		void setViewMode(PlateViewMode mode);

		void resetGroundGeometry();

		void setCloseEntityEnable(bool enable);

		void setPlateStyle(PlateStyle style);

	protected:

		void setSelected(bool selected, bool force);
		void setSize(const QSizeF size);

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

		std::string pngNameOfIcon(const std::string& icon, bool select, bool hover);
		QString qrcPathOfName(const std::string& name);
		QString qImagePathOfName(const std::string& name);

		QFont useFont();

		Qt3DRender::QTexture2D* getTexture(const QString& key);

	private:
		QSizeF m_size;
		QSizeF m_nameContainSize;

		trimesh::vec3 m_origin;

		PlatePureComponentEntity* m_groundEntity;

		PlateIconComponentEntity* m_interlayerEntity;

		qtuser_3d::PrinterGrid* m_printerGrid;

		PlateTextureComponentEntity* m_upperEntity;

		PlateIconComponentEntity* m_closeEntity;
		PlateIconComponentEntity* m_autoLayoutEntity;
		PlateIconComponentEntity* m_pickBottomEntity;
		PlateIconComponentEntity* m_lockEntity;
		PlateIconComponentEntity* m_settingEntity;
		PlateTextureComponentEntity* m_numberEntity;

		PlateTextureComponentEntity* m_leftHandleEntity;
		PlateTextureComponentEntity* m_rightHandleEntity;

		PlateIconComponentEntity* m_topLeftLabel;

		PlateTextureComponentEntity* m_topRightLabel;
		QSizeF m_topRightLabelSize;

		int m_theme;
		//bool m_highlight;   //��ӡ��ƽ̨����ģ�ͻ�����ɫ��

		int m_order;
		bool m_selected;   //��ӡ���Ƿ�ѡ��

		bool m_lock;

		bool m_closeEnable;

		PlateViewMode m_mode;
		PlateStyle m_style;

		QString m_name;
		QString m_modelNumber;

		float m_nameWidth { 0 };

		OnClickFunc m_callback;

		friend class Printer;
	};
}



#endif // !PLATE_ENTITY_202311131528_H
