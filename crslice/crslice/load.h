#ifndef CRSLICE_LOAD_1698397403190_H
#define CRSLICE_LOAD_1698397403190_H
#include "crslice/interface.h"
#include "crslice/header.h"

#define Skeletal_extension ".Skeletal"


typedef std::vector<trimesh::vec3> CrPolygon;
typedef std::vector<CrPolygon> CrPolygons;

namespace crslice
{
	struct CrSkinPart
	{
		CrPolygons outline;
		std::vector<CrPolygons> inset_paths;
		CrPolygons skin_fill;
		CrPolygons roofing_fill;
		CrPolygons top_most_surface_fill;
		CrPolygons bottom_most_surface_fill;
	};

	struct CrSliceLayerPart
	{
		CrPolygons outline;
		CrPolygons inner_area;
		std::vector<CrSkinPart> skin_parts;
		std::vector<CrPolygons> wall_toolpaths;
		std::vector<CrPolygons> infill_wall_toolpaths;
	};

	struct CrSliceLayer
	{
		std::vector<CrSliceLayerPart> parts;
	};

	struct CrSlicedData
	{
		float z = 0;
		CrPolygons polygons;
		CrPolygons open_polygons;
	};

	struct CrSkeletalParam
	{
		double allowed_distance = 0.0;
		double transitioning_angle = 0;
		double discretization_step_size = 0.8;

		double offset_insert = 0.0;
		double scaled_spacing_wall_0 = 0.0;
		double scaled_spacing_wall_X = 0.0;

		double wall_transition_length = 0.0;
		double min_even_wall_line_width = 0.0;
		double wall_line_width_0 = 0.0;
		double wall_split_middle_threshold = 1.0;

		double min_odd_wall_line_width = 0.0;
		double wall_line_width_x = 0.0;
		double wall_add_middle_threshold = 1.0;

		int wall_distribution_count = 0;
		size_t max_bead_count = 0;

		double transition_filter_dist = 0.0;
		double allowed_filter_deviation = 0.0;

		int print_thin_walls = 0;
		double min_feature_size = 0.0;
		double min_bead_width = 0.0;
		double wall_0_inset = 0.0;

		int wall_inset_count = 1;
		double stitch_distance = 0.0;
		double max_resolution = 0.0;
		double max_deviation = 0.0;
		double max_area_deviation = 0.0;
	};

	struct CrExtrusionJunction
	{
		trimesh::vec3 p;
		float w;
		size_t perimeter_index;
	};

	struct CrExtrusionLine
	{
		bool is_odd;
		bool is_closed;
		std::vector<CrExtrusionJunction> junctions;
	};

	typedef std::vector<CrExtrusionLine> CrVariableLines;

	CRSLICE_API void _load(std::fstream& in, CrPolygon& poly);
	CRSLICE_API void _save(std::fstream& out, const CrPolygon& poly);
	CRSLICE_API void _load(std::fstream& in, CrPolygons& polys);
	CRSLICE_API void _save(std::fstream& out, const CrPolygons& polys);
	CRSLICE_API void _load(std::fstream& in, CrSliceLayerPart& part);
	CRSLICE_API void _save(std::fstream& out, const CrSliceLayerPart& part);
	CRSLICE_API void _load(std::fstream& in, CrSkinPart& skin);
	CRSLICE_API void _save(std::fstream& out, const CrSkinPart& skin);
}

#include "ccglobal/serial.h"   // for _load _save

namespace crslice
{
	class CRSLICE_API SerailSlicedData : public ccglobal::Serializeable
	{
	public:
		SerailSlicedData() {}
		virtual ~SerailSlicedData() {}

		int version() override;
		bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
		bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;

		CrSlicedData data;
	};

	class CRSLICE_API SerailCrSliceLayer : public ccglobal::Serializeable
	{
	public:
		SerailCrSliceLayer() {}
		virtual ~SerailCrSliceLayer() {}

		int version() override;
		bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
		bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;

		CrSliceLayer layer;
	};

	class CRSLICE_API SerailCrSkeletal : public ccglobal::Serializeable
	{
	public:
		SerailCrSkeletal() {}
		virtual ~SerailCrSkeletal() {}

		int version() override;
		bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
		bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;

		CrPolygons polygons;
		CrSkeletalParam param;
	};

	class CRSLICE_API SerailDiscretize : public ccglobal::Serializeable
	{
	public:
		SerailDiscretize() {}
		virtual ~SerailDiscretize() {}

		int version() override;
		bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
		bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;

		int type = 0; //0 , 1, 2
		CrPolygon points;
	};

	///file name
	CRSLICE_API std::string crsliceddata_name(const std::string& root, int meshId, int layer);
	CRSLICE_API std::string crslicelayer_name(const std::string& root, int meshId, int layer);
	CRSLICE_API std::string crsliceskeletal_name(const std::string& root, int index);
}

#endif // CRSLICE_LOAD_1698397403190_H