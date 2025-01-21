#include "Serialization.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Print.hpp"

#include <fstream>
#include "serial.h"

void load_polygon(Slic3r::Polygon& poly, std::fstream& in)
{
	ccglobal::cxndLoadVectorT(in, poly.points);
}

void save_polygon(const Slic3r::Polygon& poly, std::fstream& out)
{
	ccglobal::cxndSaveVectorT(out, poly.points);
}

void load_polygons(Slic3r::Polygons& polys, std::fstream& in)
{
	int count = (int)polys.size();
	ccglobal::cxndLoadT(in, count);

	if (count > 0)
	{
		polys.resize(count);
		for (int i = 0; i < count; ++i)
			load_polygon(polys.at(i), in);
	}
}

void save_polygons(const Slic3r::Polygons& polys, std::fstream& out)
{
	int count = (int)polys.size();
	ccglobal::cxndSaveT(out, count);

	for (int i = 0; i < count; ++i)
		save_polygon(polys.at(i), out);
}

void load_expolygon(Slic3r::ExPolygon& poly, std::fstream& in)
{
	load_polygon(poly.contour, in);

	int count = (int)poly.holes.size();
	ccglobal::cxndLoadT(in, count);

	if (count > 0)
	{
		poly.holes.resize(count);
		for (int i = 0; i < count; ++i)
			load_polygon(poly.holes.at(i), in);
	}
}

void save_expolygon(const Slic3r::ExPolygon& poly, std::fstream& out)
{
	save_polygon(poly.contour, out);

	int count = (int)poly.holes.size();
	ccglobal::cxndSaveT(out, count);

	for (int i = 0; i < count; ++i)
		save_polygon(poly.holes.at(i), out);
}

void load_expolygons(Slic3r::ExPolygons& polys, std::fstream& in)
{
	int count = 0;
	ccglobal::cxndLoadT(in, count);

	if (count > 0)
	{
		polys.resize(count);
		for (int i = 0; i < count; ++i)
			load_expolygon(polys.at(i), in);
	}
}

void save_expolygons(const Slic3r::ExPolygons& polys, std::fstream& out)
{
	int count = (int)polys.size();
	ccglobal::cxndSaveT(out, count);

	for (int i = 0; i < count; ++i)
		save_expolygon(polys.at(i), out);
}

void load_vector_expolygons(std::vector<Slic3r::ExPolygons>& slices, std::fstream& in)
{
	int count = 0;
	ccglobal::cxndLoadT(in, count);

	if (count > 0)
	{
		slices.resize(count);
		for (int i = 0; i < count; ++i)
			load_expolygons(slices.at(i), in);
	}
}

void save_vector_expolygons(const std::vector<Slic3r::ExPolygons>& slices, std::fstream& out)
{
	int count = (int)slices.size();
	ccglobal::cxndSaveT(out, count);

	for (int i = 0; i < count; ++i)
		save_expolygons(slices.at(i), out);
}

void load_surface(Slic3r::Surface& surface, std::fstream& in)
{
	int type = 0;
	ccglobal::cxndLoadT(in, type);
	surface.surface_type = (Slic3r::SurfaceType)type;
	ccglobal::cxndLoadT(in, surface.bridge_angle);
	ccglobal::cxndLoadT(in, surface.extra_perimeters);
	ccglobal::cxndLoadT(in, surface.thickness);
	ccglobal::cxndLoadT(in, surface.thickness_layers);
	load_expolygon(surface.expolygon, in);
}

void save_surface(const Slic3r::Surface& surface, std::fstream& out)
{
	int type = (int)surface.surface_type;
	ccglobal::cxndSaveT(out, type);
	ccglobal::cxndSaveT(out, surface.bridge_angle);
	ccglobal::cxndSaveT(out, surface.extra_perimeters);
	ccglobal::cxndSaveT(out, surface.thickness);
	ccglobal::cxndSaveT(out, surface.thickness_layers);
	save_expolygon(surface.expolygon, out);
}

