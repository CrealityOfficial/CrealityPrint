#include "ansycworker.h"
#include "qtusercore/module/systemutil.h"
#include "qtusercore/module/progressortracer.h"

#include "sliceattain.h"
#include "dllansycslicer52.h"
#include "dllansycslicerorca.h"

#include <QtCore/QTimeLine>
#include <QtCore/QTime>
#include<QStandardPaths>
#include <QCoreApplication>

#include "interface/spaceinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/commandinterface.h"
#include "interface/machineinterface.h"
#include "interface/printerinterface.h"
#include "interface/messageinterface.h"
#include "interface/displaywarninginterface.h"
#include "internal/multi_printer/printer.h"
#include "message/sliceenginefailmessage.h"

#include "data/modeln.h"
#include "crslice2/crscene.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printprofile.h"
#include "kernel/kernel.h"
#include "cxgcode/gcodehelper.h"
#include "cxkernel/utils/utils.h"
#include <external/interface/uiinterface.h>
#include "unittest/unittestflow.h"

#include "dllansycslicerorcacache.h"

#define USE_CACHE_SLICE_INTERFACE 1
namespace creative_kernel
{
	SliceAttainTracer::SliceAttainTracer(qtuser_core::Progressor* progressor)
			: FormatTracer(progressor)
	{
	}

	int SliceAttainTracer::indexCount() {
		return 2;
	}

	QString SliceAttainTracer::indexStr(int index, va_list vars)
	{
		int layer = va_arg(vars, int);
		QString message = tr("");
		if (index == 0)
			message = tr("Start build render %1 layer").arg(layer);

		return message;
	}


	FormatSlice::FormatSlice(qtuser_core::Progressor* progressor)
	:FormatTracer(progressor)
	{
	}


	int FormatSlice::indexCount()
	{
		return 12;
	}

	QString FormatSlice::indexStr(int index, va_list vars)
	{
		int layer = va_arg(vars, int);
		QString message = tr("");
		if (index == 0)
			message = tr("Start slicing");
		if (index == 1)
			message = tr("Layer Slicing");
		if (index == 2)
			message = tr("Generating polygons");
		if (index == 3)
			message = tr("Generating walls");
		if (index == 4)
			message = tr("Generating skin and infill");
		if (index == 5)
			message = tr("Checking support necessity");
		if (index == 6)
			message = tr("Generating support area");
		if (index == 7)
			message = tr("Optimizing ZSeam");
		if (index == 8)
			message = tr("Generating gcode %1 layer").arg(layer);
		if (index == 9)
			message = tr("Slicing finished");
		if (index == 11)
			message = tr("need_support_structure");
		return message;
	}

	Crslice2Tracer::Crslice2Tracer(qtuser_core::Progressor* progressor, QObject* parent)
		:ProgressorTracer(progressor, parent)
	{
	}

	Crslice2Tracer::~Crslice2Tracer()
	{
	}

	void Crslice2Tracer::message(const char* msg)
	{
		if (m_progressor)
		{
			QString qstrMsg = msg;
			if (qstrMsg.startsWith("${UnitTest}") && BLCompareTestFlow::messageEnabled())
			{
				QString realMsg = qstrMsg.mid(QString("${UnitTest}").length());
				requestQmlTipDialog(realMsg);
			}
			else if (qstrMsg.startsWith("@@"))
			{
				QString realMsg = qstrMsg.mid(2);
				MessageObject* message = new Crslice2InfoMessage(realMsg);
				invokeBriefMessage(message);
				return;
			}
			else if (qstrMsg.contains("layer"))
			{
				QStringList list = qstrMsg.split("layer");
				if (list.size() != 2)
				{
					QString lan = QCoreApplication::translate("ParameterComponent", msg);
					m_progressor->message(lan);
				}
				else
				{
					QString msg2 = list[0] + "layer %1";
					QByteArray ba = msg2.toLatin1();
					int layer = list[1].toInt();
					QString lan = QCoreApplication::translate("ParameterComponent", ba.data()).arg(layer);
					m_progressor->message(lan);
				}
			}
			else
			{
				QString lan = QCoreApplication::translate("ParameterComponent", msg);
				m_progressor->message(lan);
			}
		}
	}

	void Crslice2Tracer::setExtraWarningDetails(const QString& warnType, const QString& warnDetail)
	{
		m_extraWarningDetails[warnType] = warnDetail;
	}

	QMap<QString, QString> Crslice2Tracer::getExtraWarningDetails()
	{
		return m_extraWarningDetails;
	}

