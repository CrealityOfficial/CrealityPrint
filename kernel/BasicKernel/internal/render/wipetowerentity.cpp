#include "wipetowerentity.h"
#include "qtuser3d/refactor/xeffect.h"
#include "qtuser3d/geometry/boxcreatehelper.h"
#include "renderpass/colorrenderpass.h"

#include "qtuser3d/geometry/bufferhelper.h"
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "qtuser3d/geometry/boxgeometrydata.h"
#include "internal/parameter/parametermanager.h"
#include "interface/machineinterface.h"
#include "entity/boxentity.h"
#include "wipetowerpickable.h"
#include <QPolygonOffset>

using namespace qtuser_3d;

namespace creative_kernel {

	WipeTowerEntity::WipeTowerEntity(Qt3DCore::QNode* parent)
		:PickXEntity(parent)
		, m_position(QVector3D(0.0f, 0.0f, 0.0f))
		, m_boundingBoxEntity(nullptr)
		, m_boundingBoxVisible(false)
	{
		setObjectName("WipeTowerEntity");

		ColorRenderPass* pass = new ColorRenderPass(nullptr);
		pass->addFilterKeyMask("alpha", 0);
		pass->setPassCullFace(Qt3DRender::QCullFace::Back);
		pass->setPassBlend();
		pass->setPassDepthTest();

		Qt3DRender::QPolygonOffset* st = new Qt3DRender::QPolygonOffset(pass);
		st->setScaleFactor(0.9);
		pass->addRenderState(st);

		qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("purePickFace");
		pickPass->addFilterKeyMask("pick", 0);
		pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
		pickPass->setPassCullFace(Qt3DRender::QCullFace::Back);
		pickPass->setPassDepthTest();

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(pass);
		effect->addRenderPass(pickPass);
		setEffect(effect);

		qtuser_3d::Pickable* pickable = new WipeTowerPickable(this);
		pickable->setNoPrimitive(true);
		pickable->setEnableSelect(true);
		bindPickable(pickable);
	}

	WipeTowerEntity::~WipeTowerEntity()
	{
	}