void load_surfaces(std::vector<Slic3r::Surface>& surfaces, std::fstream& in)
{
	int count = 0;
	ccglobal::cxndLoadT(in, count);

	if (count > 0)
	{
		surfaces.resize(count);
		for (int i = 0; i < count; ++i)
			load_surface(surfaces.at(i), in);
	}
}

void save_surfaces(const std::vector<Slic3r::Surface>& surfaces, std::fstream& out)
{
	int count = (int)surfaces.size();
	ccglobal::cxndSaveT(out, count);

	for (int i = 0; i < count; ++i)
		save_surface(surfaces.at(i), out);
}

void save_config(const Slic3r::DynamicPrintConfig& config, std::fstream& out)
{
	int count = (int)config.size();
	ccglobal::cxndSaveT(out, count);

	for (std::map<Slic3r::t_config_option_key, std::unique_ptr<Slic3r::ConfigOption>>::const_iterator it = config.cbegin();
		it != config.cend(); ++it)
	{
		ccglobal::cxndSaveStr(out, (*it).first);
		ccglobal::cxndSaveStr(out, (*it).second->serialize());
	}
}

void load_config(Slic3r::ModelConfig& config, std::fstream& in)
{
	int count = 0;
	ccglobal::cxndLoadT(in, count);

	for (int i = 0; i < count; ++i)
	{
		std::string key, value;
		ccglobal::cxndLoadStr(in, key);
		ccglobal::cxndLoadStr(in, value);

		config.set<std::string>(key, value);
	}
}

void save_config(const Slic3r::ModelConfig& config, std::fstream& out)
{
	int count = (int)config.size();
	ccglobal::cxndSaveT(out, count);

	for (std::map<Slic3r::t_config_option_key, std::unique_ptr<Slic3r::ConfigOption>>::const_iterator it = config.cbegin();
		it != config.cend(); ++it)
	{
		ccglobal::cxndSaveStr(out, (*it).first);
		ccglobal::cxndSaveStr(out, (*it).second->serialize());
	}
}

void load_spread(Slic3r::FacetsAnnotation& annotation, int faceCount, std::fstream& in)
{
	std::vector<std::string> strs;
	ccglobal::cxndLoadStrs(in, strs);
	if ((faceCount > 0) && (strs.size() == faceCount))
	{
		for (int i = 0; i < faceCount; ++i)
			annotation.set_triangle_from_string(i, strs.at(i));
	}
}

void save_spread(const Slic3r::FacetsAnnotation& annotation, int faceCount, std::fstream& out)
{
	std::vector<std::string> strs;
	if (faceCount > 0)
	{
		strs.resize(faceCount);
		for (int i = 0; i < faceCount; ++i)
			strs.at(i) = annotation.get_triangle_as_string(i);
	}
	ccglobal::cxndSaveStrs(out, strs);
}

void load_tranformation(Slic3r::Geometry::Transformation& transfomr, std::fstream& in)
{
	Slic3r::Transform3d t;
	double datas[16];
	in.read((char*)datas, 16 * sizeof(double));
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			t(i, j) = datas[i + j * 4];
		}
	}
	transfomr.set_matrix(t);
}

void save_tranformation(const Slic3r::Geometry::Transformation& transfomr, std::fstream& out)
{
	double datas[16];
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			datas[i + j * 4] = transfomr.get_matrix()(i, j);
		}
	}

	out.write((const char*)datas, 16 * sizeof(double));
}

void load_model_volume(int i, int j, const std::string& directory, Slic3r::ModelVolume& volume)
{
	std::string file_name = boost::str(boost::format("%s/model_volume_%d_%d") % directory % i % j);
	auto f = [&volume](std::fstream& in, int version)->bool {
		load_config(volume.config, in);

		int have = 0;
		ccglobal::cxndLoadT(in, have);

		int faceCount = 0;
		if (have > 0)
		{
			indexed_triangle_set its;
			ccglobal::cxndLoadVectorT(in, its.indices);
			ccglobal::cxndLoadVectorT(in, its.vertices);

			std::vector<int> flags;
			ccglobal::cxndLoadVectorT(in, flags);
			volume.set_mesh(its);

			faceCount = (int)its.indices.size();
		}

		int type = (int)volume.type();
		ccglobal::cxndLoadT(in, type);

		Slic3r::Geometry::Transformation t;
		load_tranformation(t, in);
		volume.set_transformation(t);

		load_spread(volume.mmu_segmentation_facets, faceCount, in);
		load_spread(volume.supported_facets, faceCount, in);
		load_spread(volume.seam_facets, faceCount, in);
		return true;
	};

	ccglobal::cxndLoad(file_name, f);
}