	SettingsPointer convert(SettingsPointer setting)
	{
		SettingsPointer result(new us::USettings());
		const QHash<QString, us::USetting*>& settings = setting->settings();
		for (QHash<QString, us::USetting*>::ConstIterator it = settings.begin();
			it != settings.end(); ++it)
		{
			result->add(it.key(), it.value()->str(), true);
		}

		return result;
	}

	QList<SettingsPointer> convert(const QList<SettingsPointer>& settings)
	{
		QList<SettingsPointer> result;
		for (const SettingsPointer& setting : settings)
			result.append(convert(setting));
		return result;
	}

	AnsycWorker::AnsycWorker(QObject* parent)
		:Job(parent)
		, m_sliceAttain(nullptr)
	{
	}

	AnsycWorker::~AnsycWorker()
	{
	}

	SliceAttain* AnsycWorker::sliceAttain()
	{
		return m_sliceAttain;
	}

	void AnsycWorker::forceRegenerate()
	{
		m_forceRegenerate = true;
	}

	void AnsycWorker::setRemainAttain(SliceAttain* attain)
	{
		m_remainAttain = attain;
	}
	
	void AnsycWorker::setSlicePrinter(Printer* printer)
	{
		m_printer = printer;
	}

	Printer* AnsycWorker::slicePrinter()
	{
		return m_printer;
	}

	void AnsycWorker::applyAdditionalSliceParameter(SettingsPointer& G, QList<SettingsPointer>& Es)
	{
		/* default_filament_colour 使用到的耗材列表 */
		QSet<int> usedColorIndexes;
		QList<us::USettings*> settingsList;
		for (ModelN* model : m_slicedModels)
		{
			usedColorIndexes += model->renderData()->colorIndexs();
			settingsList << model->setting();
		}
		
		usedColorIndexes += m_printer->getUsedExtruders(true);

		auto parameterManager = getKernel()->parameterManager();
		auto settingPtr = parameterManager->currentMachine()->createCurrentGlobalSettings();
		settingsList << settingPtr.get();

		for (auto settings : settingsList)
		{
			QString supportEnable = settings->value("support_enable", "0");
			if (supportEnable != "0")
			{
				QString support = settings->value("support_filament", "0");
				if (support != "0")
				{
					int index = support.toInt() - 1;
					if (index >= 0)
						usedColorIndexes.insert(index);
				}

				QString supportInterface = settings->value("support_interface_filament", "0");
				if (supportInterface != "0")
				{
					int index = supportInterface.toInt() - 1;
					if (index >= 0)
						usedColorIndexes.insert(index);
				}
			}

			QString filamentColour;
			QString filamentType;
			std::vector<trimesh::vec> colors = currentColors();
			for (int i = 0, count = colors.size(); i < count; ++i)
			{
				QString colorStr;
				QString typeStr;
				if (usedColorIndexes.contains(i))
				{
					trimesh::vec color = colors[i];
					colorStr = QString("#%1%2%3").arg((int)(color[0] * 255), 2, 16, QChar('0')).
						arg((int)(color[1] * 255), 2, 16, QChar('0')).
						arg((int)(color[2] * 255), 2, 16, QChar('0'));

					typeStr = currentMaterialsType(i);
				}
				filamentColour += (colorStr + ";");
				filamentType += (typeStr + ";");
			}
			if (filamentColour.endsWith(";"))
				filamentColour = filamentColour.remove(filamentColour.length() - 1,1);

			if (filamentType.endsWith(";"))
				filamentType = filamentType.remove(filamentType.length() - 1, 1);

			settings->add("default_filament_colour", filamentColour, true);
			settings->add("default_filament_type", filamentType, true);
		}

		_dealExtruder(G, Es);
		_dealExtruderOverrides(G, Es);

		G->add("wipe_tower_x", m_printer->primeTowerPosition(0), true);
		G->add("wipe_tower_y", m_printer->primeTowerPosition(1), true);
	}

	void AnsycWorker::waitForCompleted()
	{
		while (m_slicing)
		{
			QThread::msleep(20);
			QCoreApplication::processEvents();
		}
	}

	void AnsycWorker::cancel()
	{

	}

	QString AnsycWorker::name()
	{
		return "Slice AnsycWorker.";
	}

	QString AnsycWorker::description()
	{
		return "Slice AnsycWorker.";
	}

	void AnsycWorker::failed()
	{
		emit sliceFailed(m_failExceptionDesc);
		if (m_sliceAttain)
			delete m_sliceAttain;
		m_slicing = false;
	}