	Qt3DRender::QGeometry* WipeTowerEntity::createSandwichBox(const Box3D& box, const QList<QVector4D>& colors)
	{
		if (colors.size() == 0)
			return nullptr;

		int size = colors.size();
		float y = box.size().y();
		float pcs = y / size;

		QVector3D bmin = box.min;
		QVector3D bmax = box.max;

		QVector<QVector3D> positions;
		QVector<QVector4D> vertexColors;

		//front
		{
			positions.push_back(bmin);
			positions.push_back(QVector3D(bmax.x(), bmin.y(), bmin.z()));
			positions.push_back(QVector3D(bmax.x(), bmin.y(), bmax.z()));

			positions.push_back(bmin);
			positions.push_back(QVector3D(bmax.x(), bmin.y(), bmax.z()));
			positions.push_back(QVector3D(bmin.x(), bmin.y(), bmax.z()));

			for (size_t i = 0; i < 6; i++)
			{
				vertexColors.push_back(colors.first());
			}
		}

		for (int i = 0; i < size; i++)
		{
			float minY = bmin.y() + pcs * i;
			float maxY = minY + pcs;

			const QVector4D& color = colors.at(i);

			for (size_t i = 0; i < 24; i++)
			{
				vertexColors.push_back(color);
			}

			positions.push_back(QVector3D(bmax.x(), minY, bmin.z()));
			positions.push_back(QVector3D(bmax.x(), maxY, bmin.z()));
			positions.push_back(QVector3D(bmax.x(), maxY, bmax.z()));
			positions.push_back(QVector3D(bmax.x(), minY, bmin.z()));
			positions.push_back(QVector3D(bmax.x(), maxY, bmax.z()));
			positions.push_back(QVector3D(bmax.x(), minY, bmax.z()));


			positions.push_back(QVector3D(bmax.x(), minY, bmax.z()));
			positions.push_back(QVector3D(bmax.x(), maxY, bmax.z()));
			positions.push_back(QVector3D(bmin.x(), maxY, bmax.z()));
			positions.push_back(QVector3D(bmax.x(), minY, bmax.z()));
			positions.push_back(QVector3D(bmin.x(), maxY, bmax.z()));
			positions.push_back(QVector3D(bmin.x(), minY, bmax.z()));


			positions.push_back(QVector3D(bmin.x(), minY, bmax.z()));
			positions.push_back(QVector3D(bmin.x(), maxY, bmax.z()));
			positions.push_back(QVector3D(bmin.x(), maxY, bmin.z()));
			positions.push_back(QVector3D(bmin.x(), minY, bmax.z()));
			positions.push_back(QVector3D(bmin.x(), maxY, bmin.z()));
			positions.push_back(QVector3D(bmin.x(), minY, bmin.z()));


			positions.push_back(QVector3D(bmin.x(), minY, bmin.z()));
			positions.push_back(QVector3D(bmin.x(), maxY, bmin.z()));
			positions.push_back(QVector3D(bmax.x(), maxY, bmin.z()));
			positions.push_back(QVector3D(bmin.x(), minY, bmin.z()));
			positions.push_back(QVector3D(bmax.x(), maxY, bmin.z()));
			positions.push_back(QVector3D(bmax.x(), minY, bmin.z()));
		}

		//back
		{
			QVector3D bmin = box.min;
			QVector3D bmax = box.max;

			positions.push_back(QVector3D(bmax.x(), bmax.y(), bmin.z()));
			positions.push_back(QVector3D(bmin.x(), bmax.y(), bmin.z()));
			positions.push_back(QVector3D(bmin.x(), bmax.y(), bmax.z()));

			positions.push_back(QVector3D(bmax.x(), bmax.y(), bmin.z()));
			positions.push_back(QVector3D(bmin.x(), bmax.y(), bmax.z()));
			positions.push_back(QVector3D(bmax.x(), bmax.y(), bmax.z()));

			for (size_t i = 0; i < 6; i++)
			{
				vertexColors.push_back(colors.last());
			}
		}

		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)&positions.at(0), AttribueSlot::Position, positions.size());
		Qt3DRender::QAttribute* colorAttribute = BufferHelper::CreateVertexAttribute((const char*)&vertexColors.at(0), AttribueSlot::Color, vertexColors.size());

		return GeometryCreateHelper::create(nullptr, positionAttribute, colorAttribute);
	}

	void WipeTowerEntity::updateSandwichBoxLocal(const qtuser_3d::Box3D& box, const QList<QVector4D>& colors)
	{
		bool valid = (box.valid && colors.size() >= 2);
		if (valid)
		{
			Qt3DRender::QGeometry* geo = createSandwichBox(box, colors);
			setGeometry(geo);

			QVector3D better;
			bool should = shouldTranslateTo(m_position, better);
			if (should)
			{
				translateTo(m_position);
			}
			else {
				translateTo(better);
			}

			updateBoundingBoxEntity();
		}
	}

	void WipeTowerEntity::setBoundingBoxVisibility(bool visible)
	{
		m_boundingBoxVisible = visible;
		updateBoundingBoxEntity();
	}

	const qtuser_3d::Box3D& WipeTowerEntity::localBox()
	{
		return m_localBox;
	}

	void WipeTowerEntity::setLocalBox(const qtuser_3d::Box3D& box)
	{
		if (m_localBox != box)
		{
			m_localBox = box;
			updateSandwichBoxLocal(box, m_colors);
		}
	}

	void WipeTowerEntity::setLocalBoxOnly(const qtuser_3d::Box3D& box)
	{
		if (m_localBox != box)
		{
			m_localBox = box;
		}
	}

	void WipeTowerEntity::setColors(const QList<QVector4D>& colors)
	{
		if (m_colors != colors)
		{
			m_colors = colors;
			updateSandwichBoxLocal(m_localBox, colors);
		}
	}
	
	void WipeTowerEntity::onClick(int primitiveID)
	{

	}

	void WipeTowerEntity::onStateChanged(ControlState state)
	{
		setSelected(m_pickable->selected());
	}

	bool WipeTowerEntity::selected()
	{
		return m_pickable->selected();
	}

	void WipeTowerEntity::setSelected(bool selected)
	{
		setBoundingBoxVisibility(selected);
	}

	const qtuser_3d::Box3D& WipeTowerEntity::printerBox()
	{
		return m_printerBox;
	}

	void WipeTowerEntity::setPrinterBox(const qtuser_3d::Box3D& box)
	{
		m_printerBox = box;
		QVector3D size = box.size();
		m_position = 0.8f * QVector3D(size.x(), size.y(), 0.0f);
		
		updateSandwichBoxLocal(m_localBox, m_colors);
	}

	bool WipeTowerEntity::shouldTranslateTo(const QVector3D& position, QVector3D& betterPosition)
	{
		qtuser_3d::Box3D printerBox = this->printerBox();
		qtuser_3d::Box3D localBox = this->localBox();
		QVector3D size = localBox.size();
		QVector3D towerMargin = QVector3D(size.x() / 2.0, size.y() / 2.0, 0.0f);

		QVector3D margin = borderMargin();

		QVector3D min = printerBox.min;
		QVector3D max = printerBox.max;

		min += margin + towerMargin;
		max -= margin + towerMargin;

		qtuser_3d::Box3D newBox(min, max);
		if (newBox.contains(position))
		{
			betterPosition = position;
			return true;
		}
		else {

			QVector3D dest = position;

			if (dest.x() < newBox.min.x())
			{
				dest.setX(newBox.min.x());
			}
			if (dest.y() < newBox.min.y())
			{
				dest.setY(newBox.min.y());
			}
			if (dest.x() > newBox.max.x())
			{
				dest.setX(newBox.max.x());
			}
			if (dest.y() > newBox.max.y())
			{
				dest.setY(newBox.max.y());
			}

			betterPosition = dest;

			return false;
		}
	}

	void WipeTowerEntity::translateTo(const QVector3D& t)
	{
		m_position = t;

		QMatrix4x4 m;
		m.translate(t);
		setPose(m);
		
		emit signalPositionChange(t);
	}

	QVector3D WipeTowerEntity::position()
	{
		return m_position;
	}

	QVector3D WipeTowerEntity::borderMargin()
	{
		return QVector3D(15.0f, 15.0f, 0.0f);
	}

	QVector3D WipeTowerEntity::positionOfLeftBottom()
	{
		qtuser_3d::Box3D localBox = this->localBox();
		QVector3D size = localBox.size();
		QVector3D towerMargin = QVector3D(size.x() / 2.0, size.y() / 2.0, 0.0f);
		return m_position - towerMargin;
	}

	void WipeTowerEntity::setLeftBottomPosition(float x, float y)
	{
		qtuser_3d::Box3D localBox = this->localBox();
		QVector3D size = localBox.size();
		QVector3D towerMargin = QVector3D(size.x() / 2.0, size.y() / 2.0, 0.0f);
		QVector3D position(towerMargin.x() + x, towerMargin.y() + y, m_position.z());
		translateTo(position);
	}

	void WipeTowerEntity::updateBoundingBoxEntity()
	{
		if (!m_boundingBoxVisible && m_boundingBoxEntity == nullptr)
		{
			return;
		}

		if (m_boundingBoxEntity == nullptr)
		{
			m_boundingBoxEntity = new qtuser_3d::BoxEntity();
			m_boundingBoxEntity->setColor(QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
			m_boundingBoxEntity->setLineWidth(1.0f);
			m_boundingBoxEntity->setParent(this);
		}

		QMatrix4x4 m;
		m_boundingBoxEntity->update(m_localBox, m);

		if (m_boundingBoxEntity->isEnabled() != m_boundingBoxVisible)
		{
			m_boundingBoxEntity->setEnabled(m_boundingBoxVisible);
		}
	}

	void WipeTowerEntity::setVisibility(bool visibility)
	{
		setEnabled(visibility);
	}
	
	bool WipeTowerEntity::isVisible()
	{
		return isEnabled();
	}

}