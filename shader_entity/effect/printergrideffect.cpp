#include "printergrideffect.h"

namespace qtuser_3d
{
	PrinterGridEffect::PrinterGridEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		addRenderPass(new qtuser_3d::XRenderPass("printergrid_cx"));

		m_xshowColorParam = setParameter("xshowcolor", QVector4D(0.65f, 0.23f, 0.23f, 1.0f));
		m_yshowColorParam = setParameter("yshowcolor", QVector4D(0.65f, 0.23f, 0.23f, 1.0f));
		m_lineColorParam = setParameter("linecolor", QVector4D(0.32f, 0.32f, 0.32f, 1.0f));
		m_xIndexParam = setParameter("highlight_index_x", 30.0f);
		m_yIndexParam = setParameter("highlight_index_y", 30.0f);
		m_yIndexColorParam = setParameter("xyIndexcolor", QVector4D(0.3412f, 0.4118f, 0.4706f, 1.0f));

		m_lineWidth = new Qt3DRender::QLineWidth(this);
		m_lineWidth->setSmooth(true);
		m_lineWidth->setValue(1.0f);
		addRenderState(0, m_lineWidth);
	}

	PrinterGridEffect::~PrinterGridEffect()
	{

	}

	void PrinterGridEffect::setLineWidth(float lineWidth)
	{
		m_lineWidth->setValue(lineWidth);
	}

	void PrinterGridEffect::setLineColor(const QVector4D& color)
	{
		m_lineColorParam->setValue(color);
	}

	void PrinterGridEffect::setXShowColor(const QVector4D& color)
	{
		m_xshowColorParam->setValue(color);
	}

	void PrinterGridEffect::setYShowColor(const QVector4D& color)
	{
		m_yshowColorParam->setValue(color);
	}

	void PrinterGridEffect::setXYIndexColor(const QVector4D& color)
	{
		m_yIndexColorParam->setValue(color);
	}

	void PrinterGridEffect::setXYIndex(float xIndex, float yIndex)
	{
		m_xIndexParam->setValue(xIndex);
		m_yIndexParam->setValue(yIndex);
	}
}