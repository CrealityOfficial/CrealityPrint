#include "reuseablecache.h"

#include "qtuser3d/camera/screencamera.h"

#include "qtuser3d/utils/primitiveshapecache.h"
#include "renderpass/zprojectrenderpass.h"

#include "entity/worldindicatorentity.h"

#include "interface/renderinterface.h"
#include "interface/spaceinterface.h"
#include "internal/render/printerentity.h"
#include "interface/appsettinginterface.h"
#include "interface/camerainterface.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "internal/multi_printer/printermanager.h"

#include <QPolygonOffset>
#include "qtuser3d/utils/cachetexture.h"

namespace creative_kernel
{
	ReuseableCache::ReuseableCache(Qt3DCore::QNode* parent)
		: QNode(parent)
		, m_indicator(nullptr)
		, m_modelLayerEffect(nullptr)
	{
		m_mainCamera = new qtuser_3d::ScreenCamera(this);
		m_mainCamera->setFovyLimit(100.0f, 0.1f);

		m_modelEffect = new qtuser_3d::XEffect(this);
		m_renderModeParameter = new Qt3DRender::QParameter("renderModel", 1, this);
		m_modelEffect->addParameter(m_renderModeParameter);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modelwireframe", m_modelEffect);
			viewPass->addFilterKeyMask("view", 0);
			viewPass->setPassCullFace();
			viewPass->setPassDepthTest();
			m_modelEffect->addRenderPass(viewPass);

			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFace_pwd", m_modelEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			pickPass->setPassDepthTest();
			m_modelEffect->addRenderPass(pickPass);

			qtuser_3d::XRenderPass* shadowPass = new qtuser_3d::ZProjectRenderPass(m_modelEffect);
			shadowPass->addFilterKeyMask("modelzproject", 0);
			//shadowPass->setEnabled(false);
			m_modelEffect->addRenderPass(shadowPass);

		}
		
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
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modelpreview", m_modelCombindEffect);
			viewPass->addFilterKeyMask("alpha", 0);
			viewPass->setPassBlend(Qt3DRender::QBlendEquationArguments::SourceAlpha, Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);
			viewPass->setPassCullFace(Qt3DRender::QCullFace::Back);
			viewPass->setPassDepthTest();
			viewPass->setPassNoDepthMask();
			m_modelCombindEffect->addRenderPass(viewPass);

			qtuser_3d::XRenderPass* rtPass = new qtuser_3d::XRenderPass("modeloffscreen", m_modelCombindEffect);
			rtPass->addFilterKeyMask("rt", 0);
			rtPass->setPassCullFace();
			rtPass->setPassDepthTest();
			m_modelCombindEffect->addRenderPass(rtPass);

