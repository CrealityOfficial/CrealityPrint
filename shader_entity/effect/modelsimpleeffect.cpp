#include "modelsimpleeffect.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "renderpass/phongrenderpass.h"

namespace qtuser_3d
{
	ModelSimpleEffect::ModelSimpleEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		PhongRenderPass *pass = new PhongRenderPass;

		addRenderPass(pass);
		addPassFilter(0, "modelcapture");

		pass->setColor(QVector4D(0.55f, 0.55f, 0.55f, 1.0f));
		pass->setAmbient(QVector4D(0.8f, 0.8f, 0.8f, 1.0f));
		pass->setDiffuse(QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
		pass->setSpecular(QVector4D(0.8f, 0.8f, 0.8f, 1.0f));
		pass->setSpecularPower(12.0);

	}

	ModelSimpleEffect::~ModelSimpleEffect()
	{

	}

}