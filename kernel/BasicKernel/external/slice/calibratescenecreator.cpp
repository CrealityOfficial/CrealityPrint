#include "calibratescenecreator.h"
#include "cxgcode/us/usettings.h"

#include "interface/machineinterface.h"
#include <QtCore/QCoreApplication>
#include "cxkernel/data/trimeshutils.h"

#include "msbase/utils/cut.h"
#include "msbase/mesh/tinymodify.h"

#ifdef _DEBUG
#include "buildinfo.h"
#endif

SettingsPtr convert(const cxgcode::USettings& settings)
{
	SettingsPtr ptr(new crslice::Settings());
	const QHash<QString, cxgcode::USetting*>& ss = settings.settings();
	for (QHash<QString, cxgcode::USetting*>::ConstIterator it = ss.begin();
		it != ss.end(); ++it)
	{
		ptr->add(it.key().toStdString(), it.value()->str().toStdString());
	}
	return ptr;
}

CalibrateSceneCreator::CalibrateSceneCreator()
	: m_type(CalibrateType::ct_temperature)
	, m_flowHigh(false)
	, m_baseX(10.0f)
	, m_baseY(10.0f)
	, m_gap(20.0f)
	, m_paType(PA_type::ct_tower)
	, m_start(0.0f)
	, m_end(1.0f)
	, m_step(0.1f)
{

}

CalibrateSceneCreator::~CalibrateSceneCreator()
{

}

void CalibrateSceneCreator::reset()
{

}

void CalibrateSceneCreator::setType(CalibrateType type)
{
	m_type = type;
	reset();
}

void CalibrateSceneCreator::setValue(float _start, float _end, float _step)
{
	m_start= _start;
	m_end= _end;
	m_step= _step;
}

void CalibrateSceneCreator::setFlowHigh(bool high)
{
	m_flowHigh = high;
}