			qtuser_3d::XRenderPass* shadowPass = new qtuser_3d::ZProjectRenderPass(m_modelCombindEffect);
			shadowPass->addFilterKeyMask("modelzproject", 0);
			shadowPass->setEnabled(false);
			m_modelCombindEffect->addRenderPass(shadowPass);
		}

		m_supportEffect = new qtuser_3d::XEffect(this);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("support", m_supportEffect);
			viewPass->addFilterKeyMask("view", 0);
			viewPass->setPassCullFace();
			/*Qt3DRender::QPolygonOffset* st = new Qt3DRender::QPolygonOffset(viewPass);
			st->setDepthSteps(10.0);
			viewPass->addRenderState(st);*/
			viewPass->setParameter("ambient", QVector4D(0.0, 0.0, 0.0, 1.0));
			viewPass->setParameter("diffuse", QVector4D(1.0, 1.0, 1.0, 1.0));
			viewPass->setParameter("specular", QVector4D(0.125, 0.125, 0.125, 1.0));
			viewPass->setParameter("topVisibleHeight", 100000.0);
			viewPass->setParameter("bottomVisibleHeight", -10000.0);
			m_supportEffect->addRenderPass(viewPass);

			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFaceFlag", m_supportEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			m_supportEffect->addRenderPass(pickPass);
		}

		m_mainCamera->camera()->setParent(this);

		m_printerEntitisRoot = new Qt3DCore::QNode(this);

		m_cacheTexture = new qtuser_3d::CacheTexture(this);

		m_printerEntity = new PrinterEntity(m_printerEntitisRoot);

		m_printerMangager = new PrinterMangager(this);


		m_indicator = new qtuser_3d::WorldIndicatorEntity(this);


	}

	ReuseableCache::~ReuseableCache()
	{
	}

	void ReuseableCache::setPrinterVisible(bool visible)
	{
		m_printerMangager->getSelectedPrinter()->setVisibility(visible);
	}

	qtuser_3d::WorldIndicatorEntity* ReuseableCache::getIndicatorEntity()
	{
		return m_indicator;
	}

	qtuser_3d::ScreenCamera* ReuseableCache::mainScreenCamera()
	{
		return m_mainCamera;
	}

	qtuser_3d::XEffect* ReuseableCache::getCachedModelEffect()
	{
		return m_modelEffect;
	}

	qtuser_3d::XEffect* ReuseableCache::getCachedModelOffscreenEffect()
	{
		return m_modelOffscreenEffect;
	}

	qtuser_3d::XEffect* ReuseableCache::getCachedModelCombindEffect()
	{
		return m_modelCombindEffect;
	}

	qtuser_3d::XEffect* ReuseableCache::getCachedSupportEffect()
	{
		return m_supportEffect;
	}

	qtuser_3d::XEffect* ReuseableCache::getCacheModelLayerEffect()
	{
		if (m_modelLayerEffect == nullptr)
		{
			m_modelLayerEffect = new qtuser_3d::XEffect(this);

			qtuser_3d::XRenderPass* pass = new qtuser_3d::XRenderPass("variablelayerheight", m_modelLayerEffect);
			pass->addFilterKeyMask("view", 0);
			pass->setPassDepthTest();

			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFace_pwd", m_modelEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			pickPass->setPassDepthTest();
			
			m_modelLayerEffect->addRenderPass(pass);
			m_modelLayerEffect->addRenderPass(pickPass);
		}

		return m_modelLayerEffect;
	}

	void ReuseableCache::intialize()
	{
		//PRIMITIVEROOT->setParent(this);
		resetSection();

		QVariantList values = CONFIG_GLOBAL_VARLIST(modeleffect_statecolors, model_group);
		m_modelEffect->setParameter("stateGain[0]", values);
		m_modelEffect->setParameter("outPlatformGain", QVector4D(1.0, 0.50, 0.50, 1.0));
		m_modelEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));
		m_modelEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));

		// QVariantList emptyList;
		// setSpecificColorLayers(emptyList);

		QVariantList weights;
		for (size_t i = 0; i < 16; i++)
		{
			weights << QVector4D(1.0, 1.0, 1.0, 1.0);
		}
		m_modelEffect->setParameter("nozzleGain[0]", weights);

		QVector4D ambient = CONFIG_GLOBAL_VEC4(modeleffect_ambient, model_group);
		QVector4D diffuse = CONFIG_GLOBAL_VEC4(modeleffect_diffuse, model_group);
		QVector4D specular = CONFIG_GLOBAL_VEC4(modeleffect_specular, model_group);

		m_modelEffect->setParameter("ambient", ambient);
		m_modelEffect->setParameter("diffuse", diffuse);
		m_modelEffect->setParameter("specular", specular);
		m_modelEffect->setParameter("transparency", 1.0);
		m_modelEffect->setParameter("offset", QVector2D(0, 0));

		setModelZProjectColor(CONFIG_GLOBAL_VEC4(modeleffect_shadowcolor, model_group));
		setNeedCheckScope(1);

		values = CONFIG_GLOBAL_VARLIST(supporteffect_stateolors, model_group);
		m_supportEffect->setParameter("stateColors[0]", values);
		m_supportEffect->setParameter("ambient", ambient);
		m_supportEffect->setParameter("diffuse", diffuse);
		m_supportEffect->setParameter("specular", specular);

		setVisibleBottomHeight(-0.5f);

		tracePickable(m_indicator->pickable());
		m_indicator->setCameraController(cameraController());

		Printer* pt = new Printer(m_printerEntity, nullptr);
		m_printerMangager->addPrinter(pt);
	}

	void ReuseableCache::blockRelation()
	{
		m_mainCamera->camera()->setParent((Qt3DCore::QNode*)nullptr);
	}

	void ReuseableCache::updatePrinterBox(const qtuser_3d::Box3D& box)
	{
	}

	void ReuseableCache::setModelZProjectColor(const QVector4D& color)
	{
		m_modelEffect->setParameter("color", color);
		m_modelCombindEffect->setParameter("color", color);
		m_modelOffscreenEffect->setParameter("color", color);
	}

	void ReuseableCache::setModelClearColor(const QVector4D& color)
	{
		m_modelEffect->setParameter("clearColor", color.toVector3D());
		m_modelCombindEffect->setParameter("clearColor", color.toVector3D());
		m_modelOffscreenEffect->setParameter("clearColor", color.toVector3D());
	}

	void ReuseableCache::setSpaceBox(const QVector3D& minspace, const QVector3D& maxspace)
	{
		/*QVector3D offset(0.1f, 0.1f, 0.1f);
		m_modelEffect->setParameter("minSpace", minspace - offset);
		m_modelEffect->setParameter("maxSpace", maxspace + offset);*/

		m_supportEffect->setParameter("minSpace", minspace);
		m_supportEffect->setParameter("maxSpace", maxspace);
	}

	void ReuseableCache::setBottom(float bottom)
	{
		m_modelEffect->setParameter("bottom", bottom);
		m_modelCombindEffect->setParameter("bottom", bottom);
		m_modelOffscreenEffect->setParameter("bottom", bottom);
		m_supportEffect->setParameter("bottom", bottom);
	}

	void ReuseableCache::setVisibleBottomHeight(float bottomHeight)
	{
		m_modelEffect->setParameter("bottomVisibleHeight", bottomHeight);
		m_modelCombindEffect->setParameter("bottomVisibleHeight", bottomHeight);
		m_modelOffscreenEffect->setParameter("bottomVisibleHeight", bottomHeight);
		m_supportEffect->setParameter("bottomVisibleHeight", bottomHeight);
	}

	void ReuseableCache::setVisibleTopHeight(float topHeight)
	{
		m_modelEffect->setParameter("topVisibleHeight", topHeight);
		m_modelCombindEffect->setParameter("topVisibleHeight", topHeight);
		m_modelOffscreenEffect->setParameter("topVisibleHeight", topHeight);
		m_supportEffect->setParameter("topVisibleHeight", topHeight);
	}

	void ReuseableCache::setNeedCheckScope(int checkscope)
	{
		m_modelEffect->setParameter("checkscope", checkscope);
		m_modelCombindEffect->setParameter("checkscope", checkscope);
		m_modelOffscreenEffect->setParameter("checkscope", checkscope);
	}

	void ReuseableCache::resetSection()
	{
		setSection(QVector3D(0.0, 0.0, 100000.0), QVector3D(0.0, 0.0, -10000.0), QVector3D(0.0, 0.0, -1.0));
	}

	void ReuseableCache::setSection(const QVector3D &frontPos, const QVector3D &backPos, const QVector3D &normal)
	{
		m_modelEffect->setParameter("sectionNormal", normal);
		m_supportEffect->setParameter("sectionNormal", normal);

		m_modelEffect->setParameter("sectionFrontPos", frontPos);
		m_supportEffect->setParameter("sectionFrontPos", frontPos);

		m_modelEffect->setParameter("sectionBackPos", backPos);
		m_supportEffect->setParameter("sectionBackPos", backPos);
	}

	void ReuseableCache::setOffset(const QVector2D& offset)
	{
		m_modelEffect->setParameter("offset", offset);
		m_modelCombindEffect->setParameter("offset", offset);
		m_modelOffscreenEffect->setParameter("offset", offset);
	}

	void ReuseableCache::setSpecificColorLayers(const QVariantList& specificColorLayers)
	{
		// m_modelCombindEffect->setParameter("specificColorLayers[0]", specificColorLayers);
		// m_modelOffscreenEffect->setParameter("specificColorLayers[0]", specificColorLayers);

		// m_modelCombindEffect->setParameter("layersCount", specificColorLayers.count());
		// m_modelOffscreenEffect->setParameter("layersCount", specificColorLayers.count());
	}

	void ReuseableCache::setIndicatorScreenPos(const QPoint& p, float length)
	{
		m_indicator->setLength(length);
		m_indicator->setScreenPos(p);

		bool cap = cxkernel::jobExecutorAvaillable();
		requestVisUpdate(false);
	}

	void ReuseableCache::resetIndicatorAngle() {

		getPrinterMangager()->refittingExtendBox();

		m_indicator->resetCameraAngle();
	}
	void ReuseableCache::setTopCameraAngle() {
		m_indicator->topCameraAngle();
	}
	void ReuseableCache::setBottomCameraAngle() {
		m_indicator->bottomCameraAngle();
	}
	void ReuseableCache::setLeftCameraAngle() {
		m_indicator->leftCameraAngle();
	}
	void ReuseableCache::setRightCameraAngle() {
		m_indicator->rightCameraAngle();
	}
	void ReuseableCache::setFrontCameraAngle() {
		m_indicator->frontCameraAngle();
	}
	void ReuseableCache::setBackCameraAngle() {
		m_indicator->backCameraAngle();
	}
	void ReuseableCache::setCameraZoom(float scale) {
		qtuser_3d::ScreenCamera* sc = creative_kernel::visCamera();
		sc->zoom(scale);
	}

	Qt3DCore::QNode* ReuseableCache::getCachedPrinterEntitiesRoot()
	{
		return m_printerEntitisRoot;
	}

	PrinterMangager* ReuseableCache::getPrinterMangager()
	{
		return m_printerMangager;
	}

	QObject* ReuseableCache::getPrinterManagerObject()
	{
		return getPrinterMangager();
	}
	
	void ReuseableCache::setModelVisualMode(ModelVisualMode mode)
	{
		m_renderModeParameter->setValue((int)mode);
	}

	int ReuseableCache::getModelVisualMode()
	{
		return m_renderModeParameter->value().toInt();
	}

	Qt3DRender::QParameter* ReuseableCache::renderModeParameter()
	{
		return m_renderModeParameter;
	}

	qtuser_3d::CacheTexture* ReuseableCache::getCacheTexture()
	{
		return m_cacheTexture;
	}
}
