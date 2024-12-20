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
#include "external/interface/visualsceneinterface.h"
#include "external/interface/uiinterface.h"
#include "external/slice/sliceattain.h"

#include "internal/multi_printer/printer.h"
#include "internal/render/slicepreviewnode.h"
#include "internal/render/slicepreviewscene.h"
#include "qtuser3d/module/selector.h"
#include "interface/reuseableinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/printerinterface.h"
#include "entity/worldindicatorentity.h"
#include "QtCore/qcoreapplication.h"
#include <QElapsedTimer>
#include "interface/appsettinginterface.h"

#include "kernel/kernel.h"
#include "kernel/reuseablecache.h"

namespace creative_kernel
{
	SlicePreviewFlow::SlicePreviewFlow(Qt3DCore::QNode* parent)
		: QObject(parent)
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
		, m_accModel(std::make_unique<cxgcode::GcodePreviewRangeDivideModel>(gcode::GCodeVisualType::gvt_acc))

	{
		m_currentLayer = m_indexOffset;
		m_currentStep = m_indexOffset;
		setPosition();

		m_scene = new SlicePreviewScene();

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
		m_accModel->setColors(colors);
	}

	SlicePreviewScene* SlicePreviewFlow::scene()
	{
		return m_scene;
	}

	void SlicePreviewFlow::useCachePreview()
	{
		if (m_attain)
        {
            m_attain->saveTempGCode();
            QImage tmpImage = m_cachePreview;
            m_attain->saveTempGCodeWithImage(tmpImage);
        }
	}

	void SlicePreviewFlow::notifyClipValue()
	{
		if (!m_attain)
			return;

		QVector4D clipValue(0.0f, m_currentLayer, 0.0f, m_currentStep);
		m_scene->setClipValue(clipValue);
		QVector3D pos = qtuser_3d::vec2qvector(m_attain->traitPosition(m_currentLayer, m_currentStep));
		Printer* printer = getSelectedPrinter();
		if (printer)
			pos += printer->globalBox().min;

		m_scene->setIndicatorPos(pos);

		renderOneFrame();
	}

	bool SlicePreviewFlow::isAvailable()
	{
		return m_attain;
	}

