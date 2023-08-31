#include "splitplane.h"
#include "qtuser3d/geometry/boxcreatehelper.h"
#include "kernel/kernel.h"
#include "kernel/visualscene.h"
#include "qtuser3d/framegraph/texturerendertarget.h"
#include "data/modeln.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"

using namespace qtuser_3d;
SplitPlane::SplitPlane(Qt3DCore::QNode* parent)
	:XEntity(parent)
{
	XRenderPass* pass = new XRenderPass("splitplane", this);
	pass->addFilterKeyMask("alpha", 0);
	pass->setPassBlend(Qt3DRender::QBlendEquationArguments::SourceAlpha, Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);

	XEffect* effect = new XEffect(this);
	effect->addRenderPass(pass);
	setEffect(effect);

	m_color = setParameter("color", QVector4D(1.0f, 0.965f, 0.0f, 0.2f));

	qtuser_3d::TextureRenderTarget* rt = creative_kernel::getKernel()->visScene()->textureRenderTarget();

	if (rt)
	{
		Qt3DRender::QTexture2D* texture = rt->depthTexture();
		if (texture)
		{
			setParameter("depthTexture", QVariant::fromValue(texture));
		}
		
		texture = rt->worldPosTexture();
		if (texture)
		{
			setParameter("positionTexture", QVariant::fromValue(texture));
		}

		texture = rt->worldNormalTexture();
		if (texture)
		{
			setParameter("normalTexture", QVariant::fromValue(texture));
		}

		texture = rt->colorTexture();
		if (texture)
		{
			setParameter("faceTexture", QVariant::fromValue(texture));
		}
	}
}

SplitPlane::~SplitPlane()
{
}

void SplitPlane::updateBox(Box3D box)
{
	QVector3D size = box.size();
	float length = size.length();
	float radius = length * 0.5;

	QVector3D min(-radius, -radius, -0.1);
	QVector3D max(radius, radius, 0.1);
	qtuser_3d::Box3D thinbox(min, max);
	setGeometry(qtuser_3d::BoxCreateHelper::createWithTriangle(thinbox));
}

void SplitPlane::setGlobalNormal(const QVector3D& normal)
{
	setParameter("globalNormal", normal);
}

void SplitPlane::setTargetModel(creative_kernel::ModelN* model)
{
	if (model)
	{
		int num = model->primitiveNum();
		int facebase = model->faceBase();

		setParameter("faceRange", QPoint(facebase, num));
	}else {
		setParameter("faceRange", QPoint(0, 0));
	}
}


