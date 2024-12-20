#ifndef QTUSER_3D_XEFFECT_1679973441776_H
#define QTUSER_3D_XEFFECT_1679973441776_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QRenderState>

namespace qtuser_3d
{
	class QTUSER_3D_API XEffect : public Qt3DRender::QEffect
	{
		Q_OBJECT
	public:
		XEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~XEffect();

		Qt3DRender::QParameter* setParameter(const QString& name, const QVariant& value);
		int renderPassCount();
		void addRenderState(int index, Qt3DRender::QRenderState* state);
		void addPassFilter(int index, const QString& filter);
		void removePassFilter(int passIndex, const QString& filterName, const QVariant& filterValue);
		void addRenderPass(XRenderPass* pass);
		void removeRenderPass(XRenderPass* pass);

		void setPassCullFace(int index, Qt3DRender::QCullFace::CullingMode cullingMode = Qt3DRender::QCullFace::NoCulling);
		void setPassBlend(int index, Qt3DRender::QBlendEquationArguments::Blending source = Qt3DRender::QBlendEquationArguments::SourceAlpha,
			Qt3DRender::QBlendEquationArguments::Blending destination = Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);

		void setPassStencilMask(int index, int mask);
		void setPassStencilOperation(int index,
			Qt3DRender::QStencilOperationArguments::Operation depthFail,
			Qt3DRender::QStencilOperationArguments::Operation stencilFail,
			Qt3DRender::QStencilOperationArguments::Operation allPass);
		void setPassStencilFunction(int index, Qt3DRender::QStencilTestArguments::StencilFunction func, int reference, int comparisonMask);

		void setPassDepthTest(int index, Qt3DRender::QDepthTest::DepthFunction depthFunc = Qt3DRender::QDepthTest::Less);
		void setPassNoDepthMask(int index);

		void setPointSize(int index, float size);
		void setPassLineWidth(int index, float width);
	public:
		XRenderPass* findRenderPass(int index);
	protected:
		Qt3DRender::QTechnique* m_technique;
	};
}

#endif // QTUSER_3D_XEFFECT_1679973441776_H