#include "phongrenderpass.h"

namespace qtuser_3d
{
	PhongRenderPass::PhongRenderPass(Qt3DCore::QNode* parent)
		: XRenderPass("phong", parent)
	{
		m_color = setParameter("color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
		m_ambient = setParameter("ambient", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
		m_diffuse = setParameter("diffuse", QVector4D(0.7, 0.7, 0.7, 1.0));
		m_specular = setParameter("specular", QVector4D(0.5, 0.5, 0.5, 1.0));
		m_specularPower = setParameter("specularPower", 4.5);
		m_lightDir = setParameter("u_lightDir", QVector3D(0.0, 1.0, 0.8));

	}

	PhongRenderPass::~PhongRenderPass()
	{

	}

	void PhongRenderPass::setColor(const QVector4D& color)
	{
		m_color->setValue(color);
	}

	void PhongRenderPass::setAmbient(const QVector4D& ambient)
	{
		m_ambient->setValue(ambient);
	}

	void PhongRenderPass::setDiffuse(const QVector4D& diffuse)
	{
		m_diffuse->setValue(diffuse);
	}

	void PhongRenderPass::setSpecular(const QVector4D& specular)
	{
		m_specular->setValue(specular);
	}

	void PhongRenderPass::setSpecularPower(const float& power)
	{
		m_specularPower->setValue(power);
	}

}