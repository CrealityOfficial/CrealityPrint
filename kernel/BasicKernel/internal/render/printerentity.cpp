#include "printerentity.h"
#include "entity/boxentity.h"
#include "entity/axisentity.h"
#include "plateentity.h"
#include "simpleplateentity.h"
#include "interface/selectorinterface.h"
#include "wipetowerentity.h"

namespace creative_kernel
{
	PrinterEntity::PrinterEntity(Qt3DCore::QNode* parent)
		: XEntity(parent)
		, m_bShowEntity(true)
	{
		setObjectName("PrinterEntity");

		QVector4D greyColor = QVector4D(0.309804f, 0.313725f, 0.325490f, 1.0f);
		m_boxEntity = new qtuser_3d::BoxEntity(this);
		m_boxEntity->setColor(greyColor);

		m_axisEntity = new qtuser_3d::AxisEntity(this, 0);
		QMatrix4x4 m1;
		m1.translate(0.0f, 0.0f, 1.0f);
		m_axisEntity->setPose(m1);
		m_axisEntity->setXAxisColor(QVector4D(1.0f, 0.1098f, 0.1098f, 1.0f));
		m_axisEntity->setYAxisColor(QVector4D(0.247f, 0.933f, 0.1098f, 1.0f));
		m_axisEntity->setZAxisColor(QVector4D(0.4117f, 0.243f, 1.0f, 1.0f));

		m_simplePlateEntity = new SimplePlateEntity(this);
		
		m_plateEntity = new PlateEntity(this);
		
		//前后左右上
		//m_faceEntity = new qtuser_3d::FaceEntity(nullptr);
		
		//m_hotbed = new qtuser_3d::HotbedEntity(nullptr);

		m_wipeTowerEntity = new WipeTowerEntity(this);
		tracePickable(m_wipeTowerEntity->pickable());
	}

	PrinterEntity::~PrinterEntity()
	{
		unTracePickable(m_wipeTowerEntity->pickable());
	}
	
	void PrinterEntity::setPrinter(Printer* printer)
	{
		m_plateEntity->setPrinter(printer);
	}

	void PrinterEntity::updateBox(const qtuser_3d::Box3D& box)
	{
		{
			QVector3D size = box.size();
			float a, b, c;
			a = fmax(size.x() / size.y(), size.y() / size.x());
			b = fmax(size.y() / size.z(), size.z() / size.y());
			c = fmax(size.z() / size.x(), size.x() / size.z());

			float max = fmax(fmax(a, b), c);
			switchPrinterStyle(max >= 10.0f);
		}

		qtuser_3d::Box3D globalBox = box;
		m_boxEntity->updateGlobal(globalBox, false);
		
		m_simplePlateEntity->updateBounding(globalBox);

		m_plateEntity->updateBounding(globalBox);
		m_wipeTowerEntity->setPrinterBox(globalBox);
	}

	/*void PrinterEntity::onModelChanged(qtuser_3d::Box3D basebox, bool hasModel,
		bool bleft, bool bright, bool bfront, bool bback, bool bup, bool bdown)
	{
		if (bleft)
		{
			updateFace(basebox, qtuser_3d::faceType::left);
		}
		else
		{
			setVisibility((int)qtuser_3d::faceType::left, false);
		}
		if (bfront)
		{
			updateFace(basebox, qtuser_3d::faceType::front);
		}
		else
		{
			setVisibility((int)qtuser_3d::faceType::front, false);
		}
		//if (bdown)
		//{
		//	updateFace(basebox, qtuser_3d::faceType::down);
		//}
		//else
		//{
		//	setVisibility((int)qtuser_3d::faceType::down, false);
		//}
		if (bright)
		{
			updateFace(basebox, qtuser_3d::faceType::right);
		}
		else
		{
			setVisibility((int)qtuser_3d::faceType::right, false);
		}
		if (bback)
		{
			updateFace(basebox, qtuser_3d::faceType::back);
		}
		else
		{
			setVisibility((int)qtuser_3d::faceType::back, false);
		}
		if (bup)
		{
			updateFace(basebox, qtuser_3d::faceType::up);
		}
		else
		{
			setVisibility((int)qtuser_3d::faceType::up, false);
		}
	}

	void PrinterEntity::onCheckBed(QList<qtuser_3d::Box3D>& boxes)
	{
		m_hotbed->checkBed(boxes);
	}*/

	void PrinterEntity::enableSkirt(bool enable)
	{
		if (m_bShowEntity)
			m_simplePlateEntity->enableSkirt(enable);
	}

	void PrinterEntity::setSkirtHighlight(bool highlight)
	{
		m_simplePlateEntity->setHighlight(highlight);
		//m_plateEntity->setHighlight(highlight);
	}

	/*void PrinterEntity::updateFace(qtuser_3d::Box3D& box, qtuser_3d::faceType type)
	{
		m_faceEntity->updateFace(box, type);
	}

	void PrinterEntity::setVisibility(int type, bool visibility)
	{
		m_faceEntity->setVisibility(type, visibility);
	}*/

	/*void PrinterEntity::drawBedFaces(qtuser_3d::bedType _bedType)
	{
		if (_bedType != qtuser_3d::bedType::None)
		{
			m_hotbed->clearData();
			//m_hotbed->setParent((Qt3DCore::QNode*)nullptr);
			//m_hotbed = new qtuser_3d::HotbedEntity(nullptr);
			m_hotbed->drawFace(_bedType);
			m_hotbed->setParent(this);
		}
		else
		{
			m_hotbed->clearData();
			m_hotbed->setParent((Qt3DCore::QNode*)nullptr);
			//delete m_hotbed;
			//m_hotbed = nullptr;
		}

	}*/

	void PrinterEntity::showPrinterEntity(bool isShow)
	{
		m_bShowEntity = isShow;
		setEnabled(isShow);
	}

	void PrinterEntity::updatePrinterColor(const PrinterColorConfig& config)
	{
		m_simplePlateEntity->updatePrinterColor(config);
		
		m_boxEntity->setColor(config.box);

		//m_plateEntity->setGridColor(config.gridLine);
	}

	void PrinterEntity::setTheme(int theme)
	{
		m_plateEntity->setTheme(theme);
	}

	void PrinterEntity::setIndex(int idx)
	{
		m_axisEntity->setEnabled(idx == 0);
		m_plateEntity->setOrder(idx + 1);
	}

	PlateEntity* PrinterEntity::plateEntity()
	{
		return m_plateEntity;
	}

	void PrinterEntity::switchPrinterStyle(bool isSimple)
	{
		m_boxEntity->setEnabled(isSimple);
		m_simplePlateEntity->setEnabled(isSimple);
		m_plateEntity->setEnabled(!isSimple);
	}

	WipeTowerEntity* PrinterEntity::wipeTowerEntity()
	{
		return m_wipeTowerEntity;
	}
}