void save_volume(const Slic3r::ModelVolume& volume, std::fstream& out)
{
	save_config(volume.config, out);

	const Slic3r::TriangleMesh& mesh = volume.mesh();
	int have = mesh.its.vertices.size() > 0 && mesh.its.indices.size() > 0;
	ccglobal::cxndSaveT(out, have);

	if (have)
	{
		ccglobal::cxndSaveVectorT(out, mesh.its.indices);
		ccglobal::cxndSaveVectorT(out, mesh.its.vertices);

		std::vector<int> flags;
		ccglobal::cxndSaveVectorT(out, flags);
	}

	int faceCount = (int)mesh.its.indices.size();
	int type = (int)volume.type();
	ccglobal::cxndSaveT(out, type);
	save_tranformation(volume.get_transformation(), out);
	save_spread(volume.mmu_segmentation_facets, faceCount, out);
	save_spread(volume.supported_facets, faceCount, out);
	save_spread(volume.seam_facets, faceCount, out);
}

void cache_slice_scene(const Slic3r::Print& print, const Slic3r::Model& model, const Slic3r::DynamicPrintConfig& config, const std::string& file_name)
{
	std::fstream out(file_name, std::ios::out | std::ios::binary);
	if (!out.is_open())
	{
		out.close();
		return;
	}

	int ver = 101;
	out.write((const char*)&ver, sizeof(int));

	save_config(config, out);

	int extruderCount = 0;
	ccglobal::cxndSaveT(out, extruderCount);

	int objectsCount = (int)model.objects.size();
	ccglobal::cxndSaveT(out, objectsCount);

	for (int i = 0; i < objectsCount; ++i)
	{
		Slic3r::ModelObject* object = model.objects.at(i);
		Slic3r::ModelInstance* instance = object->instances.at(0);
		if (instance->is_printable())
		{
			save_config(object->config, out);

			int volumeCount = (int)object->volumes.size();
			ccglobal::cxndSaveT(out, volumeCount);

			for (int j = 0; j < volumeCount; ++j)
			{
				Slic3r::ModelVolume* volume = object->volumes.at(j);
				save_volume(*volume, out);
			}

			save_tranformation(instance->get_transformation(), out);
		}
	}
	out.close();
}

void save_print_volume(Slic3r::Print* print, const std::string& directory)
{
	Slic3r::ModelObjectPtrs objects = print->model().objects;
	for (int i = 0; (int)i < objects.size(); ++i)
	{
		Slic3r::ModelObject* object = objects.at(i);
		Slic3r::ModelVolumePtrs volumes = object->volumes;
		for (int j = 0; (int)j < objects.size(); ++j)
		{
			Slic3r::ModelVolume* v = volumes.at(j);

			std::string file_name = boost::str(boost::format("%s/model_volume_%d_%d") % directory % i % j);

			auto f = [&v](std::fstream& out, int version)->bool {
				save_volume(*v, out);
				return true;
			};

			ccglobal::cxndSave(file_name, 0, f);
		}
	}
}

std::string volume_slices_name(int object_index, const std::string& directory)
{
	return boost::str(boost::format("%s/volume_slices_object_%d") % directory % object_index);
}

void save_volume_slices(int object_index, const std::vector<Slic3r::VolumeSlices>& slices, const std::string& directory)
{
	save_volume_slices(slices, volume_slices_name(object_index, directory));
}

