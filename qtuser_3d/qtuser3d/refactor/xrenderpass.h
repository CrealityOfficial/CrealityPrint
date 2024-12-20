#ifndef QTUSER_3D_XRENDERPASS_1679990543678_H
#define QTUSER_3D_XRENDERPASS_1679990543678_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QCullFace>
#include <Qt3DRender/QBlendEquationArguments>
#include <Qt3DRender/QStencilOperationArguments>
#include <Qt3DRender/QStencilTestArguments>
#include <Qt3DRender/QDepthTest>
#include <Qt3DRender/QNoDepthMask>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QStencilMask>
#include <Qt3DRender/QStencilOperation>
#include <Qt3DRender/QStencilTest>
#include <Qt3DRender/QPointSize>
#include <Qt3DRender/QLineWidth>

namespace qtuser_3d
{
	class QTUSER_3D_API XRenderPass : public Qt3DRender::QRenderPass
	{
		Q_OBJECT
	public:
		XRenderPass(Qt3DCore::QNode* parent = nullptr);
		XRenderPass(const QString& shaderName, Qt3DCore::QNode* parent = nullptr);
		virtual ~XRenderPass();

		void addFilterKeyMask(const QString& key, int mask);
		Qt3DRender::QParameter* setParameter(const QString& name, const QVariant& value);

		void setPassCullFace(Qt3DRender::QCullFace::CullingMode cullingMode = Qt3DRender::QCullFace::NoCulling);
		void setPassBlend(Qt3DRender::QBlendEquationArguments::Blending source = Qt3DRender::QBlendEquationArguments::SourceAlpha,
			Qt3DRender::QBlendEquationArguments::Blending destination = Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);

		void setPassStencilMask(int mask);
		void setPassStencilOperation(
			Qt3DRender::QStencilOperationArguments::Operation depthFail,
			Qt3DRender::QStencilOperationArguments::Operation stencilFail,
			Qt3DRender::QStencilOperationArguments::Operation allPass);
		void setPassStencilFunction(Qt3DRender::QStencilTestArguments::StencilFunction func, int reference, int comparisonMask);

		void setPassDepthTest(Qt3DRender::QDepthTest::DepthFunction depthFunc = Qt3DRender::QDepthTest::Less);
		void setPassNoDepthMask();

		void setPointSize(float size);
		void setLineWidth(float width);
	};
}

#endif // QTUSER_3D_XRENDERPASS_1679990543678_H