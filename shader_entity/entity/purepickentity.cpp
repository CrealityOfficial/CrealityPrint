#include "purepickentity.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"
#include "renderpass/purerenderpass.h"

namespace qtuser_3d
{
	PurePickEntity::PurePickEntity(Qt3DCore::QNode* parent)
		: PickXEntity(parent)
	{
		m_colorParameter = setParameter("color", QVector4D(1.0f, 1.0f, 1.0f, 0.2f));

		qtuser_3d::XRenderPass* alphaPass = new qtuser_3d::XRenderPass("pure_op");
		alphaPass->addFilterKeyMask("alpha", 0);
		alphaPass->setPassBlend();
		alphaPass->setPassCullFace();
		alphaPass->setPassNoDepthMask();

		qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("purePickFace_op");
		pickPass->addFilterKeyMask("pick2nd", 0);
		pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
		pickPass->setPassCullFace();
		pickPass->setPassDepthTest();

		qtuser_3d::XEffect* effect = new qtuser_3d::XEffect(this);
		effect->addRenderPass(alphaPass);
		effect->addRenderPass(pickPass);
		setEffect(effect);
	}

	PurePickEntity::~PurePickEntity()
	{

	}

	void PurePickEntity::setColor(const QVector4D& color)
	{
		m_color = color;
	}

	void PurePickEntity::setHoverColor(const QVector4D& color)
	{
		m_hoverColor = color;
	}

	void PurePickEntity::onStateChanged(ControlState state)
	{
		if (state == qtuser_3d::ControlState::hover)
		{
			m_colorParameter->setValue(m_hoverColor);
		}
		else
		{
			m_colorParameter->setValue(m_color);
		}
	}
}