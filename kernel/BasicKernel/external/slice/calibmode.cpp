#include "calibmode.h"
#include "interface/spaceinterface.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"
#include "interface/machineinterface.h"
#include "interface/appsettinginterface.h"
#include "kernel/kernel.h"
#include "cxgcode/gcodehelper.h"
#include "cxkernel/data/trimeshutils.h"
#include "cxkernel/utils/utils.h"
#include "qtusercore/module/systemutil.h"
#include "msbase/mesh/tinymodify.h"
#include "msbase/utils/cut.h"
#include "crslice2/repair.h"

#include "internal/project_3mf/load3mf.h"
#include "cadcore/step/STEP.hpp"
#include "qtuser3d/trimesh2/conv.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QUuid>

#ifdef _DEBUG
#include "buildinfo.h"
#endif
static constexpr double EPSILON = 1e-4;
static constexpr double PI = 3.141592653589793238;

QString getCalibResourcePath()
{
#ifdef _DEBUG
	QString calibPath = SOURCE_ROOT + QString("/resources/mesh/calib");
#elif defined (__APPLE__)
	int index = QCoreApplication::applicationDirPath().lastIndexOf("/");
	QString calibPath = QCoreApplication::applicationDirPath().left(index) + QString("/Resources/resources/mesh/calib");
#else
	QString calibPath = QCoreApplication::applicationDirPath() + QString("/resources/mesh/calib");
#endif
	return calibPath;
}

QString generateTempGCodeFileName()
{
	QString path = QString("%1/%2").arg(TEMPGCODE_PATH).arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
	mkMutiDir(path);

	return path;
}


void dealData(QStringList& keys)
{
	QString keys1 = "cool_special_cds_fan_speed,cool_cds_fan_start_at_height,additional_cooling_fan_speed,bed_temperature_difference,close_fan_the_first_x_layers,cool_plate_temp,cool_plate_temp_initial_layer,default_filament_colour,default_filament_type,enable_overhang_bridge_fan,enable_pressure_advance,eng_plate_temp,eng_plate_temp_initial_layer,fan_cooling_layer_time,fan_max_speed,fan_min_speed";
	QString keys2 = "filament_start_gcode,filament_end_gcode,filament_colour,filament_cost,filament_density,filament_deretraction_speed,filament_diameter,filament_flow_ratio,filament_ids,filament_is_support,filament_max_volumetric_speed,filament_minimal_purge_on_wipe_tower,filament_retract_before_wipe,filament_retract_restart_extra,filament_retract_when_changing_layer,filament_retraction_length,filament_retraction_minimum_travel,filament_retraction_speed,filament_settings_id,filament_shrink,filament_soluble,filament_type,filament_vendor,filament_wipe,filament_wipe_distance,filament_z_hop,filament_z_hop_types";
	QString keys3 = "full_fan_speed_layer,hot_plate_temp,hot_plate_temp_initial_layer,nozzle_temperature,nozzle_temperature_initial_layer,nozzle_temperature_range_high,nozzle_temperature_range_low,overhang_fan_speed,overhang_fan_threshold,pressure_advance,reduce_fan_stop_start_freq,required_nozzle_HRC,slow_down_for_layer_cooling,slow_down_layer_time,slow_down_min_speed,support_material_interface_fan_speed,temperature_vitrification,textured_plate_temp,textured_plate_temp_initial_layer,extruder_offset,epoxy_resin_plate_temp,epoxy_resin_plate_temp_initial_layer";

	//add V1.8.1
	QString keys4 = "complete_print_exhaust_fan_speed,activate_air_filtration,activate_chamber_temp_control,chamber_temperature,during_print_exhaust_fan_speed,filament_cooling_final_speed,filament_cooling_initial_speed,filament_cooling_moves,filament_load_time,filament_loading_speed,filament_loading_speed_start,filament_multitool_ramming,filament_multitool_ramming_flow,filament_multitool_ramming_volume,filament_toolchange_delay,filament_unload_time,filament_unloading_speed,filament_unloading_speed_start,filament_retract_lift_above,filament_retract_lift_below,filament_retract_lift_enforce";

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

void _dealExtruder(SettingsPointer& G, QList<SettingsPointer>& Es)
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
					|| keys[i] == "filament_start_gcode")
				{
					values[i] += ";";
				}
				else
					values[i] += ",";

			}
			values[i] += setting->value(keys[i], "");
		}

		first = true;
	}

	for (int i = 0; i < keys.size(); i++)
	{
		G->add(keys[i], values[i], true);
	}
}

double  mm3_per_mm(float m_width, float m_height)
{
	return float(m_height * (m_width - m_height * (1. - 0.25 * PI)));
}


//SettingsPtr convert(const cxgcode::USettings& settings)
//{
//	SettingsPtr ptr(new crslice::Settings());
//	const QHash<QString, cxgcode::USetting*>& ss = settings.settings();
//	for (QHash<QString, cxgcode::USetting*>::ConstIterator it = ss.begin();
//		it != ss.end(); ++it)
//	{
//		ptr->add(it.key().toStdString(), it.value()->str().toStdString());
//	}
//	return ptr;
//}


namespace creative_kernel
{
	qtuser_3d::Box3D baseBoundingBox()
	{
		return qtuser_3d::triBox2Box3D(getBaseBoundingBox());
	}

	CalibMode::CalibMode()
	{
		m_accSpeed = 5000;
	}

	CalibMode::~CalibMode()
	{

	}

	crslice2::CrScene* CalibMode::createScene()
	{
		//Extruder Settings
		crslice2::CrScene* scene = new crslice2::CrScene();
		scene->m_extruders.emplace_back(new crslice2::Settings());
		crslice2::SettingsPtr& extruder = scene->m_extruders[0];
		QList<SettingsPointer> ESettingsList = creative_kernel::createCurrentExtruderSettings();
		const QHash<QString, us::USetting*>& E = ESettingsList.at(0)->settings();
		for (QHash<QString, us::USetting*>::const_iterator it = E.constBegin(); it != E.constEnd(); ++it)
		{
			extruder->add(it.key().toStdString(), it.value()->str().toStdString());
		}
		//scene->machine_center_is_zero = G.value("machine_center_is_zero")->enabled();

		SettingsPointer GSettings = creative_kernel::createCurrentGlobalSettings();
		_dealExtruder(GSettings, ESettingsList);

		const QHash<QString, us::USetting*>& G = GSettings->settings();
		for (QHash<QString, us::USetting*>::const_iterator it = G.constBegin(); it != G.constEnd(); ++it)
		{
			scene->m_settings->add(it.key().toStdString(), it.value()->str().toStdString());
		}

		return scene;
	}

