#include "calibmode.h"
#include "interface/spaceinterface.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"
#include "interface/machineinterface.h"
#include "interface/appsettinginterface.h"
#include "kernel/kernel.h"
#include "cxgcode/gcodehelper.h"
#include "cxkernel/data/trimeshutils.h"
#include "qtusercore/module/systemutil.h"
#include "msbase/mesh/tinymodify.h"
#include "msbase/utils/cut.h"

#include "internal/project_3mf/load3mf.h"
#include "cadcore/step/STEP.hpp"

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
	QString keys1 = "cool_special_cds_fan_speed,cool_cds_fan_start_at_height,additional_cooling_fan_speed,bed_temperature_difference,close_fan_the_first_x_layers,cool_plate_temp,cool_plate_temp_initial_layer,default_filament_colour,enable_overhang_bridge_fan,enable_pressure_advance,eng_plate_temp,eng_plate_temp_initial_layer,fan_cooling_layer_time,fan_max_speed,fan_min_speed";
	QString keys2 = "filament_start_gcode,filament_end_gcode,filament_colour,filament_cost,filament_density,filament_deretraction_speed,filament_diameter,filament_flow_ratio,filament_ids,filament_is_support,filament_max_volumetric_speed,filament_minimal_purge_on_wipe_tower,filament_retract_before_wipe,filament_retract_restart_extra,filament_retract_when_changing_layer,filament_retraction_length,filament_retraction_minimum_travel,filament_retraction_speed,filament_settings_id,filament_shrink,filament_soluble,filament_type,filament_vendor,filament_wipe,filament_wipe_distance,filament_z_hop,filament_z_hop_types";
	QString keys3 = "full_fan_speed_layer,hot_plate_temp,hot_plate_temp_initial_layer,nozzle_temperature,nozzle_temperature_initial_layer,nozzle_temperature_range_high,nozzle_temperature_range_low,overhang_fan_speed,overhang_fan_threshold,pressure_advance,reduce_fan_stop_start_freq,required_nozzle_HRC,slow_down_for_layer_cooling,slow_down_layer_time,slow_down_min_speed,support_material_interface_fan_speed,temperature_vitrification,textured_plate_temp,textured_plate_temp_initial_layer,extruder_offset";

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

CalibMode::CalibMode()
{

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

void CalibMode::setValue(float _start, float _end, float _step)
{
	m_start = _start;
	m_end = _end;
	m_step = _step;
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
			msbase::CutPlane plane_pts;
			plane_pts.normal = trimesh::vec3(0.0, 0.0, 1.0);
			plane_pts.position = trimesh::vec3(0, 0, new_height);
			msbase::planeCut(meshptr.get(), plane_pts, meshes);
			meshptr.reset(meshes[1]);
		}
	}
	// cut bottom
	block_count = lround((350 - m_start) / 5);
	if (block_count > 0) {
		auto new_height = block_count * 10.0 + EPSILON;
		meshptr->need_bbox();
		if (new_height < meshptr->bbox.max.z)
		{
			std::vector<trimesh::TriMesh*> meshes;
			msbase::CutPlane plane_pts;
			plane_pts.normal = trimesh::vec3(0.0, 0.0, 1.0);
			plane_pts.position = trimesh::vec3(0, 0, new_height);
			msbase::planeCut(meshptr.get(), plane_pts, meshes);
			meshptr.reset(meshes[0]);
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
	// cut upper
	//auto obj_bb = creative_kernel::baseBoundingBox();;
	//auto height = 5 * ((m_end - m_start) / m_step + 1);
	//if (height < obj_bb.size().z())
	//{
	//	std::vector<trimesh::TriMesh*> meshes;
	//	msbase::CutPlane plane_pts;
	//	plane_pts.normal = trimesh::vec3(0.0, 0.0, 1.0);
	//	plane_pts.position = trimesh::vec3(0.0, 0.0, height);
	//	msbase::planeCut(meshptr.get(), plane_pts, meshes);
	//	meshptr.reset(meshes[1]);
	//}
	//msbase::moveTrimesh2Center(meshptr.get());
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
	QString calibPathName = getCalibResourcePath() + "/filament_flow/flowrate-test-pass1.3mf";
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
	QString calibPathName = getCalibResourcePath() + "/filament_flow/flowrate-test-pass2.3mf";
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
	trimesh::TriMesh* amesh = cadcore::load_step(calibPathName.toLocal8Bit().data(), nullptr);
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

	//  cut upper
	auto obj_bb = creative_kernel::baseBoundingBox();
	auto height = (m_end - m_start + 1) / m_step;
	if (height < obj_bb.size().z())
	{
		std::vector<trimesh::TriMesh*> meshes;
		msbase::CutPlane plane_pts;
		plane_pts.normal = trimesh::vec3(0.0, 0.0, 1.0);
		plane_pts.position = trimesh::vec3(0.0, 0.0, height);
		msbase::planeCut(amesh, plane_pts, meshes);
		amesh = meshes[1];
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
	int num = (m_end - m_start) / m_step / 10;
	num > 10 ? num = 10 : num < 3 ? num = 3 : num;
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

	//  cut upper
	auto obj_bb = creative_kernel::baseBoundingBox();;
	auto height = (m_end - m_start + 1) / m_step;
	if (height < obj_bb.size().z())
	{
		std::vector<trimesh::TriMesh*> meshes;
		msbase::CutPlane plane_pts;
		plane_pts.normal = trimesh::vec3(0.0, 0.0, 1.0);
		plane_pts.position = trimesh::vec3(0.0, 0.0, height);
		if (msbase::planeCut(meshptr.get(), plane_pts, meshes))
		{
			meshptr.reset(meshes[1]);
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
	scene->m_calibParams.mode = crslice2::CalibMode::Calib_Retraction_tower;
	scene->m_gcodeFileName = QString("%1/retract.gcode").arg(generateTempGCodeFileName()).toStdString();
	int num = (m_end - m_start) / m_step / 10;
	num > 8 ? num = 8 : num;
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
