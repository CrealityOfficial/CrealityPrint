#include "modelnentity.h"
#include "qtuser3d/trimesh2/renderprimitive.h"
#include "qtuser3d/trimesh2/trimeshrender.h"
#include "entity/nozzleregionentity.h"
#include "qtuser3d/refactor/xeffect.h"
#include "entity/pureentity.h"
#include "data/modeln.h"
#include "qtuser3d/utils/texturecreator.h"
#include <QVector2D>
#include "interface/spaceinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/visualsceneinterface.h"
#include "external/slice/sliceflow.h"
#include "kernel/kernel.h"

namespace creative_kernel
{
	ModelNEntity::ModelNEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		

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

		m_paletteParameter = setParameter("palette[0]", QVariantList());

		m_modelOffscreenEffect = new qtuser_3d::XEffect(this);
		{
			qtuser_3d::XRenderPass* rtPass = new qtuser_3d::XRenderPass("modeloffscreen", m_modelOffscreenEffect);
			rtPass->addFilterKeyMask("rt", 0);
			rtPass->setPassCullFace();
			rtPass->setPassDepthTest();
			m_modelOffscreenEffect->addRenderPass(rtPass);
		}

		m_modelCombindEffect = new qtuser_3d::XEffect(this);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modeltransparent", m_modelCombindEffect);
			viewPass->addFilterKeyMask("alpha", 0);
			viewPass->setPassBlend(Qt3DRender::QBlendEquationArguments::SourceAlpha, Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);
			viewPass->setPassCullFace(Qt3DRender::QCullFace::Back);
			viewPass->setPassNoDepthMask();
			m_modelCombindEffect->addRenderPass(viewPass);

