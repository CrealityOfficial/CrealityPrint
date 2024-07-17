#include "sliceattain.h"
#include "cxgcode/gcodehelper.h"
#include "crslice/gcode/gcodedata.h"
#include "data/kernelmacro.h"
#include "crslice/gcode/parasegcode.h"
#include "crslice2/gcodeprocessor.h"

#include "interface/machineinterface.h"
#include "interface/appsettinginterface.h"

#include "qtusercore/string/resourcesfinder.h"
#include "qtusercore/module/systemutil.h"
#include "qtuser3d/trimesh2/conv.h"

#include <Qt3DRender/QAttribute>
#include <QtCore/QUuid>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QThread>


using namespace qtuser_3d;
namespace creative_kernel
{
	SliceAttain::SliceAttain(SliceResultPointer result, SliceAttainType type, QObject* parent)
		:QObject(parent)
		, m_cache(nullptr)
		//, m_attribute(nullptr)
		, m_type(type)
		, m_result(result)
	{
		QString path = QString("%1/%2").arg(TEMPGCODE_PATH).arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
		mkMutiDir(path);

		QString name = QString(QString::fromLocal8Bit(m_result->sliceName().c_str()));
		m_tempGCodeFileName = QString("%1/%2").arg(path).arg(name);
		int maxPath = qtuser_core::getSystemMaxPath() - 7;
		if (m_tempGCodeFileName.length() > maxPath)
		{
			m_tempGCodeFileName = m_tempGCodeFileName.left(maxPath);
		}
		m_tempGCodeImageFileName = QString("%1_image").arg(m_tempGCodeFileName);
		if (m_tempGCodeImageFileName.length() > maxPath)
		{
			m_tempGCodeImageFileName = QString("%1_image").arg(m_tempGCodeImageFileName.left(maxPath - 6));
		}
		m_tempGCodeFileName += ".gcode";
		m_tempGCodeImageFileName += ".gcode";
	}

	SliceAttain::~SliceAttain()
	{
		QFileInfo info(m_tempGCodeFileName);
		QString path = info.absolutePath();

		clearPath(path);
		QDir dir;
		dir.setPath(path);
		dir.removeRecursively();
	}

	void SliceAttain::build(ccglobal::Tracer* tracer)
	{
		builder.build(m_result, tracer);
	}

	void SliceAttain::getDataFromOrca()
	{
		//getTime
		std::vector<std::vector<std::pair<int, float>>> times;
		crslice2::process_file(m_result->fileName(), times);
		if (times.size() > 1)
		{
			gcode::GCodeParseInfo& info = builder.getGCodeStruct().getParam();
			info.roles_time = times[0];

			if (times.size() > 2)
				if (!times[2].empty())
					info.printTime = times[2][0].second;

			//merge cura time
			{
				if (info.timeParts.OuterWall > 0.0f)
					info.roles_time.push_back(std::pair<int, float>(1, info.timeParts.OuterWall));
				if (info.timeParts.InnerWall > 0.0f)
					info.roles_time.push_back(std::pair<int, float>(2, info.timeParts.InnerWall));
				if (info.timeParts.Skin > 0.0f)
					info.roles_time.push_back(std::pair<int, float>(3, info.timeParts.Skin));
				if (info.timeParts.Support > 0.0f)
					info.roles_time.push_back(std::pair<int, float>(4, info.timeParts.Support));
				if (info.timeParts.SkirtBrim > 0.0f)
					info.roles_time.push_back(std::pair<int, float>(5, info.timeParts.SkirtBrim));
				if (info.timeParts.Infill > 0.0f)
					info.roles_time.push_back(std::pair<int, float>(6, info.timeParts.Infill));
				if (info.timeParts.SupportInfill > 0.0f)
					info.roles_time.push_back(std::pair<int, float>(7, info.timeParts.SupportInfill));
				if (info.timeParts.MoveCombing > 0.0f)
					info.roles_time.push_back(std::pair<int, float>(8, info.timeParts.MoveCombing));
				if (info.timeParts.MoveRetraction > 0.0f)
					info.roles_time.push_back(std::pair<int, float>(9, info.timeParts.MoveRetraction));
				if (info.timeParts.PrimeTower > 0.0f)
					info.roles_time.push_back(std::pair<int, float>(11, info.timeParts.PrimeTower));
			}
			builder.getGCodeStruct().setParam(info);
			//builder.parseInfo.layers_time = times[1];
			for (auto& time : times[1])
			{
				if (time.first >= -1)
				{
					auto iter = builder.getGCodeStruct().m_layerTimes.find(time.first+1);
					if (iter != builder.getGCodeStruct().m_layerTimes.end())
						iter->second = time.second;
					else
						builder.getGCodeStruct().m_layerTimes.insert(std::pair<int, float>(time.first+1, time.second));
				}
			}
		}
	}