	int SlicePreviewFlow::gCodeVisualType()
	{
		return m_gCodeVisualType;
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
		m_gCodeVisualType = type;
		emit gCodeVisualTypeChanged();
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
		m_stepsMax = layerSteps;
		notifyClipValue();

		m_attain->updateStepIndexMap(m_currentLayer);

		setPosition();
		setLayerHeight();
		setTemperature();
		setAcc();
		setLineWidth();
		setFlow();
		setLayerTime();
		setFanSpeed();
		setSpeed();
		emit stepsChanged();
		// emit stepsMaxChanged();
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

	QAbstractListModel* SlicePreviewFlow::getAccModel() const {
		return m_accModel.get();
	}

	void SlicePreviewFlow::setCurrentStep(int step)
	{
		m_currentStep = step;
		notifyClipValue();
		setPosition();
		setTemperature();
		setAcc();
		setLineWidth();
		setFlow();
		setLayerTime();
		setFanSpeed();
		setSpeed();
		// emit stepsChanged();
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

	int SlicePreviewFlow::stepsMax()
	{
		return m_stepsMax;
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

	QVector3D SlicePreviewFlow:: position()
	{
		return m_position;
	};


	void SlicePreviewFlow::setPosition()
	{
		if (!m_attain)
			return ;

		trimesh::vec3 trimeshVec= m_attain->traitPosition(m_currentLayer, m_currentStep);
		QVector3D qVec(trimeshVec.x, trimeshVec.y, trimeshVec.z);
		m_position = qVec;
		emit positionChanged();
	}

	float  SlicePreviewFlow::layerHeight()
	{
		return m_layerHeight;
	}

	void SlicePreviewFlow::setLayerHeight()
	{
		if (!m_attain)
			return;

		m_layerHeight = m_attain->layerHeight(m_currentLayer);
		emit layerHeightChanged();
	}

	float  SlicePreviewFlow::temperature()
	{
		return m_temperature;
	}

	void SlicePreviewFlow::setTemperature()
	{
		if (!m_attain)
			return;

		m_temperature = m_attain->traitTemperature(m_currentLayer, m_currentStep);
		emit temperatureChanged();
	}

	float  SlicePreviewFlow::acc()
	{
		return m_acc;
	}

	void SlicePreviewFlow::setAcc()
	{
		if (!m_attain)
			return;
		m_acc = m_attain->traitAcc(m_currentLayer, m_currentStep);
		emit accChanged();
	}

	float SlicePreviewFlow::lineWidth()
	{
		return m_lineWidth;
	}

	void SlicePreviewFlow::setLineWidth()
	{
		if (!m_attain)
			return;
		m_lineWidth = m_attain->traitLineWidth(m_currentLayer, m_currentStep);
		emit lineWidthChanged();
	}

	float SlicePreviewFlow::flow()
	{
		return m_flow;
	}

	void SlicePreviewFlow::setFlow()
	{
		if (!m_attain)
			return;
		m_flow = m_attain->traitFlow(m_currentLayer, m_currentStep);
		emit flowChanged();
	}

	float SlicePreviewFlow::layerTime()
	{
		return m_layerTime;
	}

	void SlicePreviewFlow::setLayerTime()
	{
		if (!m_attain)
			return;
		m_layerTime = m_attain->traitLayerTime(m_currentLayer, m_currentStep);
		emit layerTimeChanged();
	}

	float SlicePreviewFlow::fanSpeed()
	{
		return m_fanSpeed;
	}

	void SlicePreviewFlow::setFanSpeed()
	{
		if (!m_attain)
			return;
		m_fanSpeed = m_attain->traitFanSpeed(m_currentLayer, m_currentStep);
		emit fanSpeedChanged();
	}

	float SlicePreviewFlow::speed()
	{
		return m_speed;
	}

	void SlicePreviewFlow::setSpeed()
	{
		if (!m_attain)
			return;
		m_speed = m_attain->traitSpeed(m_currentLayer, m_currentStep);
		emit speedChanged();
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
		//m_attain->saveTempGCode();
		//m_attain->saveTempGCodeWithImage(*(m_attain->getImageFromGcode()));
	}

	void SlicePreviewFlow::previewSliceAttain(SliceAttain* attain, int layer)
	{
		if (attain == nullptr) return;

		//提前预览阶段使用 gvt_structure 类型，其他类型数据可能不准确
		m_scene->setGCodeVisualType(gcode::GCodeVisualType::gvt_structure);

		Qt3DRender::QGeometry* geometry = attain->createGeometry();
		m_scene->setGeometry(geometry, Qt3DRender::QGeometryRenderer::Points);
		
		m_scene->setLayerHeight(attain->layerHeight());
		m_scene->setLineWidth(attain->lineWidth());

		QMatrix4x4 pose = qtuser_3d::xform2QMatrix(attain->modelMatrix());
		Printer* printer = getSelectedPrinter();
		if (printer)
			pose.translate(printer->globalBox().min);
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
		if(m_attain && attain && 
			m_attain->id() == attain->id())
		{
			return;
		}
		if (attain)
		{
			int totalLayers = attain->layers(), totalLayers2 = m_lastTotalLayers;
			reload = totalLayers == totalLayers2;
		}

		m_attain = attain;

		if (m_attain == NULL)
		{
			m_scene->clear();
			m_scene->setIndicatorVisible(false);
		}
		else if (isUsingPreviewScene())
		{
			setPreviewEnabled(false);
			// m_scene->setIndicatorVisible(indicatorVisible());

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
			Printer* printer = getSelectedPrinter();
			if (printer)
				pose.translate(printer->globalBox().min);
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
			m_accModel->setRange(m_attain->minAcc(), m_attain->maxAcc());



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


			m_lastTotalLayers = m_attain->layers();
			m_scene->setLayerHeight(m_attain->layerHeight());
			m_scene->setLineWidth(m_attain->lineWidth());

			setPreviewEnabled(true);
			setCurrentLayerFocused(false);
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

	void SlicePreviewFlow::setExtruderTableModel()
	{
		if (!m_attain || !m_extruderTableModel)
			return;

		std::vector<std::pair<int, double>> volumes_per_extruder;
		std::vector<std::pair<int, double>> flush_per_filament;
		std::vector<std::pair<int, double>> volumes_per_tower;

		m_attain->getFilamentsList(volumes_per_extruder, flush_per_filament, volumes_per_tower);
		m_extruderTableModel->setData(volumes_per_extruder, flush_per_filament, volumes_per_tower);
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

		//喷嘴颜色使用前面50个，第51个作为暂停层的标志位
		{
			QVariantList list_51;
			list_51 << list;
			for (size_t i = list.size(); i < 51; i++)
			{
				list_51 << QColor(128, 128, 128);
			}
			m_scene->previewNode()->setNozzleColorList(list_51);
		}
	}
}
