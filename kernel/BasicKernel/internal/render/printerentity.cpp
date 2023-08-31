#include "printerentity.h"

namespace creative_kernel
{
	PrinterEntity::PrinterEntity(Qt3DCore::QNode* parent)
		: QEntity(parent)
		, m_printerText(nullptr)
	{
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

		m_printerSkirt = new qtuser_3d::PrinterSkirtEntity(this);
		m1.setToIdentity();
		m_printerSkirt->setPose(m1);

		//m_printerText = new PrinterText(this);
		m_printerGrid = new qtuser_3d::PrinterGrid(this);
		m_printerGrid->setLineColor(greyColor);
		m1.translate(0.0f, 0.0f, -0.05f);
		m_printerGrid->setPose(m1);
		
		m_bottom = new qtuser_3d::TexFaces(this);
		m1.translate(0.0f, 0.0f, -0.05f);
		m_bottom->setPose(m1);
		
		m_faceEntity = new qtuser_3d::FaceEntity(this);
		
		m_hotbed = new qtuser_3d::HotbedEntity(nullptr);

		m_bShowEntity = true;
	}

	PrinterEntity::~PrinterEntity()
	{
	}

	void PrinterEntity::updateBox(const qtuser_3d::Box3D& box)
	{
		qtuser_3d::Box3D globalBox = box;
		m_boxEntity->updateGlobal(globalBox, false);
		m_printerSkirt->updateBoundingBox(globalBox);
		m_printerGrid->updateBounding(globalBox, 1);
		//m_printerText->updateLen(globalBox, 10.0f, 4);
		//m_faceEntity->drawFace(globalBox);

		//m_imageEntity->updateGlobal(globalBox);
		m_bottom->updateBox(box);
	}

	void PrinterEntity::onModelChanged(qtuser_3d::Box3D basebox, bool hasModel, 
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
	}

	void PrinterEntity::enableSkirt(bool enable)
	{
		if(m_bShowEntity)
			m_printerSkirt->setEnabled(enable);
	}

	void PrinterEntity::setSkirtHighlight(bool highlight)
	{
		m_printerSkirt->setHighlight(highlight);
	}

	void PrinterEntity::visibleSubGrid(bool visible)
	{

	}

	void PrinterEntity::updateFace(qtuser_3d::Box3D& box, qtuser_3d::faceType type)
	{
		m_faceEntity->updateFace(box, type);
	}

	void PrinterEntity::setVisibility(int type, bool visibility)
	{
		m_faceEntity->setVisibility(type, visibility);
	}

	void PrinterEntity::drawBedFaces(qtuser_3d::bedType _bedType)
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

	}

	void PrinterEntity::showPrinterEntity(bool isShow)
	{
		m_bShowEntity = isShow;
		m_boxEntity->setEnabled(isShow);//蓝色边框
		m_printerSkirt->setEnabled(isShow);//灰色边线
		//m_printerText->setEnabled(isShow);//刻度
		m_printerGrid->setEnabled(isShow);//网格线
		m_axisEntity->setEnabled(false);//坐标指示
		//m_imageEntity->setEnabled(isShow);//logo
		m_bottom->setEnabled(isShow);
	}

	void PrinterEntity::updatePrinterColor(const PrinterColorConfig& config)
	{
		m_printerSkirt->setInnerColor(config.skirtInner);
		m_printerSkirt->setOuterColor(config.skirtOuter);
		m_printerSkirt->setVerticalBottomColor(config.skirtVerticalBottom);

		m_bottom->setColor(config.bottom);
		m_printerGrid->setLineColor(config.gridLine);
		m_boxEntity->setColor(config.box);
	}
}
