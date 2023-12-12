#include "entity/manipulateentity.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"
#include "renderpass/phongrenderpass.h"
#include "renderpass/purerenderpass.h"

namespace qtuser_3d
{
	ManipulateEntity::ManipulateEntity(Qt3DCore::QNode* parent, ManipulateEntityFlags flags)
		:PickXEntity(parent)
	{
		XRenderPass* showPass;
		if (flags & Light)
			showPass = new PhongRenderPass(this);
		else
			showPass = new PureRenderPass(this);
		
		Qt3DRender::QFilterKey* showFilterKey = new Qt3DRender::QFilterKey(showPass);
		showFilterKey->setValue(0);
		showFilterKey->setName("view");
		if (flags & Alpha)
			showFilterKey->setName("alpha");

		if (flags & Overlay)
			showFilterKey->setName("overlay");

		showPass->addFilterKey(showFilterKey);
		showPass->setPassCullFace(Qt3DRender::QCullFace::CullingMode::NoCulling);
		showPass->setPassDepthTest(flags & DepthTest ? Qt3DRender::QDepthTest::Less : Qt3DRender::QDepthTest::Always);
		if (flags & Alpha)
		{
			showPass->setPassBlend();
		}

		XRenderPass* pickPass = new XRenderPass("pickFace_pwd", this);
		pickPass->addFilterKeyMask("pick2nd", 0);
		pickPass->setPassCullFace(Qt3DRender::QCullFace::CullingMode::NoCulling);
		pickPass->setPassDepthTest(Qt3DRender::QDepthTest::Always);
		pickPass->setEnabled(flags & Pickable);
		XEffect* effect = new XEffect(this);
		effect->addRenderPass(showPass);
		effect->addRenderPass(pickPass);
		setEffect(effect);

		m_color = QVector4D(1.0f, 1.0f, 1.0f, 1.0f);
		m_triggeredColor = QVector4D(0.0f, 0.0f, 0.0f, 0.0f);
		m_isTriggerible = false;

		setParameter("color", m_color);
	}

	ManipulateEntity::~ManipulateEntity()
	{

	}

	void ManipulateEntity::setColor(const QVector4D& color)
	{
		m_color = color;
		setParameter("color", m_color);
	}

	void ManipulateEntity::setTriggeredColor(const QVector4D& color)
	{
		m_triggeredColor = color;
	}

	void ManipulateEntity::setTriggerible(bool enable)
	{
		m_isTriggerible = enable;
	}

	void ManipulateEntity::setVisualState(ControlState state)
	{
		if (m_isTriggerible)
		{
			if ((int)state)
				setParameter("color", m_color);
			else
				setParameter("color", m_triggeredColor);
		}
	}
}
