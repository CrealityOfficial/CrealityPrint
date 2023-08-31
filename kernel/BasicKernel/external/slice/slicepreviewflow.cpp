#include "slicepreviewflow.h"

#include <QtCore/qmath.h>

#include <mmesh/camera/screencamera.h>

#include <qtuser3d/camera/screencamera.h>
#include <qtuser3d/framegraph/colorpicker.h>
#include <qtuser3d/framegraph/surface.h>
#include <qtuser3d/framegraph/texturerendertarget.h>
#include <qtusercore/string/stringtool.h>

#include <qcxutil/trimesh2/conv.h>

#include "external/data/modeln.h"
#include "external/interface/camerainterface.h"
#include "external/interface/eventinterface.h"
#include "external/interface/renderinterface.h"
#include "external/interface/reuseableinterface.h"
#include "external/interface/selectorinterface.h"
#include "external/interface/spaceinterface.h"
#include "external/interface/uiinterface.h"
#include "external/slice/sliceattain.h"

#include "internal/render/slicepreviewnode.h"
#include "internal/render/slicepreviewscene.h"
#include "qtuser3d/module/selector.h"
#include "interface/reuseableinterface.h"
#include "cxkernel/render/worldindicatorentity.h"
#include "QtCore/qcoreapplication.h"

namespace creative_kernel
{
	SlicePreviewFlow::SlicePreviewFlow(Qt3DCore::QNode* parent)
		: RenderGraph(parent)
#if INDEX_START_AT_ONE
		, m_indexOffset(1)
#else
		, m_indexOffset(0)
#endif
		, m_visualType(gcode::GCodeVisualType::gvt_structure)
		, m_speedModel(std::make_unique<cxgcode::GcodeSpeedListModel>())
		, m_structureModel(std::make_unique<cxgcode::GcodeStructureListModel>())
		, m_extruderModel(std::make_unique<cxgcode::GcodeExtruderListModel>())
		, m_layerHightModel(std::make_unique<cxgcode::GcodeLayerHightListModel>())
		, m_lineWidthModel(std::make_unique<cxgcode::GcodeLineWidthListModel>())
		, m_flowModel(std::make_unique<cxgcode::GcodeFlowListModel>())
		, m_layerTimeModel(std::make_unique<cxgcode::GcodeLayerTimeListModel>())
		, m_fanSpeedModel(std::make_unique<cxgcode::GcodeFanSpeedListModel>())
		, m_temperatureModel(std::make_unique<cxgcode::GcodeTemperatureListModel>())
	{
		m_currentLayer = m_indexOffset;
		m_currentStep = m_indexOffset;

		m_scene = new SlicePreviewScene(this);
		m_surface = new qtuser_3d::Surface(this);
		m_surface->setClearColor(QColor("transparent"));

		m_textureRenderTarget = new qtuser_3d::TextureRenderTarget(m_scene);

		m_colorPicker = new qtuser_3d::ColorPicker(m_surface);
		m_colorPicker->useSelfCameraSelector(true);
		m_colorPicker->setEnabled(true);
		m_colorPicker->setDebugName("slicer2t.bmp");
		/*m_colorPicker->setFilterKey("rt", 0);
		m_colorPicker->setClearColor(QColor(0, 0, 0, 0));
		QSize size = QSize(400, 300);
		m_colorPicker->resize(size);*/

		m_colorPicker->setTextureRenderTarget(m_textureRenderTarget);

		m_entitySelector = new qtuser_3d::Selector(this);
		m_entitySelector->setPickerSource(m_colorPicker);
		m_entitySelector->addPickable(getIndicatorEntity()->pickable());

		connect(m_colorPicker, SIGNAL(signalUpdate()), this, SIGNAL(signalUpdate()));

		connect(m_structureModel.get(), &cxgcode::GcodeStructureListModel::itemCheckedChanged,
						this, &SlicePreviewFlow::showGCodeType);
	}

	SlicePreviewFlow::~SlicePreviewFlow()
	{
		delete m_scene;
	}

	void SlicePreviewFlow::registerContext() {}

	void SlicePreviewFlow::initialize()
	{
		m_scene->initialize();
		QList<QColor> colors = m_scene->previewNode()->getSpeedColorValues();
		m_speedModel->setColors(colors);
		m_layerHightModel->setColors(colors);
		m_lineWidthModel->setColors(colors);
		m_flowModel->setColors(colors);
		m_layerTimeModel->setColors(colors);
		m_fanSpeedModel->setColors(colors);
		m_temperatureModel->setColors(colors);
	}

	SlicePreviewScene* SlicePreviewFlow::scene()
	{
		return m_scene;
	}