	void AnsycWorker::successed(qtuser_core::Progressor* progressor)
	{
		if (!m_sliceAttain)
		{
			SliceAttain* attain = dynamic_cast<SliceAttain*>(m_printer->attain());
			if (attain)
				emit sliceSuccess(attain, m_printer);
			else
				emit sliceFailed(m_failExceptionDesc);
		}
		else
		{
			qtuser_3d::Box3D pathBox = m_sliceAttain->getGCodePathBoundingBox();

			bool canSlice = true;
			//gcode轨迹超出平台可打印区域
			if (checkGCodePathScope(pathBox, m_printer))
			{
				canSlice = false;
			}

			if (checkSliceWarning(m_sliceAttain->getSliceWarningDetails()))
			{
				// fix: when slice warning appears, can still export gcode
				//canSlice = false;
			}
			
			
			//if (pathSize.x() > printerSize.x() || pathSize.y() > printerSize.y())
			//{
			//	float x = pathSize.x() > printerSize.x() ? printerSize.x() - pathSize.x() : 0.0f;
			//	float y = pathSize.y() > printerSize.y() ? printerSize.y() - pathSize.y() : 0.0f;
			//
			//	QVector3D offset = QVector3D(x, y, 0.0f);
			//	m_printer->moveWipeTower(offset);
			//
			//	//TODO:slice Failed
			//	return;
			//}



			for (auto m : m_slicedModels)
			{
				m->recordSetting();
				m->resetDirty();
			}
			
			for (auto group : m_sliceModelGroups)
			{
				group->recordSetting();
				group->resetDirty();
			}

			clearSettingsDirty();

			// SliceAttain* sliceCache = dynamic_cast<SliceAttain*>(m_printer->sliceCache());
			// if (sliceCache)
			// {
			// 	if (sliceCache->layers() != m_sliceAttain->layers())
			// 	{
			// 		m_printer->clearLayersConfig();
			// 	}
			// }
			m_printer->resetDirty();
			m_printer->resetGCodeSettingDirty();
			m_printer->setAttain(m_sliceAttain);
			m_printer->setIsSliced(canSlice);
			
			emit sliceSuccess(m_sliceAttain, m_printer);
			m_sliceAttain = nullptr;
		}
		

		m_slicing = false;
	}

	bool AnsycWorker::needReSlice()
	{
		if (m_printer->canSlice())
			return true;

		bool modelDirty = false;
		for (auto m : m_slicedModels)
		{
			if (m->isDirty())
			 	modelDirty = true;
		}

		if (modelns().size() > 0 && modelDirty)
			return true;

		return false;
	}

	void AnsycWorker::prepareSliceData()
	{
		/* init model list */
		m_slicedModels.clear();
		m_sliceModelGroups.clear();
		for (ModelGroup* group : m_printer->modelGroupsInside())
		{
			// if (group->isVisible())
				m_sliceModelGroups << group;

			for (ModelN* model : group->models())
			{
				// if (model->isVisible())
				m_slicedModels << model;
				if (m_forceRegenerate)
					model->needRegenerate();
			}
		}

		/* init picture */
		QImage image = m_printer->picture();
		auto parameterManager = getKernel()->parameterManager();
		QString thumbnails = parameterManager->currentMachine()->settingsValue("thumbnails");
		//QString thumbnails = "96x96,300x300";
		QList<QPair<int, int>> sizes;
		QStringList sizeStrs = thumbnails.split(",");
		for (QString str : sizeStrs)
		{
			QStringList splits = str.split("x");
			sizes << QPair<int, int>(splits[0].toInt(), splits[1].toInt());
		}

		m_thumbnailDatas.clear();
		for (auto size : sizes)
		{
		    QImage tempImg = image.scaled(QSize(size.first, size.second), Qt::KeepAspectRatio, Qt::SmoothTransformation);

			auto ptr = tempImg.bits();
			crslice2::ThumbnailData data;
			int start = cxsw::getLineStart(&tempImg);
			int end = cxsw::getLineEnd(&tempImg);
			data.pos_s = start;
			data.pos_e = end;
			
			for (int y = size.second - 1; y >= 0; --y)
			{
				int base = y * size.first;
				for (int x = 0; x < size.first; ++x)
				{
					int index = (base + x) * 4;
					data.pixels.push_back(ptr[index + 2]);
					data.pixels.push_back(ptr[index + 1]);
					data.pixels.push_back(ptr[index + 0]);
					data.pixels.push_back(ptr[index + 3]);
				}
			}

			data.width = size.first;
			data.height = size.second;
			m_thumbnailDatas.push_back(data);
		}

        m_printer->applySettings();
		m_plateInfo = m_printer->getPlateInfo();
	}

