#include "alonepointentity.h"
#include "qtuser3d/utils/primitiveshapecache.h"
#include "qtuser3d/refactor/xeffect.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/utils/shaderprogrammanager.h"
#include "renderpass/purerenderpass.h"

namespace qtuser_3d
{
	AlonePointEntity::AlonePointEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		m_colorParameter = setParameter("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));

		setGeometry(PRIMITIVESHAPE("point"), Qt3DRender::QGeometryRenderer::Points);

		m_pointSizeState = new Qt3DRender::QPointSize(this);
		m_pointSizeState->setSizeMode(Qt3DRender::QPointSize::Fixed);
		m_pointSizeState->setValue(1.0);
		
		m_pass = new PureRenderPass(this);
		m_pass->addRenderState(m_pointSizeState);
		m_pass->addFilterKeyMask("view", 0);

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(m_pass);

		setEffect(effect);
	}
	
	AlonePointEntity::~AlonePointEntity()
	{
	}

	void AlonePointEntity::setColor(const QVector4D& color)
	{
		m_colorParameter->setValue(color);
	}

	void AlonePointEntity::updateGlobal(QVector3D& point)
	{
		QMatrix4x4 m;

		m.translate(point);
		setPose(m);
	}

	void AlonePointEntity::updateLocal(QVector3D& point, const QMatrix4x4& parentMatrix)
	{
		QVector3D globalPoint = parentMatrix * point;
		updateGlobal(globalPoint);
	}

	void AlonePointEntity::setPointSize(float pointSize)
	{
		m_pointSizeState->setValue(pointSize);
	}

	void AlonePointEntity::setShader(const QString& shaderName)
	{
		Qt3DRender::QShaderProgram* program = CREATE_SHADER(shaderName);
		if (program)
			m_pass->setShaderProgram(program);
	}

	void AlonePointEntity::setFilterType(const QString& filterType)
	{
		if (m_pass)
		{
			m_pass->addFilterKeyMask(filterType, 0);
		}
		
	}
}