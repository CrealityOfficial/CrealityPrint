#include "reuseablecache.h"

#include "qtuser3d/camera/screencamera.h"

#include "qtuser3d/utils/primitiveshapecache.h"
#include "qtuser3d/trimesh2/conv.h"

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
		m_mainCamera->addCameraObserver(this);

		m_mainCamera->setFovyLimit(100.0f, 0.1f);

		m_modelEffect = new qtuser_3d::XEffect(this);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modelnormal", m_modelEffect);
			viewPass->addFilterKeyMask("view", 0);
			viewPass->setPassCullFace();
			viewPass->setPassDepthTest();
			viewPass->setParameter("transparency", 1.0);
			m_modelEffect->addRenderPass(viewPass);

			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFace_model", m_modelEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			pickPass->setPassDepthTest();
			m_modelEffect->addRenderPass(pickPass);

		}

		m_modelWireFrameEffect = new qtuser_3d::XEffect(this);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modelwireframe", m_modelWireFrameEffect);
			viewPass->addFilterKeyMask("view", 0);
			viewPass->setPassCullFace();
			viewPass->setPassDepthTest();
			viewPass->setParameter("transparency", 1.0);
			m_modelWireFrameEffect->addRenderPass(viewPass);

			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFace_model", m_modelWireFrameEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			pickPass->setPassDepthTest();
			m_modelWireFrameEffect->addRenderPass(pickPass);

		}

		m_captureModelEffect = new qtuser_3d::XEffect(this);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modelnormal", m_captureModelEffect);
			viewPass->addFilterKeyMask("view", 0);
			viewPass->setPassCullFace();
			viewPass->setPassDepthTest();
			viewPass->setParameter("transparency", 1.0);
			m_captureModelEffect->addRenderPass(viewPass);

			qtuser_3d::XRenderPass* rtPass = new qtuser_3d::XRenderPass("modeloffscreen", m_captureModelEffect);
			rtPass->addFilterKeyMask("rt", 0);
			// rtPass->addFilterKeyMask("view", 0);
			rtPass->setPassCullFace();
			rtPass->setPassDepthTest();
			m_captureModelEffect->addRenderPass(rtPass);

			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFace_model", m_captureModelEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			pickPass->setPassDepthTest();
			m_captureModelEffect->addRenderPass(pickPass);

		}

		m_captureModelWireFrameEffect = new qtuser_3d::XEffect(this);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modelwireframe", m_captureModelWireFrameEffect);
			viewPass->addFilterKeyMask("view", 0);
			viewPass->setPassCullFace();
			viewPass->setPassDepthTest();
			viewPass->setParameter("transparency", 1.0);
			m_captureModelWireFrameEffect->addRenderPass(viewPass);

			qtuser_3d::XRenderPass* rtPass = new qtuser_3d::XRenderPass("modeloffscreen", m_captureModelWireFrameEffect);
			// qtuser_3d::XRenderPass* rtPass = new qtuser_3d::XRenderPass("modelwireframe", m_captureModelWireFrameEffect);
			rtPass->addFilterKeyMask("rt", 0);
			// rtPass->addFilterKeyMask("view", 0);
			rtPass->setPassCullFace();
			rtPass->setPassDepthTest();
			m_captureModelWireFrameEffect->addRenderPass(rtPass);

			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFace_model", m_captureModelWireFrameEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			pickPass->setPassDepthTest();
			m_captureModelWireFrameEffect->addRenderPass(pickPass);

		}
		
		m_transparentModelEffect = new qtuser_3d::XEffect(this);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modeltransparent", m_transparentModelEffect);
			viewPass->addFilterKeyMask("alpha", 0);
			viewPass->setPassBlend(Qt3DRender::QBlendEquationArguments::SourceAlpha, Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);
			viewPass->setPassCullFace(Qt3DRender::QCullFace::Back);
			viewPass->setPassNoDepthMask();
			m_transparentModelEffect->addRenderPass(viewPass);

			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFace_model", m_transparentModelEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			pickPass->setPassDepthTest();
			m_transparentModelEffect->addRenderPass(pickPass);

		}
		
		m_previewModelEffect = new qtuser_3d::XEffect(this);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modeltransparent", m_previewModelEffect);
			viewPass->addFilterKeyMask("alpha", 0);
			viewPass->setPassBlend(Qt3DRender::QBlendEquationArguments::SourceAlpha, Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);
			viewPass->setPassCullFace(Qt3DRender::QCullFace::Back);
			viewPass->setPassNoDepthMask();
			m_previewModelEffect->addRenderPass(viewPass);
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

	qtuser_3d::XEffect* ReuseableCache::getCachedModelWireFrameEffect()
	{
		return m_modelWireFrameEffect;
	}

	qtuser_3d::XEffect* ReuseableCache::getCachedCaptureModelEffect()
	{
		return m_captureModelEffect;
	}

	qtuser_3d::XEffect* ReuseableCache::getCachedCaptureModelWireFrameEffect()
	{
		return m_captureModelWireFrameEffect;
	}

	qtuser_3d::XEffect* ReuseableCache::getCachedTransparentModelEffect()
	{
		return m_transparentModelEffect;
	}

	qtuser_3d::XEffect* ReuseableCache::getCachedPreviewModelEffect()
	{
		return m_previewModelEffect;
	}

	qtuser_3d::XEffect* ReuseableCache::getCacheModelLayerEffect()
	{
		if (m_modelLayerEffect == nullptr)
		{
			m_modelLayerEffect = new qtuser_3d::XEffect(this);

			qtuser_3d::XRenderPass* pass = new qtuser_3d::XRenderPass("variablelayerheight", m_modelLayerEffect);
			pass->addFilterKeyMask("view", 0);
			pass->setPassDepthTest();

			qtuser_3d::XRenderPass* rtPass = new qtuser_3d::XRenderPass("modeloffscreen", m_captureModelEffect);
			rtPass->addFilterKeyMask("rt", 0);
			rtPass->setPassCullFace();
			rtPass->setPassDepthTest();

			qtuser_3d::XRenderPass* rtPass1 = new qtuser_3d::XRenderPass("modeloffscreen", m_captureModelWireFrameEffect);
			rtPass1->addFilterKeyMask("rt", 0);
			rtPass1->setPassCullFace();
			rtPass1->setPassDepthTest();

			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFace_model", m_modelEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			pickPass->setPassDepthTest();

			qtuser_3d::XRenderPass* pickPass1 = new qtuser_3d::XRenderPass("pickFace_model", m_modelWireFrameEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			pickPass->setPassDepthTest();
			
			m_modelLayerEffect->addRenderPass(pass);
			m_modelLayerEffect->addRenderPass(rtPass);
			m_modelLayerEffect->addRenderPass(pickPass);
			m_modelLayerEffect->addRenderPass(pickPass1);
		}

		return m_modelLayerEffect;
	}

	void ReuseableCache::intialize()
	{
		resetSection();

		QVariantList values = CONFIG_GLOBAL_VARLIST(modeleffect_statecolors, model_group);
		m_modelEffect->setParameter("stateGain[0]", values);
		m_modelEffect->setParameter("outPlatformGain", QVector4D(1.0, 0.50, 0.50, 1.0));
		m_modelEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));
		m_modelEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));

		m_modelWireFrameEffect->setParameter("stateGain[0]", values);
		m_modelWireFrameEffect->setParameter("outPlatformGain", QVector4D(1.0, 0.50, 0.50, 1.0));
		m_modelWireFrameEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));
		m_modelWireFrameEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));

		m_captureModelEffect->setParameter("stateGain[0]", values);
		m_captureModelEffect->setParameter("outPlatformGain", QVector4D(1.0, 0.50, 0.50, 1.0));
		m_captureModelEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));
		m_captureModelEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));

		m_captureModelWireFrameEffect->setParameter("stateGain[0]", values);
		m_captureModelWireFrameEffect->setParameter("outPlatformGain", QVector4D(1.0, 0.50, 0.50, 1.0));
		m_captureModelWireFrameEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));
		m_captureModelWireFrameEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));

		m_transparentModelEffect->setParameter("stateGain[0]", values);
		m_transparentModelEffect->setParameter("outPlatformGain", QVector4D(1.0, 0.50, 0.50, 1.0));
		m_transparentModelEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));
		m_transparentModelEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));

		m_previewModelEffect->setParameter("stateGain[0]", values);
		m_previewModelEffect->setParameter("outPlatformGain", QVector4D(1.0, 0.50, 0.50, 1.0));
		m_previewModelEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));
		m_previewModelEffect->setParameter("insideGain", QVector4D(1.0, 1.0, 0.50, 1.0));

		QVariantList weights;
		for (size_t i = 0; i < 16; i++)
		{
			weights << QVector4D(1.0, 1.0, 1.0, 1.0);
		}
		m_modelEffect->setParameter("nozzleGain[0]", weights);
		m_modelWireFrameEffect->setParameter("nozzleGain[0]", weights);
		m_captureModelEffect->setParameter("nozzleGain[0]", weights);
		m_captureModelWireFrameEffect->setParameter("nozzleGain[0]", weights);
		m_transparentModelEffect->setParameter("nozzleGain[0]", weights);
		m_previewModelEffect->setParameter("nozzleGain[0]", weights);

		setModelZProjectColor(CONFIG_GLOBAL_VEC4(modeleffect_shadowcolor, model_group));
		setNeedCheckScope(1);

		setVisibleBottomHeight(-0.5f);

		tracePickable(m_indicator->pickable());
		m_indicator->setCameraController(cameraController());
		m_indicator->setWorldIndicatorTracer(this);

		Printer* pt = new Printer(m_printerEntity, nullptr);
		m_printerMangager->addPrinter(pt);
	}

	void ReuseableCache::blockRelation()
	{
		m_mainCamera->camera()->setParent((Qt3DCore::QNode*)nullptr);
	}

	void ReuseableCache::updatePrinterBox(const trimesh::dbox3& box)
	{
		if (!box.valid) return;

		getPrinterMangager()->allPrinterUpdateBox(qtuser_3d::triBox2Box3D(box));

		//	cameraController()->setRotateCenter(visCamera()->orignCenter());


		renderOneFrame();
	}

	void ReuseableCache::setModelZProjectColor(const QVector4D& color)
	{
		m_modelEffect->setParameter("color", color);
		m_modelWireFrameEffect->setParameter("color", color);
		m_captureModelEffect->setParameter("color", color);
		m_captureModelWireFrameEffect->setParameter("color", color);
		m_transparentModelEffect->setParameter("color", color);
		m_previewModelEffect->setParameter("color", color);
	}

	void ReuseableCache::setModelClearColor(const QVector4D& color)
	{
		m_modelEffect->setParameter("clearColor", color.toVector3D());
		m_modelWireFrameEffect->setParameter("clearColor", color.toVector3D());
		m_captureModelEffect->setParameter("clearColor", color.toVector3D());
		m_captureModelWireFrameEffect->setParameter("clearColor", color.toVector3D());
		m_transparentModelEffect->setParameter("clearColor", color.toVector3D());
		m_previewModelEffect->setParameter("clearColor", color.toVector3D());
	}

	void ReuseableCache::setSpaceBox(const QVector3D& minspace, const QVector3D& maxspace)
	{

	}

	void ReuseableCache::setBottom(float bottom)
	{

	}

	void ReuseableCache::setVisibleBottomHeight(float bottomHeight)
	{
		m_modelEffect->setParameter("bottomVisibleHeight", bottomHeight);
		m_modelWireFrameEffect->setParameter("bottomVisibleHeight", bottomHeight);
	}

	void ReuseableCache::setVisibleTopHeight(float topHeight)
	{
		m_modelEffect->setParameter("topVisibleHeight", topHeight);
		m_modelWireFrameEffect->setParameter("topVisibleHeight", topHeight);
	}

	void ReuseableCache::setNeedCheckScope(int checkscope)
	{
		m_modelEffect->setParameter("checkscope", checkscope);
		m_modelWireFrameEffect->setParameter("checkscope", checkscope);
		m_transparentModelEffect->setParameter("checkscope", checkscope);
		m_previewModelEffect->setParameter("checkscope", checkscope);
	}

	void ReuseableCache::resetSection()
	{
		setSection(QVector3D(0.0, 0.0, 100000.0), QVector3D(0.0, 0.0, -10000.0), QVector3D(0.0, 0.0, -1.0));
	}

	void ReuseableCache::setSection(const QVector3D &frontPos, const QVector3D &backPos, const QVector3D &normal)
	{
		m_modelEffect->setParameter("sectionNormal", normal);
		m_modelWireFrameEffect->setParameter("sectionNormal", normal);
		m_transparentModelEffect->setParameter("sectionNormal", normal);
		m_previewModelEffect->setParameter("sectionNormal", normal);

		m_modelEffect->setParameter("sectionFrontPos", frontPos);
		m_modelWireFrameEffect->setParameter("sectionFrontPos", frontPos);
		m_transparentModelEffect->setParameter("sectionFrontPos", frontPos);
		m_previewModelEffect->setParameter("sectionFrontPos", frontPos);

		m_modelEffect->setParameter("sectionBackPos", backPos);
		m_modelWireFrameEffect->setParameter("sectionBackPos", backPos);
		m_transparentModelEffect->setParameter("sectionBackPos", backPos);
		m_previewModelEffect->setParameter("sectionBackPos", backPos);
	}

	void ReuseableCache::setOffset(const QVector3D& offset)
	{
		m_modelEffect->setParameter("offset", offset);
		m_modelWireFrameEffect->setParameter("offset", offset);
		m_transparentModelEffect->setParameter("offset", offset);
		m_previewModelEffect->setParameter("offset", offset);
	}

	void ReuseableCache::setZProjectEnabled(bool enabled)
	{
		m_zProjectPass->setEnabled(enabled);
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

	void ReuseableCache::onBoxChanged(const trimesh::dbox3& box)
	{
		updatePrinterBox(box);
		visCamera()->fittingBoundingBox(getPrinterMangager()->boundingBox());
	}

	void ReuseableCache::onSceneChanged(const trimesh::dbox3& box)
	{
		updateSceneSelectionsGlobalBox(selectionsBoundingbox());
	}

	void ReuseableCache::onCameraChanged(qtuser_3d::ScreenCamera* screenCamera)
	{
		Qt3DRender::QCamera* camera = screenCamera->camera();
		if (camera)
		{
			QVector3D viewDir = camera->viewVector();
			viewDir.normalize();
			QVector3D up = QVector3D(0.0f, 0.0f, 1.0f);
			float a = 0.5f - 0.625f * QVector3D::dotProduct(viewDir, up);
			if (a > 0.5f)
				a = 0.5f;
			else if (a < 0.0f)
				a = 0.0f;

			QVector4D color = QVector4D(0.0f, 0.0f, 0.0f, a);
			setModelZProjectColor(color);
		}
	}

	void ReuseableCache::onSelectionsChanged()
	{
		updateSceneSelectionsGlobalBox(selectionsBoundingbox());
	}

	void ReuseableCache::onSelectionsBoxChanged()
	{
		updateSceneSelectionsGlobalBox(selectionsBoundingbox());
	}

	qtuser_3d::Box3D ReuseableCache::resetCameraWithNewBox(qtuser_3d::WorldIndicatorEntity* entity)
	{
		if (m_printerMangager)
		{
			m_printerMangager->relayout();
			qtuser_3d::Box3D box = m_printerMangager->boundingBox();
			return box;
		}
		return qtuser_3d::Box3D();
	}

	qtuser_3d::CacheTexture* ReuseableCache::getCacheTexture()
	{
		return m_cacheTexture;
	}
}
