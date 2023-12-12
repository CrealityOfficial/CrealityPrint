#include "zprojectrenderpass.h"

namespace qtuser_3d
{
	ZProjectRenderPass::ZProjectRenderPass(Qt3DCore::QNode* parent)
		: XRenderPass("zproject", parent)
	{
		m_color = setParameter("color", QVector4D(0.2275, 0.2275, 0.2353, 0.5));

		addFilterKeyMask("alpha", 0);
		setPassCullFace(Qt3DRender::QCullFace::Back);
		setPassBlend();
		setPassStencilMask(0xFF);
		setPassStencilOperation(Qt3DRender::QStencilOperationArguments::Keep, Qt3DRender::QStencilOperationArguments::Keep, Qt3DRender::QStencilOperationArguments::Increment);
		setPassStencilFunction(Qt3DRender::QStencilTestArguments::Equal, 0x0, 0xFF);
		setPassDepthTest(Qt3DRender::QDepthTest::Less);
	}

	ZProjectRenderPass::~ZProjectRenderPass()
	{

	}

	void ZProjectRenderPass::setColor(const QVector4D& color)
	{
		m_color->setValue(color);
	}

}