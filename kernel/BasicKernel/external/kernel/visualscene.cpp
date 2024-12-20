#include "visualscene.h"
#include <QtCore/QDebug>

#include "cxkernel/interface/undointerface.h"
#include "interface/eventinterface.h"
#include "interface/spaceinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"

#include "qtuser3d/framegraph/colorpicker.h"
#include "qtuser3d/framegraph/surface.h"
#include "qtuser3d/framegraph/texturerendertarget.h"

#include "interface/appsettinginterface.h"
#include "interface/renderinterface.h"
#include "interface/camerainterface.h"

#include "internal/render/printerentity.h"
#include "internal/utils/visscenehandler.h"
#include "internal/utils/wipetowerhandler.h"
#include "internal/render/modelgroupentity.h"
#include "internal/render/modelnentity.h"

#include "entity/rectlineentity.h"
#include "qtuser3d/geometry/basicshapecreatehelper.h"
#include "qtuser3d/framegraph/rendercaptor.h"
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
#include "entity/boxentity.h"

#include <QCoreApplication>

namespace creative_kernel
{
	VisualScene::VisualScene(Qt3DCore::QNode* parent)
		:RenderGraph(parent)
		, m_handlerEnabled(true)
	{
		m_surface = new qtuser_3d::Surface(this);
		// m_captureSurface = new qtuser_3d::Surface(this);

		m_rootEntity = new Qt3DCore::QEntity(this);
		m_customRootEntity = new Qt3DCore::QEntity(this);
		m_space_root = new Qt3DCore::QEntity(this);



		/* color picker */
		{
			m_colorPickerParent = m_surface->getCameraViewportFrameGraphNode();
			m_colorPicker = new qtuser_3d::ColorPicker();
			m_colorPicker->setParent(m_colorPickerParent);
			
			m_textureRenderTarget = new qtuser_3d::TextureRenderTarget(m_rootEntity, QSize(500, 500), true, true);
			m_colorPicker->setTextureRenderTarget(m_textureRenderTarget);

			connect(m_colorPicker, SIGNAL(signalUpdate()), this, SIGNAL(signalUpdate()));
			connect(m_colorPicker, &qtuser_3d::ColorPicker::updateColorPickerFinished, this, [=] ()
			{
				m_captureFinished = true;
			});
		}

		/* captor */
		{
			m_captureRenderTarget = new qtuser_3d::TextureRenderTarget();

			m_captor = new qtuser_3d::ColorPicker();
			m_captor->setParent(m_surface->getCameraViewportFrameGraphNode());
			m_captor->useSelfCameraSelector(true);
			m_captor->setEnabled(true);
			m_captor->setClearColor(QColor(0, 0, 0, 0));
			m_captor->setFilterKey("rt", 0);
			m_captor->setTextureRenderTarget(m_captureRenderTarget);
			// m_captor->setTextureRenderTarget(m_captureRenderTarget);
			// m_captor->setFilterKey("rt", 0);

			m_captor->resize(QSize(2000, 2000));
			m_captureRenderTarget->resize(QSize(2000, 2000));

		}

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
			m_heightReferEntity->setEnabled(false);
		}

		m_boxEntity = new qtuser_3d::BoxEntity();
		m_boxEntity->setParent(m_rootEntity);
		m_boxEntity->setColor(QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
		m_boxEntity->setLineWidth(3.0f);

		m_updateTimer = new QTimer(this);
		connect(m_updateTimer, &QTimer::timeout, this, [=]()
		{
			updateRender(true);
			m_updateTimer->stop();
		});

	}
	
	VisualScene::~VisualScene()
	{
		//m_space_root->setParent((Qt3DCore::QNode*)nullptr);
		//QList<Qt3DCore::QEntity*> entities = m_space_root->findChildren<Qt3DCore::QEntity*>(QString(), Qt::FindDirectChildrenOnly);
		//for (Qt3DCore::QEntity* entity : entities)
		//	entity->setParent((Qt3DCore::QNode*)nullptr);
	}

	void VisualScene::requestCapture(Qt3DRender::QCamera* camera, QObject* receiver, qtuser_3d::RenderCaptor::ReceiverHandleReplyFunc func)
	{
		if (m_requestCaptureMap.contains(receiver))
		{
			m_requestCaptureMap[receiver]->setEnabled(false);
			m_requestCaptureMap.remove(receiver);
		}

		requestRender(receiver);
		m_captor->resize(QSize(2000, 2000));
		m_captureRenderTarget->resize(QSize(2000, 2000));
		camera->setParent(this);
		qtuser_3d::RenderCaptor* captor = m_captor->requestCapture(camera, receiver, func);
		//m_captorCount++;
		m_requestCaptureMap[receiver] = captor;
		connect(captor, &QObject::destroyed, this, [=] ()
		{
			//m_captorCount--;
			m_requestCaptureMap.remove(receiver);
			//if (m_captorCount == 0)
			if (m_requestCaptureMap.isEmpty())
			{
				clearRenderRequestors();
				emit pictureCaptureCompleted();
			}
				
		});

		// m_colorPicker->requestCapture(camera, receiver, func);
	}

	void VisualScene::setPreviewScene(Qt3DCore::QEntity* entity)
	{
		m_previewEntity = entity;
		m_previewEntity->setParent(this);
	}

	Qt3DCore::QEntity* VisualScene::spaceEntity()
	{
		return m_space_root;
	}

	ModelGroupEntity* VisualScene::createModelGroupEntity()
	{
		return new ModelGroupEntity();
	}

	void VisualScene::destroyModelGroupEntity(ModelGroupEntity* entity)
	{
		if (entity)
			entity->deleteLater();
	}