	void SliceAttain::build_paraseGcode(ccglobal::Tracer* tracer)
	{
		gcode::paraseGcodeAndPreview(m_result->fileName(), this, tracer);

		gcode::GCodeParseInfo& info = builder.getGCodeStruct().getParam();
		if (info.producer == gcode::GProducer::OrcaSlicer && !info.have_roles_time)
		{
			getDataFromOrca();
		}

		builder.build_with_image(m_result, tracer);
	}

	void SliceAttain::preparePreviewData(ccglobal::Tracer* tracer)
	{
		builder.build();
		//提前预览阶段使用 gvt_structure 类型
		prepareVisualTypeData(gcode::GCodeVisualType::gvt_structure);
	}

	QString SliceAttain::sliceName()
	{
		
		return QString(QString::fromLocal8Bit(m_result->sliceName().c_str()));
	}

	QString SliceAttain::sourceFileName()
	{
		return QString(QString::fromLocal8Bit(m_result->fileName().c_str()));
	}

	bool SliceAttain::isFromFile()
	{
		if (getEngineType() == EngineType::ET_ORCA)
		{
			return false;
		}
		return m_type == SliceAttainType::sat_file;
	}

	int SliceAttain::printTime()
	{
		return builder.parseInfo.printTime;
	}

	QString SliceAttain::material_weight()
	{

		if (builder.parseInfo.weight > 0.0f)
		{
			return QString::number(builder.parseInfo.weight, 'f', 2);
		}

		return QString::number(builder.parseInfo.materialLenth
			* builder.parseInfo.materialDensity, 'f', 2);
	}

	QString SliceAttain::printing_time()
	{
		int printTime = builder.parseInfo.printTime;
		QString str = QString("%1h%2m%3s").arg(printTime / 3600)
			.arg(printTime / 60 % 60)
			.arg(printTime % 60);
		return str;
	}

	QString SliceAttain::material_money()
	{
		if (builder.parseInfo.cost > 0.0f)
		{
			return QString::number(builder.parseInfo.cost, 'f', 2);
		}

		int nTemp = (builder.parseInfo.materialLenth + 0.005) * 100;
		float materialLength = nTemp / 100.0f;
		return QString::number(materialLength * builder.parseInfo.unitPrice, 'f', 2);
	}

	QString SliceAttain::material_length()
	{
		return QString::number(builder.parseInfo.materialLenth);
	}

	trimesh::box3 SliceAttain::box()
	{
		return m_result->inputBox;
	}

	int SliceAttain::layers()
	{
		return builder.baseInfo.layers;
	}

	int SliceAttain::steps(int layer)
	{
		int _layer = layer - INDEX_START_AT_ONE;

		if (_layer < 0 || _layer >= (int)builder.baseInfo.steps.size())
			return 0;

		return builder.baseInfo.steps.at(_layer);
	}

	int SliceAttain::totalSteps()
	{
		return builder.baseInfo.totalSteps;
	}

	int SliceAttain::nozzle()
	{
		return builder.baseInfo.nNozzle;
	}

	float SliceAttain::minSpeed()
	{
		return FLT_MAX != builder.baseInfo.speedMin ? builder.baseInfo.speedMin : 0.0f;
	}

	float SliceAttain::maxSpeed()
	{
		return FLT_MIN != builder.baseInfo.speedMax ? builder.baseInfo.speedMax : 0.0f;
	}

	float SliceAttain::minTimeOfLayer()
	{
		return FLT_MAX != builder.baseInfo.minTimeOfLayer ? builder.baseInfo.minTimeOfLayer : 0.0f;
	}

	float SliceAttain::maxTimeOfLayer()
	{
		return FLT_MIN != builder.baseInfo.maxTimeOfLayer ? builder.baseInfo.maxTimeOfLayer : 0.0f;
	}

