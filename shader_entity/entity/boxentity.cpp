#include "boxentity.h"

#include "qtuser3d/utils/primitiveshapecache.h"
#include "qtuser3d/geometry/boxcreatehelper.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"
#include "renderpass/purerenderpass.h"

namespace qtuser_3d
{
	BoxEntity::BoxEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
		, m_viewPass(nullptr)
	{
		m_colorParameter = setParameter("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));

		XRenderPass* viewPass = new PureRenderPass(this);
		viewPass->addFilterKeyMask("view", 0);
		viewPass->setPassDepthTest();
		m_viewPass = viewPass;

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(viewPass);
		setEffect(effect);

		setObjectName("BoxEntity");
	}

	BoxEntity::~BoxEntity()
	{

	}

	void BoxEntity::updateGlobal(const Box3D& box, bool need_bottom)
	{
		QMatrix4x4 m;

		m.translate(box.min);
		m.scale(box.size());

		if (need_bottom)
		{
			setGeometry(PRIMITIVESHAPE("box"), Qt3DRender::QGeometryRenderer::Lines);
		}
		else
		{
			setGeometry(PRIMITIVESHAPE("box_nobottom"), Qt3DRender::QGeometryRenderer::Lines);
		}
		setPose(m);
	}

	void BoxEntity::updateLocal(const Box3D& box, const QMatrix4x4& parentMatrix)
	{
		setGeometry(BoxCreateHelper::createPartBox(box, 0.3f), Qt3DRender::QGeometryRenderer::Lines);

		QMatrix4x4 m = parentMatrix.inverted();
		setPose(m);
	}

	void BoxEntity::update(const Box3D& box, const QMatrix4x4& parentMatrix)
	{
		setGeometry(BoxCreateHelper::createPartBox(box, 0.3f), Qt3DRender::QGeometryRenderer::Lines);

		QMatrix4x4 matrix;
		matrix.setToIdentity();
		setPose(matrix);
	}

	void BoxEntity::setColor(const QVector4D& color)
	{
		m_colorParameter->setValue(color);
	}

	void BoxEntity::setLineWidth(float width)
	{
		if (m_viewPass)
		{
			m_viewPass->setLineWidth(width);
		}
	}
}