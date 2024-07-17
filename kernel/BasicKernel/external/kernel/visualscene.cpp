#include "visualscene.h"
#include <QtCore/QDebug>

#include "cxkernel/interface/undointerface.h"
#include "interface/eventinterface.h"
#include "interface/spaceinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/selectorinterface.h"

#include "qtuser3d/framegraph/colorpicker.h"
#include "qtuser3d/framegraph/surface.h"
#include "qtuser3d/framegraph/texturerendertarget.h"

#include "interface/appsettinginterface.h"
#include "interface/renderinterface.h"
#include "interface/camerainterface.h"

#include "internal/render/printerentity.h"
#include "internal/utils/visscenehandler.h"
#include "internal/utils/wipetowerhandler.h"

#include "entity/rectlineentity.h"
#include "qtuser3d/geometry/basicshapecreatehelper.h"

#include "qtuser3d/scene/sceneoperatemode.h"
#include "kernel/kernel.h"
#include "data/modelspaceundo.h"
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/refactor/xeffect.h"
#include "renderpass/purerenderpass.h"
#include "qtuser3d/geometry/boxcreatehelper.h"
#include <QElapsedTimer>
#include "mainthreadjob.h"
#include "cxkernel/interface/jobsinterface.h"

#include <QCoreApplication>

namespace creative_kernel
{
	VisualScene::VisualScene(Qt3DCore::QNode* parent)
		:RenderGraph(parent)
		, m_handlerEnabled(true)
	{
		m_rootEntity = new Qt3DCore::QEntity(this);
		m_customRootEntity = new Qt3DCore::QEntity(this);

		m_surface = new qtuser_3d::Surface(this);

		m_colorPicker = new qtuser_3d::ColorPicker();
		m_colorPicker->setParent(m_surface->getCameraViewportFrameGraphNode());
		
		m_textureRenderTarget = new qtuser_3d::TextureRenderTarget(m_rootEntity, QSize(500, 500), true, true);
		m_colorPicker->setTextureRenderTarget(m_textureRenderTarget);

		connect(m_colorPicker, SIGNAL(signalUpdate()), this, SIGNAL(signalUpdate()));

		m_handler = new VisSceneHandler(this);
		m_towerHandler = new WipeTowerHandler(this);

		m_rectLine = new qtuser_3d::RectLineEntity(m_rootEntity);
		m_rectLine->setColor(QVector4D(1.0f, 0.7530f, 0.0f, 1.0f));

		m_heightReferEntity = new qtuser_3d::XEntity(m_rootEntity);
		{
			qtuser_3d::PureRenderPass* pass = new qtuser_3d::PureRenderPass(m_heightReferEntity);
			pass->setLineWidth(2.0f);
			pass->addFilterKeyMask("view", 0);
			pass->setPassDepthTest();
			pass->setColor(QVector4D(0.3f, 0.3f, 0.9f, 1.0f));

			qtuser_3d::XEffect* effect = new qtuser_3d::XEffect(m_heightReferEntity);
			effect->addRenderPass(pass);
			m_heightReferEntity->setEffect(effect);

			m_heightReferEntity->setGeometry(qtuser_3d::BoxCreateHelper::createNoBottom(), Qt3DRender::QGeometryRenderer::Lines);
		}

		m_updateTimer = new QTimer(this);
		connect(m_updateTimer, &QTimer::timeout, this, [=]()
			{
				updateRender(true);
				m_updateTimer->stop();
			});
	}
	
	VisualScene::~VisualScene()
	{
	}

	bool VisualScene::isDefaultScene()
	{
		return !m_customRootEntity->isEnabled();
	}

	void VisualScene::useDefaultScene()
	{
		m_rootEntity->setEnabled(true);
		m_customRootEntity->setEnabled(false);
		emit sceneChanged();
	}

	void VisualScene::useCustomScene()
	{
		m_rootEntity->setEnabled(false);
		m_customRootEntity->setEnabled(true);
		emit sceneChanged();
	}

	void VisualScene::showCustom(Qt3DCore::QEntity* entity)
	{
		if (!entity)
			return;

		if (entity->parent() != m_customRootEntity)
			entity->setParent(m_customRootEntity);

		if (!entity->isEnabled())
			entity->setEnabled(true);
	}

	void VisualScene::hideCustom(Qt3DCore::QEntity* entity)
	{
		if (entity->parent() != m_customRootEntity)
			entity->setParent(m_customRootEntity);

		if (entity->isEnabled())
			entity->setEnabled(false);
	}

	qtuser_3d::FacePicker* VisualScene::facePicker()
	{
		return m_colorPicker;
	}

	void VisualScene::initialize()
	{
		useDefaultScene();
		//QColor clearColor = CONFIG_GLOBAL_COLOR(visualscene_surfacecolor, visualscene_group);
		//m_surface->setClearColor(clearColor);
	}