	float SliceAttain::minFlowOfStep()
	{
		return FLT_MAX != builder.baseInfo.minFlowOfStep ? builder.baseInfo.minFlowOfStep : 0.0f;
	}

	float SliceAttain::maxFlowOfStep()
	{
		return FLT_MIN != builder.baseInfo.maxFlowOfStep ? builder.baseInfo.maxFlowOfStep : 0.0f;
	}

	float SliceAttain::minLineWidth()
	{
		return FLT_MAX != builder.baseInfo.minLineWidth ? builder.baseInfo.minLineWidth : 0.0f;
	}

	float SliceAttain::maxLineWidth()
	{
		return FLT_MIN != builder.baseInfo.maxLineWidth ? builder.baseInfo.maxLineWidth : 0.0f;
	}

	float SliceAttain::minLayerHeight()
	{
		return FLT_MAX != builder.baseInfo.minLayerHeight ? builder.baseInfo.minLayerHeight : 0.0f;
	}

	float SliceAttain::maxLayerHeight()
	{
		return FLT_MIN != builder.baseInfo.maxLayerHeight ? builder.baseInfo.maxLayerHeight : 0.0f;
	}

	float SliceAttain::minTemperature()
	{
		return FLT_MAX != builder.baseInfo.minTemperature ? builder.baseInfo.minTemperature : 0.0f;
	}

	float SliceAttain::maxTemperature()
	{
		return FLT_MIN != builder.baseInfo.maxTemperature ? builder.baseInfo.maxTemperature : 0.0f;
	}

	float SliceAttain::layerHeight(int layer)
	{
		trimesh::vec3 pos = traitPosition(layer, 1);
		float height = builder.layerHeight(layer);
		if (height < 0)
			return pos.z;
		else
			return /*pos.z + */height;
	}

	float SliceAttain::layerHeight()
	{
		return builder.parseInfo.layerHeight;
	}

	float SliceAttain::lineWidth()
	{
		return builder.parseInfo.lineWidth;
	}

	gcode::TimeParts SliceAttain::getTimeParts() const {
		return builder.parseInfo.timeParts;
	}

	std::vector<std::pair<int, float>> SliceAttain::getTimeParts_orca()
	{
		return builder.parseInfo.roles_time;
	}

	bool SliceAttain::isCuraProducer()
	{
		gcode::GCodeParseInfo& info = builder.getGCodeStruct().getParam();
		return info.producer != gcode::GProducer::OrcaSlicer;
	}

	QImage* SliceAttain::getImageFromGcode()
	{
		if (m_result)
		{
			if (!m_result->previewsData.empty())
			{
				QImage* image=new QImage();
				image->loadFromData(&m_result->previewsData.back()[0], m_result->previewsData.back().size());
				return image;
			}
		}
		return nullptr;
	}

	QVariantList SliceAttain::getNozzleColorList()
	{
		std::vector<std::string> colorList = builder.getGCodeStruct().m_nozzleColorList;
		QVariantList _colotList;
		for (auto& color : colorList)
		{
			QColor _color(color.c_str());
			_colotList.push_back(_color);
		}
		return _colotList;
	}

	void SliceAttain::getFilamentsList(std::vector<std::pair<int, double>>& volumes_per_extruder, std::vector<std::pair<int, double>>& flush_per_filament, std::vector<std::pair<int, double>>& volumes_per_tower)
	{
		volumes_per_extruder = builder.parseInfo.volumes_per_extruder;
		flush_per_filament = builder.parseInfo.flush_per_filament;
		volumes_per_tower = builder.parseInfo.volumes_per_tower;
	}

	int SliceAttain::get_filamentchanges()
	{
		return builder.parseInfo.total_filamentchanges;
	}

	QString SliceAttain::fileNameFromGcode()
	{
		if (m_result)
		{
			QString str = QString(QString::fromLocal8Bit(m_result->sliceName().c_str()));
			int index = str.lastIndexOf('/');
			if (index)
			{
				str = str.right(str.length() - index-1);
			}
			int index2 = str.lastIndexOf(".gcode");
			if (index2)
			{
				str = str.left(index2);
			}
			return str;
		}

		return "";
	}

	float SliceAttain::traitSpeed(int layer, int step)
	{
		return builder.traitSpeed(layer, step);
	}

	trimesh::vec3 SliceAttain::traitPosition(int layer, int step)
	{
		return builder.traitPosition(layer, step);
	}

