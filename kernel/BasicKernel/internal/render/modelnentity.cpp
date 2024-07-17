#include "modelnentity.h"
#include "entity/boxentity.h"
#include "qtuser3d/trimesh2/renderprimitive.h"
#include "qtuser3d/trimesh2/trimeshrender.h"
#include "entity/nozzleregionentity.h"
#include "qtuser3d/refactor/xeffect.h"
#include "entity/pureentity.h"
#include "qtuser3d/utils/texturecreator.h"
#include <QVector2D>
#include "interface/spaceinterface.h"

namespace creative_kernel
{
	ModelNEntity::ModelNEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		m_boxEntity = new qtuser_3d::BoxEntity();
		m_boxEntity->setColor(QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
		m_boxEntity->setLineWidth(3.0f);

		m_lineEntity = new qtuser_3d::PureEntity();
		qtuser_3d::XRenderPass* pass = m_lineEntity->xeffect()->findRenderPass(0);
		if (pass)
		{
			pass->addFilterKeyMask("alpha2nd", 0);
			pass->setParameter("color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
			pass->setLineWidth(3.0f);
			pass->setPassDepthTest(Qt3DRender::QDepthTest::LessOrEqual);
			pass->setPassNoDepthMask();
		}
		setOuterLinesVisibility(false);
		
		m_nozzleRegionEntity = new qtuser_3d::NozzleRegionEntity(this);
		setNozzleRegionVisibility(false);

		m_stateParameter = setParameter("state", 0);

		m_vertexBaseParameter = setParameter("vertexBase", QPoint(0, 0));
		
		m_supportCosParameter = setParameter("supportCos", 0.5);
		m_hoverParameter = setParameter("hoverState", 0);
		
		// m_transparencyParameter = setParameter("transparency", 1.0f);
		m_lightingFlagParameter = setParameter("lightingEnable", 1);

		m_nozzleParameter = setParameter("nozzle", 0);

		m_textureDiffuse = setParameter("textureDiffuse", QVariant::fromValue(0));
		m_textureAmbient = setParameter("textureAmbient", QVariant::fromValue(0));
		m_textureSpecular = setParameter("textureSpecular", QVariant::fromValue(0));
		m_textureNormal = setParameter("textureNormal", QVariant::fromValue(0));
		
		m_layersTextureParameter = setParameter("layersTexture", QVariant::fromValue(0));
		m_modelHeightParameter = setParameter("modelHeightRange", QVariant::fromValue(0));

		connect(this, &ModelNEntity::enabledChanged, this, [=](bool enabled)
		{
			m_lineEntity->setEnabled(enabled && m_lineEntityVisible);
			m_nozzleRegionEntity->setEnabled(enabled && m_nozzleRegionVisible);
		});
	}
	
	ModelNEntity::~ModelNEntity()
	{
		if (!m_boxEntity->parent())
			delete m_boxEntity;

		m_lineEntity->setParent((Qt3DCore::QNode *)nullptr);
		delete m_lineEntity;

		m_nozzleRegionEntity->setParent((Qt3DCore::QNode*)nullptr);
		delete m_nozzleRegionEntity;
	}

	void ModelNEntity::setBoxEnabled(bool enabled)
	{
		m_boxEntity->setEnabled(enabled);
	}

	void ModelNEntity::setBoxVisibility(bool visible)
	{
		m_boxEntity->setParent(visible ? (Qt3DCore::QNode*)this->parent() : nullptr);
	}

	void ModelNEntity::updateBoxLocal(const qtuser_3d::Box3D& box, const QMatrix4x4& parentMatrix)
	{
		m_boxEntity->update(box, parentMatrix);
		m_modelHeightParameter->setValue(QVector2D(box.min.z(), box.max.z()));
	}

	void ModelNEntity::setLinesPose(const QMatrix4x4& pose)
	{
		m_lineEntity->setPose(pose);
		m_nozzleRegionEntity->setPose(pose);
	}

	void ModelNEntity::updateLines(const std::vector<trimesh::vec3>& lines)
	{
		if (lines.size() <= 0)
			return;

		std::vector<trimesh::vec3> rlines;
		qtuser_3d::loopPolygon2Lines(lines, rlines);
		m_lineEntity->setGeometry(qtuser_3d::createLinesGeometry(rlines), Qt3DRender::QGeometryRenderer::Lines);

		trimesh::box3 box;
		for (auto line : lines)
		{
			box += line;
		}

		trimesh::vec3 center = box.center();
		std::vector<trimesh::vec3> newLines;
		newLines.push_back(center);
		newLines.insert(newLines.cend(), lines.begin(), lines.end());
		newLines.push_back(lines.front());
		m_nozzleRegionEntity->setGeometry(qtuser_3d::createTriangles(newLines), Qt3DRender::QGeometryRenderer::TriangleFan);
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

	void ModelNEntity::setState(int state)
	{
		m_stateParameter->setValue(state);
	}

	int ModelNEntity::getState()
	{
		return m_stateParameter->value().toInt();
	}

	void ModelNEntity::setNozzle(int nozzle)
	{
		m_nozzleParameter->setValue(nozzle);
	}

	void ModelNEntity::setVertexBase(QPoint vertexBase)
	{
		m_vertexBaseParameter->setValue(vertexBase);
	}

	/*void ModelNEntity::setErrorState(bool error)
	{
		m_errorParameter->setValue(error ? 1.0f : 0.0f);
	}*/

	void ModelNEntity::setSupportCos(float supportCos)
	{
		m_supportCosParameter->setValue(supportCos);
	}

	/*void ModelNEntity::setFanZhuan(int fz)
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
	}*/

	void ModelNEntity::setTransparency(float alpha)
	{
		// m_transparencyParameter->setValue(alpha);
	}

	void ModelNEntity::setLightingEnable(bool enable)
	{
		m_lightingFlagParameter->setValue(enable ? 1 : 0);
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

	void ModelNEntity::setLayersTexture(Qt3DRender::QTexture2D* tex)
	{
		m_layersTextureParameter->setValue(QVariant::fromValue(tex));
	}

	void ModelNEntity::setAssociatePrinterBox(const qtuser_3d::Box3D& box)
	{
		QVector3D offset(0.1f, 0.1f, 0.1f);
		QVector3D minspace = box.min;
		QVector3D maxspace = box.max;
		setParameter("minSpace", minspace - offset);
		setParameter("maxSpace", maxspace + offset);
	}

	void ModelNEntity::setOuterLinesColor(const QVector4D& color)
	{
		m_lineEntity->setColor(color);
	}

	void ModelNEntity::setOuterLinesVisibility(bool visible)
	{
		if (m_lineEntity->parent() != this->parent())
		{
			m_lineEntity->setParent((Qt3DCore::QNode*)this->parent());
		}
		if (m_lineEntity->isEnabled() != visible)
		{
			m_lineEntity->setEnabled(visible);
		}
		m_lineEntityVisible = visible;
	}

	void ModelNEntity::setNozzleRegionVisibility(bool visible)
	{
		if (m_nozzleRegionEntity->parent() != this->parent())
		{
			m_nozzleRegionEntity->setParent((Qt3DCore::QNode*)this->parent());
		}
		if (m_nozzleRegionEntity->isEnabled() != visible)
		{
			m_nozzleRegionEntity->setEnabled(visible);
		}
		m_nozzleRegionVisible = visible;
	}

	void ModelNEntity::setLayerHeightProfile(const std::vector<double>& layers, float minLayerHeight, float maxLayerHeight, float uniqueLayerHeight)
	{
		if (layers.empty() || minLayerHeight <= 0.0f || maxLayerHeight <= 0.0f || uniqueLayerHeight <= 0.0f)
		{
			return;
		}

		if (maxLayerHeight < minLayerHeight || minLayerHeight > uniqueLayerHeight || uniqueLayerHeight > maxLayerHeight)
		{
			return;
		}

		QVector4D orange(1.000000, 0.427451, 0.000000, 1.0);
		QVector4D grey(0.427451, 0.427451, 0.427451, 1.0);
		QVector4D green(0.090196, 0.800000, 0.372549, 1.0);
	
		int pixelHeight = 4096;

		double maxHeight = layers.back();
		std::vector<double>::const_iterator dataPtr = layers.begin();

		QImage image(1, pixelHeight, QImage::Format::Format_RGB888);

		for (int i = 0; i < pixelHeight; i++) {

			double currentH = i * maxHeight / (pixelHeight - 1);
			if (*dataPtr <= currentH && currentH <= *(dataPtr + 1)) {
			}
			else {
				dataPtr += 2;
			}

			double bottom = (*dataPtr);
			double top = *(dataPtr + 1);
			double middle = (top + bottom) * 0.5;
			double layerHeight = top - bottom; //��ǰ���
			double diff = abs(currentH - middle) / layerHeight * 2.0;
			double light = 1.0 - diff * diff;
			//light = sqrt(light);
#if DEBUG
			printf("light = %f;  layerHeight = %f\n", light, layerHeight);
#endif // DEBUG

			light = light * 0.5 + 0.5;

			QVector4D interposeColor;

			if (abs(uniqueLayerHeight - layerHeight) < 0.0001)
			{
				interposeColor = grey;
			}
			else {
				if (layerHeight > uniqueLayerHeight)
				{
					float interpose = (layerHeight - uniqueLayerHeight) / (maxLayerHeight - uniqueLayerHeight);
					interpose = std::min(std::max(interpose, 0.0f), 1.0f);
					interposeColor = grey * (1.0 - interpose) + orange * interpose;
				}
				else if (layerHeight < uniqueLayerHeight)
				{
					float interpose = (layerHeight - minLayerHeight) / (uniqueLayerHeight - minLayerHeight);
					interpose = std::min(std::max(interpose, 0.0f), 1.0f);
					interposeColor = green * (1.0 - interpose) + grey * interpose;
				}
			}

			QVector4D diffuseLight = interposeColor * light;

			image.setPixelColor(0, i, QColor(diffuseLight.x() * 255, diffuseLight.y() * 255, diffuseLight.z() * 255));
		}

		Qt3DRender::QTexture2D* tex = qtuser_3d::createFromImage(image);
		tex->setGenerateMipMaps(true);

		setLayersTexture(tex);
	}
}

