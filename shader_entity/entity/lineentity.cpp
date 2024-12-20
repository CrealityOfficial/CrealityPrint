#include "lineentity.h"
#include "qtuser3d/geometry/linecreatehelper.h"
#include <Qt3DRender/QRenderPass>
#include "qtuser3d/refactor/xeffect.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "../renderpass/purerenderpass.h"
#include "../renderpass/colorrenderpass.h"

namespace qtuser_3d
{
	LineEntity::LineEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
		, m_usePure(true)
	{
		m_colorParameter = setParameter("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));

		m_colorPass = new ColorRenderPass(this);
		m_colorPass->addFilterKeyMask("overlay", 0);

		m_purePass = new PureRenderPass(this);
		m_purePass->addFilterKeyMask("overlay", 0);

		XEffect *effect = new XEffect(this);
		effect->addRenderPass(m_colorPass);
		effect->addRenderPass(m_purePass);
		setEffect(effect);

		m_purePass->setEnabled(true);
		m_colorPass->setEnabled(false);
		
		m_lineWidth = new Qt3DRender::QLineWidth(this);
		
		m_purePass->addRenderState(m_lineWidth);
		m_colorPass->addRenderState(m_lineWidth);
	}
	
	LineEntity::~LineEntity()
	{
	}

	void LineEntity::setLineWidth(float width)
	{
		m_lineWidth->setValue(width);
	}

	void LineEntity::updateGeometry(const QVector<QVector3D>& positions, const QVector<QVector4D>& colors, bool loop)
	{
		int pointsNum = (int)positions.size();
		if (pointsNum <= 0)
			updateGeometry(pointsNum, nullptr, nullptr, loop);
		else
		{
			updateGeometry(pointsNum, (float*)&positions.at(0), (colors.size() > 0 ? (float*)&colors.at(0) : nullptr), loop);
		}
	}

	void LineEntity::updateGeometry(const QVector<QVector3D>& positions, bool loop)
	{
		updateGeometry(positions.size(), positions.size() > 0 ? (float*)&positions.at(0) : nullptr, nullptr, loop);
	}

	void LineEntity::updateGeometry(int pointsNum, float* positions, float* colors, bool loop)
	{
		if (m_usePure && colors)
		{
			m_colorPass->setEnabled(true);
			m_purePass->setEnabled(false);
			m_usePure = false;
		}
		else if (!m_usePure && !colors)
		{
			m_colorPass->setEnabled(false);
			m_purePass->setEnabled(true);
			m_usePure = true;
		}

		Qt3DRender::QGeometryRenderer::PrimitiveType type = Qt3DRender::QGeometryRenderer::Lines;
		if (loop) type = Qt3DRender::QGeometryRenderer::LineLoop;
		setGeometry(LineCreateHelper::create(pointsNum, positions, colors), type);
	}

	void LineEntity::updateGeometry(Qt3DRender::QGeometry* geometry, bool color, bool loop)
	{
		if (m_usePure && color)
		{
			m_colorPass->setEnabled(true);
			m_purePass->setEnabled(false);
			m_usePure = false;
		}
		else if (!m_usePure && !color)
		{
			m_colorPass->setEnabled(false);
			m_purePass->setEnabled(true);
			m_usePure = true;
		}

		Qt3DRender::QGeometryRenderer::PrimitiveType type = Qt3DRender::QGeometryRenderer::Lines;
		if (loop) type = Qt3DRender::QGeometryRenderer::LineLoop;
		setGeometry(geometry, type);
	}

	void LineEntity::setColor(const QVector4D& color)
	{
		m_colorParameter->setValue(color);
	}
}