	void CalibMode::loadImage(crslice2::CrScene* scene)
	{
		QImage calibrate_image(m_calibPNGName);
		//add thumbnails
		auto parameterManager = creative_kernel::getKernel()->parameterManager();
		QString thumbnails = parameterManager->currentMachine()->settingsValue("thumbnails");
		QList<QPair<int, int>> sizes;
		QStringList sizeStrs = thumbnails.split(",");
		for (QString str : sizeStrs)
		{
			QStringList splits = str.split("x");
			sizes << QPair<int, int>(splits[0].toInt(), splits[1].toInt());
		}

		scene->thumbnailDatas.clear();
		for (auto size : sizes)
		{
			if (calibrate_image.size().isEmpty())
				continue;

			QString dest_file = QString("%1/imgPreview.png").arg(SLICE_PATH);
			calibrate_image.save(dest_file);

			QImage tempImg = calibrate_image.scaled(QSize(size.first, size.second), Qt::KeepAspectRatio, Qt::SmoothTransformation);

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
			scene->thumbnailDatas.push_back(data);
		}
	}

	void CalibMode::setConfig(crslice2::CrScene* scene)
	{

	}

	void CalibMode::setType(CalibrateType type)
	{
		m_type = type;
	}

	void CalibMode::setValue(float _start, float _end, float _step, float bedTemp)
	{
		m_start = _start;
		m_end = _end;
		m_step = _step;
		m_bedTemp = bedTemp;
	}

	void CalibMode::setHighValue(float _highStep)
	{
		m_highStep = _highStep;
	}

	void CalibMode::setAccSpeed(float _accSpeed)
	{
		m_accSpeed = _accSpeed;
	}

	void CalibMode::setSpeed(float _speed)
	{
		m_speed = _speed;
	}


	void CalibMode::setHighFlow(float _highFlow)
	{
		m_highflow = _highFlow;
	}

	crslice2::CrScene* CalibMode::generateScene()
	{
		crslice2::CrScene* scene = createScene();
		setConfig(scene);
		loadImage(scene);
		return scene;
	}


	Temp::Temp()
	{

	}

	Temp::~Temp()
	{

	}