	void VisualScene::updateRender(bool updatePick)
	{
		if (updatePick)
			m_colorPicker->requestCapture();
		
		emit signalUpdate();
	}

	void VisualScene::updatePick(bool sync)
	{
		if (sync)
			m_colorPicker->requestSyncCapture();
		else 
			m_colorPicker->requestCapture();
	}

	void VisualScene::show(Qt3DCore::QEntity* entity)
	{
		if (!entity)
			return;

		if (entity->parent() != m_rootEntity)
			entity->setParent(m_rootEntity);

		if (!entity->isEnabled())
			entity->setEnabled(true);
	}

	void VisualScene::hide(Qt3DCore::QEntity* entity)
	{
		if (entity->parent() != m_rootEntity)
			entity->setParent(m_rootEntity);

		if (entity->isEnabled())
			entity->setEnabled(false);
	}

	void VisualScene::enableHandler(bool enable)
	{
		if (enable == m_handlerEnabled)
			return;

		m_handlerEnabled = enable;
		if (m_handlerEnabled)
		{
			m_handler->setEnabled(true);
			addLeftMouseEventHandler(m_towerHandler);

		} 
		else
		{
			m_handler->setEnabled(false);
			removeLeftMouseEventHandler(m_towerHandler);
		}
	}

	Qt3DCore::QEntity* VisualScene::sceneGraph()
	{
		return m_rootEntity;
	}

	void VisualScene::begineRender()
	{
		qInfo() << "scene begineRender";
		cacheReuseable(this);

		Qt3DRender::QCamera* qcamera = getCachedCameraEntity();
		m_surface->setCamera(qcamera);

		show(spaceEntity());

		m_handler->setEnabled(true);
		addLeftMouseEventHandler(m_towerHandler);

		m_handlerEnabled = true;

		if (m_operateMode)
		{
			m_operateMode->onAttach();
		}

		cxkernel::setUndoStack(getKernel()->modelSpaceUndo());
	}

	void VisualScene::endRender()
	{
		qInfo() << "scene endRender";
		hide(spaceEntity());

		if (m_operateMode)
		{
			m_operateMode->onDettach();
		}

		m_handler->setEnabled(false);
		removeLeftMouseEventHandler(m_towerHandler);
		cxkernel::setUndoStack(nullptr);
	}

	void VisualScene::updateRenderSize(const QSize& size)
	{
		if (size.width() <= 0 || size.height() <= 0)
			return;
			
		m_colorPicker->resize(size);
		m_surface->updateSurfaceSize(size * m_colorPicker->renderTargetRatio());
		updateRender(true);
	}

	void VisualScene::onStartPhase()
	{
		static bool firstStart = true;
		if (firstStart)
		{
			renderDefaultRenderGraph();
			firstStart = false;
		}
		else 
		{
			 QTimer::singleShot(40, [=] ()
			 {
				if (getKernel()->currentPhase() == 0)
					renderDefaultRenderGraph();
			 });		
		}
	}

	void VisualScene::onStopPhase()
	{
	}

	qtuser_3d::TextureRenderTarget* VisualScene::textureRenderTarget()
	{
		return m_textureRenderTarget;
	}

	void VisualScene::showRectangleSelector(const QRect& rect)
	{
		m_rectLine->setCamera(visCamera());
		m_rectLine->setRectOfScreen(rect);
		//m_rectLine->setDepthTest(false);
		show(m_rectLine);
	}

	void VisualScene::dismissRectangleSelector()
	{
		hide(m_rectLine);
	}

	bool VisualScene::shouldMultipleSelect()
	{
		if (m_operateMode)
		{
			return m_operateMode->shouldMultipleSelect();
		}
		return false;
	}

	void VisualScene::delayCapture(int msec)
	{
		m_updateTimer->start(msec);
	}

	bool VisualScene::didSelectAnyEntity(const QPoint& p)
	{
		return m_colorPicker->pick(p, nullptr);
	}

	Qt3DRender::QFrameGraphNode* VisualScene::getCameraViewportFrameGraphNode()
	{
		return m_surface->getCameraViewportFrameGraphNode();
	}
	
	QSize VisualScene::surfaceSize()
	{
		return m_surface->externalRenderTargetSize();
	}
#ifdef ENABLE_DEBUG_OVERLAY
	bool VisualScene::showDebugOverlay()
	{
		return m_surface->showDebugOverlay();
	}
	
	void VisualScene::setShowDebugOverlay(bool showDebugOverlay)
	{
		m_surface->setShowDebugOverlay(showDebugOverlay);
	}
#endif

	void VisualScene::setSceneClearColor(const QColor& color)
	{
		m_surface->setClearColor(color);
	}

	qtuser_3d::XEntity* VisualScene::heightReferEntity()
	{
		return m_heightReferEntity;
	}
}
