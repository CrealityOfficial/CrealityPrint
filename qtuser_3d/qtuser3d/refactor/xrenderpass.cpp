#include "xrenderpass.h"
#include "qtuser3d/utils/shaderprogrammanager.h"

#include <QtCore/QDebug>

namespace qtuser_3d
{
	XRenderPass::XRenderPass(Qt3DCore::QNode* parent)
		: QRenderPass(parent)
	{
	}

	XRenderPass::XRenderPass(const QString& shaderName, Qt3DCore::QNode* parent)
		: QRenderPass(parent)
	{
		Qt3DRender::QShaderProgram* program = CREATE_SHADER(shaderName);

		if(program)
			setShaderProgram(program);

		if (!program)
			qDebug() << QString("XRenderPass load shader [%1] error.").arg(shaderName);

		setObjectName(shaderName);
	}

	XRenderPass::~XRenderPass()
	{

	}

	Qt3DRender::QParameter* XRenderPass::setParameter(const QString& name, const QVariant& value)
	{
		Qt3DRender::QParameter* parameter = nullptr;
		QVector<Qt3DRender::QParameter*> parameters = this->parameters();

		for (Qt3DRender::QParameter* param : parameters)
		{
			if (name == param->name())
			{
				param->setValue(value);
				parameter = param;
				break;
			}
		}

		if (!parameter)
		{
			parameter = new Qt3DRender::QParameter(name, value, this);
			addParameter(parameter);
		}

		return parameter;
	}

	void XRenderPass::addFilterKeyMask(const QString& key, int mask)
	{
		Qt3DRender::QFilterKey* filterKey = new Qt3DRender::QFilterKey(this);
		filterKey->setName(key);
		filterKey->setValue(mask);
		addFilterKey(filterKey);
	}

	void XRenderPass::setPassCullFace(Qt3DRender::QCullFace::CullingMode cullingMode)
	{
		Qt3DRender::QCullFace* cullFace = new Qt3DRender::QCullFace(this);
		cullFace->setMode(cullingMode);
		addRenderState(cullFace);
	}

	void XRenderPass::setPassBlend(Qt3DRender::QBlendEquationArguments::Blending source,
		Qt3DRender::QBlendEquationArguments::Blending destination)
	{
		Qt3DRender::QBlendEquationArguments* blend = new Qt3DRender::QBlendEquationArguments(this);
		blend->setSourceRgba(source);
		blend->setDestinationRgba(destination);
		addRenderState(blend);
	}

	void XRenderPass::setPassStencilMask(int mask)
	{
		Qt3DRender::QStencilMask* maskItem = new Qt3DRender::QStencilMask(this);
		maskItem->setFrontOutputMask(mask);
		addRenderState(maskItem);
	}

	void XRenderPass::setPassStencilOperation(
		Qt3DRender::QStencilOperationArguments::Operation depthFail,
		Qt3DRender::QStencilOperationArguments::Operation stencilFail,
		Qt3DRender::QStencilOperationArguments::Operation allPass)
	{
		Qt3DRender::QStencilOperation* op = new Qt3DRender::QStencilOperation(this);
		Qt3DRender::QStencilOperationArguments* face = op->front();

		face->setDepthTestFailureOperation(depthFail);
		face->setStencilTestFailureOperation(stencilFail);
		face->setAllTestsPassOperation(allPass);

		addRenderState(op);
	}


	void XRenderPass::setPassStencilFunction(Qt3DRender::QStencilTestArguments::StencilFunction func, int reference, int comparisonMask)
	{
		Qt3DRender::QStencilTest* state = new Qt3DRender::QStencilTest(this);
		Qt3DRender::QStencilTestArguments* arg = state->front();
		arg->setComparisonMask(comparisonMask);
		arg->setReferenceValue(reference);
		arg->setStencilFunction(func);

		addRenderState(state);
	}

	void XRenderPass::setPassDepthTest(Qt3DRender::QDepthTest::DepthFunction depthFunc)
	{
		Qt3DRender::QDepthTest* state = new Qt3DRender::QDepthTest(this);
		state->setDepthFunction(depthFunc);

		addRenderState(state);
	}

	void XRenderPass::setPassNoDepthMask()
	{
		Qt3DRender::QNoDepthMask* state = new Qt3DRender::QNoDepthMask(this);
		addRenderState(state);
	}

	void XRenderPass::setPointSize(float size)
	{
		Qt3DRender::QPointSize* pSize = new Qt3DRender::QPointSize(this);
		pSize->setSizeMode(Qt3DRender::QPointSize::Fixed);
		pSize->setValue(size);
		addRenderState(pSize);
	}

	void XRenderPass::setLineWidth(float width)
	{
		Qt3DRender::QLineWidth* qLine = new Qt3DRender::QLineWidth(this);
		qLine->setValue(width);
		addRenderState(qLine);
	}

}