	void Temp::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/temperature_tower/temperature_tower.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());

		meshptr->need_bbox();
		msbase::moveTrimesh2Center(meshptr.get());

		// cut upper
		auto block_count = lround((350 - m_end) / 5 + 1);
		if (block_count > 0) {
			// add EPSILON offset to avoid cutting at the exact location where the flat surface is
			auto new_height = block_count * 10.0 + EPSILON;
			if (new_height < meshptr->bbox.max.z)
			{
				std::vector<trimesh::TriMesh*> meshes;
				if (crslice2::cut_horizontal(meshptr.get(), new_height, meshes, 1))
				{
					meshptr.reset(meshes[0]);
				}
			}
		}
		// cut bottom
		msbase::moveTrimesh2Center(meshptr.get());
		block_count = lround((350 - m_start) / 5);
		if (block_count > 0) {
			auto new_height = block_count * 10.0 + EPSILON;
			meshptr->need_bbox();
			if (new_height < meshptr->bbox.max.z)
			{
				std::vector<trimesh::TriMesh*> meshes;
				if (crslice2::cut_horizontal(meshptr.get(), new_height, meshes,0))
				{
					meshptr.reset(meshes[0]);
				}
			}
		}
		msbase::moveTrimesh2Center(meshptr.get());
		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);

		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, TriMeshPtr(meshptr));
		scene->m_settings->add("nozzle_temperature_initial_layer", std::to_string(m_start));
		scene->m_settings->add("nozzle_temperature", std::to_string(m_start));
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		crslice2::SettingsPtr groupSettings(new crslice2::Settings());
		groupSettings->add("brim_type", "outer_only");
		groupSettings->add("brim_width", std::to_string(5.0));
		groupSettings->add("brim_object_gap", std::to_string(0.0));
		scene->setGroupSettings(groupID, groupSettings);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_Temp_Tower;

		scene->m_gcodeFileName = QString("%1/temperature.gcode").arg(generateTempGCodeFileName()).toStdString();
		int num = (m_start - m_end) / 5 + 1;
		num > 20 ? num = 25 : num;
		m_calibPNGName = getCalibResourcePath() + QString("/temperature_tower/%1.png").arg(num);

	}


	VFA::VFA()
	{

	}

	VFA::~VFA()
	{

	}

	void VFA::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/vfa/VFA.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());
		msbase::moveTrimesh2Center(meshptr.get());
		//cut upper
		meshptr->need_bbox();
		auto obj_bb = meshptr->bbox;
		auto height = 5 * ((m_end - m_start) / m_step + 1);
		if (height < obj_bb.size().z)
		{
			std::vector<trimesh::TriMesh*> meshes;
			if (crslice2::cut_horizontal(meshptr.get(), height, meshes, 1))
			{
				meshptr.reset(meshes[0]);
			}
		}
		msbase::moveTrimesh2Center(meshptr.get());
		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);

		scene->m_settings->add("slow_down_layer_time", "0");
		scene->m_settings->add("filament_max_volumetric_speed", "200");
		scene->m_settings->add("enable_overhang_speed", "0"); //false
		scene->m_settings->add("timelapse_type", "0");//0 == Traditional == tlTraditional
		scene->m_settings->add("wall_loops", "1");
		scene->m_settings->add("top_shell_layers", "0");
		scene->m_settings->add("bottom_shell_layers", "1");
		scene->m_settings->add("sparse_infill_density", "0");
		scene->m_settings->add("spiral_mode", "1");//true
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, meshptr);

		crslice2::SettingsPtr groupSettings(new crslice2::Settings());
		groupSettings->add("brim_type", "outer_only");
		groupSettings->add("brim_width", std::to_string(3.0));
		groupSettings->add("brim_object_gap", std::to_string(0.0));
		scene->setGroupSettings(groupID, groupSettings);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_VFA_Tower;

		scene->m_gcodeFileName = QString("%1/VFA.gcode").arg(generateTempGCodeFileName()).toStdString();
		int num = (m_end - m_start) / m_step / 5;
		num > 9 ? num = 9 : num;
		m_calibPNGName = getCalibResourcePath() + QString("/vfa/%1.png").arg(num);
	}

	LowFlow::LowFlow()
	{

	}

	LowFlow::~LowFlow()
	{

	}

	void LowFlow::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = QString("%1/filament_flow/flowrate-test-pass1.3mf").arg(getCalibResourcePath());
		std::vector<creative_kernel::MeshWithName> meshWithNames = creative_kernel::load_meshes_from_3mf(calibPathName);

		/// --- scale ---
		// model is created for a 0.4 nozzle, scale z with nozzle size.
		float nozzle_diameter = std::atof(scene->m_settings->getString("nozzle_diameter").c_str());
		float xyScale = nozzle_diameter / 0.6;
		//scale z to have 7 layers
		double first_layer_height = std::atof(scene->m_settings->getString("initial_layer_print_height").c_str());
		double layer_height = nozzle_diameter / 2.0; // prefer 0.2 layer height for 0.4 nozzle
		first_layer_height = std::max(first_layer_height, layer_height);
		float zscale = (first_layer_height + 6 * layer_height) / 1.4;

		// only enlarge
		if (xyScale > 1.2)
		{
			for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
			{
				trimesh::xform xf = trimesh::xform::scale(xyScale, xyScale, zscale);
				trimesh::apply_xform(ameshWithName.mesh.get(), xf);
			}
		}
		else
		{
			for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
			{
				trimesh::xform xf = trimesh::xform::scale(1.0, 1.0, zscale);
				trimesh::apply_xform(ameshWithName.mesh.get(), xf);
			}
		}
		trimesh::TriMesh::BBox allMeshBox;
		for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
		{
			ameshWithName.mesh->need_bbox();
			allMeshBox += ameshWithName.mesh->bbox;
		}
		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x() - allMeshBox.center().x, abox3.center().y() - allMeshBox.center().y, 0.0);
		for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
		{
			trimesh::trans(ameshWithName.mesh.get(), offset);
		}

		double filament_max_volumetric_speed = std::atof(scene->m_extruders[0]->getString("filament_max_volumetric_speed").c_str());
		double max_infill_speed = filament_max_volumetric_speed / (mm3_per_mm(nozzle_diameter * 1.2f, layer_height) * (m_type == CalibrateType::ct_lowflow ? 1.2 : 1));
		double internal_solid_speed = std::floor(std::min(std::atof(scene->m_settings->getString("internal_solid_infill_speed").c_str()), max_infill_speed));
		double top_surface_speed = std::floor(std::min(std::atof(scene->m_settings->getString("top_surface_speed").c_str()), max_infill_speed));


		// adjust parameters
		for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
		{
			int groupID = scene->addOneGroup();
			int objectID = scene->addObject2Group(groupID);
			scene->setOjbectMesh(groupID, objectID, TriMeshPtr(ameshWithName.mesh));
			crslice2::SettingsPtr groupSettings(new crslice2::Settings());
			groupSettings->add("wall_loops", std::to_string(3));
			groupSettings->add("only_one_wall_top", "1");//true
			groupSettings->add("sparse_infill_density", std::to_string(35));
			groupSettings->add("bottom_shell_layers", std::to_string(1));
			groupSettings->add("top_shell_layers", std::to_string(5));
			groupSettings->add("detect_thin_wall", "1");//true
			groupSettings->add("filter_out_gap_fill", std::to_string(0));
			groupSettings->add("sparse_infill_pattern", "zig-zag");
			groupSettings->add("top_surface_line_width", std::to_string(nozzle_diameter * 1.2));
			groupSettings->add("internal_solid_infill_line_width", std::to_string(nozzle_diameter * 1.2));
			groupSettings->add("top_surface_pattern", "monotonic");
			groupSettings->add("top_solid_infill_flow_ratio", std::to_string(1.0));
			groupSettings->add("infill_direction", std::to_string(45));
			groupSettings->add("ironing_type", "no ironing");
			groupSettings->add("internal_solid_infill_speed", std::to_string(internal_solid_speed));
			groupSettings->add("top_surface_speed", std::to_string(top_surface_speed));

			// extract flowrate from name, filename format: flowrate_xxx
			std::string obj_name = ameshWithName.name;
			assert(obj_name.length() > 9);
			obj_name = obj_name.substr(9);
			if (obj_name[0] == 'm')
				obj_name[0] = '-';
			auto modifier = stof(obj_name);
			groupSettings->add("print_flow_ratio", std::to_string(1.0f + modifier / 100.f));
			scene->setGroupSettings(groupID, groupSettings);
		}
		scene->m_settings->add("layer_height", std::to_string(layer_height));
		scene->m_settings->add("initial_layer_print_height", std::to_string(first_layer_height));
		scene->m_settings->add("reduce_crossing_wall", "1");

		scene->m_calibParams.start = 0.0;
		scene->m_calibParams.step = 0.0;
		scene->m_calibParams.end = 0.0;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_None;

		scene->m_gcodeFileName = QString("%1/pass 1.gcode").arg(generateTempGCodeFileName()).toStdString();
		m_calibPNGName = getCalibResourcePath() + "/filament_flow/flowrate-test-pass1.png";
	}

	HighFlow::HighFlow()
	{

	}

	HighFlow::~HighFlow()
	{

	}

	void HighFlow::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = QString("%1/filament_flow/flowrate-test-pass2.3mf").arg(getCalibResourcePath());
		std::vector<creative_kernel::MeshWithName> meshWithNames = creative_kernel::load_meshes_from_3mf(calibPathName);

		/// --- scale ---
		// model is created for a 0.4 nozzle, scale z with nozzle size.
		float nozzle_diameter = std::atof(scene->m_settings->getString("nozzle_diameter").c_str());
		float xyScale = nozzle_diameter / 0.6;
		//scale z to have 7 layers
		double first_layer_height = std::atof(scene->m_settings->getString("initial_layer_print_height").c_str());
		double layer_height = nozzle_diameter / 2.0; // prefer 0.2 layer height for 0.4 nozzle
		first_layer_height = std::max(first_layer_height, layer_height);
		float zscale = (first_layer_height + 6 * layer_height) / 1.4;

		// only enlarge
		if (xyScale > 1.2)
		{
			for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
			{
				trimesh::xform xf = trimesh::xform::scale(xyScale, xyScale, zscale);
				trimesh::apply_xform(ameshWithName.mesh.get(), xf);
			}
		}
		else
		{
			for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
			{
				trimesh::xform xf = trimesh::xform::scale(1.0, 1.0, zscale);
				trimesh::apply_xform(ameshWithName.mesh.get(), xf);
			}
		}
		trimesh::TriMesh::BBox allMeshBox;
		for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
		{
			ameshWithName.mesh->need_bbox();
			allMeshBox += ameshWithName.mesh->bbox;
		}
		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x() - allMeshBox.center().x, abox3.center().y() - allMeshBox.center().y, 0.0);
		for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
		{
			trimesh::trans(ameshWithName.mesh.get(), offset);
		}

		double filament_max_volumetric_speed = std::atof(scene->m_extruders[0]->getString("filament_max_volumetric_speed").c_str());
		double max_infill_speed = filament_max_volumetric_speed / (mm3_per_mm(nozzle_diameter * 1.2f, layer_height) * (m_type == CalibrateType::ct_lowflow ? 1.2 : 1));
		double internal_solid_speed = std::floor(std::min(std::atof(scene->m_settings->getString("internal_solid_infill_speed").c_str()), max_infill_speed));
		double top_surface_speed = std::floor(std::min(std::atof(scene->m_settings->getString("top_surface_speed").c_str()), max_infill_speed));


		// adjust parameters
		for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
		{
			int groupID = scene->addOneGroup();
			int objectID = scene->addObject2Group(groupID);
			scene->setOjbectMesh(groupID, objectID, TriMeshPtr(ameshWithName.mesh));
			crslice2::SettingsPtr groupSettings(new crslice2::Settings());
			groupSettings->add("wall_loops", std::to_string(3));
			groupSettings->add("only_one_wall_top", "1");//true
			groupSettings->add("sparse_infill_density", std::to_string(35));
			groupSettings->add("bottom_shell_layers", std::to_string(1));
			groupSettings->add("top_shell_layers", std::to_string(5));
			groupSettings->add("detect_thin_wall", "1");//true
			groupSettings->add("filter_out_gap_fill", std::to_string(0));
			groupSettings->add("sparse_infill_pattern", "zig-zag");
			groupSettings->add("top_surface_line_width", std::to_string(nozzle_diameter * 1.2));
			groupSettings->add("internal_solid_infill_line_width", std::to_string(nozzle_diameter * 1.2));
			groupSettings->add("top_surface_pattern", "monotonic");
			groupSettings->add("top_solid_infill_flow_ratio", std::to_string(1.0));
			groupSettings->add("infill_direction", std::to_string(45));
			groupSettings->add("ironing_type", "no ironing");
			groupSettings->add("internal_solid_infill_speed", std::to_string(internal_solid_speed));
			groupSettings->add("top_surface_speed", std::to_string(top_surface_speed));

			// extract flowrate from name, filename format: flowrate_xxx
			std::string obj_name = ameshWithName.name;
			assert(obj_name.length() > 9);
			obj_name = obj_name.substr(9);
			if (obj_name[0] == 'm')
				obj_name[0] = '-';
			auto modifier = stof(obj_name);
			groupSettings->add("print_flow_ratio", std::to_string((1 + m_highflow * 0.01)*(1+modifier * 0.01)));
			scene->setGroupSettings(groupID, groupSettings);
		}
		scene->m_settings->add("layer_height", std::to_string(layer_height));
		scene->m_settings->add("initial_layer_print_height", std::to_string(first_layer_height));
		scene->m_settings->add("reduce_crossing_wall", "1");

		scene->m_calibParams.start = 0.0;
		scene->m_calibParams.step = 0.0;
		scene->m_calibParams.end = 0.0;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_None;
		scene->m_gcodeFileName = QString("%1/pass 2.gcode").arg(generateTempGCodeFileName()).toStdString();
		m_calibPNGName = getCalibResourcePath() + "/filament_flow/flowrate-test-pass2.png";
	}

	MaxFlowValume::MaxFlowValume()
	{

	}

	MaxFlowValume::~MaxFlowValume()
	{

	}

	void MaxFlowValume::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/volumetric_speed/SpeedTestStructure.step";
		trimesh::TriMesh* amesh = cadcore::load_step(cxkernel::qString2String(calibPathName), nullptr);
		msbase::moveTrimesh2Center(amesh);

		double nozzle_diameter = std::atof(scene->m_settings->getString("nozzle_diameter").c_str());
		double line_width = nozzle_diameter * 1.75;
		double layer_height = nozzle_diameter * 0.8;
		auto max_lh = std::atof(scene->m_settings->getString("max_layer_height").c_str());
		if (max_lh < layer_height)
		{
			scene->m_settings->add("max_layer_height", std::to_string(layer_height));
		}
		scene->m_settings->add("filament_max_volumetric_speed", std::to_string(200));
		scene->m_settings->add("slow_down_layer_time", std::to_string(0.0));

		scene->m_settings->add("enable_overhang_speed", "0");//false
		scene->m_settings->add("timelapse_type", "0");//0 == Traditional == tlTraditional
		scene->m_settings->add("wall_loops", std::to_string(0));
		scene->m_settings->add("top_shell_layers", std::to_string(0));
		scene->m_settings->add("bottom_shell_layers", std::to_string(1));
		scene->m_settings->add("sparse_infill_density", std::to_string(0));
		scene->m_settings->add("spiral_mode", "1");//true
		scene->m_settings->add("outer_wall_line_width", std::to_string(line_width));
		scene->m_settings->add("initial_layer_print_height", std::to_string(layer_height));
		scene->m_settings->add("layer_height", std::to_string(layer_height));
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		//  cut upper
		auto obj_bb = creative_kernel::baseBoundingBox();
		auto height = (m_end - m_start + 1) / m_step;
		height > obj_bb.size().z() ? height = obj_bb.size().z() : height;
		std::vector<trimesh::TriMesh*> meshes;
		if (crslice2::cut_horizontal(amesh, height, meshes, 1))
		{
			amesh=meshes[0];
			msbase::moveTrimesh2Center(amesh);
		}
		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(amesh, offset);
		//auto new_params = params;
		auto _mm3_per_mm = mm3_per_mm(line_width, layer_height) * std::atof(scene->m_extruders[0]->getString("filament_flow_ratio").c_str());

		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, TriMeshPtr(amesh));

		crslice2::SettingsPtr groupSettings(new crslice2::Settings());
		groupSettings->add("brim_type", "outer_and_inner");
		groupSettings->add("brim_width", std::to_string(3.0));
		groupSettings->add("brim_object_gap", std::to_string(0.0));
		scene->setGroupSettings(groupID, groupSettings);

		scene->m_calibParams.start = m_start / _mm3_per_mm;
		scene->m_calibParams.step = m_step / _mm3_per_mm;
		scene->m_calibParams.end = m_end / _mm3_per_mm;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_Vol_speed_Tower;
		scene->m_gcodeFileName = QString("%1/maxflowrate.gcode").arg(generateTempGCodeFileName()).toStdString();
		int iRatio = 10;
		int num = (m_end - m_start + m_step) / iRatio;
		num = std::max(std::min(num, 10), 0);
		m_calibPNGName = getCalibResourcePath() + QString("/volumetric_speed/%1.png").arg(num);
	}

	RetractionTower::RetractionTower()
	{

	}

	RetractionTower::~RetractionTower()
	{

	}

	void RetractionTower::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/retraction/retraction_tower.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());

		msbase::moveTrimesh2Center(meshptr.get());
		double layer_height = 0.2;
		auto max_lh = std::atof(scene->m_settings->getString("max_layer_height").c_str());
		if (max_lh < layer_height)
		{
			scene->m_settings->add("max_layer_height", std::to_string(layer_height));
		}

		scene->m_settings->add("wall_loops", std::to_string(2));
		scene->m_settings->add("top_shell_layers", std::to_string(0));
		scene->m_settings->add("bottom_shell_layers", std::to_string(1));
		scene->m_settings->add("sparse_infill_density", std::to_string(0));
		scene->m_settings->add("initial_layer_print_height", std::to_string(layer_height));
		scene->m_settings->add("layer_height", std::to_string(layer_height));
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		//  cut upper
		auto obj_bb = creative_kernel::baseBoundingBox();;
		auto height = ((m_end - m_start) * 1 / m_step + 1.4);
		height > obj_bb.size().z() ? height = obj_bb.size().z() : height;
		std::vector<trimesh::TriMesh*> meshes;
		if (crslice2::cut_horizontal(meshptr.get(), height, meshes, 1))
		{
			meshptr.reset(meshes[0]);
			msbase::moveTrimesh2Center(meshptr.get());
		}

		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);
		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, meshptr);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_Retraction_tower;
		scene->m_gcodeFileName = QString("%1/retract.gcode").arg(generateTempGCodeFileName()).toStdString();

		float iRatio = 1;
		int num = (m_end - m_start + m_step) / iRatio +1;
		num = std::max(std::min(num, 9), 1);
		m_calibPNGName = getCalibResourcePath() + QString("/retraction/%1.png").arg(num);
	}

	PaLine::PaLine()
	{

	}

	PaLine::~PaLine()
	{

	}

	void PaLine::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/pressure_advance/pressure_advance_test.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());
		msbase::moveTrimesh2Center(meshptr.get());
		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);
		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, TriMeshPtr(meshptr));
		crslice2::SettingsPtr groupSettings(new crslice2::Settings());
		groupSettings->add("seam_position", "back");
		scene->setGroupSettings(groupID, groupSettings);
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.print_numbers = true;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_PA_Line;
		scene->m_gcodeFileName = QString("%1/pressureAdvance.gcode").arg(generateTempGCodeFileName()).toStdString();
		m_calibPNGName = getCalibResourcePath() + "/PressureAdvance/pressure_advance_test_line.png";

	}

	PaTower::PaTower()
	{

	}

	PaTower::~PaTower()
	{

	}

	void PaTower::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/PressureAdvance/tower_with_seam.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());
		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);

		//scene->m_extruders[0]->add("slow_down_layer_time", std::to_string(1.0));
		scene->m_settings->add("slow_down_layer_time", std::to_string(1.0));
		scene->m_settings->add("default_jerk", std::to_string(1.0));
		scene->m_settings->add("outer_wall_jerk", std::to_string(1.0));
		scene->m_settings->add("inner_wall_jerk", std::to_string(1.0));
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		if (scene->m_settings->getString("wall_generator") == "arachne")
		{
			scene->m_settings->add("wall_transition_angle", std::to_string(25));
		}
		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, TriMeshPtr(meshptr));
		crslice2::SettingsPtr groupSettings(new crslice2::Settings());
		groupSettings->add("seam_position", "back");
		scene->setGroupSettings(groupID, groupSettings);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_PA_Tower;
		scene->m_gcodeFileName = QString("%1/pressureAdvance.gcode").arg(generateTempGCodeFileName()).toStdString();
		m_calibPNGName = getCalibResourcePath() + "/PressureAdvance/tower_with_seam.png";
	}

	RetractionSpeed::RetractionSpeed()
	{

	}

	RetractionSpeed::~RetractionSpeed()
	{

	}

	void RetractionSpeed::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/retraction/retraction_tower.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());

		msbase::moveTrimesh2Center(meshptr.get());
		double layer_height = 0.2;
		auto max_lh = std::atof(scene->m_settings->getString("max_layer_height").c_str());
		if (max_lh < layer_height)
		{
			scene->m_settings->add("max_layer_height", std::to_string(layer_height));
		}

		scene->m_settings->add("wall_loops", std::to_string(2));
		scene->m_settings->add("top_shell_layers", std::to_string(0));
		scene->m_settings->add("bottom_shell_layers", std::to_string(1));
		scene->m_settings->add("sparse_infill_density", std::to_string(0));
		scene->m_settings->add("initial_layer_print_height", std::to_string(layer_height));
		scene->m_settings->add("layer_height", std::to_string(layer_height));
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		//  cut upper
		auto obj_bb = creative_kernel::baseBoundingBox();;
		auto height = 0.4 + (m_end - m_start) * 2.63 / m_step + 2.63;
		height > obj_bb.size().z() ? height = obj_bb.size().z() : height;
		std::vector<trimesh::TriMesh*> meshes;
		if (crslice2::cut_horizontal(meshptr.get(), height, meshes, 1))
		{
			meshptr.reset(meshes[0]);
			msbase::moveTrimesh2Center(meshptr.get());
		}

		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);
		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, meshptr);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_Retraction_tower_speed;
		scene->m_gcodeFileName = QString("%1/retractionSpeed.gcode").arg(generateTempGCodeFileName()).toStdString();

		int iRatio = 16.7;
		int num = (m_end - m_start + m_step) / iRatio;
		num = std::max(std::min(num, 9), 1);
		m_calibPNGName = getCalibResourcePath() + QString("/retraction/%1.png").arg(num);
	}

	LimitSpeed::LimitSpeed()
	{

	}

	LimitSpeed::~LimitSpeed()
	{

	}

	void LimitSpeed::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/limit_speed/limitSpeed.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());

		msbase::moveTrimesh2Center(meshptr.get());
		double layer_height = 0.2;
		auto max_lh = std::atof(scene->m_settings->getString("max_layer_height").c_str());
		if (max_lh < layer_height)
		{
			scene->m_settings->add("max_layer_height", std::to_string(layer_height));
		}
		scene->m_settings->add("filament_max_volumetric_speed", std::to_string(200));
		scene->m_settings->add("slow_down_layer_time", std::to_string(0.0));
		scene->m_settings->add("top_shell_layers", std::to_string(0));
		scene->m_settings->add("bottom_shell_layers", std::to_string(0));
		scene->m_settings->add("acceleration_limit_mess_enable", "0");
		scene->m_settings->add("speed_limit_to_height_enable", "0");
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		//modify acclSpeed
		scene->m_settings->add("default_acceleration", std::to_string(m_accSpeed));
		scene->m_settings->add("inner_wall_acceleration", std::to_string(m_accSpeed));
		scene->m_settings->add("outer_wall_acceleration", std::to_string(m_accSpeed));
		scene->m_settings->add("sparse_infill_acceleration", std::to_string(m_accSpeed));
		//scene->m_settings->add("top_surface_acceleration", std::to_string(m_accSpeed));
		
		
		//  cut upper
		auto obj_bb = creative_kernel::baseBoundingBox();;
		auto height = ((m_end - m_start) / m_step + 1)*5;
		height > obj_bb.size().z() ? height = obj_bb.size().z() : height;
		std::vector<trimesh::TriMesh*> meshes;
		if (crslice2::cut_horizontal(meshptr.get(), height, meshes, 1))
		{
			meshptr.reset(meshes[0]);
			msbase::moveTrimesh2Center(meshptr.get());
		}

		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);
		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, meshptr);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_Limit_Speed;
		scene->m_gcodeFileName = QString("%1/limitSpeed.gcode").arg(generateTempGCodeFileName()).toStdString();
		int iRatio = 75;
		int num = (m_end - m_start + m_step) / iRatio;
		num =std::max(std::min(num, 7),1);
		m_calibPNGName = getCalibResourcePath() + QString("/limit_speed/%1.png").arg(num);
	}

	LimitAcceleration::LimitAcceleration()
	{

	}

	LimitAcceleration::~LimitAcceleration()
	{

	}

	void LimitAcceleration::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/limit_speed/limitSpeed.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());

		msbase::moveTrimesh2Center(meshptr.get());
		double layer_height = 0.2;
		auto max_lh = std::atof(scene->m_settings->getString("max_layer_height").c_str());
		if (max_lh < layer_height)
		{
			scene->m_settings->add("max_layer_height", std::to_string(layer_height));
		}
		scene->m_settings->add("filament_max_volumetric_speed", std::to_string(200));
		scene->m_settings->add("slow_down_layer_time", std::to_string(0.0));
		scene->m_settings->add("top_shell_layers", std::to_string(0));
		scene->m_settings->add("bottom_shell_layers", std::to_string(0));
		scene->m_settings->add("acceleration_limit_mess_enable", "0");
		scene->m_settings->add("speed_limit_to_height_enable", "0");
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));
		scene->m_settings->add("inner_wall_speed", std::to_string(m_speed));
		scene->m_settings->add("outer_wall_speed", std::to_string(m_speed));
		scene->m_settings->add("sparse_infill_speed", std::to_string(m_speed));
		scene->m_settings->add("gap_infill_speed", std::to_string(m_speed));

		//  cut upper
		auto obj_bb = creative_kernel::baseBoundingBox();;
		auto height = ((m_end - m_start) / m_step + 1) * 5;
		height > obj_bb.size().z() ? height = obj_bb.size().z() : height;
		std::vector<trimesh::TriMesh*> meshes;
		if (crslice2::cut_horizontal(meshptr.get(), height, meshes, 1))
		{
			meshptr.reset(meshes[0]);
			msbase::moveTrimesh2Center(meshptr.get());
		}

		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);
		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, meshptr);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_Limit_Acceleration;
		scene->m_gcodeFileName = QString("%1/limitAcceleration.gcode").arg(generateTempGCodeFileName()).toStdString();

		int iRatio = 3687;
		int num = (m_end - m_start + m_step) / iRatio;
		num = std::max(std::min(num, 7), 1);
		m_calibPNGName = getCalibResourcePath() + QString("/limit_acc_speed/%1.png").arg(num);
	}

	SpeedTower::SpeedTower()
	{

	}

	SpeedTower::~SpeedTower()
	{

	}

	void SpeedTower::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/speed_tower/speedtower.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());

		msbase::moveTrimesh2Center(meshptr.get());
		double layer_height = 0.2;
		auto max_lh = std::atof(scene->m_settings->getString("max_layer_height").c_str());
		if (max_lh < layer_height)
		{
			scene->m_settings->add("max_layer_height", std::to_string(layer_height));
		}
		scene->m_settings->add("filament_max_volumetric_speed", std::to_string(200));
		scene->m_settings->add("slow_down_layer_time", std::to_string(0.0));
		scene->m_settings->add("top_shell_layers", std::to_string(0));
		scene->m_settings->add("bottom_shell_layers", std::to_string(0));
		scene->m_settings->add("acceleration_limit_mess_enable", "0");
		scene->m_settings->add("speed_limit_to_height_enable", "0");
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		//  cut upper
		auto obj_bb = creative_kernel::baseBoundingBox();;
		auto height = ((m_end - m_start) / m_step + 1) * 5;
		height > obj_bb.size().z() ? height = obj_bb.size().z() : height;
		std::vector<trimesh::TriMesh*> meshes;
		if (crslice2::cut_horizontal(meshptr.get(), height, meshes, 1))
		{
			meshptr.reset(meshes[0]);
			msbase::moveTrimesh2Center(meshptr.get());
		}

		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);
		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, meshptr);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_Speed_Tower;
		scene->m_gcodeFileName = QString("%1/speedTower.gcode").arg(generateTempGCodeFileName()).toStdString();
		int iRatio = 15;
		int num = (m_end - m_start + m_step) / iRatio;
		num = std::max(std::min(num, 16), 1);
		m_calibPNGName = getCalibResourcePath() + QString("/speed_tower/%1.png").arg(num);
	}

	AccelerationTower::AccelerationTower()
	{

	}

	AccelerationTower::~AccelerationTower()
	{

	}

	void AccelerationTower::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/acc_speed_tower/accSpeedTower.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());

		msbase::moveTrimesh2Center(meshptr.get());
		double layer_height = 0.2;
		auto max_lh = std::atof(scene->m_settings->getString("max_layer_height").c_str());
		if (max_lh < layer_height)
		{
			scene->m_settings->add("max_layer_height", std::to_string(layer_height));
		}
		scene->m_settings->add("filament_max_volumetric_speed", std::to_string(200));
		scene->m_settings->add("slow_down_layer_time", std::to_string(0.0));
		scene->m_settings->add("top_shell_layers", std::to_string(0));
		scene->m_settings->add("bottom_shell_layers", std::to_string(0));
		scene->m_settings->add("acceleration_limit_mess_enable", "0");
		scene->m_settings->add("speed_limit_to_height_enable", "0");
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		//  cut upper
		meshptr->need_bbox();
		auto obj_bb = meshptr->bbox;
		auto height = ((m_end - m_start) / m_step + 1) * 5;
		if (height < obj_bb.size().z)
		{
			std::vector<trimesh::TriMesh*> meshes;
			if (crslice2::cut_horizontal(meshptr.get(), height, meshes, 1))
			{
				meshptr.reset(meshes[0]);
				msbase::moveTrimesh2Center(meshptr.get());
			}
		}
		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);
		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, meshptr);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_Acceleration_Tower;
		scene->m_gcodeFileName = QString("%1/accelerationTower.gcode").arg(generateTempGCodeFileName()).toStdString();

		int iRatio = 600;
		int num = (m_end - m_start + m_step) / iRatio;
		num = std::max(std::min(num, 16), 1);
		m_calibPNGName = getCalibResourcePath() + QString("/acc_speed_tower/%1.png").arg(num);
	}

	Arc2lerance::Arc2lerance()
	{

	}

	Arc2lerance::~Arc2lerance()
	{

	}

	void Arc2lerance::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/arc/arc.3mf";
		std::vector<creative_kernel::MeshWithName> meshWithNames = creative_kernel::load_meshes_from_3mf(calibPathName);

		/// --- scale ---
		// model is created for a 0.4 nozzle, scale z with nozzle size.
		float nozzle_diameter = std::atof(scene->m_settings->getString("nozzle_diameter").c_str());
		float xyScale = nozzle_diameter / 0.6;
		//scale z to have 7 layers
		double first_layer_height = std::atof(scene->m_settings->getString("initial_layer_print_height").c_str());
		double layer_height = nozzle_diameter / 2.0; // prefer 0.2 layer height for 0.4 nozzle
		first_layer_height = std::max(first_layer_height, layer_height);
		float zscale = (first_layer_height + 6 * layer_height) / 1.4;

		// only enlarge
		if (xyScale > 1.2)
		{
			for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
			{
				trimesh::xform xf = trimesh::xform::scale(xyScale, xyScale, zscale);
				trimesh::apply_xform(ameshWithName.mesh.get(), xf);
			}
		}
		else
		{
			for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
			{
				trimesh::xform xf = trimesh::xform::scale(1.0, 1.0, zscale);
				trimesh::apply_xform(ameshWithName.mesh.get(), xf);
			}
		}
		trimesh::TriMesh::BBox allMeshBox;
		for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
		{
			ameshWithName.mesh->need_bbox();
			allMeshBox += ameshWithName.mesh->bbox;
		}
		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x() - allMeshBox.center().x, abox3.center().y() - allMeshBox.center().y, 0.0);
		for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
		{
			trimesh::trans(ameshWithName.mesh.get(), offset);
		}

		double filament_max_volumetric_speed = std::atof(scene->m_extruders[0]->getString("filament_max_volumetric_speed").c_str());
		double max_infill_speed = filament_max_volumetric_speed / (mm3_per_mm(nozzle_diameter * 1.2f, layer_height) * (m_type == CalibrateType::ct_lowflow ? 1.2 : 1));
		double internal_solid_speed = std::floor(std::min(std::atof(scene->m_settings->getString("internal_solid_infill_speed").c_str()), max_infill_speed));
		double top_surface_speed = std::floor(std::min(std::atof(scene->m_settings->getString("top_surface_speed").c_str()), max_infill_speed));


		// adjust parameters
		for (creative_kernel::MeshWithName ameshWithName : meshWithNames)
		{
			int groupID = scene->addOneGroup();
			int objectID = scene->addObject2Group(groupID);
			scene->setOjbectMesh(groupID, objectID, TriMeshPtr(ameshWithName.mesh));
			crslice2::SettingsPtr groupSettings(new crslice2::Settings());
			scene->m_settings->add("enable_arc_fitting", "1");

			// extract flowrate from name, filename format: flowrate_xxx
			std::string obj_name = ameshWithName.name;
			obj_name = obj_name.substr(0, obj_name.rfind("."));
			int modifier = stof(obj_name);

			groupSettings->add("arc_tolerance", std::to_string(modifier));
			scene->setGroupSettings(groupID, groupSettings);
		}
		scene->m_settings->add("layer_height", std::to_string(layer_height));
		scene->m_settings->add("initial_layer_print_height", std::to_string(first_layer_height));

		scene->m_calibParams.start = 0.0;
		scene->m_calibParams.step = 0.0;
		scene->m_calibParams.end = 0.0;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_Arc2Lerance;
		scene->m_gcodeFileName = QString("%1/arc.gcode").arg(generateTempGCodeFileName()).toStdString();
		m_calibPNGName = getCalibResourcePath() + "/arc/1.png";
	}

	Accel2decel::Accel2decel()
	{

	}

	Accel2decel::~Accel2decel()
	{

	}

	void Accel2decel::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/dec_speed/accel2decel.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());

		msbase::moveTrimesh2Center(meshptr.get());
		double layer_height = 0.2;
		auto max_lh = std::atof(scene->m_settings->getString("max_layer_height").c_str());
		if (max_lh < layer_height)
		{
			scene->m_settings->add("max_layer_height", std::to_string(layer_height));
		}
		scene->m_settings->add("filament_max_volumetric_speed", std::to_string(200));
		scene->m_settings->add("slow_down_layer_time", std::to_string(0.0));
		scene->m_settings->add("top_shell_layers", std::to_string(0));
		scene->m_settings->add("bottom_shell_layers", std::to_string(0));
		scene->m_settings->add("acceleration_limit_mess_enable", "0");
		scene->m_settings->add("speed_limit_to_height_enable", "0");
		scene->m_settings->add("accel_to_decel_enable", "1");//true
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		//  cut upper
		auto obj_bb = creative_kernel::baseBoundingBox();;
		auto height = ((m_start -m_end) * m_highStep / m_step + m_highStep);
		height > obj_bb.size().z() ? height = obj_bb.size().z() : height;
		std::vector<trimesh::TriMesh*> meshes;
		if (crslice2::cut_horizontal(meshptr.get(), height, meshes, 1))
		{
			meshptr.reset(meshes[0]);
			msbase::moveTrimesh2Center(meshptr.get());
		}

		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);
		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, meshptr);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.highStep = m_highStep;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_Accel2Decel;
		scene->m_gcodeFileName = QString("%1/accel2decel.gcode").arg(generateTempGCodeFileName()).toStdString();

		int iRatio = 10;
		int num = (m_start - m_end + m_step) / iRatio;
		num = std::max(std::min(num, 10), 1);
		m_calibPNGName = getCalibResourcePath() + QString("/dec_speed/%1.png").arg(num);
	}

	X_Y_Jerk::X_Y_Jerk()
	{

	}

	X_Y_Jerk::~X_Y_Jerk()
	{

	}

	void X_Y_Jerk::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/x_y_jerk/X_Y_Jerk.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());

		msbase::moveTrimesh2Center(meshptr.get());
		double layer_height = 0.2;
		auto max_lh = std::atof(scene->m_settings->getString("max_layer_height").c_str());
		if (max_lh < layer_height)
		{
			scene->m_settings->add("max_layer_height", std::to_string(layer_height));
		}
		scene->m_settings->add("filament_max_volumetric_speed", std::to_string(200));
		scene->m_settings->add("slow_down_layer_time", std::to_string(0.0));
		scene->m_settings->add("top_shell_layers", std::to_string(0));
		scene->m_settings->add("bottom_shell_layers", std::to_string(0));
		scene->m_settings->add("acceleration_limit_mess_enable", "0");
		scene->m_settings->add("speed_limit_to_height_enable", "0");
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));

		//  cut upper
		meshptr->need_bbox();
		auto obj_bb = meshptr->bbox;
		auto height = ((m_end - m_start) * 5 / m_step + 5);
		if (height < obj_bb.size().z)
		{
			std::vector<trimesh::TriMesh*> meshes;
			if (crslice2::cut_horizontal(meshptr.get(), height, meshes, 1))
			{
				meshptr.reset(meshes[0]);
				msbase::moveTrimesh2Center(meshptr.get());
			}

		}
		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);
		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, meshptr);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_X_Y_Jerk;
		scene->m_gcodeFileName = QString("%1/X_Y_Jerk.gcode").arg(generateTempGCodeFileName()).toStdString();

		int iRatio = 1;
		int num = (m_start - m_end + m_step) / iRatio;
		num = std::max(std::min(num, 12), 1);
		m_calibPNGName = getCalibResourcePath() + QString("/x_y_jerk/%1.png").arg(num);
	}

	FanSpeed::FanSpeed()
	{

	}

	FanSpeed::~FanSpeed()
	{

	}

	void FanSpeed::setConfig(crslice2::CrScene* scene)
	{
		QString calibPathName = getCalibResourcePath() + "/fan_speed/fanSpeed.stl";
		TriMeshPtr meshptr(cxkernel::loadMeshFromName(calibPathName, nullptr));
		if (!meshptr)
			meshptr.reset(new trimesh::TriMesh());

		msbase::moveTrimesh2Center(meshptr.get());

		scene->m_settings->add("fan_cooling_layer_time", std::to_string(0));
		scene->m_settings->add("slow_down_layer_time", std::to_string(0));
		scene->m_settings->add("close_fan_the_first_x_layers", std::to_string(0));
		scene->m_settings->add("enable_overhang_bridge_fan", "0");
		scene->m_settings->add("slow_down_for_layer_cooling", "0");
		scene->m_settings->add("hot_plate_temp", std::to_string(m_bedTemp));
		scene->m_settings->add("hot_plate_temp_initial_layer", std::to_string(m_bedTemp));
		scene->m_settings->add("cool_cds_fan_start_at_height", std::to_string(0));

		//  cut upper
		auto obj_bb = creative_kernel::baseBoundingBox();;
		auto height = ((m_end - m_start) * 7.8 / m_step + 7.8);
		height > obj_bb.size().z() ? height = obj_bb.size().z() : height;
		std::vector<trimesh::TriMesh*> meshes;
		if (crslice2::cut_horizontal(meshptr.get(), height, meshes, 1))
		{
			meshptr.reset(meshes[0]);
		}
		msbase::moveTrimesh2Center(meshptr.get());
		qtuser_3d::Box3D abox3 = creative_kernel::baseBoundingBox();
		trimesh::vec3 offset(abox3.center().x(), abox3.center().y(), 0.0);
		trimesh::trans(meshptr.get(), offset);
		int groupID = scene->addOneGroup();
		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, meshptr);

		scene->m_calibParams.start = m_start;
		scene->m_calibParams.step = m_step;
		scene->m_calibParams.end = m_end;
		scene->m_calibParams.mode = crslice2::CalibMode::Calib_Fan_Speed;
		scene->m_gcodeFileName = QString("%1/fanSpeed.gcode").arg(generateTempGCodeFileName()).toStdString();
		
		int iRatio = 10;
		int num = (m_end - m_start + m_step) / iRatio;
		m_calibPNGName = getCalibResourcePath() + QString("/fan_speed/%1.png").arg(num);
	}

}