	float SliceAttain::traitDuration(int layer, int step)
	{
		return builder.traitDuration(layer, step);
	}

	trimesh::fxform SliceAttain::modelMatrix()
	{
		return builder.parseInfo.xf4;
	}

	int SliceAttain::findViewIndexFromStep(int layer, int nStep)
	{
		int _layer = layer - INDEX_START_AT_ONE;
		int _nStep = nStep - INDEX_START_AT_ONE;

		if (_layer >= 0 && _layer < layers())
		{
			const std::vector<int>& maps = builder.m_stepGCodesMaps.at(_layer);
			if (_nStep >= 0 && _nStep < maps.size())
				return maps.at(_nStep);
		}
		return -1;
	}

	int SliceAttain::findStepFromViewIndex(int layer, int nViewIndex)
	{
		int _layer = layer - INDEX_START_AT_ONE;
		if (_layer >= 0 && _layer < layers())
		{
			if (_layer>=0 && _layer < builder.m_stepGCodesMaps.size())
			{
				const std::vector<int>& maps = builder.m_stepGCodesMaps.at(_layer);
				for (int step = 0; step < maps.size(); step++)
				{
					int viewIndex = maps[step];
					if (viewIndex == nViewIndex)
					{
						return step;
					}
				}
			}
		}
		return -1;
	}

	void SliceAttain::updateStepIndexMap(int layer)
	{
	}

	const QString SliceAttain::layerGcode(int layer)
	{
		int _layer = layer - INDEX_START_AT_ONE;
		return QString::fromStdString(m_result->layer(_layer));
	}

	void SliceAttain::prepareVisualTypeData(gcode::GCodeVisualType type)
	{
		builder.updateFlagAttribute(nullptr, type);
	}

	Qt3DRender::QGeometry* SliceAttain::createGeometry()
	{
		/*if (m_attribute)
		{
			m_attribute->setParent((Qt3DCore::QNode*)nullptr);
			delete m_attribute;
			m_attribute = nullptr;
		}*/
		/*if (m_cache)
		{
			m_cache->setParent((Qt3DCore::QNode*)nullptr);
			delete m_cache;
			m_cache = nullptr;
		}*/
		
		//if (!m_cache)
		{
			m_cache = builder.buildGeometry();
			/*m_attribute = new Qt3DRender::QAttribute(m_cache);
			m_cache->addAttribute(m_attribute);*/
		}

		return m_cache;
	}

	Qt3DRender::QGeometry* SliceAttain::rebuildGeometryVisualTypeData()
	{
		if (m_cache) {
			builder.rebuildGeometryVisualTypeData();
		}
		return m_cache;
	}

    float SliceAttain::getMachineHeight()
    {
        return builder.parseInfo.machine_height;
    }

    float SliceAttain::getMachineWidth()
    {
        return builder.parseInfo.machine_width;
    }

    float SliceAttain::getMachineDepth()
    {
        return builder.parseInfo.machine_depth;
    }

    int SliceAttain::getBeltType()
    {
        return builder.parseInfo.beltType;
    }

	Qt3DRender::QGeometry* SliceAttain::createRetractionGeometry()
	{
#if SHOW_ZSEAM_POINTS
		return builder.buildRetractionGeometry();
#else
		return nullptr;
#endif
	}

	Qt3DRender::QGeometry* SliceAttain::createZSeamsGeometry()
	{
#if SHOW_ZSEAM_POINTS
		return builder.buildZSeamsGeometry();
#else
		return nullptr;
#endif
	}

	Qt3DRender::QGeometry* SliceAttain::createUnretractGeometry()
	{
		return builder.buildUnretractGeometry();
	}

	Qt3DRender::QGeometryRenderer* SliceAttain::createRetractionGeometryRenderer()
	{
#if SHOW_ZSEAM_POINTS
		return builder.buildRetractionGeometryRenderer();
#else
		return nullptr;
#endif
	}

	Qt3DRender::QGeometryRenderer* SliceAttain::createZSeamsGeometryRenderer()
	{
#if SHOW_ZSEAM_POINTS
		return builder.buildZSeamsGeometryRenderer();
#else
		return nullptr;
#endif
	}

