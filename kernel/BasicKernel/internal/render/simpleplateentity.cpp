#include "simpleplateentity.h"
#include "entity/printerskirtentity.h"
#include "entity/printergrid.h"
#include "entity/texfaces.h"
#include "printerentity.h"

namespace creative_kernel {

	SimplePlateEntity::SimplePlateEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		QMatrix4x4 m1;
		m1.translate(0.0f, 0.0f, -1.0f);
		m_printerSkirt = new qtuser_3d::PrinterSkirtEntity(this);
		m_printerSkirt->setPose(m1);

		QVector4D greyColor = QVector4D(0.309804f, 0.313725f, 0.325490f, 1.0f);
		m_printerGrid = new qtuser_3d::PrinterGrid(this);
		m_printerGrid->setLineColor(greyColor);
		m1.translate(0.0f, 0.0f, -0.05f);
		m_printerGrid->setPose(m1);

		m_bottom = new qtuser_3d::TexFaces(this);
		m1.translate(0.0f, 0.0f, -0.05f);
		m_bottom->setPose(m1);
	}

	SimplePlateEntity::~SimplePlateEntity()
	{
	}

	void SimplePlateEntity::updateBounding(qtuser_3d::Box3D& box)
	{
		m_printerSkirt->updateBoundingBox(box);
		m_printerGrid->updateBounding(box, 1);
		m_bottom->updateBox(box);
	}

	void SimplePlateEntity::setHighlight(bool highlight)
	{
		m_printerSkirt->setHighlight(highlight);
	}

	void SimplePlateEntity::updatePrinterColor(const PrinterColorConfig& config)
	{
		m_printerSkirt->setInnerColor(config.skirtInner);
		m_printerSkirt->setOuterColor(config.skirtOuter);
		m_printerSkirt->setVerticalBottomColor(config.skirtVerticalBottom);

		m_bottom->setColor(config.bottom);
	}

	void SimplePlateEntity::enableSkirt(bool enable)
	{
		m_printerSkirt->setEnabled(enable);
	}
}