	void SlicePreviewFlow::requestPreview(qtuser_3d::namedReplyFunc func)
	{
		m_colorPicker->setFilterKey("rt", 0);
		m_colorPicker->setClearColor(QColor(0, 0, 0, 0));

		m_textureRenderTarget->resize(QSize(2000, 2000));
		
		//使用外部摄像头
		//Qt3DRender::QCamera* pickerCamera = m_colorPicker->camera();
		Qt3DRender::QCamera* pickerCamera = new Qt3DRender::QCamera(this);
		
		pickerCamera->setProjectionType(Qt3DRender::QCameraLens::PerspectiveProjection);

		pickerCamera->setAspectRatio(1.0f);

		float fovy = 15.0f;
		mmesh::ScreenCameraMeta meta = mmesh::viewAllByProjection(m_attain->box(), trimesh::vec3(-1.0, -1.0, 1.0), fovy);

		pickerCamera->setPosition(qcxutil::vec2qvector(meta.position));
		pickerCamera->setViewCenter(qcxutil::vec2qvector(meta.viewCenter));
		pickerCamera->setUpVector(qcxutil::vec2qvector(meta.upVector));

		pickerCamera->setFieldOfView(fovy);
		pickerCamera->setNearPlane(meta.fNear);
		pickerCamera->setFarPlane(meta.fFar);
		
		m_colorPicker->useCamera(pickerCamera);
		m_previewCamera.reset(pickerCamera);

		spaceEntity()->setParent(m_scene);
		spaceEntity()->setEnabled(true);

		// 改变场景模型颜色
		QColor previewColor = QColor(162, 162, 162, 255);
		QList<creative_kernel::ModelN*> modelns = getModelSpace()->modelns();
		for (creative_kernel::ModelN* model : modelns)
		{
			model->setProperty("statecache", model->getState());
			model->setProperty("customcolorcache", model->getCustomColor());

			model->setState(6);
			model->setCustomColor(previewColor);
		}

		// 刷新场景，以便后续生成预览图
		renderOneFrame();

		m_colorPicker->requestNamedCapture(func);

		// 刷新场景渲染
		renderOneFrame();
		//pickerCamera->setNearPlane(np);
		//pickerCamera->setFarPlane(fp);
	}

	void SlicePreviewFlow::endRequest()
	{
		spaceEntity()->setParent((Qt3DCore::QNode*)nullptr);
		m_surface->setViewport(QRectF(0.0f, 0.0f, 1.0f, 1.0f));

		// 重置场景颜色
		QList<creative_kernel::ModelN*> modelns = getModelSpace()->modelns();
		for (int i = 0; i < modelns.size(); i++)
		{
			creative_kernel::ModelN* model = modelns[i];
			model->setState(model->property("statecache").toFloat());
			model->setCustomColor(model->property("customcolorcache").value<QColor>());
		}

		// 还原colorpick原本的设置
		m_colorPicker->setFilterKey("pick", 0);
		m_colorPicker->setClearColor(QColor(255, 255, 255, 255));
		m_colorPicker->useSelfCamera();
		m_textureRenderTarget->resize(m_surfaceSize);

		/*renderOneFrame();*/
		requestCapture(true);
	}

	void SlicePreviewFlow::notifyClipValue()
	{
		if (m_attain.isNull())
			return;

		QVector4D clipValue(0.0f, m_currentLayer, 0.0f, m_currentStep);
		m_scene->setClipValue(clipValue);
		trimesh::vec3 pos = m_attain->traitPosition(m_currentLayer, m_currentStep);
		m_scene->setIndicatorPos(QVector3D(pos.x, pos.y, pos.z));

		renderOneFrame();
	}

	bool SlicePreviewFlow::isAvailable()
	{
		return !m_attain.isNull();
	}

	void SlicePreviewFlow::setGCodeVisualType(int type)
	{
		m_visualType = (gcode::GCodeVisualType)type;
		m_scene->setGCodeVisualType((gcode::GCodeVisualType)type);
		if (type ==1)
		{
			m_scene->setZSeamsVisible();
			m_scene->setRetractionVisible();
		}
		else
		{
			m_scene->setZseamRetractDis();
		}
		if(m_attain)
			m_attain->setGCodeVisualType((gcode::GCodeVisualType)type);
		renderOneFrame();
	}

	void SlicePreviewFlow::setIndicatorVisible(bool visible)
	{
		m_scene->setIndicatorVisible(visible);
		renderOneFrame();
	}

	void SlicePreviewFlow::setPrinterVisible(bool visible)
	{
		creative_kernel::setPrinterVisible(visible);
		renderOneFrame();
	}

	void SlicePreviewFlow::showGCodeType(int type, bool show)
	{
		m_scene->showGCodeType(type, show);
		renderOneFrame();
	}

