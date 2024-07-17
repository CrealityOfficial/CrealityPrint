#ifndef SIMPLE_PLATE_ENTITY_202401031005_H
#define SIMPLE_PLATE_ENTITY_202401031005_H

#include "basickernelexport.h"
#include "qtuser3d/refactor/xentity.h"


namespace qtuser_3d {
	class PrinterGrid;
	class PrinterSkirtEntity;
	class TexFaces;
	class Box3D;
}

namespace creative_kernel {
	struct PrinterColorConfig;

	class BASIC_KERNEL_API SimplePlateEntity : public qtuser_3d::XEntity
	{
	public:
		SimplePlateEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~SimplePlateEntity();

		void updateBounding(qtuser_3d::Box3D& box);
		void setHighlight(bool highlight);

		void updatePrinterColor(const PrinterColorConfig& config);

		void enableSkirt(bool enable);
	private:
		qtuser_3d::PrinterGrid* m_printerGrid;
		qtuser_3d::PrinterSkirtEntity* m_printerSkirt;
		qtuser_3d::TexFaces* m_bottom;
	};
}

#endif //SIMPLE_PLATE_ENTITY_202401031005_H