void load_volume_slices(int object_index, std::vector<Slic3r::VolumeSlices>& slices, const std::string& directory)
{
	load_volume_slices(slices, volume_slices_name(object_index, directory));
}

void save_volume_slices(const std::vector<Slic3r::VolumeSlices>& slices, const std::string& file_name)
{
	auto f = [&slices](std::fstream& out, int version)->bool {
		int count = (int)slices.size();
		ccglobal::cxndSaveT(out, count);
		for (int i = 0; i < count; ++i)
			save_vector_expolygons(slices.at(i).slices, out);
		return true;
	};

	ccglobal::cxndSave(file_name, 0, f);
}

void load_volume_slices(std::vector<Slic3r::VolumeSlices>& slices, const std::string& file_name)
{
	auto f = [&slices](std::fstream& in, int version)->bool {
		int count = 0;
		ccglobal::cxndLoadT(in, count);
		if (count > 0)
		{
			slices.resize(count);
			for (int i = 0; i < count; ++i)
				load_vector_expolygons(slices.at(i).slices, in);
		}

		return true;
	};

	ccglobal::cxndLoad(file_name, f);
}

void save_zs(int index, const std::vector<float>& slices, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/slices_zs_%d") % directory % index);

	auto f = [&slices](std::fstream& out, int version)->bool {
		ccglobal::cxndSaveVectorT(out, slices);
		return true;
	};

	ccglobal::cxndSave(file_name, 0, f);
}

void load_zs(int index, std::vector<float>& slices, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/slices_zs_%d") % directory % index);

	auto f = [&slices](std::fstream& in, int version)->bool {
		ccglobal::cxndLoadVectorT(in, slices);
		return true;
	};

	ccglobal::cxndLoad(file_name, f);
}

void save_layer_region_surfaces(int object_index, const std::string& prefix, Slic3r::PrintObject* object, const std::string& directory, bool only_fill)
{
	std::string file_name = boost::str(boost::format("%s/layer_region_surfaces_%d_%s") % directory % object_index % prefix);

	auto f = [&object, &only_fill](std::fstream& out, int version)->bool {
		int layer_count = (int)object->layer_count();
		ccglobal::cxndSaveT(out, layer_count);

		for (int i = 0; i < layer_count; ++i)
		{
			Slic3r::Layer* layer = object->get_layer(i);
			int region_count = (int)layer->region_count();
			ccglobal::cxndSaveT(out, region_count);
			for (int j = 0; j < region_count; ++j)
			{				
				if (only_fill)
				{
					Slic3r::Surfaces proxy;
					save_surfaces(proxy, out);
				}
				else {
					const Slic3r::SurfaceCollection& scs = layer->get_region(j)->slices;
					save_surfaces(scs.surfaces, out);
				}

				save_surfaces(layer->get_region(j)->fill_surfaces.surfaces, out);
			}
		}

		return true;
	};

	ccglobal::cxndSave(file_name, 0, f);
}

void load_layer_region_surfaces(int object_index, const std::string& prefix, std::vector<std::vector<Slic3r::SurfaceCollection>>& surfaces, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/layer_region_surfaces_%d_%s") % directory % object_index % prefix);

	auto f = [&surfaces](std::fstream& in, int version)->bool {
		int count = 0;
		ccglobal::cxndLoadT(in, count);
		if (count > 0)
		{
			surfaces.resize(count);
			for (int i = 0; i < count; ++i)
			{
				int region_count = 0;
				ccglobal::cxndLoadT(in, region_count);
				if (region_count > 0)
				{
					surfaces.at(i).resize(region_count);
					for (int j = 0; j < region_count; ++j)
					{
						load_surfaces(surfaces.at(i).at(j).surfaces, in);

						Slic3r::SurfaceCollection other;
						load_surfaces(other.surfaces, in);
						surfaces.at(i).at(j).append(other);
					}
				}
			}
		}

		return true;
	};

	ccglobal::cxndLoad(file_name, f);
}

