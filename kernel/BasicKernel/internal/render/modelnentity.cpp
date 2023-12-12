#include "modelnentity.h"
#include "entity/boxentity.h"
#include "qtuser3d/trimesh2/renderprimitive.h"
#include "qtuser3d/trimesh2/trimeshrender.h"

namespace creative_kernel
{
	ModelNEntity::ModelNEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		m_boxEntity = new qtuser_3d::BoxEntity();
		m_boxEntity->setColor(QVector4D(0.306f, 0.702f, 0.769f, 1.0f));

		m_lineEntity = new qtuser_3d::PureEntity(parent);
		m_lineEntity->setColor(QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
		m_lineEntity->addPassFilter(0, "view");

		m_stateParameter = setParameter("state", 0.0f);
		m_vertexBaseParameter = setParameter("vertexBase", QPoint(0, 0));
		m_errorParameter = setParameter("error", 0.0f);
		m_supportCosParameter = setParameter("supportCos", 0.5);
		m_hoverParameter = setParameter("hoverState", 0);
		m_fanzhuanParameter = setParameter("fanzhuan", 0);

		m_customColorParameter = setParameter("customColor", QVector4D(0.0f, 0.0f, 0.0f, 1.0f));
		m_transparencyParameter = setParameter("transparency", 1.0f);
		m_lightingFlagParameter = setParameter("lightingEnable", 1);

		m_nozzleParameter = setParameter("nozzle", 0);

		m_renderModeParameter = setParameter("renderModel", 1);

		m_textureDiffuse = setParameter("textureDiffuse", QVariant::fromValue(0));
		m_textureAmbient = setParameter("textureAmbient", QVariant::fromValue(0));
		m_textureSpecular = setParameter("textureSpecular", QVariant::fromValue(0));
		m_textureNormal = setParameter("textureNormal", QVariant::fromValue(0));

		m_useVertexColor = setParameter("useVertexColor", QVariant::fromValue(0));
	}
	
	ModelNEntity::~ModelNEntity()
	{
		if (!m_boxEntity->parent())
			delete m_boxEntity;
		delete m_lineEntity;
	}

	void ModelNEntity::update()
	{
		m_lineEntity->setParent(qobject_cast<Qt3DCore::QNode*>(parent()));
	}

	void ModelNEntity::setBoxVisibility(bool visible)
	{
		m_boxEntity->setParent(visible ? (Qt3DCore::QNode*)this->parent() : nullptr);
	}

	void ModelNEntity::updateBoxLocal(const qtuser_3d::Box3D& box, const QMatrix4x4& parentMatrix)
	{
		m_boxEntity->update(box, parentMatrix);
	}

	void ModelNEntity::updateLines(const std::vector<trimesh::vec3>& lines)
	{
		std::vector<trimesh::vec3> rlines;
		qtuser_3d::loopPolygon2Lines(lines, rlines);
		m_lineEntity->setGeometry(qtuser_3d::createLinesGeometry(rlines), Qt3DRender::QGeometryRenderer::Lines);
	}

	void ModelNEntity::setBoxColor(QVector4D color)
	{
		m_boxEntity->setColor(color);
	}

	void ModelNEntity::enterSupportStatus()
	{
		m_hoverParameter->setValue(1);
	}

	void ModelNEntity::leaveSupportStatus()
	{
		m_hoverParameter->setValue(0);
	}

	void ModelNEntity::setState(float state)
	{
		m_stateParameter->setValue(state);
	}

	float ModelNEntity::getState()
	{
		return m_stateParameter->value().toFloat();
	}

	void ModelNEntity::setNozzle(float nozzle)
	{
		m_nozzleParameter->setValue(nozzle);
	}

	void ModelNEntity::setVertexBase(QPoint vertexBase)
	{
		m_vertexBaseParameter->setValue(vertexBase);
	}

	void ModelNEntity::setErrorState(bool error)
	{
		m_errorParameter->setValue(error ? 1.0f : 0.0f);
	}

	void ModelNEntity::setSupportCos(float supportCos)
	{
		m_supportCosParameter->setValue(supportCos);
	}

	void ModelNEntity::setFanZhuan(int fz)
	{
		m_fanzhuanParameter->setValue(fz);
	}

	void ModelNEntity::setCustomColor(QColor color)
	{
		m_customColorParameter->setValue(color);
	}

	QColor ModelNEntity::getCustomColor()
	{
		return m_customColorParameter->value().value<QColor>();
	}

	void ModelNEntity::setTransparency(float alpha)
	{
		m_transparencyParameter->setValue(alpha);
	}

	void ModelNEntity::setLightingEnable(bool enable)
	{
		m_lightingFlagParameter->setValue(enable ? 1 : 0);
	}

	void ModelNEntity::setRenderMode(int mode)
	{
		m_renderModeParameter->setValue(mode);
	}

	void ModelNEntity::setTDiffuse(Qt3DRender::QTexture2D* aDiffuse)
	{
		m_textureDiffuse->setValue(QVariant::fromValue(aDiffuse));
	}

	void ModelNEntity::setTAmbient(Qt3DRender::QTexture2D* aAmbient)
	{
		m_textureAmbient->setValue(QVariant::fromValue(aAmbient));
	}

	void ModelNEntity::setTSpecular(Qt3DRender::QTexture2D* aSpecular)
	{
		m_textureSpecular->setValue(QVariant::fromValue(aSpecular));
	}

	void ModelNEntity::setTNormal(Qt3DRender::QTexture2D* aNormal)
	{
		m_textureNormal->setValue(QVariant::fromValue(aNormal));
	}

	void ModelNEntity::setUseVertexColor(bool used)
	{
		m_useVertexColor->setValue(QVariant::fromValue(used));
	}
}