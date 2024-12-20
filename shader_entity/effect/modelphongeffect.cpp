#include "modelphongeffect.h"

namespace qtuser_3d
{
	ModelPhongEffect::ModelPhongEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		qtuser_3d::XRenderPass* pass = new qtuser_3d::XRenderPass("modelface");
		pass->setPassCullFace();
		addRenderPass(pass);

		setColor(QVector4D(1.0f, 1.0f, 0.0f, 1.0f));

		QVector4D ambient(0.3f, 0.3f, 0.3f, 1.0f);
		QVector4D diffuse(0.65f, 0.65f, 0.65f, 1.0f);
		QVector4D specular(0.3f, 0.3f, 0.3f, 1.0f);

		setParameter("ambient", ambient);
		setParameter("diffuse", diffuse);
		setParameter("specular", specular);
		setParameter("checkscope", 0);

		m_remParameter = setParameter("renderModel", 1);
		m_colorParameter = setParameter("useColor", 0);

		qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFace_model");
		pickPass->setPassCullFace();
		addRenderPass(pickPass);
		setPassBlend(1, Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
	}

	ModelPhongEffect::~ModelPhongEffect()
	{

	}

	void ModelPhongEffect::setRenderEffectMode(RenderEffectMode mode)
	{
		m_remParameter->setValue((int)mode);
	}

	void ModelPhongEffect::useColor(bool use)
	{
		m_colorParameter->setValue(use ? 1 : 0);
	}

	void ModelPhongEffect::setColor(const QVector4D& color)
	{
		QVariantList values;
		values << color
			<< color
			<< color
			<< color
			<< color
			<< color;

		setParameter("stateColors[0]", values);
	}
}