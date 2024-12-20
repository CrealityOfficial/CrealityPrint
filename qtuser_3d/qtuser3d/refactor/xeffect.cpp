#include "xeffect.h"
#include "qtuser3d/utils/techniquecreator.h"

namespace qtuser_3d
{
	XEffect::XEffect(Qt3DCore::QNode* parent)
		: QEffect(parent)
	{
		m_technique = qtuser_3d::TechniqueCreator::createOpenGLBase(this);
		addTechnique(m_technique);
	}

	XEffect::~XEffect()
	{

	}

	Qt3DRender::QParameter* XEffect::setParameter(const QString& name, const QVariant& value)
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

	void XEffect::addRenderPass(XRenderPass* pass)
	{
		if(pass)
			m_technique->addRenderPass(pass);
	}
	
	void XEffect::removeRenderPass(XRenderPass* pass)
	{
		if(pass)
			m_technique->removeRenderPass(pass);
	}

	int XEffect::renderPassCount()
	{
		return m_technique->renderPasses().count();
	}

	XRenderPass* XEffect::findRenderPass(int index)
	{
		if (index < 0 || index >= renderPassCount())
		{
			qDebug() << QString("XEffect::findRenderPass invalid index [%1]").arg(index);
			return nullptr;
		}

		XRenderPass* pass = qobject_cast<XRenderPass*>(m_technique->renderPasses().at(index));

		if (!pass)
		{
			qDebug() << QString("qobject_cast<XRenderPass*> error. not XRenderPass. ");
			return nullptr;
		}

		return pass;
	}

	void XEffect::addRenderState(int index, Qt3DRender::QRenderState* state)
	{
		XRenderPass* pass = findRenderPass(index);
		if (!pass)
			return;

		pass->addRenderState(state);
	}

	void XEffect::addPassFilter(int index, const QString& filter)
	{
		XRenderPass* pass = findRenderPass(index);
		if (!pass)
			return;

		Qt3DRender::QFilterKey* filterKey = new Qt3DRender::QFilterKey();
		filterKey->setName(filter);
		filterKey->setValue(0);
		pass->addFilterKey(filterKey);
	}

	void XEffect::removePassFilter(int passIndex, const QString& filterName, const QVariant& filterValue)
	{
		XRenderPass* pass = findRenderPass(passIndex);
		if (!pass)
			return;

		const auto keys = pass->filterKeys();
		for (Qt3DRender::QFilterKey* c : keys)
		{
			if (c->name() == filterName && c->value() == filterValue)
			{
				pass->removeFilterKey(c);
				//break;
			}
			
		}
			
	}

	void XEffect::setPassCullFace(int index, Qt3DRender::QCullFace::CullingMode cullingMode)
	{
		XRenderPass* pass = findRenderPass(index);
		if (pass)
		{
			pass->setPassCullFace(cullingMode);
		}
	}

	void XEffect::setPassBlend(int index, Qt3DRender::QBlendEquationArguments::Blending source,
		Qt3DRender::QBlendEquationArguments::Blending destination)
	{
		XRenderPass* pass = findRenderPass(index);
		if (pass)
		{
			pass->setPassBlend(source, destination);
		}
	}

	void XEffect::setPassStencilMask(int index, int mask)
	{
		XRenderPass* pass = findRenderPass(index);
		if (pass)
		{
			pass->setPassStencilMask(mask);
		}
	}

	void XEffect::setPassStencilOperation(int index,
		Qt3DRender::QStencilOperationArguments::Operation depthFail,
		Qt3DRender::QStencilOperationArguments::Operation stencilFail,
		Qt3DRender::QStencilOperationArguments::Operation allPass)
	{
		XRenderPass* pass = findRenderPass(index);
		if (pass)
		{
			pass->setPassStencilOperation(depthFail, stencilFail, allPass);
		}
	}


	void XEffect::setPassStencilFunction(int index, Qt3DRender::QStencilTestArguments::StencilFunction func, int reference, int comparisonMask)
	{
		XRenderPass* pass = findRenderPass(index);
		if (pass)
		{
			pass->setPassStencilFunction(func, reference, comparisonMask);
		}
	}

	void XEffect::setPassDepthTest(int index, Qt3DRender::QDepthTest::DepthFunction depthFunc)
	{
		XRenderPass* pass = findRenderPass(index);
		if (pass)
		{
			pass->setPassDepthTest(depthFunc);
		}
	}

	void XEffect::setPassNoDepthMask(int index)
	{
		XRenderPass* pass = findRenderPass(index);
		if (pass)
		{
			pass->setPassNoDepthMask();
		}
	}

	void XEffect::setPointSize(int index, float size)
	{
		XRenderPass* pass = findRenderPass(index);
		if (pass)
		{
			pass->setPointSize(size);
		}
	}

	void XEffect::setPassLineWidth(int index, float width)
	{
		XRenderPass* pass = findRenderPass(index);
		if (pass)
		{
			pass->setLineWidth(width);
		}
	}
}