crslice::CrScene* CalibrateSceneCreator::create(ccglobal::Tracer* tracer)
{
	SettingsPointer GSettings = creative_kernel::createCurrentGlobalSettings();
	QList<SettingsPointer> ESettingsList = creative_kernel::createCurrentExtruderSettings();

	crslice::CrScene* scene = new crslice::CrScene();
	int groupID = scene->addOneGroup();

	const QHash<QString, us::USetting*>& G = GSettings->settings();
	for (QHash<QString, us::USetting*>::const_iterator it = G.constBegin(); it != G.constEnd(); ++it)
	{
		scene->m_settings->add(it.key().toStdString(), it.value()->str().toStdString());
	}

	//Extruder Settings
	size_t extruder_count = std::stoul(scene->m_settings->getString("machine_extruder_count"));
	if (extruder_count == 0)
		extruder_count = 1;

	for (size_t extruder_nr = 0; extruder_nr < extruder_count; extruder_nr++)
		scene->m_extruders.emplace_back(new crslice::Settings());

	for (size_t i = 0; i < extruder_count; ++i)
	{
		int32_t extruder_nr = i;
		if (extruder_nr < 0 || extruder_nr >= static_cast<int32_t>(ESettingsList.size()))
		{
			return nullptr;
		}
		SettingsPtr& extruder = scene->m_extruders[i];
		const QHash<QString, us::USetting*>& E = ESettingsList.at(i)->settings();
		for (QHash<QString, us::USetting*>::const_iterator it = E.constBegin(); it != E.constEnd(); ++it)
		{
			extruder->add(it.key().toStdString(), it.value()->str().toStdString());
		}
	}
	scene->machine_center_is_zero = G.value("machine_center_is_zero")->enabled();
	scene->addSceneParameter("special_object_cancel","false");
	auto create = [](const QString& fileName)->TriMeshPtr {

#ifdef _DEBUG
		QString name = SOURCE_ROOT + QString("/resources/mesh/calibrate/") + fileName;
#elif defined (__APPLE__)
		int index = QCoreApplication::applicationDirPath().lastIndexOf("/");
		QString name = QCoreApplication::applicationDirPath().left(index) + QString("/Resources/resources/mesh/calibrate/") + fileName;
#else
		QString name = QCoreApplication::applicationDirPath() + QString("/resources/mesh/calibrate/") + fileName;
#endif

		TriMeshPtr mesh(cxkernel::loadMeshFromName(name, nullptr));
		if (!mesh)
			mesh.reset(new trimesh::TriMesh());

		mesh->need_bbox();
		msbase::moveTrimesh2Center(mesh.get());
		return mesh;
	};

	if (m_type == CalibrateType::ct_temperature)
	{
		for (SettingsPtr& asettings : scene->m_extruders)
		{
			asettings->add("material_final_print_temperature", std::to_string(0));
			asettings->add("material_initial_print_temperature", std::to_string(0));
		}
		float h = 10.0f;
		int isize = std::abs(m_start - m_end) / m_step + 1;

		for (int i = 0; i < isize; ++i)
		{
			int objectID = scene->addObject2Group(groupID);
			QString name = QString("%1.stl").arg(int(m_start - i * m_step));
			TriMeshPtr mesh = create(name);

			trimesh::apply_xform(mesh.get(), trimesh::xform::trans(0.0f, 0.0f, (float)i * h));
			scene->setOjbectMesh(groupID, objectID, mesh);
			SettingsPtr mPtr(new crslice::Settings());
			mPtr->add("calibration_temperature", std::to_string(m_start - i * m_step));
			mPtr->add("center_object", "true");

			scene->setObjectSettings(groupID, objectID, mPtr);
		}
	}
	else if (m_type == CalibrateType::ct_highflow || m_type == CalibrateType::ct_lowflow)
	{
		int flows[9] = { 0,-1,-2,-3,-4,-5,-6,-7,-8};
		int initValue = 100;
		if (m_type == CalibrateType::ct_highflow)
		{
			initValue = 100 + m_highFlowValue;
			//for (int i = 0; i < 9; i++)
			//{
			//	flows[i] = m_highFlowValue - i;
			//}
		}
		if (m_type == CalibrateType::ct_lowflow)
		{
			for (int i = 0; i < 9; i++)
			{
				flows[i] = 20 - i * 5;
			}
		}

		scene->m_settings->add("adhesion_type","none");

		for (int i = 0; i < 9; i++)
		{
			QString name = QString("%1.stl").arg(flows[i]);
			TriMeshPtr mesh = create(name);

			trimesh::box3 b = mesh->bbox;
			trimesh::vec3 s = b.size();
			int gapnumX = i / 3-1;
			int gapnumY = i % 3-1;
			int flow = initValue + flows[i];//m_flowHigh ? (100 - i) : (102 - 5 * i);

			TriMeshPtr _mesh(new trimesh::TriMesh());
			*_mesh = *mesh;


			float offsetX = mesh->bbox.size().x + m_gap;
			float offsetY = mesh->bbox.size().y + m_gap;
			trimesh::vec3 offset = trimesh::vec3(offsetX * gapnumX, offsetY * gapnumY, 0.0f);
			trimesh::apply_xform(_mesh.get(), trimesh::xform::trans(offset));

			int objectID = scene->addObject2Group(groupID);
			scene->setOjbectMesh(groupID, objectID, _mesh);
			SettingsPtr mPtr(new crslice::Settings());
			mPtr->add("wall_0_material_flow", std::to_string(flow));
			mPtr->add("wall_x_material_flow", std::to_string(flow));
			mPtr->add("skin_material_flow", std::to_string(flow));
			mPtr->add("roofing_material_flow", std::to_string(flow));
			mPtr->add("material_flow_layer_0", std::to_string(flow));
			mPtr->add("wall_x_material_flow_layer_0", std::to_string(flow));
			mPtr->add("wall_0_material_flow_layer_0", std::to_string(flow));
			mPtr->add("skin_material_flow_layer_0", std::to_string(flow));

			scene->setObjectSettings(groupID, objectID, mPtr);
		}
	}
	else if (m_type == CalibrateType::ct_pressure)
	{
		QString name = QString("Pressure Advance Tower.stl");
		TriMeshPtr mesh = create(name);

		std::vector<trimesh::TriMesh*> meshes;
		int machine_width = GSettings->vvalue("machine_width").toInt();
		int machine_depth = GSettings->vvalue("machine_depth").toInt();
		int split_h = (m_end - m_start) / m_step +1.0;

		msbase::CutPlane aplane;
		aplane.normal = trimesh::vec3(0.0, 0.0, 1.0);
		aplane.position = trimesh::vec3(machine_width/2, machine_depth/2, split_h);
		msbase::planeCut(mesh.get(), aplane, meshes);

		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, TriMeshPtr(meshes[1]));
		SettingsPtr mPtr(new crslice::Settings());

		scene->m_settings->add("acceleration_enabled", "false");
		mPtr->add("pressure_start", std::to_string(m_start));
		mPtr->add("pressure_step", std::to_string(m_step));
		mPtr->add("pressure_end", std::to_string(m_end));
		mPtr->add("center_object", "true");
		scene->setObjectSettings(groupID, objectID, mPtr);
	}
	else if (m_type == CalibrateType::ct_maxvolumetric)
	{
		QString name = QString("Max volumetric speed test.stl");
		TriMeshPtr mesh = create(name);

		std::vector<trimesh::TriMesh*> meshes;
		int machine_width = GSettings->vvalue("machine_width").toInt();
		int machine_depth = GSettings->vvalue("machine_depth").toInt();
		int split_h = (m_end - m_start) / m_step + 2.0;
		msbase::CutPlane aplane;
		aplane.normal = trimesh::vec3(0.0, 0.0, 1.0);
		aplane.position = trimesh::vec3(machine_width / 2, machine_depth / 2, split_h);
		msbase::planeCut(mesh.get(), aplane, meshes);

		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, TriMeshPtr(meshes[1]));
		SettingsPtr mPtr(new crslice::Settings());
		scene->m_settings->add("layer_height", std::to_string(0.32));
		scene->m_settings->add("layer_height_0", std::to_string(0.32));
		scene->m_settings->add("bottom_layers", std::to_string(1));
		scene->m_settings->add("adhesion_type", "brim");
		scene->m_settings->add("brim_line_count", std::to_string(5));
		scene->m_settings->add("magic_spiralize", "true");
		scene->m_settings->add("initial_bottom_layers", "1");
		for (SettingsPtr& extrSettings : scene->m_extruders)
		{
			extrSettings->add("cool_min_layer_time", std::to_string(1));
		}
		
		mPtr->add("maxvolumetricspeed_start", std::to_string(m_start));
		mPtr->add("maxvolumetricspeed_step", std::to_string(m_step));
		mPtr->add("maxvolumetricspeed_end", std::to_string(m_end));
		mPtr->add("speed_wall_0", std::to_string(500));
		mPtr->add("wall_line_width_0", std::to_string(0.7));
		mPtr->add("skirt_brim_line_width", std::to_string(0.7));
		mPtr->add("set_wall_overhang_grading", "false");
		mPtr->add("center_object", "true");

		

		scene->setObjectSettings(groupID, objectID, mPtr);
	}
	else if (m_type == CalibrateType::ct_VFA)
	{
		QString name = QString("VFA 50-300test.stl");
		TriMeshPtr mesh = create(name);

		std::vector<trimesh::TriMesh*> meshes;
		int machine_width = GSettings->vvalue("machine_width").toInt();
		int machine_depth = GSettings->vvalue("machine_depth").toInt();
		int split_h = ((m_end - m_start) / m_step + 1) *5;
		msbase::CutPlane aplane;
		aplane.normal = trimesh::vec3(0.0, 0.0, 1.0);
		aplane.position = trimesh::vec3(machine_width / 2, machine_depth / 2, split_h);
		msbase::planeCut(mesh.get(), aplane, meshes);

		int objectID = scene->addObject2Group(groupID);
		scene->setOjbectMesh(groupID, objectID, TriMeshPtr(meshes[1]));
		SettingsPtr mPtr(new crslice::Settings());
		scene->m_settings->add("layer_height", std::to_string(0.2));
		scene->m_settings->add("layer_height_0", std::to_string(0.2));
		scene->m_settings->add("top_layers", std::to_string(0));
		scene->m_settings->add("bottom_layers", std::to_string(1));
		scene->m_settings->add("infill_line_distance", std::to_string(0));
		scene->m_settings->add("adhesion_type", "brim");
		scene->m_settings->add("brim_line_count", std::to_string(5));
		scene->m_settings->add("wall_line_count", std::to_string(1));
		scene->m_settings->add("magic_spiralize", "true");
		scene->m_settings->add("speed_print_layer_0", std::to_string(50));
		scene->m_settings->add("skirt_brim_speed", std::to_string(50));
		scene->m_settings->add("speed_travel_layer_0", std::to_string(100));
		scene->m_settings->add("initial_bottom_layers", "1");
		for (SettingsPtr& extrSettings : scene->m_extruders)
		{
			extrSettings->add("material_max_volumetric_speed", std::to_string(200));
			extrSettings->add("cool_min_layer_time", std::to_string(1));
		}
	
		mPtr->add("VFA_start", std::to_string(m_start));
		mPtr->add("VFA_step", std::to_string(m_step));
		mPtr->add("VFA_end", std::to_string(m_end));
		mPtr->add("set_wall_overhang_grading", "false");
		mPtr->add("center_object", "true");
	
		scene->setObjectSettings(groupID, objectID, mPtr);
	}

	return scene;
}