void save_layer_region_lslices(int object_index, const std::string& prefix, Slic3r::PrintObject* object, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/layer_region_lslices_%d_%s") % directory % object_index % prefix);

	auto f = [&object](std::fstream& out, int version)->bool {
		int layer_count = (int)object->layer_count();
		ccglobal::cxndSaveT(out, layer_count);

		for (int i = 0; i < layer_count; ++i)
		{
			Slic3r::Layer* layer = object->get_layer(i);
			save_expolygons(layer->lslices, out);
		}

		return true;
	};

	ccglobal::cxndSave(file_name, 0, f);
}

void load_layer_region_lslices(int object_index, const std::string& prefix, std::vector<Slic3r::ExPolygons>& lslices, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/layer_region_lslices_%d_%s") % directory % object_index % prefix);

	auto f = [&lslices](std::fstream& in, int version)->bool {
		int count = 0;
		ccglobal::cxndLoadT(in, count);
		if (count > 0)
		{
			lslices.resize(count);
			for (int i = 0; i < count; ++i)
			{
				load_expolygons(lslices.at(i), in);
			}
		}

		return true;
	};

	ccglobal::cxndLoad(file_name, f);
}

void save_perimeters(int object_index, const std::string& prefix, Slic3r::PrintObject* object, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/layer_perimeters_%d_%s") % directory % object_index % prefix);

	auto f = [&object](std::fstream& out, int version)->bool {
		int layer_count = (int)object->layer_count();
		ccglobal::cxndSaveT(out, layer_count);

		for (int i = 0; i < layer_count; ++i)
		{
			Slic3r::Layer* layer = object->get_layer(i);
			int region_count = (int)layer->region_count();
			ccglobal::cxndSaveT(out, region_count);
			for (int j = 0; j < region_count; ++j)
			{
				Slic3r::LayerRegion* region = layer->get_region(j);
				save_surfaces(region->fill_surfaces.surfaces, out);
				save_expolygons(region->fill_expolygons, out);
				save_expolygons(region->fill_no_overlap_expolygons, out);

				Slic3r::Polygons thin_fills_polys = region->thin_fills.polygons_covered_by_spacing();
				save_polygons(thin_fills_polys, out);
				Slic3r::Polygons perimeters_polys = region->perimeters.polygons_covered_by_spacing();
				save_polygons(perimeters_polys, out);
			}
		}

		return true;
	};

	ccglobal::cxndSave(file_name, 0, f);
}

void load_perimeters(int object_index, const std::string& prefix, std::vector<std::vector<PerimeterSets>>& perimeters, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/layer_perimeters_%d_%s") % directory % object_index % prefix);

	auto f = [&perimeters](std::fstream& in, int version)->bool {
		int count = 0;
		ccglobal::cxndLoadT(in, count);
		if (count > 0)
		{
			perimeters.resize(count);
			for (int i = 0; i < count; ++i)
			{
				int region_count = 0;
				ccglobal::cxndLoadT(in, region_count);
				if (region_count > 0)
				{
					perimeters.at(i).resize(region_count);
					for (int j = 0; j < region_count; ++j)
					{
						PerimeterSets& ps = perimeters.at(i).at(j);
						load_surfaces(ps.fill_surfaces.surfaces, in);
						load_expolygons(ps.fill_expolygons, in);
						load_expolygons(ps.fill_no_overlap_expolygons, in);

						load_polygons(ps.thin_fills, in);
						load_polygons(ps.perimeters, in);
					}
				}
			}
		}

		return true;
	};

	ccglobal::cxndLoad(file_name, f);
}

void save_infills(int object_index, const std::string& prefix, Slic3r::PrintObject* object, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/layer_infills_%d_%s") % directory % object_index % prefix);

	auto f = [&object](std::fstream& out, int version)->bool {
		int layer_count = (int)object->layer_count();
		ccglobal::cxndSaveT(out, layer_count);

		for (int i = 0; i < layer_count; ++i)
		{
			Slic3r::Layer* layer = object->get_layer(i);
			int region_count = (int)layer->region_count();
			ccglobal::cxndSaveT(out, region_count);
			for (int j = 0; j < region_count; ++j)
			{
				Slic3r::Polygons thin_fills_polys = layer->get_region(j)->fills.polygons_covered_by_spacing();
				save_polygons(thin_fills_polys, out);
			}
		}

		return true;
	};

	ccglobal::cxndSave(file_name, 0, f);
}

