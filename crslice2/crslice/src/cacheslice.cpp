#include "crslice2/cacheslice.h"
#include "crslice2/crscene.h"
#include "../wrapper/orcaslicewrapper.h"
#include "libslic3r/I18N.hpp"
#include "libslic3r/BuildVolume.hpp"

#include "conv.h"

#include "crsliceexception.h"
#include "ccglobal/profile.h"
#include "../wrapper/baselineorcinput.h"
#include "../wrapper/serialization.h"

namespace crslice2
{
	//cache interface
	class CrSlicePrintImpl
	{
	public:
		CrSlicePrintImpl() {

		}

		~CrSlicePrintImpl() {

		}

		Slic3r::Print print;
	};

	CrSlicePrint::CrSlicePrint()
		:impl(new CrSlicePrintImpl())
	{
	}

	CrSlicePrint::~CrSlicePrint()
	{
		delete impl;
		impl = nullptr;
	}

	class CrSliceModelImpl
	{
	public:
		CrSliceModelImpl() {

		}

		~CrSliceModelImpl() {

		}

		Slic3r::Model model;
		Slic3r::DynamicPrintConfig config;
	};

	class CrSliceVolumeImpl
	{
	public:
		CrSliceVolumeImpl() {

		}

		~CrSliceVolumeImpl() {

		}

		Slic3r::ModelVolume* volume = nullptr;
		int64_t host_id = -1;
	};

	class CrSliceObjectImpl
	{
	public:
		CrSliceObjectImpl() {

		}

		~CrSliceObjectImpl() {

		}

		Slic3r::ModelObject* object = nullptr;
		Slic3r::ModelInstance* instacne = nullptr;
		int64_t host_id = -1;
	};

	CrSliceModel::CrSliceModel()
		:impl(new CrSliceModelImpl())
	{
	}

	CrSliceModel::~CrSliceModel()
	{
		delete impl;
		impl = nullptr;
	}

	void CrSliceModel::setPlateInfo(const PlateInfo& plate)
	{
		Slic3r::CustomGCode::Info info;
		info.mode = Slic3r::CustomGCode::Mode(plate.mode);
		for (const Plate_Item& item : plate.gcodes)
		{
			Slic3r::CustomGCode::Item _item;
			_item.color = item.color;
			_item.print_z = item.print_z;
			_item.type = Slic3r::CustomGCode::Type(item.type);
			_item.extruder = item.extruder;
			_item.color = item.color;
			_item.extra = item.extra;
			info.gcodes.push_back(_item);
		}

		impl->model.curr_plate_index = 0;
		impl->model.plates_custom_gcodes.clear();
		impl->model.plates_custom_gcodes.emplace(0, info);
	}

	CrSliceObject* CrSliceModel::add_object()
	{
		CrSliceObject* object = new CrSliceObject();
		object->impl->object = impl->model.add_object();
		object->impl->instacne = object->impl->object->add_instance();
		m_objects.push_back(object);
		return object;
	}

	void CrSliceModel::remove_object(CrSliceObject* object)
	{
		if (!object)
			return;

		impl->model.delete_object(object->impl->object);
		m_objects.erase(std::find(m_objects.begin(), m_objects.end(), object));
		delete object;
	}

	void CrSliceModel::setParameter(const std::string& key, const std::string& value)
	{
		set_key_value(&impl->config, key, value);
	}

	CrSliceObject::CrSliceObject()
		:impl(new CrSliceObjectImpl())
	{
	}

	CrSliceObject::~CrSliceObject()
	{

	}

	void CrSliceObject::setName(const std::string& name)
	{
		if (!impl->object)
			return;

		impl->object->name = name;
	}

	void CrSliceObject::setParameter(const std::string& key, const std::string& value)
	{
		set_key_value(&impl->object->config, key, value);
	}

	void CrSliceObject::clearParameter()
	{
		if (!impl->object)
			return;

		impl->object->config.reset();
	}

	void CrSliceObject::setMatrix(const trimesh::xform& matrix)
	{
		if (!impl->object)
			return;

		impl->object->instances[0]->set_transformation(convert_matrix(matrix));
	}

	void CrSliceObject::setLayerHeight(const std::vector<double>& layer_heights)
	{
		impl->object->layer_height_profile.set(layer_heights);
	}

	void CrSliceObject::setHostID(int64_t id)
	{
		impl->host_id = id;
	}

	void CrSliceObject::setVisible(bool visible)
	{
		impl->object->instances[0]->printable = visible;
	}

	CrSliceVolume* CrSliceObject::add_volume()
	{
		CrSliceVolume* vol = new CrSliceVolume();
		if (impl->object)
		{
			vol->impl->volume = impl->object->add_volume(Slic3r::TriangleMesh());
		}
		return vol;
	}

