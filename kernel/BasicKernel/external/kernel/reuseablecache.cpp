#include "reuseablecache.h"

#include "qtuser3d/camera/screencamera.h"

#include "qtuser3d/utils/primitiveshapecache.h"

#include "cxkernel/render/worldindicatorentity.h"

#include "interface/renderinterface.h"
#include "interface/spaceinterface.h"
#include "internal/render/printerentity.h"
#include "interface/appsettinginterface.h"
#include "interface/camerainterface.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"
#include "cxkernel/interface/jobsinterface.h"

#include <QPolygonOffset>

namespace creative_kernel
{
	ReuseableCache::ReuseableCache(Qt3DCore::QNode* parent)
		: QNode(parent)
		, m_indicator(nullptr)
	{
		m_mainCamera = new qtuser_3d::ScreenCamera(this);

		m_modelEffect = new qtuser_3d::XEffect(this);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("modelwireframe", m_modelEffect);
			viewPass->addFilterKeyMask("view", 0);
			viewPass->addFilterKeyMask("rt", 0);
			viewPass->setPassCullFace();
			viewPass->setPassDepthTest();
			m_modelEffect->addRenderPass(viewPass);


			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFace_pwd", m_modelEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			pickPass->setPassDepthTest();
			m_modelEffect->addRenderPass(pickPass);


			qtuser_3d::XRenderPass* shadowPass = new qtuser_3d::XRenderPass("zproject", m_modelEffect);
			shadowPass->addFilterKeyMask("alpha", 0);
			shadowPass->addFilterKeyMask("modelzproject", 0);
			shadowPass->setPassCullFace(Qt3DRender::QCullFace::Back);
			shadowPass->setPassBlend();
			shadowPass->setPassStencilMask(0xFF);
			shadowPass->setPassStencilOperation(Qt3DRender::QStencilOperationArguments::Keep, Qt3DRender::QStencilOperationArguments::Keep, Qt3DRender::QStencilOperationArguments::Increment);
			shadowPass->setPassStencilFunction(Qt3DRender::QStencilTestArguments::Equal, 0x0, 0xFF);
			shadowPass->setPassDepthTest(Qt3DRender::QDepthTest::Less);

			m_modelEffect->addRenderPass(shadowPass);

		}

		m_supportEffect = new qtuser_3d::XEffect(this);
		{
			qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("support", m_supportEffect);
			viewPass->addFilterKeyMask("view", 0);
			viewPass->setPassCullFace();
			/*Qt3DRender::QPolygonOffset* st = new Qt3DRender::QPolygonOffset(viewPass);
			st->setDepthSteps(10.0);
			viewPass->addRenderState(st);*/
			m_supportEffect->addRenderPass(viewPass);

			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFaceFlag", m_supportEffect);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			m_supportEffect->addRenderPass(pickPass);
		}

		m_mainCamera->camera()->setParent(this);

		m_printerEntity = new PrinterEntity(this);
		m_indicator = new cxkernel::WorldIndicatorEntity(this);
	}
	
	ReuseableCache::~ReuseableCache()
	{
	}

	void ReuseableCache::setPrinterVisible(bool visible)
	{
		m_printerEntity->showPrinterEntity(visible);
	}

	PrinterEntity* ReuseableCache::getCachedPrinterEntity()
	{
		return m_printerEntity;
	}

	cxkernel::WorldIndicatorEntity* ReuseableCache::getIndicatorEntity()
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

	qtuser_3d::XEffect* ReuseableCache::getCachedSupportEffect()
	{
		return m_supportEffect;
	}

	void ReuseableCache::intialize()
	{
		//PRIMITIVEROOT->setParent(this);

		QVariantList values = CONFIG_GLOBAL_VARLIST(modeleffect_statecolors, model_group);
		m_modelEffect->setParameter("stateColors[0]", values);

		QVector4D ambient = CONFIG_GLOBAL_VEC4(modeleffect_ambient, model_group);
		QVector4D diffuse = CONFIG_GLOBAL_VEC4(modeleffect_diffuse, model_group);
		QVector4D specular = CONFIG_GLOBAL_VEC4(modeleffect_specular, model_group);

		m_modelEffect->setParameter("ambient", ambient);
		m_modelEffect->setParameter("diffuse", diffuse);
		m_modelEffect->setParameter("specular", specular);
		setModelZProjectColor(CONFIG_GLOBAL_VEC4(modeleffect_shadowcolor, model_group));

		values = CONFIG_GLOBAL_VARLIST(supporteffect_stateolors, model_group);
		m_supportEffect->setParameter("stateColors[0]", values);

		tracePickable(m_indicator->pickable());
		m_indicator->setCameraController(cameraController());
	}

	void ReuseableCache::blockRelation()
	{
		m_mainCamera->camera()->setParent((Qt3DCore::QNode*)nullptr);
	}

	void ReuseableCache::updatePrinterBox(const qtuser_3d::Box3D& box)
	{
		m_printerEntity->updateBox(box);
		renderOneFrame();
	}

	void ReuseableCache::setModelZProjectColor(const QVector4D& color)
	{
		m_modelEffect->setParameter("color", color);
	}

	void ReuseableCache::setModelClearColor(const QVector4D& color)
	{
		m_modelEffect->setParameter("clearColor", color.toVector3D());
	}

	void ReuseableCache::setSpaceBox(const QVector3D& minspace, const QVector3D& maxspace)
	{
		QVector3D offset(0.1f, 0.1f, 0.1f);
		m_modelEffect->setParameter("minSpace", minspace - offset);
		m_modelEffect->setParameter("maxSpace", maxspace + offset);
		m_supportEffect->setParameter("minSpace", minspace);
		m_supportEffect->setParameter("maxSpace", maxspace);
	}

	void ReuseableCache::setBottom(float bottom)
	{
		m_modelEffect->setParameter("bottom", bottom);
		m_supportEffect->setParameter("bottom", bottom);
	}

	void ReuseableCache::setVisibleBottomHeight(float bottomHeight)
	{
		m_modelEffect->setParameter("bottomVisibleHeight", bottomHeight);
		m_supportEffect->setParameter("bottomVisibleHeight", bottomHeight);
	}

	void ReuseableCache::setVisibleTopHeight(float topHeight)
	{
		m_modelEffect->setParameter("topVisibleHeight", topHeight);
		m_supportEffect->setParameter("topVisibleHeight", topHeight);
	}

	void ReuseableCache::setNeedCheckScope(int checkscope)
	{
		m_modelEffect->setParameter("checkscope", checkscope);
	}

	void ReuseableCache::setIndicatorScreenPos(const QPoint& p, float length)
	{
		m_indicator->setLength(length);
		m_indicator->setScreenPos(p);

		bool cap = cxkernel::jobExecutorAvaillable();
		requestVisUpdate(false);
	}

	void ReuseableCache::resetIndicatorAngle() {
		m_indicator->resetCameraAngle();
	}
}