			qtuser_3d::XRenderPass* rtPass = new qtuser_3d::XRenderPass("modeloffscreen", m_modelCombindEffect);
			rtPass->addFilterKeyMask("rt", 0);
			rtPass->setPassCullFace();
			rtPass->setPassDepthTest();
			m_modelCombindEffect->addRenderPass(rtPass);
		}
		
		QVariantList emptyList;
		setLayersColor(emptyList);

	}
	
	ModelNEntity::~ModelNEntity()
	{
	}

	void ModelNEntity::updateBoxLocal(const qtuser_3d::Box3D& box, const QMatrix4x4& parentMatrix)
	{
		m_modelHeightParameter->setValue(QVector2D(box.min.z(), box.max.z()));
	}

	void ModelNEntity::updateRender()
	{
		applyModelEffect();
	}

	void ModelNEntity::setOffscreenRenderEnabled(bool enabled, bool applyNow)
	{
		m_offscreenRenderEnabled = enabled;
		if (applyNow)
			applyModelEffect();
	}
		
	void ModelNEntity::setEntityType(int type, bool applyNow)
	{
		m_entityType = type;
		if (applyNow)
			applyModelEffect();
	}

	void ModelNEntity::setVisible(bool visible, bool applyNow)
	{
		int p = phase();
		if (p == 0)
			m_prepareVisible = visible;
		else
			m_previewVisible = visible;

		if (applyNow)
			applyModelEffect();
	}

	bool ModelNEntity::isVisible()
	{
		return m_prepareVisible;
	}

	void ModelNEntity::useCustomEffect(qtuser_3d::XEffect* effect)
	{
		m_customEffect = effect;
		if (m_customEffect)
		{
			setEffect(m_customEffect);
			if (!isEnabled())
				setEnabled(true);
		}
		else
			updateRender();
	}

	int ModelNEntity::phase()
	{
		int p = getKernel()->currentPhase();
		if (p == Preview)
		{
			SliceFlow* sliceFlow = getKernel()->sliceFlow();
			if (sliceFlow->isPreviewGCodeFile())
			{
				p = GcodePreview;
			}
		}

		return p;
	}

	void ModelNEntity::applyModelEffect()
	{
		if (m_customEffect)
			return;

		qtuser_3d::XEffect* effect = NULL;
		int p = phase();
		if (p == Prepare)
		{
			if (m_entityType == NormalEntity)
			{	
				if (m_prepareVisible)
				{
					if (m_usingLayer)
						effect = getCacheModelLayerEffect();
					else 
					{
						ModelVisualMode mode = modelVisualMode();
						if (!m_offscreenRenderEnabled)
						{
							if (mode == ModelVisualMode::mvm_face)
								effect = getCachedModelEffect();
							else
								effect = getCachedModelWireFrameEffect();
						}
						else 
						{
							if (mode == ModelVisualMode::mvm_face)
								effect = getCachedCaptureModelEffect();
							else
								effect = getCachedCaptureModelWireFrameEffect();
						}
					}
				}
				else 
				{
					effect = NULL;
				}
			}
			else if (m_entityType == SpecialEntity)
			{
				if (m_prepareVisible)
					effect = getCachedTransparentModelEffect();
				else
				 	effect = NULL;
			}
		}
		else if (p == Preview)
		{
			if (m_entityType == NormalEntity)
			{
				if (m_prepareVisible && m_previewVisible)
				{
					if (m_offscreenRenderEnabled)
						effect = m_modelCombindEffect;
					else 
						effect = getCachedPreviewModelEffect();
				}
				else 
				{
					if (m_offscreenRenderEnabled)
						effect = m_modelOffscreenEffect;
					else 
						effect = NULL;
				}
			}
			else if (m_entityType == SpecialEntity)
			{
				effect = NULL;
			}
		}
		else if (p == GcodePreview)
		{
			/* always unvisible in gcode preview */
			if (m_entityType == NormalEntity)
			{
				if (m_offscreenRenderEnabled)
					effect = m_modelOffscreenEffect;
				else 
					effect = NULL;
			}
			else if (m_entityType == SpecialEntity)
			{
				effect = NULL;
			}
		}

		if (effect)
		{
			setEnabled(true);
			if (effect != xeffect())
				setEffect(effect);
		}
		else 
		{
			setEnabled(false);
		}
	}

	void ModelNEntity::setPalette(const QVariantList& colors)
	{
		if (colors.size() <= 0)
			return;

		 m_paletteParameter->setValue(colors);
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

	void ModelNEntity::setLayersColor(const QVariantList& layersColor)
	{
		// m_modelCombindEffect->setParameter("specificColorLayers[0]", layersColor);
		// m_modelOffscreenEffect->setParameter("specificColorLayers[0]", layersColor);
		setParameter("specificColorLayers[0]", layersColor);

		// m_modelCombindEffect->setParameter("layersCount", layersColor.count());
		// m_modelOffscreenEffect->setParameter("layersCount", layersColor.count());
		setParameter("layersCount", layersColor.count());
	}

	void ModelNEntity::setOffscreenRenderOffset(const QVector3D& offset)
	{
		// m_modelCombindEffect->setParameter("offset1", offset);
		// m_modelOffscreenEffect->setParameter("offset1", offset);
		setParameter("offset1", offset);
	}

	void ModelNEntity::useVariableLayerHeightEffect()
	{
		m_usingLayer = true;
		applyModelEffect();
	}

	void ModelNEntity::useDefaultModelEffect()
	{
		m_usingLayer = false;
		applyModelEffect();
	}

	void ModelNEntity::setAssociatePrinterBox(const qtuser_3d::Box3D& box)
	{
		QVector3D offset(0.1f, 0.1f, 0.1f);
		QVector3D minspace = box.min;
		QVector3D maxspace = box.max;
		setParameter("minSpace", minspace - offset);
		setParameter("maxSpace", maxspace + offset);
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

		QVector4D orange(1.000000f, 0.427451f, 0.000000f, 1.0f);
		QVector4D grey(0.427451f, 0.427451f, 0.427451f, 1.0f);
		QVector4D green(0.090196f, 0.800000f, 0.372549f, 1.0f);
	
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

