#include "slicepreviewflow.h"

#include <QtCore/qmath.h>

#include <qtuser3d/camera/screencamera.h>
#include"qtuser3d/trimesh2/camera.h"

#include <qtuser3d/framegraph/colorpicker.h>
#include <qtuser3d/framegraph/surface.h>
#include <qtuser3d/framegraph/texturerendertarget.h>
#include <qtuser3d/trimesh2/conv.h>
#include <qtusercore/string/stringtool.h>

#include "external/data/modeln.h"
#include "external/interface/camerainterface.h"
#include "external/interface/eventinterface.h"
#include "external/interface/renderinterface.h"
#include "external/interface/reuseableinterface.h"
#include "external/interface/selectorinterface.h"
#include "external/interface/spaceinterface.h"
#include "external/interface/uiinterface.h"
#include "external/slice/sliceattain.h"

#include "internal/multi_printer/printer.h"
#include "internal/render/slicepreviewnode.h"
#include "internal/render/slicepreviewscene.h"
#include "qtuser3d/module/selector.h"
#include "interface/reuseableinterface.h"
#include "entity/worldindicatorentity.h"
#include "QtCore/qcoreapplication.h"
#include <QElapsedTimer>
#include "interface/appsettinginterface.h"

#include "kernel/kernel.h"
#include "kernel/reuseablecache.h"

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
		, m_extruderTableModel(std::make_unique<cxgcode::GcodeExtruderTableModel>())
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

		m_textureRenderTarget = new qtuser_3d::TextureRenderTarget(m_scene);
		m_textureRenderTarget->resize(QSize(2000, 2000));

		// qtuser_3d::TextureRenderTarget* renderTarget = new qtuser_3d::TextureRenderTarget(m_scene);
		// m_textureRenderTarget->resize(QSize(2000, 2000));

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

	void SlicePreviewFlow::useCachePreview()
	{
		if (m_attain)
        {
#if _DEBUG
            m_cachePreview.save(m_attain->tempImageFileName());
#endif
            m_attain->saveTempGCode();
            QImage tmpImage = m_cachePreview;
            m_attain->saveTempGCodeWithImage(tmpImage);
        }
	}

	void SlicePreviewFlow::beginCapturePreview()
	{
		setContinousRender();
		m_colorPicker->setFilterKey("rt", 0);
		// m_colorPicker->setFilterKey("view", 0);
		m_colorPicker->setClearColor(QColor(0, 0, 0, 0));

		m_textureRenderTarget->resize(QSize(2000, 2000));

		spaceEntity()->setParent(m_scene);
		spaceEntity()->setEnabled(true);

	}

	void SlicePreviewFlow::capturePrinter(Printer* printer, qtuser_3d::RenderCaptor::ReceiverHandleReplyFunc func)
	{
		//使用外部摄像头
		Qt3DRender::QCamera* pickerCamera = new Qt3DRender::QCamera(this);
		
		pickerCamera->setProjectionType(Qt3DRender::QCameraLens::OrthographicProjection);

		pickerCamera->setAspectRatio(1.0f);

		float fovy = 5.0f;

		float minX, minY, minZ, maxX, maxY, maxZ;
		minX = minY = minZ = 100000;
		maxX = maxY = maxZ = -100000;
		QVector3D printerCenter = printer->globalBox().center();
		QVector2D printerOffset = QVector2D(printerCenter.x(), printerCenter.y()) * 2;
		for (auto m : printer->modelsInsideVisible())
		{
			// 计算预览区域
			auto box = m->boxWithSup();
			minX = box.min.x() < minX ? box.min.x() : minX;
			minY = box.min.y() < minY ? box.min.y() : minY;
			minZ = box.min.z() < minZ ? box.min.z() : minZ;
			maxX = box.max.x() > maxX ? box.max.x() : maxX;
			maxY = box.max.y() > maxY ? box.max.y() : maxY;
			maxZ = box.max.z() > maxZ ? box.max.z() : maxZ;

			QMatrix4x4 matrix1 = m->globalMatrix();
			matrix1(0, 3) = 0;
			matrix1(1, 3) = 0;
			matrix1(2, 3) = 0;
			matrix1 = matrix1.inverted();
			QVector3D temp1 = printerOffset;
			temp1 = matrix1.map(temp1);
			 m->setOffscreenRenderOffset(temp1);
		}
		minX += printerOffset.x();
		minY += printerOffset.y();
		maxX += printerOffset.x();
		maxY += printerOffset.y();

		float cameraDir[3] = {-1.0, -1.0, 1.15};
		auto rbox = trimesh::box3(trimesh::point3(minX, minY, minZ), trimesh::point3(maxX, maxY, maxZ));
		qtuser_3d::ScreenCameraMeta meta = qtuser_3d::viewAllByProjection(rbox, trimesh::vec3(cameraDir[0], cameraDir[1], cameraDir[2]), fovy);

		pickerCamera->setPosition(qtuser_3d::vec2qvector(meta.position));
		pickerCamera->setViewCenter(qtuser_3d::vec2qvector(meta.viewCenter));
		pickerCamera->setUpVector(qtuser_3d::vec2qvector(meta.upVector));

		pickerCamera->setFarPlane(10000.0f);
		pickerCamera->setNearPlane(1.0f);

		QVector3D box(maxX - minX, maxY - minY, maxZ - minZ);
		QMatrix4x4 view;
        view.lookAt(QVector3D(cameraDir[0], cameraDir[1], cameraDir[2]), QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f)); // 相机位置和方向

		QVector3D p1(box.x() / 2, box.y() / 2, box.z() / 2);
		p1 = view.map(p1);

		QVector3D p2(-box.x() / 2, -box.y() / 2, -box.z() / 2);
		p2 = view.map(p2);

		QVector3D p3(box.x() / 2, -box.y() / 2, -box.z() / 2);
		p3 = view.map(p3);

		QVector3D p4(-box.x() / 2, box.y() / 2, -box.z() / 2);
		p4 = view.map(p4);

		float height = p2.distanceToPoint(p1);
		float width = p3.distanceToPoint(p4);
		float borderLength = width > height ? width : height;

		pickerCamera->setBottom(-borderLength / 2.0);
		pickerCamera->setTop(borderLength / 2.0);
		pickerCamera->setLeft(-borderLength / 2.0);
		pickerCamera->setRight(borderLength / 2.0);

		m_colorPicker->requestCapture(pickerCamera, printer, func);

	}

	void SlicePreviewFlow::endCapturePreview()
	{
		//spaceEntity()->setParent((Qt3DCore::QNode*)nullptr);
		m_surface->setViewport(QRectF(0.0f, 0.0f, 1.0f, 1.0f));

		// 还原colorpick原本的设置
		m_colorPicker->setFilterKey("pick", 0);
		m_colorPicker->setClearColor(QColor(255, 255, 255, 255));
		m_colorPicker->useSelfCamera();
		m_textureRenderTarget->resize(m_surfaceSize);
        setCommandRender();
		renderOneFrame();
	}

	void SlicePreviewFlow::notifyClipValue()
	{
		if (!m_attain)
			return;

		QVector4D clipValue(0.0f, m_currentLayer, 0.0f, m_currentStep);
		m_scene->setClipValue(clipValue);
		QVector3D pos = qtuser_3d::vec2qvector(m_attain->traitPosition(m_currentLayer, m_currentStep));
		if (m_printer)
			pos += m_printer->globalBox().min;

		m_scene->setIndicatorPos(pos);

		renderOneFrame();
	}

	bool SlicePreviewFlow::isAvailable()
	{
		return m_attain;
	}

	void SlicePreviewFlow::setGCodeVisualType(int type)
	{
		m_visualType = (gcode::GCodeVisualType)type;
		m_scene->setGCodeVisualType((gcode::GCodeVisualType)type);
		if (type ==1)
		{
			m_scene->setZSeamsVisible();
			m_scene->setRetractionVisible();
			m_scene->setUnretractVisible();
		}
		else
		{
			m_scene->setZseamRetractDis();
		}

		if (m_attain) {
			m_attain->prepareVisualTypeData((gcode::GCodeVisualType)type);
			m_attain->rebuildGeometryVisualTypeData();
		}

		renderOneFrame();
	}

	void SlicePreviewFlow::showGCodeType(int type, bool show)
	{
		if (!m_attain)
			return;

		m_scene->showGCodeType(type, show, m_attain->isCuraProducer());
		//renderOneFrame();
	}

	void SlicePreviewFlow::setCurrentLayer(int layer, bool randonStep)
	{
		if (!m_attain)
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

	QAbstractTableModel* SlicePreviewFlow::getExtruderTableModel() const {
		return m_extruderTableModel.get();
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
		if (!m_attain)
			return -1;

		return m_attain->findViewIndexFromStep(m_currentLayer, nStep);
	}

	int SlicePreviewFlow::findStepFromViewIndex(int nViewIndex)
	{
		if (!m_attain)
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
	/*	if(m_focused==focused)
		{
			return;
		}else{
			m_focused = focused;
		}*/
		//m_focused = focused;
		m_scene->setAnimation(focused ? 1 : 0);
	}

	int SlicePreviewFlow::currentLayer()
	{
		return m_attain ? m_currentLayer : 0;
	}

	int SlicePreviewFlow::layers()
	{
		return m_attain ? m_attain->layers() : 0;
	}

	int SlicePreviewFlow::steps()
	{
		if (!m_attain)
			return 0;

		return m_attain->steps(m_currentLayer);
	}
	float SlicePreviewFlow::height()
	{
		if (!m_attain)
			return 0;
		trimesh::vec3 pos = m_attain->traitPosition(m_currentLayer+1, 0);
		return pos.z;
	}
	QStringList SlicePreviewFlow::layerGCodes()
	{
		if (!m_attain)
			return QStringList();

		QString gcode = m_attain->layerGcode(m_currentLayer);
		gcode.replace("\n\n", "\n");
		QStringList codes = gcode.split("\n");
		return codes;
	}

	float SlicePreviewFlow::currentStepSpeed()
	{
		if (!m_attain)
			return 10.0f;

		return m_attain->traitDuration(m_currentLayer, m_currentStep);
	}

	void SlicePreviewFlow::clear()
	{
		if (m_attain == NULL)
			return;

		m_attain->deleteLater();
		m_attain = NULL;
	}

	void SlicePreviewFlow::addCustomGcode(const QString& gcode)
	{
		if (!m_attain)
			return;
		QString insert_gcode = gcode;
		QString origin = m_attain->layerGcode(m_currentLayer);
		if (!gcode.endsWith("\n"))
		{
			insert_gcode += "\n";
		}
		m_attain->set_data_gcodelayer(m_currentLayer-1, (insert_gcode + origin).toStdString());
		emit layerGCodesChanged();
	}

	void SlicePreviewFlow::delCustomGcode()
	{
		if (!m_attain)
			return;
		QString origin = m_attain->layerGcode(m_currentLayer);
		int index = origin.indexOf(";LAYER:");
		auto temp = origin.mid(index);
		m_attain->set_data_gcodelayer(m_currentLayer - 1, temp.toStdString());
		emit layerGCodesChanged();
	}

	void SlicePreviewFlow::saveCustomGcode()
	{
		if (!m_attain)
			return;
		m_attain->saveTempGCode();
		m_attain->saveTempGCodeWithImage(*(m_attain->getImageFromGcode()));
	}

	void SlicePreviewFlow::setPrinter(Printer* printer)
	{
		m_printer = printer;
	}

	void SlicePreviewFlow::previewSliceAttain(SliceAttain* attain, int layer)
	{
		if (attain == nullptr) return;
		
		/*updatePrinterBox(attain->getMachineWidth()
			, attain->getMachineDepth()
			, attain->getMachineHeight());*/

		//提前预览阶段使用 gvt_structure 类型，其他类型数据可能不准确
		m_scene->setGCodeVisualType(gcode::GCodeVisualType::gvt_structure);

		Qt3DRender::QGeometry* geometry = attain->createGeometry();
		m_scene->setGeometry(geometry, Qt3DRender::QGeometryRenderer::Points);
		
		m_scene->setLayerHeight(attain->layerHeight());
		m_scene->setLineWidth(attain->lineWidth());

		QMatrix4x4 pose = qtuser_3d::xform2QMatrix(attain->modelMatrix());
		if (m_printer)
			pose.translate(m_printer->globalBox().min);
		m_scene->setPreviewNodeModelMatrix(pose);

		m_scene->setZseamRetractDis();

		m_scene->setIndicatorVisible(indicatorVisible());

		m_layerHightModel->setRange(attain->minLayerHeight(), attain->maxLayerHeight());

		setCurrentLayer(layer, false);
		setCurrentStep(attain->steps(layer));

		emit layersChanged();
	}


	void SlicePreviewFlow::setSliceAttain(SliceAttain* attain)
	{
		bool reload = false;
		if (attain)
		{
			int layerCount1 = attain->layers(), layerCount2 = m_lastLayersCount;
			reload = layerCount1 == layerCount2;
		}

		m_attain = attain;

		if (m_attain == NULL)
		{
			m_scene->clear();
			m_scene->setIndicatorVisible(false);
		}
		else if (isRenderRenderGraph(this))
		{
			// m_scene->setIndicatorVisible(indicatorVisible());

			//设置平台尺寸
			updatePrinterBox(m_attain->getMachineWidth()
                , m_attain->getMachineDepth()
                , m_attain->getMachineHeight());


			int stepCount = 0;
			m_attain->prepareVisualTypeData(m_visualType);
			Qt3DRender::QGeometry* geometry = m_attain->createGeometry();
			m_scene->setGeometry(geometry, Qt3DRender::QGeometryRenderer::Points);
			m_scene->setGCodeVisualType(m_visualType);

			m_scene->setRetractionGeometry(m_attain->createRetractionGeometry(), Qt3DRender::QGeometryRenderer::Points);
			m_scene->setZSeamsGeometry(m_attain->createZSeamsGeometry(), Qt3DRender::QGeometryRenderer::Points);
			m_scene->setUnretractGeometry(m_attain->createUnretractGeometry(), Qt3DRender::QGeometryRenderer::Points);

			m_scene->setLayerHeight(m_attain->layerHeight());
			m_scene->setLineWidth(m_attain->lineWidth());

			QMatrix4x4 pose = qtuser_3d::xform2QMatrix(attain->modelMatrix());
			if (m_printer)
				pose.translate(m_printer->globalBox().min);
			m_scene->setPreviewNodeModelMatrix(pose);

			m_scene->setIndicatorVisible(indicatorVisible());

			QVariantList _colorList = m_attain->getNozzleColorList();
			if (!_colorList.empty())
			{
				setNozzleColorList(m_attain->getNozzleColorList());
			}

			stepCount = m_attain->totalSteps();
			m_speedModel->setRange(m_attain->minSpeed(), m_attain->maxSpeed());
			if (m_attain->isCuraProducer())
			{
				m_structureModel->setTimeParts(m_attain->getTimeParts());
			}
			else {
				m_structureModel->setOrcaTimeParts(m_attain->getTimeParts_orca(),m_attain->printTime());
			}
			
			m_layerHightModel->setRange(m_attain->minLayerHeight(), m_attain->maxLayerHeight());
			m_lineWidthModel->setRange(m_attain->minLineWidth(), m_attain->maxLineWidth());
			m_flowModel->setRange(m_attain->minFlowOfStep(), m_attain->maxFlowOfStep());
			m_layerTimeModel->setRange(m_attain->minTimeOfLayer(), m_attain->maxTimeOfLayer());
			//m_fanSpeedModel->setRange(0.0, 1.0);
			m_temperatureModel->setRange(m_attain->minTemperature(), m_attain->maxTemperature());

			if (!reload)
			{
				setCurrentLayer(m_attain->layers(), false);
				setCurrentStep(m_attain->steps(m_attain->layers()));
			}
			else
			{
				int step = m_currentStep;
				setCurrentLayer(m_currentLayer, false);
				setCurrentStep(step);
			}

			m_lastLayersCount = m_attain->layers();

			m_scene->setLayerHeight(m_attain->layerHeight());
			m_scene->setLineWidth(m_attain->lineWidth());
		}

		emit layersChanged();

		setExtruderTableModel();
	}

	SliceAttain* SlicePreviewFlow::attain()
	{
		return m_attain;
	}

	SliceAttain* SlicePreviewFlow::takeAttain()
	{
		//return m_attain.take();
		SliceAttain* attain = m_attain;
		m_attain = NULL;
		return attain;
	}

	qtuser_3d::ColorPicker* SlicePreviewFlow::colorPicker()
	{ 
		return m_colorPicker;
	}

    
    bool SlicePreviewFlow::indicatorVisible()
    {
        return m_indicatorVisible;
    }

    void SlicePreviewFlow::setIndicatorVisible(bool visible)
    {
        if (m_indicatorVisible != visible)
        {
            m_indicatorVisible = visible;
            emit indicatorVisibleChanged();
			
			m_scene->setIndicatorVisible(m_indicatorVisible);
			renderOneFrame();
        }   
    }

	bool SlicePreviewFlow::isAttainDisplayCompletly()
	{
		if (!m_attain)
			return false;

		return m_currentLayer == m_attain->layers();
	}

	Qt3DCore::QEntity* SlicePreviewFlow::sceneGraph()
	{
		return m_scene;
	}

	void SlicePreviewFlow::begineRender()
	{
		qInfo() << "fdm slice preview begineRender";
		cacheReuseable(this);
		m_scene->setParent(this);
		m_surface->setCamera(getCachedCameraEntity());

		if (!m_attain)
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
		// m_surface->setCamera(nullptr);
		m_scene->clear();
		m_scene->setParent((Qt3DCore::QNode*)nullptr);
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

		QSize newSize = size * m_colorPicker->renderTargetRatio();

		if (m_surfaceSize == newSize)
			return;

		m_colorPicker->resize(size);
		m_surface->updateSurfaceSize(newSize);
		
		m_surfaceSize = newSize;

		requestCapture(true);
	}

	void SlicePreviewFlow::onKeyPress(QKeyEvent* event)
	{
		//if (event->key() == Qt::Key_P && m_attain)
		//{
		//	auto f = [this](const QImage& image) {
		//		image.save("fdm.bmp");
		//		endRequest(image);
		//	};

		//	//requestPreview(modelGroups().first(), f);
		//}
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

	void SlicePreviewFlow::setExtruderTableModel()
	{
		if (!m_attain || !m_extruderTableModel)
			return;

		std::vector<QString> colors;
		std::vector<std::pair<int, double>> volumes_per_extruder;
		std::vector<std::pair<int, double>> flush_per_filament;
		std::vector<std::pair<int, double>> volumes_per_tower;

		m_attain->getFilamentsList(volumes_per_extruder, flush_per_filament, volumes_per_tower);
		m_extruderTableModel->setData(volumes_per_extruder, flush_per_filament, volumes_per_tower);
	}

	void SlicePreviewFlow::requestCapture(bool capture)
	{
		if (capture)
			m_colorPicker->requestCapture();
		emit signalUpdate();
	}

#ifdef DEBUG
	bool SlicePreviewFlow::showDebugOverlay()
	{
#ifdef ENABLE_DEBUG_OVERLAY
		return m_surface->showDebugOverlay();
#else
		return false;
#endif
	}
	void SlicePreviewFlow::setShowDebugOverlay(bool showDebugOverlay)
	{
#ifdef ENABLE_DEBUG_OVERLAY
		m_surface->setShowDebugOverlay(showDebugOverlay);
#endif
	}
#endif

	void SlicePreviewFlow::setSceneClearColor(const QColor& color)
	{
		m_surface->setClearColor(color);
	}

	void SlicePreviewFlow::setNozzleColorList(const QVariantList& list)
	{
		{
			QList<cxgcode::GcodeExtruderData> dataList;

			for (size_t i = 0; i < list.size(); i++)
			{
				cxgcode::GcodeExtruderData data;
				data.color = list.at(i).value<QColor>();
				data.index = i + 1;
				dataList.push_back(data);
			}
			m_extruderTableModel->setColorList(dataList);
			m_extruderModel->setDataList(dataList);
		}

		//喷嘴颜色使用前面16个，第17个作为暂停层的标志位
		{
			QVariantList list_17;
			list_17 << list;
			for (size_t i = list.size(); i < 17; i++)
			{
				list_17 << QColor(128, 128, 128);
			}
			m_scene->previewNode()->setNozzleColorList(list_17);
		}
		
	}
}