	void SliceAttain::saveGCode(const QString& fileName, QImage* previewImage)
	{
		QString imageStr;
		if (previewImage && !isFromFile())
		{
			float layerHeight = builder.parseInfo.layerHeight;
			QString screenSize = QString(QString::fromLocal8Bit(builder.parseInfo.screenSize.c_str()));
            QString exportFormat = QString(QString::fromLocal8Bit(builder.parseInfo.exportFormat.c_str()));

			previewImage = cxsw::resizeModule(previewImage);
			if (exportFormat == "bmp")
			{
				cxsw::image2String(*previewImage, 64, 64, true, imageStr);
				cxsw::image2String(*previewImage, 400, 400, false, imageStr);
			}
			else
			{
				QImage minPreImg;
				QImage maxPreImg;
				if (screenSize == "CR-10 Inspire")
				{
					minPreImg = previewImage->scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation);
					maxPreImg = previewImage->scaled(600, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
				}
				else if (screenSize == "CR-200B Pro")
				{
					minPreImg = previewImage->scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
					maxPreImg = previewImage->scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
				}
				else if (screenSize == "Ender-3 S1")
				{
					maxPreImg = previewImage->scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
				}
				else if (screenSize == "Ender-5 S1")
				{
					maxPreImg = previewImage->scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
				}
				else//Sermoon D3
				{
					minPreImg = previewImage->scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation);
					maxPreImg = previewImage->scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
				}

				if (!minPreImg.isNull())
				{
					cxsw::getImageStr(imageStr, &minPreImg, builder.baseInfo.layers, exportFormat, layerHeight, false, SLICE_PATH);
				}
				cxsw::getImageStr(imageStr, &maxPreImg, builder.baseInfo.layers, exportFormat, layerHeight, exportFormat == "png", SLICE_PATH);
			}
			if (previewImage)
				delete previewImage;
		}
		else if (isFromFile())
		{
			//float layerHeight = builder.parseInfo.layerHeight;
			QString screenSize = QString(QString::fromLocal8Bit(builder.parseInfo.screenSize.c_str()));
			QString exportFormat = QString(QString::fromLocal8Bit(builder.parseInfo.exportFormat.c_str()));

			QString imgSavePath = QString("%1/imgPreview.%2").arg(SLICE_PATH).arg(exportFormat);
			QImage* image = getImageFromGcode();
			if(image)
				image->save(imgSavePath);
			else
			{
				QFile::remove(imgSavePath);
			}
			//cxsw::getImageStr(imageStr, getImageFromGcode(), builder.baseInfo.layers, exportFormat, layerHeight, false, SLICE_PATH);
		}

