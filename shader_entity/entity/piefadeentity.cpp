#include "entity/piefadeentity.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"

namespace qtuser_3d
{
	PieFadeEntity::PieFadeEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		m_colorParameter = setParameter("color", QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
		
		m_rotModeParameter = setParameter("rotMode", 0);
		m_rotRadiansParameter = setParameter("rotRadians", 0.0f);
		m_rotCenterParameter = setParameter("rotCenter", QVector3D(0.0, 0.0, 0.0));
		m_rotInitDirParameter = setParameter("rotInitDir", QVector3D(0.0, 1.0, 0.0));
		m_rotAxisParameter = setParameter("rotAxis", QVector3D(0.0, 0.0, 1.0));

		m_lightEnableParameter = setParameter("lightEnable", 0);

		setParameter("lightDirection", QVector3D(1.0, 0.0, 1.0));
		setParameter("ambient", QVector4D(0.6, 0.6, 0.6, 1.0));
		setParameter("diffuse", QVector4D(0.6, 0.6, 0.6, 1.0));
		setParameter("specular", QVector4D(0.6, 0.6, 0.6, 1.0));
		setParameter("specularPower", 4.5);

		m_viewPass = new XRenderPass("piefade", this);
		m_viewPass->addFilterKeyMask("alpha2nd", 0);
		m_viewPass->setPassDepthTest(Qt3DRender::QDepthTest::Always);
		m_viewPass->setPassCullFace(Qt3DRender::QCullFace::CullingMode::Back);

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(m_viewPass);
		setEffect(effect);
	}

	PieFadeEntity::~PieFadeEntity()
	{

	}

	void PieFadeEntity::setColor(const QVector4D& color)
	{
		m_colorParameter->setValue(color);
	}

	void PieFadeEntity::setRotMode(int mode)
	{
		m_rotModeParameter->setValue(mode);
	}

	void PieFadeEntity::setRotRadians(float radians)
	{
		m_rotRadiansParameter->setValue(radians);
	}

	void PieFadeEntity::setRotCenter(QVector3D center)
	{
		m_rotCenterParameter->setValue(center);
	}

	void PieFadeEntity::setRotInitDir(QVector3D dir)
	{
		m_rotInitDirParameter->setValue(dir);
	}
	void PieFadeEntity::setRotAxis(QVector3D axis)
	{
		m_rotAxisParameter->setValue(axis);
	}

	void PieFadeEntity::setLigthEnable(bool enable)
	{
		m_lightEnableParameter->setValue(enable);
	}

	void PieFadeEntity::setPassBlend()
	{
		m_viewPass->setPassBlend();
	}
}