	void SlicePreviewFlow::setCurrentLayer(int layer, bool randonStep)
	{
		if (m_attain.isNull())
			return;

		m_currentLayer = layer;
		int layerSteps = m_attain->steps(m_currentLayer);
		m_currentStep = std::min(layerSteps, m_currentStep);
		notifyClipValue();

		m_attain->updateStepIndexMap(m_currentLayer);

		emit stepsChanged();
		emit layerGCodesChanged();
		emit currentStepSpeedChanged();
	}

	QAbstractListModel* SlicePreviewFlow::getSpeedModel() const {
		return m_speedModel.get();
	}

	QAbstractListModel* SlicePreviewFlow::getStructureModel() const {
		return m_structureModel.get();
	}

	QAbstractListModel* SlicePreviewFlow::getExtruderModel() const {
		return m_extruderModel.get();
	}

	QAbstractListModel* SlicePreviewFlow::getLayerHightModel() const {
		return m_layerHightModel.get();
	}

	QAbstractListModel* SlicePreviewFlow::getLineWidthModel() const {
		return m_lineWidthModel.get();
	}

	QAbstractListModel* SlicePreviewFlow::getFlowModel() const {
		return m_flowModel.get();
	}

	QAbstractListModel* SlicePreviewFlow::getLayerTimeModel() const {
		return  m_layerTimeModel.get();
	}

	QAbstractListModel* SlicePreviewFlow::getFanSpeedModel() const {
		return m_fanSpeedModel.get();
	}

	QAbstractListModel* SlicePreviewFlow::getTemperatureModel() const {
		return m_temperatureModel.get();
	}

	void SlicePreviewFlow::setCurrentStep(int step)
	{
		m_currentStep = step;
		notifyClipValue();

		emit currentStepSpeedChanged();
	}

	int SlicePreviewFlow::findViewIndexFromStep(int nStep)
	{
		if (m_attain.isNull())
			return -1;

		return m_attain->findViewIndexFromStep(m_currentLayer, nStep);
	}

	int SlicePreviewFlow::findStepFromViewIndex(int nViewIndex)
	{
		if (m_attain.isNull())
			return -1;

		return m_attain->findStepFromViewIndex(m_currentLayer, nViewIndex);
	}

	void SlicePreviewFlow::showOnlyLayer(int layer)
	{
		if(layer >= 0)
			m_scene->setOnlyLayer(layer);
		else
			m_scene->unsetOnlyLayer();
		renderOneFrame();
	}

	void SlicePreviewFlow::setAnimationState(int state)
	{
		m_scene->setAnimation(state);
	}

	void SlicePreviewFlow::setCurrentLayerFocused(bool focused)
	{
		if(m_focused==focused)
		{
			return;
		}else{
			m_focused = focused;
		}
		m_scene->setAnimation(m_focused ? 1 : 0);
	}

	int SlicePreviewFlow::layers()
	{
		return m_attain.data() ? m_attain->layers() : 0;
	}

	int SlicePreviewFlow::steps()
	{
		if (!m_attain.data())
			return 0;

		return m_attain->steps(m_currentLayer);
	}

	QStringList SlicePreviewFlow::layerGCodes()
	{
		if (!m_attain)
			return QStringList();

		QStringList codes = m_attain->layerGcode(m_currentLayer).split("\n");
		QStringList result;
		for (const QString& code : codes)
			if (!code.isEmpty())
				result.append(code);
		return result;
	}

	float SlicePreviewFlow::currentStepSpeed()
	{
		if (!m_attain)
			return 0.0f;

		return m_attain->traitSpeed(m_currentLayer, m_currentStep);
	}

	void SlicePreviewFlow::clear()
	{
		m_attain.reset();
	}