		if (!isFromFile())
		{
			QString srcName = m_result->fileName().c_str();
			if (imageStr.isEmpty())
			{
				if (!QFile(fileName).exists())
				{
					qtuser_core::copyFile(srcName, fileName);
				}
			}
			else
			{
				QFile srcfile(srcName);
				QFile file(fileName);
				if (srcfile.open(QIODevice::ReadOnly) && file.open(QIODevice::WriteOnly))
				{
					file.write(imageStr.toLocal8Bit(), imageStr.size());
					file.write("\r\n", 2);
					file.write(srcfile.readAll());
				}
			}
		}
	}

	void SliceAttain::saveTempGCode()
	{
		if(!isFromFile())
			saveGCode(m_tempGCodeFileName, nullptr);
	}

	void SliceAttain::saveTempGCodeWithImage(QImage& image)
	{
		saveGCode(m_tempGCodeImageFileName, &image);
	}

	QString SliceAttain::tempGCodeFileName()
	{
        return m_tempGCodeFileName;
    }

    QString SliceAttain::tempGcodeThumbnail()
    {
		QString exportFormat = QString(QString::fromLocal8Bit(builder.parseInfo.exportFormat.c_str()));
        return QString("%1/imgPreview.%2").arg(SLICE_PATH).arg(exportFormat);
    }

	QString SliceAttain::tempGCodeImageFileName()
	{
		if (m_tempGCodeImageFileName.isEmpty())
			return m_tempGCodeFileName;
		return m_tempGCodeImageFileName;
	}

	QString SliceAttain::tempImageFileName()
	{
		return QString("%1/slice_flow_gcode_preview.png").arg(SLICE_PATH);
	}

	void SliceAttain::getPathData(const trimesh::vec3 point, float e, int type, bool isOrca, bool isseam)
	{
		if (m_type == SliceAttainType::sat_file)
			builder.getGCodeStruct().getPathData(point, e, type, true, isOrca,isseam);
		else 
			builder.getGCodeStruct().getPathData(point,e, type, false,isOrca,isseam);
	}
	void SliceAttain::getPathDataG2G3(const trimesh::vec3 point, float i, float j, float e, int type,int p, bool isG2, bool isOrca, bool isseam)
	{
		if (m_type == SliceAttainType::sat_file)
			builder.getGCodeStruct().getPathDataG2G3(point, i, j, e, type, p, isG2, true,isOrca,isseam);
		else
			builder.getGCodeStruct().getPathDataG2G3(point, i, j, e, type, p, isG2, false,isOrca, isseam);
	}
	void SliceAttain::setParam(gcode::GCodeParseInfo& pathParam)
	{
		builder.getGCodeStruct().setParam(pathParam);
	}
	void SliceAttain::setLayer(int layer)
	{
		builder.getGCodeStruct().setLayer(layer);

		// qDebug() << "SliceAttain setLayer thread " << QThread::currentThread() << "layer =" << layer;
		return;

		//if (!isFromFile())
		//{
		//	if (layer > 0 && layer % 5 == 0)
		//	{
		//		preparePreviewData();
		//		emit layerChanged(layer);
		//	}
		//}
	}

	void SliceAttain::setLayers(int layer)
	{
		if (layer >= 0)
			builder.baseInfo.layers = layer;
	}

	void SliceAttain::setSpeed(float s)
	{
		builder.getGCodeStruct().setSpeed(s);
	}
	void SliceAttain::setAcc(float acc)
	{
		builder.getGCodeStruct().setAcc(acc);
	}
	void SliceAttain::setTEMP(float temp)
	{
		builder.getGCodeStruct().setTEMP(temp);
	}
	void SliceAttain::setExtruder(int nr)
	{
		builder.getGCodeStruct().setExtruder(nr);
	}
	void SliceAttain::setFan(float fan)
	{
		builder.getGCodeStruct().setFan(fan);
	}
	void SliceAttain::setZ(float z, float h)
	{
		if (m_type == SliceAttainType::sat_file)
			builder.getGCodeStruct().setZ_from_gcode(z);
		else
			builder.getGCodeStruct().setZ(z,h);
	}
	void SliceAttain::setE(float e)
	{
		builder.getGCodeStruct().setE(e);
	}


	void SliceAttain::setWidth(float width)
	{
		builder.getGCodeStruct().setWidth(width);
	}

	void SliceAttain::setLayerHeight(float height)
	{
		builder.getGCodeStruct().setLayerHeight(height);
	}

	void SliceAttain::setLayerPause(int pause)
	{
		builder.getGCodeStruct().setLayerPause(pause);
	}
	void SliceAttain::setTime(float time)
	{
		builder.getGCodeStruct().setTime(time);
	}

	void SliceAttain::getNotPath()
	{
		builder.getGCodeStruct().getNotPath();
	}

	void SliceAttain::set_data_gcodelayer(int layer, const std::string& gcodelayer)//set a layer data
	{
		if (!m_result)
		{
			return ;
		}

		if (layer == -1)
			m_result->set_gcodeprefix(gcodelayer);
		else
			m_result->set_data_gcodelayer(layer, gcodelayer);
	}

	void SliceAttain::setNozzleColorList(std::string& colorList)
	{
		builder.getGCodeStruct().setNozzleColorList(colorList);
	}

	void SliceAttain::writeImages(const std::vector<std::pair<trimesh::ivec2, std::vector<unsigned char>>>& images)
	{
		for (size_t i = 0; i < images.size(); i++)
		{
			QString imgSavePath = QString("%1/imgPreview.%2").arg(SLICE_PATH).arg("png");
			if (i)
				imgSavePath = QString("%1/imgPreview_1.%2").arg(SLICE_PATH).arg("png");

			std::unique_ptr<QImage>image(new QImage());
			image->loadFromData(&images[i].second[0], images[i].second.size());
			if (image)
				image->save(imgSavePath);
		}
	}

	void SliceAttain::onSupports(int layerIdx, float z, float thickness, const std::vector<std::vector<trimesh::vec3>>& paths)
	{
	}

	void SliceAttain::setSceneBox(const trimesh::box3& box)
	{
	}

	qtuser_3d::Box3D SliceAttain::getGCodePathBoundingBox()
	{
		return qtuser_3d::triBox2Box3D(builder.pathBox());
	}
}
