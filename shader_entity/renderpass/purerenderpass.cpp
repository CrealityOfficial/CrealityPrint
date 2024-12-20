#include "purerenderpass.h"

namespace qtuser_3d
{
	PureRenderPass::PureRenderPass(Qt3DCore::QNode* parent)
		: XRenderPass("pure", parent)
	{
		m_color = setParameter("color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
	}

	PureRenderPass::~PureRenderPass()
	{

	}

	void PureRenderPass::setColor(const QVector4D& color)
	{
		m_color->setValue(color);

		setPassDepthTest();
		if (color[3] < 1.0)	// transparent
		{
			setPassBlend(Qt3DRender::QBlendEquationArguments::SourceAlpha, Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);
			setPassCullFace(Qt3DRender::QCullFace::Back);
		}
		else 
		{
			setPassBlend();
			setPassCullFace();
		}
	}
}