	void CrSliceObject::remove_volume(CrSliceVolume* volume)
	{
		if (!volume)
			return;

		size_t idx = -1;
		for (size_t i = 0; i < impl->object->volumes.size(); ++i)
		{
			if (impl->object->volumes.at(i) == volume->impl->volume)
			{
				idx = i;
				break;
			}
		}
		impl->object->delete_volume(idx);
		delete volume;
	}

	CrSliceVolume::CrSliceVolume()
		:impl(new CrSliceVolumeImpl())
	{
	}

	CrSliceVolume::~CrSliceVolume()
	{
	};

	void CrSliceVolume::setParameter(const std::string& key, const std::string& value)
	{
		set_key_value(&impl->volume->config, key, value);
	}

	void CrSliceVolume::clearParameter()
	{
		if (!impl->volume)
			return;

		impl->volume->config.reset();
	}

	void CrSliceVolume::setMatrix(const trimesh::xform& matrix)
	{
		if (!impl->volume)
			return;

		impl->volume->set_transformation(convert_matrix(matrix));
	}

	void CrSliceVolume::regenerateID()
	{
		impl->volume->set_new_unique_id();
	}

	void CrSliceVolume::setMeshData(TriMeshPtr mesh)
	{
		if (!mesh || !impl->volume)
			return;

		Slic3r::TriangleMesh tmesh;
		trimesh2Slic3rTriangleMesh(mesh.get(), tmesh);

		impl->volume->set_mesh(tmesh);
		impl->volume->calculate_convex_hull();
	}

	void CrSliceVolume::setSpreadColor(const std::vector<std::string>& colors)
	{
		if (!impl->volume)
			return;

		if (impl->volume->mesh().facets_count() == colors.size())
		{
			impl->volume->mmu_segmentation_facets.reset();
			for (size_t i = 0; i < colors.size(); i++) {
				if (!colors[i].empty())
					impl->volume->mmu_segmentation_facets.set_triangle_from_string(i, colors[i]);
			}
		}
	}

	void CrSliceVolume::setSpreadSeam(const std::vector<std::string>& seams)
	{
		if (!impl->volume)
			return;

		if (impl->volume->mesh().facets_count() == seams.size())
		{
			impl->volume->seam_facets.reset();
			for (size_t i = 0; i < seams.size(); i++) {
				if (!seams[i].empty())
					impl->volume->seam_facets.set_triangle_from_string(i, seams[i]);
			}
		}
	}

	void CrSliceVolume::setSpreadSupport(const std::vector<std::string>& supports)
	{
		if (!impl->volume)
			return;

		if (impl->volume->mesh().facets_count() == supports.size())
		{
			impl->volume->supported_facets.reset();
			for (size_t i = 0; i < supports.size(); i++) {
				if (!supports[i].empty())
					impl->volume->supported_facets.set_triangle_from_string(i, supports[i]);
			}
		}
	}

	void CrSliceVolume::setName(const std::string& name)
	{
		if (!impl->volume)
			return;

		impl->volume->name = name;
	}

	void CrSliceVolume::setModelType(const int model_type)
	{
		if (!impl->volume)
			return;

		impl->volume->set_type(static_cast<Slic3r::ModelVolumeType>(model_type));
	}

	void CrSliceVolume::setHostID(int64_t id)
	{
		impl->host_id = id;
	}

	int64_t find_host_id(CrSliceModel& model, int64_t id) {
		int64_t sceneObjId = -1;
		for (CrSliceObject* object : model.m_objects)
		{
			if (object->impl->object->id() == id)
			{
				sceneObjId = object->impl->host_id;
				break;
			}
		}
		return sceneObjId;
	}

	void update_print_volume_state(CrSliceModel& model, const std::vector<trimesh::dvec2>& shapes, double height)
	{
		//update models inside or outside
		std::vector<Slic3r::Vec2d> shape;
		for (const trimesh::dvec2& v : shapes)
			shape.push_back(Slic3r::Vec2d(v.x, v.y));
		Slic3r::BuildVolume build_volume(shape, height);
		model.impl->model.update_print_volume_state(build_volume);
	}