	void AnsycWorker::work(qtuser_core::Progressor* progressor)
	{
		EngineType engine_type = getEngineType();

		m_slicing = true;
		if (!needReSlice())
			return;

		emit prepareSlice(m_printer);

		prepareSliceData();

		m_sliceAttain = NULL;
		SliceInput input;
		if (m_slicedModels.size() == 0)
		{
			progressor->message("nothing to slice.");
		}
		else
		{
			produceSliceInput(input, m_printer, engine_type);
		}

		QSharedPointer<qtuser_core::ProgressorTracer> tracer(new FormatSlice(progressor));
		if (engine_type == EngineType::ET_ORCA)
		{
			tracer.reset(new Crslice2Tracer(progressor));
		}
		tracer->resetProgressScope(0.0f, 0.85f);
		SliceResultPointer result;
		if (input.canSlice())
		{
			result.reset(new gcode::SliceResult());
			result->inputBox = input.sceneBox();
			m_sliceAttain = new SliceAttain(result, m_printer->generateSceneName(), SliceAttainType::sat_file);

			applyAdditionalSliceParameter(input.G, input.Es);
#if USE_CACHE_SLICE_INTERFACE
			if (m_printer)
			{
				OrcaCacheSlice slicer;
				slicer.cacheSlice(m_printer, input.G, input.Es, m_thumbnailDatas, m_plateInfo, tracer.data(), m_sliceAttain);
			}
#else
			input.pictures = m_thumbnailDatas;
			input.plate = m_plateInfo;
			if (engine_type == EngineType::ET_ORCA)
			{
				QScopedPointer<AnsycSlicer> slicer(new DLLAnsycSlicerOrca());
				slicer->doSlice(input, *tracer, m_sliceAttain);
			}
			else
			{
				QScopedPointer<AnsycSlicer> slicer(new DLLAnsycSlicer52());
				slicer->doSlice(input, *tracer, m_sliceAttain);
			}
#endif
		}
		else
		{
			tracer->failed("can not slice@");
		}

		if (tracer->error())
		{
			m_failExceptionDesc = tracer->getFailReason();
			return;
		}

		if (result)
		{
			tracer->resetProgressScope(0.85f, 1.0f);

			m_sliceAttain->build_paraseGcode(tracer.get());

			tracer->resetProgressScope(1.0f, 1.0f);

			qDebug() << QString("Slice : SliceAttain build over . [%1]").arg(currentProcessMemory());
		}

		if (tracer->interrupt())
		{
			m_failExceptionDesc = "User Cancelled@";
		}
	}

	void dealDataOverrides(QStringList& keys)
	{
		QString keys1 = "filament_deretraction_speed,filament_retract_before_wipe,filament_retract_lift_above,filament_retract_lift_below,filament_retract_lift_enforce,filament_retract_restart_extra,filament_retract_when_changing_layer,filament_retraction_length,filament_retraction_minimum_travel,filament_retraction_speed,filament_wipe,filament_wipe_distance,filament_z_hop,filament_z_hop_types";
		QString  _keys = keys1;
		keys = _keys.split(",");
	}

	void dealData(QStringList& keys)
	{
		QString keys1 = "material_flow_dependent_temperature,material_flow_temp_graph,cool_special_cds_fan_speed,cool_cds_fan_start_at_height,additional_cooling_fan_speed,bed_temperature_difference,close_fan_the_first_x_layers,cool_plate_temp,cool_plate_temp_initial_layer,enable_overhang_bridge_fan,enable_pressure_advance,eng_plate_temp,eng_plate_temp_initial_layer,fan_cooling_layer_time,fan_max_speed,fan_min_speed,filament_ramming_parameters";
		QString keys2 = "filament_start_gcode,filament_end_gcode,filament_colour,filament_cost,filament_density,filament_diameter,filament_flow_ratio,filament_ids,filament_is_support,filament_max_volumetric_speed,filament_minimal_purge_on_wipe_tower,filament_settings_id,filament_shrink,filament_soluble,filament_type,filament_vendor,epoxy_resin_plate_temp,epoxy_resin_plate_temp_initial_layer";
		QString keys3 = "full_fan_speed_layer,hot_plate_temp,hot_plate_temp_initial_layer,nozzle_temperature,nozzle_temperature_initial_layer,nozzle_temperature_range_high,nozzle_temperature_range_low,overhang_fan_speed,overhang_fan_threshold,pressure_advance,reduce_fan_stop_start_freq,required_nozzle_HRC,slow_down_for_layer_cooling,slow_down_layer_time,slow_down_min_speed,support_material_interface_fan_speed,temperature_vitrification,textured_plate_temp,textured_plate_temp_initial_layer,extruder_offset";

		//add V1.8.1
		QString keys4 = "complete_print_exhaust_fan_speed,activate_air_filtration,activate_chamber_temp_control,chamber_temperature,during_print_exhaust_fan_speed,filament_cooling_final_speed,filament_cooling_initial_speed,filament_cooling_moves,filament_load_time,filament_loading_speed,filament_loading_speed_start,filament_multitool_ramming,filament_multitool_ramming_flow,filament_multitool_ramming_volume,filament_toolchange_delay,filament_unload_time,filament_unloading_speed,filament_unloading_speed_start,enable_special_area_additional_cooling_fan";

		QString  _keys = keys1 + "," + keys2 + "," + keys3 + "," + keys4;
		keys = _keys.split(",");
	}