void load_infills(int object_index, const std::string& prefix, std::vector<std::vector<Slic3r::Polygons>>& infills, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/layer_infills_%d_%s") % directory % object_index % prefix);

	auto f = [&infills](std::fstream& in, int version)->bool {
		int count = 0;
		ccglobal::cxndLoadT(in, count);
		if (count > 0)
		{
			infills.resize(count);
			for (int i = 0; i < count; ++i)
			{
				int region_count = 0;
				ccglobal::cxndLoadT(in, region_count);
				if (region_count > 0)
				{
					infills.at(i).resize(region_count);
					for (int j = 0; j < region_count; ++j)
					{
						load_polygons(infills.at(i).at(j), in);
					}
				}
			}
		}

		return true;
	};

	ccglobal::cxndLoad(file_name, f);
}

void save_curled_extrusions(int object_index, Slic3r::PrintObject* object, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/layer_curled_extrusions_%d") % directory % object_index);

	auto f = [&object](std::fstream& out, int version)->bool {
		int layer_count = (int)object->layer_count();
		ccglobal::cxndSaveT(out, layer_count);

		for (int i = 0; i < layer_count; ++i)
		{
			const Slic3r::CurledLines& lines = object->get_layer(i)->curled_lines;
			int line_count = (int)lines.size();
			ccglobal::cxndSaveT(out, line_count);

			for (int j = 0; j < line_count; ++j)
			{
				const Slic3r::CurledLine& line = lines.at(j);
				ccglobal::cxndSaveT(out, line.curled_height);
				ccglobal::cxndSaveT(out, line.a);
				ccglobal::cxndSaveT(out, line.b);
			}
		}

		return true;
	};

	ccglobal::cxndSave(file_name, 0, f);
}

void load_curled_extrusions(int object_index, std::vector<Slic3r::CurledLines>& curled_lines, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/layer_curled_extrusions_%d") % directory % object_index);

	auto f = [&curled_lines](std::fstream& in, int version)->bool {
		int count = 0;
		ccglobal::cxndLoadT(in, count);
		if (count > 0)
		{
			curled_lines.resize(count);
			for (int i = 0; i < count; ++i)
			{
				int line_count = 0;
				ccglobal::cxndLoadT(in, line_count);
				if (line_count > 0)
				{
					Slic3r::CurledLines& lines = curled_lines.at(i);
					lines.resize(line_count);

					for (int j = 0; j < line_count; ++j)
					{
						ccglobal::cxndLoadT(in, lines.at(j).curled_height);
						ccglobal::cxndLoadT(in, lines.at(j).a);
						ccglobal::cxndLoadT(in, lines.at(j).b);
					}
				}
			}
		}

		return true;
	};

	ccglobal::cxndLoad(file_name, f);
}