	ModelNEntity* VisualScene::createModelNEntity()
	{
		ModelNEntity* entity = new ModelNEntity();
		entity->setEnabled(false);
		m_modelNEntitys << entity;
		return entity;
	}

	void VisualScene::destroyModelNEntity(ModelNEntity* entity)
	{
		if (entity)
		{
			m_modelNEntitys.removeOne(entity);
			
			requestRender(entity);
			entity->setEnabled(false);
			entity->setParent((Qt3DCore::QNode*)nullptr);
			entity->deleteLater();
			endRequestRender(entity);
		}
	}

	bool VisualScene::isDefaultScene()
	{
		return !m_customRootEntity->isEnabled();
	}

	bool VisualScene::isPreviewScene()
	{
		return m_previewEntity->isEnabled();
	}

	void VisualScene::useScene(int scenes)
	{
		m_rootEntity->setEnabled(scenes & Default);
		m_space_root->setEnabled(scenes & ModelSpace);
		m_previewEntity->setEnabled(scenes & Preview);
		m_customRootEntity->setEnabled(scenes & Custom);

		emit sceneChanged();
	}

	void VisualScene::show(Qt3DCore::QEntity* entity, Scene scene)
	{
		if (!entity)
			return;

		Qt3DCore::QEntity* sceneRoot = NULL;
		if (scene == Default)
			sceneRoot = m_rootEntity;
		else if (scene == ModelSpace)
			sceneRoot = m_space_root;
		else if (scene == Preview)
			sceneRoot = m_previewEntity;
		else if (scene == Custom)
			sceneRoot = m_customRootEntity;

		if (!sceneRoot)
			return;

		if (entity->parent() != sceneRoot)
			entity->setParent(sceneRoot);

		if (!entity->isEnabled())
			entity->setEnabled(true);

	}

	void VisualScene::hide(Qt3DCore::QEntity* entity)
	{
		if (entity->isEnabled())
			entity->setEnabled(false);
	}

	qtuser_3d::FacePicker* VisualScene::facePicker()
	{
		return m_colorPicker;
	}

	void VisualScene::initialize()
	{
		renderDefaultRenderGraph();
	}

	void VisualScene::updateRender(bool updatePick)
	{
		if (updatePick)
		{
			m_captureFinished = false;
			requestRender(this);
			m_colorPicker->requestCapture();
		}
		else 
		{
			if (m_captureFinished)
			{
				endRequestRender(this);
			}
		}
		
		emit signalUpdate();
	}

	void VisualScene::updatePick(bool sync)
	{
		m_captureFinished = false;
		requestRender(this);

		if (sync)
			m_colorPicker->requestSyncCapture();
		else 
			m_colorPicker->requestCapture();
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

		//show(spaceEntity());

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
		//hide(spaceEntity());

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
		usePrepareScene();

		setModelOffset(QVector2D(0, 0));

        for (auto model : modelns())
        {
            model->updateEntityRender();
        }

		setBoxEnabled(true);
	}

	void VisualScene::onStopPhase()
	{
		creative_kernel::setVisOperationMode(NULL);
		setBoxEnabled(false);
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
		show(m_rectLine, Default);
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

	void VisualScene::updateSelectionsGlobalBox(const qtuser_3d::Box3D& box)
	{
		requestRender(this);
		if (box.valid)
		{
			setBoxEnabled(false);
			QMatrix4x4 parentMatrix;
			m_boxEntity->update(box, parentMatrix);

			if (!m_previewEntity->isEnabled() && !m_customRootEntity->isEnabled())
				setBoxEnabled(true);

		} else {
			setBoxEnabled(false);
		}
		updateRender();
		endRequestRender(this);
	}

	Qt3DRender::QCamera* VisualScene::getCaptureCamera()
	{
		return m_captor->camera();
	}

	qtuser_3d::ColorPicker* VisualScene::getCaptor()
	{
		return m_captor;
	}

	void VisualScene::setBoxEnabled(bool enabled)
	{
		m_boxEnabled = enabled;
		enabled = m_forceHideBox ? false : m_boxEnabled;
		if (enabled)
		{
			if (!selectionms().isEmpty())
			{
				m_boxEntity->setEnabled(true);
			}
		}
		else 
		{
			m_boxEntity->setEnabled(false);
		}
	}
	
	void VisualScene::forceHideBox(bool enabled)
	{
		m_forceHideBox = enabled;
		setBoxEnabled(m_boxEnabled);
	}
	
	void VisualScene::setPreviewEnabled(bool enabled)
	{
		m_previewEntity->setEnabled(enabled);
	}

	void VisualScene::clearRenderRequestors()
	{
		m_renderRequestors.clear();
		setCommandRender();
	}

	void VisualScene::requestRender(QObject* requestor)
	{
		if (m_renderRequestors.contains(requestor))
			return;
		
		m_renderRequestors << requestor;
		connect(requestor, &QObject::destroyed, this, &VisualScene::endRequestRender, Qt::UniqueConnection);
		
		setContinousRender();
	}

	void VisualScene::endRequestRender(QObject* requestor)
	{
		if (!m_renderRequestors.contains(requestor))
			return;

		m_renderRequestors.removeOne(requestor);

		if (m_renderRequestors.isEmpty())
		{
			setCommandRender();
		}
	}
	
	void VisualScene::setModelVisualMode(ModelVisualMode mode)
	{
		m_modelVisualMode = mode;
		for (ModelNEntity* entity: m_modelNEntitys)
		{
			entity->updateRender();
		}

		emit modelVisualModeChanged();
	}

	ModelVisualMode VisualScene::modelVisualMode()
	{
		return m_modelVisualMode;
	}
}
