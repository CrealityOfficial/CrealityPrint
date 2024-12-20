#ifndef DEBUG_CACHE_SERIALIZATION_H
#define DEBUG_CACHE_SERIALIZATION_H
#include "libslic3r/Print.hpp"

void cache_slice_scene(const Slic3r::Print& print, const Slic3r::Model& model, const Slic3r::DynamicPrintConfig& config, const std::string& file_name);

void save_print_volume(Slic3r::Print* print, const std::string& directory);
void load_model_volume(int i, int j, const std::string& directory, Slic3r::ModelVolume& volume);

void save_volume_slices(int object_index, const std::vector<Slic3r::VolumeSlices>& slices, const std::string& directory);
void load_volume_slices(int object_index, std::vector<Slic3r::VolumeSlices>& slices, const std::string& directory);
void save_volume_slices(const std::vector<Slic3r::VolumeSlices>& slices, const std::string& file_name);
void load_volume_slices(std::vector<Slic3r::VolumeSlices>& slices, const std::string& file_name);

void save_zs(int index, const std::vector<float>& slices, const std::string& directory);
void load_zs(int index, std::vector<float>& slices, const std::string& directory);

void save_layer_region_surfaces(int object_index, const std::string& prefix, Slic3r::PrintObject* object, const std::string& directory, bool only_fill = false);
void load_layer_region_surfaces(int object_index, const std::string& prefix, std::vector<std::vector<Slic3r::SurfaceCollection>>& surfaces, const std::string& directory);

void save_layer_region_lslices(int object_index, const std::string& prefix, Slic3r::PrintObject* object, const std::string& directory);
void load_layer_region_lslices(int object_index, const std::string& prefix, std::vector<Slic3r::ExPolygons>& lslices, const std::string& directory);

void save_perimeters(int object_index, const std::string& prefix, Slic3r::PrintObject* object, const std::string& directory);

struct PerimeterSets
{
	Slic3r::SurfaceCollection fill_surfaces;
	Slic3r::ExPolygons fill_expolygons;
	Slic3r::ExPolygons fill_no_overlap_expolygons;

	Slic3r::Polygons thin_fills;
	Slic3r::Polygons perimeters;
};

void load_perimeters(int object_index, const std::string& prefix, std::vector<std::vector<PerimeterSets>>& perimeters, const std::string& directory);

void save_infills(int object_index, const std::string& prefix, Slic3r::PrintObject* object, const std::string& directory);
void load_infills(int object_index, const std::string& prefix, std::vector<std::vector<Slic3r::Polygons>>& infills, const std::string& directory);

void save_curled_extrusions(int object_index, Slic3r::PrintObject* object, const std::string& directory);
void load_curled_extrusions(int object_index, std::vector<Slic3r::CurledLines>& curled_lines, const std::string& directory);

void save_seamplacer(Slic3r::SeamPlacer* placer, const std::string& directory);

struct SeamCandidatePoint
{
	Slic3r::Vec3f position;
	float visibility;
	float overhang;
	float unsupported_dist;
	float embedded_distance;
	float local_ccw_angle;
};

struct SeamPerimeter
{
	size_t start_index{};
	size_t end_index{}; //inclusive!
	size_t seam_index{};
	float flow_width{};
	Slic3r::Vec3f final_seam_position = Slic3r::Vec3f::Zero();
};

struct LayerSeamCandidate
{
	std::vector<SeamCandidatePoint> points;
	std::vector<SeamPerimeter> perimeters;
};

struct ObjectSeamCandidate
{
	std::vector<LayerSeamCandidate> layers;
};

void load_seamplacer(std::vector<ObjectSeamCandidate>& datas, const std::string& directory);

void save_gcode(const std::string& name, int layer, bool by_object, const std::string& gcode, const std::string& directory);
#endif // DEBUG_CACHE_SERIALIZATION_H