	CrSliceResult slice(CrSlicePrint& print, CrSliceModel& model, const std::vector<ThumbnailData>& thumbnails,
		const CacheSliceParam& param, ccglobal::Tracer* tracer)
	{
		if (!param.fileName.empty())
		{
			cache_slice_scene(print.impl->print, model.impl->model, model.impl->config, param.fileName);
		}

		CrSliceResult result;

		OrcaResult orca_result;
		Slic3r::Calib_Params calib_params;
		auto f = [&](const Slic3r::ThumbnailsParams&) { 
			Slic3r::ThumbnailsList nails;
			int size = (int)thumbnails.size();
			if (size > 0)
			{
				nails.resize(size);
				for (int i = 0; i < size; ++i)
				{
					Slic3r::ThumbnailData& data = nails.at(i);
					const crslice2::ThumbnailData& thunm = thumbnails.at(i);
					data.height = thunm.height;
					data.width = thunm.width;
					data.pixels = thunm.pixels;
					data.pos_s = thunm.pos_s; //for cr_png
					data.pos_e = thunm.pos_e; //for cr_png
					data.pos_h = thunm.pos_h; //for cr_png
				}
			}

			return nails;
		};
		Slic3r::ThumbnailsGeneratorCallback thumbnail_callback = f;

		try
		{
			orca_slice_impl_result(print.impl->print, model.impl->model, model.impl->config, param.outName, param.tempDirectory, thumbnail_callback, calib_params, orca_result, tracer);
			Slic3r::ThumbnailsList  thumbnails = f(Slic3r::ThumbnailsParams{});
			if (param.baselineType > 0)
			{
				//run baseline test
				TempParamater tp;
				tp.is_bbl_printer = print.impl->print.is_BBL_printer();
				tp.extruderCount = (int)print.impl->print.extruders(true).size();
				tp.plate_origin = print.impl->print.get_plate_origin();
				tp.plate_index = print.impl->print.get_plate_index();
				bool ret = slice_bl_function(model.impl->model,model.impl->config,calib_params,thumbnails,tp,param.baselineType,param.blName,param.blDir,param.resultDir);
				if (!ret)
				{
					tracer->message("${UnitTest}BaseLine Test Failed");
				}
				else
				{
					tracer->message("${UnitTest}BaseLine Test Success");
				}
		
			}

		}
		catch (const crslice2::CrSliceException& e)
		{
			size_t sliceObjectId = e.sliceObjectId();
			std::string failStr;

			if (0 == sliceObjectId)
			{
				failStr = std::string(e.what()) + "@";
				tracer->failed(failStr.c_str());
			}
			else {
				int64_t sceneObjId = find_host_id(model, sliceObjectId);

				failStr = std::string(e.what()) + "@" + std::to_string(sceneObjId);
				tracer->failed(failStr.c_str());
			}

			result.success = false;
		}

		{
			const Slic3r::PrintEstimatedStatistics& process_result = orca_result.gcode_result.print_statistics;
			result.volumes_per_color_change = process_result.volumes_per_color_change;
			result.volumes_per_extruder = process_result.volumes_per_extruder;
			result.wipe_tower_volumes_per_extruder = process_result.wipe_tower_volumes_per_extruder;
			result.flush_per_filament = process_result.flush_per_filament;
			for (std::map<Slic3r::ExtrusionRole, std::pair<double, double>>::const_iterator it = process_result.used_filaments_per_role.begin();
				it != process_result.used_filaments_per_role.end(); ++it)
			{
				result.used_filaments_per_role.insert(std::pair<int, std::pair<double, double>>((int)it->first, it->second));
			}
			result.total_filamentchanges = process_result.total_filamentchanges;
		}

		if (orca_result.gcode_result.conflict_result.has_value())
		{
			const Slic3r::GCodeProcessorResult& process_result = orca_result.gcode_result;
			static std::string text;
			std::string objName1 = process_result.conflict_result.value()._objName1;
			std::string objName2 = process_result.conflict_result.value()._objName2;
			double      height = process_result.conflict_result.value()._height;
			int  layer = process_result.conflict_result.value().layer + 1;  // "+1" to align with zslider display layer value
			size_t sliceObj2Id = process_result.conflict_result.value()._sliceObject2Id;

			text = (boost::format(_u8L("Conflicts of gcode paths have been found at layer# %d, #height$ %.2f mm.$ Please separate the conflicted objects further@%s")) % layer % height % objName1).str();

			int64_t sceneObjId = find_host_id(model, sliceObj2Id);
			result.warnings["Path_Conflict"] = std::make_pair(text, sceneObjId);
		}

		if (orca_result.key_warnings.size() > 0)
		{
			for (auto itr = orca_result.key_warnings.begin(); itr != orca_result.key_warnings.end(); itr++)
			{
				int64_t sliceObjId = itr->second.second;
				if (sliceObjId == -1)
					continue;

				int64_t sceneObjId = find_host_id(model, sliceObjId);

				result.warnings[itr->first] = std::make_pair(itr->second.first, sceneObjId);
			}
		}

		if (result.warnings.size() > 0)
			result.success = false;

		return result;
	}
}