	void SlicePreviewFlow::setSliceAttain(SliceAttain* attain)
	{

		m_attain.reset(attain);

        //设置平台尺寸
        if (attain != nullptr)
        {
			updatePrinterBox(m_attain->getMachineWidth()
                , m_attain->getMachineDepth()
                , m_attain->getMachineHeight());
        }

		int stepCount = 0;

		if (m_attain.data())
		{
			Qt3DRender::QGeometry* geometry = m_attain->createGeometry();
			m_scene->setGeometry(geometry, Qt3DRender::QGeometryRenderer::Triangles);
			m_scene->setRetractionGeometry(m_attain->createRetractionGeometry(), Qt3DRender::QGeometryRenderer::Points);
			m_scene->setZSeamsGeometry(m_attain->createZSeamsGeometry(), Qt3DRender::QGeometryRenderer::Points);

			m_scene->setLayerHeight(m_attain->layerHeight());
			m_scene->setLineWidth(m_attain->lineWidth());

			//m_scene->setRetractionGeometryRenderer(m_attain->createRetractionGeometryRenderer());
			//m_scene->setZSeamsGeometryRenderer(m_attain->createZSeamsGeometryRenderer());

			m_scene->setPreviewNodeModelMatrix(qcxutil::xform2QMatrix(m_attain->modelMatrix()));

			m_attain->setGCodeVisualType(m_visualType);

			stepCount = m_attain->totalSteps();
			m_speedModel->setRange(m_attain->minSpeed(), m_attain->maxSpeed());
			m_structureModel->setTimeParts(m_attain->getTimeParts());
			m_layerHightModel->setRange(m_attain->minLayerHeight(), m_attain->maxLayerHeight());
			m_lineWidthModel->setRange(m_attain->minLineWidth(), m_attain->maxLineWidth());
			m_flowModel->setRange(m_attain->minFlowOfStep(), m_attain->maxFlowOfStep());
			m_layerTimeModel->setRange(m_attain->minTimeOfLayer(), m_attain->maxTimeOfLayer());
			//m_fanSpeedModel->setRange(0.0, 1.0);
			m_temperatureModel->setRange(m_attain->minTemperature(), m_attain->maxTemperature());


			setCurrentLayer(m_attain->layers(), false);
			setCurrentStep(m_attain->steps(m_attain->layers()));

		}

		// 模型越大，从加载到显示所需的时间会越多，甚至会大于 1 秒，
		// 所以需要先开启持续渲染，直到模型显示出来后，再换回按需渲染，
		// 以避免加载时间超出按需阈值，所导致的模型不显示
		setContinousRender();
		// 持续渲染模式时长
		int continousMSecs = stepCount / 160;
		QTimer::singleShot(continousMSecs, []() { setCommandRender(); });

		emit layersChanged();
	}

	SliceAttain* SlicePreviewFlow::attain()
	{
		return m_attain.data();
	}

	SliceAttain* SlicePreviewFlow::takeAttain()
	{
		return m_attain.take();
	}

	Qt3DCore::QEntity* SlicePreviewFlow::sceneGraph()
	{
		return m_scene;
	}

	void SlicePreviewFlow::begineRender()
	{
		qInfo() << "fdm slice preview begineRender";
		cacheReuseable(this);

		m_surface->setCamera(getCachedCameraEntity());

		if (!m_attain.data())
		{
			spaceEntity()->setParent(m_scene);
		}
		else
		{
			m_scene->visual();
		}

#ifdef _DEBUG
		addKeyEventHandler(this);
#endif
		
		addLeftMouseEventHandler(this);
		addHoverEventHandler(this);

		connect(creative_kernel::cameraController(), SIGNAL(signalViewChanged(bool)), this, SLOT(requestCapture(bool)));
	}

	void SlicePreviewFlow::endRender()
	{
		qInfo() << "fdm slice preview endRender";
		m_surface->setCamera(nullptr);
		m_scene->clear();
		spaceEntity()->setParent((Qt3DCore::QNode*)nullptr);
		setModelEffectClipMaxZ();

#ifdef _DEBUG
		removeKeyEventHandler(this);
#endif
		removeLeftMouseEventHandler(this);
		removeHoverEventHandler(this);

		disconnect(creative_kernel::cameraController(), SIGNAL(signalViewChanged(bool)), this, SLOT(requestCapture(bool)));
	}

	void SlicePreviewFlow::updateRenderSize(const QSize& size)
	{
		if (size.width() == 0 || size.height() == 0)
			return;

		m_surface->updateSurfaceSize(size);
		m_colorPicker->resize(size);
		m_textureRenderTarget->resize(size);
		m_surfaceSize = size;
	}

	void SlicePreviewFlow::onKeyPress(QKeyEvent* event)
	{
		if (event->key() == Qt::Key_P && m_attain.data())
		{
			auto f = [this](const QImage& image) {
				image.save("fdm.bmp");
				endRequest();
			};

			requestPreview(f);
		}
	}

	void SlicePreviewFlow::onKeyRelease(QKeyEvent* event)
	{
	}

	void SlicePreviewFlow::onHoverEnter(QHoverEvent* event)
	{
	}
	void SlicePreviewFlow::onHoverLeave(QHoverEvent* event)
	{
	}
	void SlicePreviewFlow::onHoverMove(QHoverEvent* event)
	{
		if (m_entitySelector->hover(event->pos()))
			renderOneFrame();
	}

	void SlicePreviewFlow::onLeftMouseButtonPress(QMouseEvent* event)
	{
	}
	void SlicePreviewFlow::onLeftMouseButtonRelease(QMouseEvent* event)
	{
	}
	void SlicePreviewFlow::onLeftMouseButtonMove(QMouseEvent* event)
	{
	}
	void SlicePreviewFlow::onLeftMouseButtonClick(QMouseEvent* event)
	{
		m_entitySelector->select(event->pos(), false);
		renderOneFrame();
	}

	void SlicePreviewFlow::requestCapture(bool capture)
	{
		if (capture)
			m_colorPicker->requestCapture();
		emit signalUpdate();
	}
}