void save_seamplacer(Slic3r::SeamPlacer* placer, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/seam_placer") % directory);
	auto f = [&placer](std::fstream& out, int version)->bool {
		std::vector<Slic3r::PrintObjectSeamData*> datas;
		for (std::unordered_map<const Slic3r::PrintObject*, Slic3r::PrintObjectSeamData>::iterator it = placer->m_seam_per_object.begin();
			it != placer->m_seam_per_object.end(); ++it)
		{
			datas.push_back(&it->second);
		}

		int size = (int)datas.size();
		ccglobal::cxndSaveT(out, size);
		for (int i = 0; i < size; ++i)
		{
			Slic3r::PrintObjectSeamData* data = datas.at(i);
			int layer_count = (int)data->layers.size();
			ccglobal::cxndSaveT(out, layer_count);

			for (int j = 0; j < layer_count; ++j)
			{
				const Slic3r::PrintObjectSeamData::LayerSeams& ls = data->layers.at(j);

				int point_count = (int)ls.points.size();
				ccglobal::cxndSaveT(out, point_count);
				for (int k = 0; k < point_count; ++k)
				{
					const Slic3r::SeamPlacerImpl::SeamCandidate& candidate = ls.points.at(k);

					ccglobal::cxndSaveT(out, candidate.position);
					ccglobal::cxndSaveT(out, candidate.embedded_distance);
					ccglobal::cxndSaveT(out, candidate.local_ccw_angle);
					ccglobal::cxndSaveT(out, candidate.overhang);
					ccglobal::cxndSaveT(out, candidate.unsupported_dist);
					ccglobal::cxndSaveT(out, candidate.visibility);
				}

				int perimeter_count = (int)ls.perimeters.size();
				ccglobal::cxndSaveT(out, perimeter_count);
				for (int k = 0; k < perimeter_count; ++k)
				{
					const Slic3r::SeamPlacerImpl::Perimeter& candidate = ls.perimeters.at(k);

					ccglobal::cxndSaveT(out, candidate.start_index);
					ccglobal::cxndSaveT(out, candidate.end_index);
					ccglobal::cxndSaveT(out, candidate.seam_index);
					ccglobal::cxndSaveT(out, candidate.flow_width);
					ccglobal::cxndSaveT(out, candidate.final_seam_position);
				}
			}
		}
		return true;
	};

	ccglobal::cxndSave(file_name, 0, f);
}

void load_seamplacer(std::vector<ObjectSeamCandidate>& datas, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/seam_placer") % directory);

	auto f = [&datas](std::fstream& in, int version)->bool {
		int count = 0;
		ccglobal::cxndLoadT(in, count);
		if (count > 0)
		{
			datas.resize(count);
			for (int i = 0; i < count; ++i)
			{
				ObjectSeamCandidate& object = datas.at(i);
				int l_count = 0;
				ccglobal::cxndLoadT(in, l_count);
				if (l_count > 0)
				{
					object.layers.resize(l_count);
					for (int j = 0; j < l_count; ++j)
					{
						LayerSeamCandidate& layer = object.layers.at(j);
						int p_count = 0;
						ccglobal::cxndLoadT(in, p_count);
						if (p_count > 0)
						{
							layer.points.resize(p_count);
							for (int k = 0; k < p_count; ++k)
							{
								SeamCandidatePoint& candidate = layer.points.at(k);

								ccglobal::cxndLoadT(in, candidate.position);
								ccglobal::cxndLoadT(in, candidate.embedded_distance);
								ccglobal::cxndLoadT(in, candidate.local_ccw_angle);
								ccglobal::cxndLoadT(in, candidate.overhang);
								ccglobal::cxndLoadT(in, candidate.unsupported_dist);
								ccglobal::cxndLoadT(in, candidate.visibility);
							}
						}

						int pe_count = 0;
						ccglobal::cxndLoadT(in, pe_count);
						if (pe_count > 0)
						{
							layer.perimeters.resize(pe_count);
							for (int k = 0; k < pe_count; ++k)
							{
								SeamPerimeter& perimeter = layer.perimeters.at(k);

								ccglobal::cxndLoadT(in, perimeter.start_index);
								ccglobal::cxndLoadT(in, perimeter.end_index);
								ccglobal::cxndLoadT(in, perimeter.seam_index);
								ccglobal::cxndLoadT(in, perimeter.flow_width);
								ccglobal::cxndLoadT(in, perimeter.final_seam_position);
							}
						}
					}
				}
			}
		}

		return true;
	};

	ccglobal::cxndLoad(file_name, f);
}

void save_gcode(const std::string& name, int layer, bool by_object, const std::string& gcode, const std::string& directory)
{
	std::string file_name = boost::str(boost::format("%s/gcode_%d_%s_%s") % directory % layer % name % (by_object ? "by_object" : "by_layer"));

	std::fstream out(file_name, std::ios::out);
	if (!out.is_open())
	{
		out.close();
		return;
	}

	out << gcode;
	out.close();
	return;
}