	void flush_volumes(SettingsPointer& G, int colorSize)
	{
		QString flush_volumes_matrix = G->value("flush_volumes_matrix", "280");
		QStringList _flush_volumes_matrixs = flush_volumes_matrix.split(",");
		if (_flush_volumes_matrixs.size() != colorSize * colorSize)
		{
			QString _flush_volumes_matrix = "";
			if (colorSize) {
				for (int i = 0; i < colorSize; i++) {
					for (int j = 0; j < colorSize; j++) {
						if (i == j)
						{
							if (i == colorSize - 1)
								_flush_volumes_matrix += "0";
							else
								_flush_volumes_matrix += "0,";
						}
						else
							_flush_volumes_matrix += "280,";
					}
				}
				G->add("flush_volumes_matrix", _flush_volumes_matrix, true);
				G->add("flush_volumes_multiplier", "0.5", true);
			}
		}
	}

	void _dealExtruderOverrides(SettingsPointer& G, const QList<SettingsPointer>& Es)
	{
		QStringList keys;
		dealDataOverrides(keys);
		std::vector<QString> values(keys.size(), "");
		int size = Es.size();
		for (int i = 0; i < keys.size(); i++)
		{
			for (int j = 0; j < size; j++)
			{
				auto& setting = Es.at(j);
				us::USetting* _setting = setting->findSetting(keys[i]);
				if (_setting != nullptr)
				{
					values[i] += _setting->str();
				}
				else
					values[i] += "nil";

				if (j != size - 1)
				{
					values[i] += ",";
				}
			}
		}

		for (int i = 0; i < keys.size(); i++)
		{
			G->add(keys[i], values[i], true);
		}
	}

	void _dealExtruder(SettingsPointer& G, const QList<SettingsPointer>& Es)
	{
		QStringList keys;
		dealData(keys);
		flush_volumes(G, Es.size());
		bool first = false;
		std::vector<QString> values(keys.size(), "");
		for (auto& setting : Es)
		{
			for (int i = 0; i < keys.size(); i++)
			{
				if (first)
				{
					if (keys[i] == "filament_colour"
						|| keys[i] == "filament_ids"
						|| keys[i] == "filament_settings_id"
						|| keys[i] == "filament_type"
						|| keys[i] == "filament_vendor"
						|| keys[i] == "filament_end_gcode"
						|| keys[i] == "filament_start_gcode"
						|| keys[i] == "filament_ramming_parameters")
					{
						values[i] += ";";
					}
					else
						values[i] += ",";

				}
				QString v = setting->value(keys[i], "");

				if (keys[i] == "filament_end_gcode"
					|| keys[i] == "filament_start_gcode")
					v = QString("\"%1\"").arg(v);
				values[i] += v;
			}

			first = true;
		}

		for (int i = 0; i < keys.size(); i++)
		{
			G->add(keys[i], values[i], true);
		}

		//material_flow_dependent_temperature,material_flow_temp_graph,取实际用到的耗材的自动温度
		if (Es.size())
		{
			G->add("material_flow_dependent_temperature", Es[0]->value("material_flow_dependent_temperature", ""), true);
			G->add("material_flow_temp_graph", Es[0]->value("material_flow_temp_graph", ""), true);

			QString default_filament_colour = G->value("default_filament_colour", "");
			QStringList values = default_filament_colour.split(";");
			for (int i = 0; i < values.size(); i++)
			{
				if (!values[i].isEmpty() && Es.size() > i)
				{
					G->add("material_flow_dependent_temperature", Es[i]->value("material_flow_dependent_temperature", ""), true);
					G->add("material_flow_temp_graph", Es[i]->value("material_flow_temp_graph", ""), true);
					break;
				}
			}
		}
	}
}
