#include "crslice2/cacheslicedebug.h"
#include "crslice2/crscene.h"
#include "../wrapper/orcaslicewrapper.h"
#include "../wrapper/serialization.h"
#include "libslic3r/I18N.hpp"
#include "libslic3r/BuildVolume.hpp"
#include "libslic3r/Debugger.hpp"

#include "conv.h"

#include <boost/format.hpp>
#include "crsliceexception.h"
#include "ccglobal/profile.h"

#ifdef BENCH_DEBUG
#include "ccglobal/debugger.h"
#endif

namespace crslice2
{
	int index_object(Slic3r::Print* print, Slic3r::PrintObject* object)
	{
		int index = 0;
		for (Slic3r::PrintObject* obj : print->objects_mutable())
		{
			if (obj == object)
			{
				return index;
			}
			++index;
		}
		return -1;
	}

	trimesh::vec4 indexColor(int index)
	{
		trimesh::vec4 colors[6] = {
			trimesh::vec4(1.0f, 0.0f, 0.0f, 1.0f),
			trimesh::vec4(0.0f, 1.0f, 0.0f, 1.0f),
			trimesh::vec4(0.0f, 0.0f, 1.0f, 1.0f),
			trimesh::vec4(1.0f, 1.0f, 0.0f, 1.0f),
			trimesh::vec4(1.0f, 0.0f, 1.0f, 1.0f),
			trimesh::vec4(0.0f, 1.0f, 1.0f, 1.0f)
		};

		return colors[index % 6];
	}

	trimesh::vec4 surfaceTypeColor(Slic3r::SurfaceType type)
	{
		trimesh::vec4 colors[Slic3r::stCount] = {
			trimesh::vec4(1.0f, 0.0f, 0.0f, 1.0f),
			trimesh::vec4(0.0f, 1.0f, 0.0f, 1.0f),
			trimesh::vec4(1.0f, 1.0f, 0.0f, 1.0f),
			trimesh::vec4(0.0f, 0.0f, 1.0f, 1.0f),
			trimesh::vec4(1.0f, 1.0f, 1.0f, 1.0f),
			trimesh::vec4(0.0f, 0.0f, 0.0f, 1.0f),
			trimesh::vec4(1.0f, 1.0f, 1.0f, 1.0f),
			trimesh::vec4(0.5f, 0.5f, 0.5f, 1.0f)
		};
		return colors[(int)type % (int)Slic3r::stCount];
	}

	float surfaceTypeWidth(Slic3r::SurfaceType type)
	{
		float widths[Slic3r::stCount] = {
			2.0f,
			2.0f,
			1.0f,
			1.0f,
			1.0f,
			1.0f,
			1.0f,
			1.0f
		};
		return widths[(int)type % (int)Slic3r::stCount];
	}

	std::string getStepPrefix(int step)
	{
		std::string prefixes[5] = {
			"infill_raw",
			"infill_external",
			"infill_clip",
			"infill_bridge",
			"infill_combine"
		};

		return prefixes[step % 5];
	}
	
	class CacheSliceImpl : public Slic3r::Debugger
	{
	public:
		CacheSliceImpl() {
			m_object_slice_zs.resize(10);
		}

		~CacheSliceImpl() {

		}

		void load_cache(const std::string& _directory)
		{
			directory = _directory;
			for (int i = 0; i < 10; ++i)
				load_zs(i, m_object_slice_zs.at(i), directory);
		}

		void volume_slices(Slic3r::Print* print, Slic3r::PrintObject* object, const std::vector<Slic3r::VolumeSlices>& slices, const std::vector<float>& slice_zs) override
		{
			save_print_volume(print, directory);

			save_volume_slices(index_object(print, object), slices, directory);
			save_zs(index_object(print, object), slice_zs, directory);

			m_object_slice_zs.at(index_object(print, object)) = slice_zs;
		}

		void raw_lslices(Slic3r::Print* print, Slic3r::PrintObject* object)
		{
			save_layer_region_surfaces(index_object(print, object), "raw", object, directory);
		}

		void begin_concial_overhang(Slic3r::Print* print, Slic3r::PrintObject* object)
		{
			save_layer_region_surfaces(index_object(print, object), "before_concial_overhang", object, directory);
		}

		void end_concial_overhang(Slic3r::Print* print, Slic3r::PrintObject* object)
		{
			save_layer_region_surfaces(index_object(print, object), "after_concial_overhang", object, directory);
		}

		void begin_hole_to_polyhole(Slic3r::Print* print, Slic3r::PrintObject* object)
		{
			save_layer_region_lslices(index_object(print, object), "before_hole_to_polyhole", object, directory);
		}

		void end_hole_to_polyhole(Slic3r::Print* print, Slic3r::PrintObject* object)
		{
			save_layer_region_lslices(index_object(print, object), "after_hole_to_polyhole", object, directory);
		}

		void perimeters(Slic3r::Print* print, Slic3r::PrintObject* object)
		{
			save_perimeters(index_object(print, object), "raw", object, directory);
		}

		void curled_extrusions(Slic3r::Print* print, Slic3r::PrintObject* object)
		{
			save_curled_extrusions(index_object(print, object), object, directory);
		}

		void surfaces_type(Slic3r::Print* print, Slic3r::PrintObject* object)
		{
			save_layer_region_surfaces(index_object(print, object), "all", object, directory);
		}

		void infill_surfaces_type(Slic3r::Print* print, Slic3r::PrintObject* object, int step)
		{
			save_layer_region_surfaces(index_object(print, object), getStepPrefix(step), object, directory, true);
		}

		void infills(Slic3r::Print* print, Slic3r::PrintObject* object)
		{
			save_infills(index_object(print, object), "infills", object, directory);
		}

		void seamplacer(Slic3r::Print* print, Slic3r::SeamPlacer* placer)
		{
			save_seamplacer(placer, directory);
		}

		void gcode(const std::string& name, int layer, bool by_object, const std::string& gcode)
		{
			save_gcode(name, layer, by_object, gcode, directory);
		}

		float slice_z(int object_index, int layer)
		{
			if (layer >= 0 && layer < layer_count(object_index))
				return m_object_slice_zs.at(object_index).at(layer);
			return 0.0f;
		}

		int layer_count(int object_index)
		{
			return (int)m_object_slice_zs.at(object_index).size();
		}

		std::vector<std::vector<float>> m_object_slice_zs;
		std::string directory;
	};

	CacheSlice::CacheSlice()
		:m_impl(new CacheSliceImpl())
	{

	}

	CacheSlice::~CacheSlice()
	{
		delete m_impl;
	}

	void CacheSlice::initialize(const std::string& directory)
	{
		m_impl->load_cache(directory);
	}

	bool CacheSlice::slice(const CacheSliceParam& param, ccglobal::Tracer* tracer)
	{
		SYSTEM_TICK("load scene->");
		CrScenePtr scene(new crslice2::CrScene());
		scene->load(param.fileName);
		SYSTEM_TICK("load scene<-");

		if (scene->m_groups.size() == 0)
		{
			return false;
		}

		Slic3r::Print print;
		OrcaResult result;
		result.gcode_result.reset();

		print.debugger = m_impl;
		m_impl->directory = param.tempDirectory;

		Slic3r::Model model;
		Slic3r::DynamicPrintConfig config;

		Slic3r::Calib_Params calibParams;
		calibParams.end = 0.0;
		calibParams.start = 0.0;
		calibParams.step = 0.0;
		calibParams.print_numbers = false;
		Slic3r::ThumbnailsList thumbnailData;

		SYSTEM_TICK("convert ->");
		convert_scene_2_orca(scene, model, config, calibParams, thumbnailData);
		SYSTEM_TICK("convert <-");

		OrcaResult orca_result;
		Slic3r::Calib_Params calib_params;
		auto f = [&](const Slic3r::ThumbnailsParams&) { return Slic3r::ThumbnailsList(); };
		Slic3r::ThumbnailsGeneratorCallback thumbnail_callback = f;

		try
		{
			orca_slice_impl_result(print, model, config, param.outName, "./", thumbnail_callback, calib_params, orca_result, tracer);
		}
		catch (const crslice2::CrSliceException& e)
		{
			_handle_slice_exception_ex(scene, e.sliceObjectId(), e.what(), tracer);
			return false;
		}

		(void)orca_result;
		return true;
	}

	void CacheSlice::volume_slices(int index, ccglobal::VisualDebugger* debugger)
	{
#ifdef BENCH_DEBUG
		if (!debugger)
			return;

		debugger->clear_visual("volume_slices*");

		std::vector<Slic3r::VolumeSlices> volume_slices;
		load_volume_slices(0, volume_slices, m_impl->directory);

		int volume_count = (int)volume_slices.size();
		for (int i = 0; i < volume_count; ++i)
		{
			const Slic3r::VolumeSlices& vs = volume_slices.at(i);
			int layer_count = (int)vs.slices.size();
			for (int j = 0; j < layer_count; ++j)
			{
				if (index >= 0 && j != index)
					continue;
		
				std::string name = boost::str(boost::format("volume_slices_%d_%d") % i % j);
		
				CovertParam param;
				param.z = m_impl->slice_z(0, j);
				std::vector<trimesh::vec3> lines;
				std::vector<trimesh::vec4> colors;
				convert(vs.slices.at(j), lines, &colors, param);
				debugger->visual_color_polygon(name, lines, colors, 1.0f);
			}
		}
#endif
	}

	struct VisualSurfaceParam
	{
		bool visual_absolute = false;
		trimesh::vec4 color = trimesh::vec4();
		bool as_type = false;

		float width = 1.0f;
	};

	void visual_polygons(const Slic3r::Polygons& polys, const std::string& name, ccglobal::VisualDebugger* debugger, const CovertParam& param = CovertParam())
	{
		if (!debugger)
			return;

		std::vector<trimesh::vec3> lines;
		std::vector<trimesh::vec4> colors;
		convert(polys, lines, &colors, param);
		debugger->visual_color_polygon(name, lines, colors, 1.0f);
	}

	void visual_expolygons(const Slic3r::ExPolygons& polys, const std::string& name, ccglobal::VisualDebugger* debugger, const CovertParam& param = CovertParam())
	{
		if (!debugger)
			return;

		std::vector<trimesh::vec3> lines;
		std::vector<trimesh::vec4> colors;
		convert(polys, lines, &colors, param);
		debugger->visual_color_polygon(name, lines, colors, param.width);
	}

	void visual_expolygon(const Slic3r::ExPolygon& poly, const std::string& name, ccglobal::VisualDebugger* debugger, const CovertParam& param = CovertParam())
	{
		if (!debugger)
			return;

		std::vector<trimesh::vec3> lines;
		std::vector<trimesh::vec4> colors;

		lines.clear();
		colors.clear();
		append(poly, lines, &colors, param);
		debugger->visual_color_polygon(name, lines, colors, param.width);
	}

	void visual_lslices(const std::vector<Slic3r::ExPolygons>& polys, const std::string& prefix, int index, ccglobal::VisualDebugger* debugger, CacheSliceImpl* impl,
		const VisualSurfaceParam& vparam = VisualSurfaceParam())
	{
		int layer_count = (int)polys.size();
		for (int i = 0; i < layer_count; ++i)
		{
			if (index >= 0 && i != index)
				continue;

			float z = impl->slice_z(0, i);
			const Slic3r::ExPolygons& poly = polys.at(i);

			std::string name = boost::str(boost::format("%s_%d") % prefix % i);

			CovertParam param;
			if (vparam.visual_absolute)
			{
				param.out = vparam.color;
				param.in = vparam.color;
			}
			param.z = impl->slice_z(0, i);
			param.width = vparam.width;

			visual_expolygons(polys.at(i), name, debugger, param);
		}
	}

	void visual_surfaces(const std::vector<std::vector<Slic3r::SurfaceCollection>>& surfaces, const std::string& prefix, int index, ccglobal::VisualDebugger* debugger, CacheSliceImpl* impl,
		const VisualSurfaceParam& vparam = VisualSurfaceParam())
	{
		int layer_count = (int)surfaces.size();
		for (int i = 0; i < layer_count; ++i)
		{
			if (index >= 0 && i != index)
				continue;

			float z = impl->slice_z(0, i);
			const std::vector<Slic3r::SurfaceCollection>& scs = surfaces.at(i);
			int region_count = (int)scs.size();
			for (int j = 0; j < region_count; ++j)
			{
				const Slic3r::SurfaceCollection& sc = scs.at(j);
				int index = 0;
				for (const Slic3r::Surface& surf : sc.surfaces)
				{
					std::string name = boost::str(boost::format("%s_%d_%d_%d") % prefix % i % j % index);

					CovertParam param;
					param.in = vparam.visual_absolute ? vparam.color : indexColor(index);
					if (vparam.as_type)
					{
						param.in = surfaceTypeColor(surf.surface_type);
					}
					param.width = surfaceTypeWidth(surf.surface_type);
					param.out = param.in;
					param.z = z;

					visual_expolygon(surf.expolygon, name, debugger, param);
					++index;
				}
			}
		}
	}

	void CacheSlice::raw_surfaces(int index, ccglobal::VisualDebugger* debugger)
	{
#ifdef BENCH_DEBUG
		if (!debugger)
			return;

		debugger->clear_visual("raw_surfaces*");

		std::vector<std::vector<Slic3r::SurfaceCollection>> layer_region_slices;
		load_layer_region_surfaces(0, "raw", layer_region_slices, m_impl->directory);

		VisualSurfaceParam param;
		param.visual_absolute = true;
		param.color = trimesh::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		visual_surfaces(layer_region_slices, "raw_surfaces", index, debugger, m_impl, param);
#endif
	}

	void CacheSlice::concial_overhang(int index, ccglobal::VisualDebugger* debugger)
	{
#ifdef BENCH_DEBUG
		if (!debugger)
			return;

		debugger->clear_visual("before_concial_overhang*");
		debugger->clear_visual("after_concial_overhang*");

		std::vector<std::vector<Slic3r::SurfaceCollection>> before_concial_overhang;
		load_layer_region_surfaces(0, "before_concial_overhang", before_concial_overhang, m_impl->directory);
		std::vector<std::vector<Slic3r::SurfaceCollection>> after_concial_overhang;
		load_layer_region_surfaces(0, "after_concial_overhang", after_concial_overhang, m_impl->directory);

		VisualSurfaceParam param;
		param.visual_absolute = true;
		param.color = trimesh::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		visual_surfaces(before_concial_overhang, "before_concial_overhang", index, debugger, m_impl, param);
		param.color = trimesh::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		visual_surfaces(after_concial_overhang, "after_concial_overhang", index, debugger, m_impl, param);
#endif
	}

	void CacheSlice::hole_to_polyhole(int index, ccglobal::VisualDebugger* debugger)
	{
#ifdef BENCH_DEBUG
		if (!debugger)
			return;

		debugger->clear_visual("before_hole_to_polyhole*");
		debugger->clear_visual("after_hole_to_polyhole*");

		std::vector<Slic3r::ExPolygons> before_hole_to_polyhole;
		load_layer_region_lslices(0, "before_hole_to_polyhole", before_hole_to_polyhole, m_impl->directory);
		std::vector<Slic3r::ExPolygons> after_hole_to_polyhole;
		load_layer_region_lslices(0, "after_hole_to_polyhole", after_hole_to_polyhole, m_impl->directory);

		VisualSurfaceParam param;
		param.visual_absolute = true;
		param.color = trimesh::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		visual_lslices(before_hole_to_polyhole, "before_hole_to_polyhole", index, debugger, m_impl, param);
		param.color = trimesh::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		visual_lslices(after_hole_to_polyhole, "after_hole_to_polyhole", index, debugger, m_impl, param);
#endif
	}

	void CacheSlice::perimeters(int index, ccglobal::VisualDebugger* debugger)
	{
#ifdef BENCH_DEBUG
		if (!debugger)
			return;

		debugger->clear_visual("perimeters*");

		std::vector<std::vector<PerimeterSets>> perimeters_sets;
		load_perimeters(0, "raw", perimeters_sets, m_impl->directory);

		int layer_count = (int)perimeters_sets.size();
		for (int i = 0; i < layer_count; ++i)
		{
			if (index >= 0 && i != index)
				continue;

			float z = m_impl->slice_z(0, i);
			CovertParam param;
			param.z = z;
			param.out = trimesh::vec4(1.0f, 0.0f, 0.0f, 1.0f);
			param.in = param.out;

			const std::vector<PerimeterSets>& region_peri = perimeters_sets.at(i);
			int region_count = (int)region_peri.size();
			for (int j = 0; j < region_count; ++j)
			{
				const PerimeterSets& peri = region_peri.at(j);

				visual_expolygons(peri.fill_expolygons, boost::str(boost::format("perimeters_fill_expolygons_%d_%d") % i % j), debugger, param);

				param.out = trimesh::vec4(0.0f, 1.0f, 0.0f, 1.0f);
				param.in = param.out;
				visual_expolygons(peri.fill_no_overlap_expolygons, boost::str(boost::format("perimeters_fill_no_overlap_expolygons_%d_%d") % i % j), debugger, param);

				param.out = trimesh::vec4(1.0f, 1.0f, 0.0f, 1.0f);
				param.in = param.out;
				visual_polygons(peri.thin_fills, boost::str(boost::format("perimeters_thin_fills_%d_%d") % i % j), debugger, param);

				param.out = trimesh::vec4(0.0f, 1.0f, 1.0f, 1.0f);
				param.in = param.out;
				visual_polygons(peri.perimeters, boost::str(boost::format("perimeters_perimeters_%d_%d") % i % j), debugger, param);
			}
		}
#endif
	}

	void CacheSlice::curled_extrusions(int index, ccglobal::VisualDebugger* debugger)
	{
#ifdef BENCH_DEBUG
		if (!debugger)
			return;

		debugger->clear_visual("curled_extrusions*");

		std::vector<Slic3r::CurledLines> curled_lines;
		load_curled_extrusions(0, curled_lines, m_impl->directory);

		int layer_count = (int)curled_lines.size();
		for (int i = 0; i < layer_count; ++i)
		{
			if (index >= 0 && i != index)
				continue;

			float z = m_impl->slice_z(0, i);

			std::vector<trimesh::vec3> lines;
			std::vector<trimesh::vec4> colors;
			const Slic3r::CurledLines& cls = curled_lines.at(i);
			for (const Slic3r::CurledLine& l : cls)
			{

				lines.push_back(convert(l.a, z));
				lines.push_back(convert(l.b, z));
				colors.push_back(trimesh::vec4(1.0f, 0.0f, 1.0f, 1.0f));
				colors.push_back(trimesh::vec4(1.0f, 0.0f, 1.0f, 1.0f));
			}
			debugger->visual_color_polygon(boost::str(boost::format("curled_extrusions_%d") % i), lines, colors, 1.0f);
		}
#endif
	}

	void CacheSlice::surface_type(int index, ccglobal::VisualDebugger* debugger)
	{
#ifdef BENCH_DEBUG
		if (!debugger)
			return;

		debugger->clear_visual("all_surfaces*");

		std::vector<std::vector<Slic3r::SurfaceCollection>> layer_region_slices;
		load_layer_region_surfaces(0, "all", layer_region_slices, m_impl->directory);

		VisualSurfaceParam param;
		param.as_type = true;
		visual_surfaces(layer_region_slices, "all_surfaces", index, debugger, m_impl, param);
#endif
	}

	void CacheSlice::infill_surface_type(int index, int step, ccglobal::VisualDebugger* debugger)
	{
#ifdef BENCH_DEBUG
		if (!debugger)
			return;

		std::string prefix = getStepPrefix(step);

		std::string n = prefix + "*";
		debugger->clear_visual(n);

		std::vector<std::vector<Slic3r::SurfaceCollection>> layer_region_slices;
		load_layer_region_surfaces(0, prefix, layer_region_slices, m_impl->directory);

		VisualSurfaceParam param;
		param.as_type = true;
		visual_surfaces(layer_region_slices, prefix, index, debugger, m_impl, param);

#endif
	}

	void CacheSlice::infills(int index, ccglobal::VisualDebugger* debugger)
	{
#ifdef BENCH_DEBUG
		if (!debugger)
			return;

		debugger->clear_visual("path_infills*");

		std::vector<std::vector<Slic3r::Polygons>> infills;
		load_infills(0, "infills", infills, m_impl->directory);

		int layer_count = (int)infills.size();
		for (int i = 0; i < layer_count; ++i)
		{
			if (index >= 0 && i != index)
				continue;

			float z = m_impl->slice_z(0, i);
			CovertParam param;
			param.z = z;
			param.out = trimesh::vec4(1.0f, 0.0f, 0.0f, 1.0f);
			param.in = param.out;

			const std::vector<Slic3r::Polygons>& region_infills = infills.at(i);
			int region_count = (int)region_infills.size();
			for (int j = 0; j < region_count; ++j)
			{
				const Slic3r::Polygons& _infills = region_infills.at(j);

				param.out = trimesh::vec4(0.0f, 1.0f, 1.0f, 1.0f);
				param.in = param.out;
				visual_polygons(_infills, boost::str(boost::format("path_infills_%d_%d") % i % j), debugger, param);
			}
		}

#endif
	}

	void CacheSlice::seam(int index, ccglobal::VisualDebugger* debugger)
	{
#ifdef BENCH_DEBUG
		if (!debugger)
			return;

		debugger->clear_visual("seam*");

		std::vector<ObjectSeamCandidate> seams;
		load_seamplacer(seams, m_impl->directory);

		if (seams.size() == 0)
			return;


		int layer_count = (int)seams.at(0).layers.size();
		for (int i = 0; i < layer_count; ++i)
		{
			if (index >= 0 && i != index)
				continue;

			const LayerSeamCandidate& layer = seams.at(0).layers.at(i);
			float z = m_impl->slice_z(0, i);

			std::vector<trimesh::vec3> can_points;
			for (const SeamCandidatePoint& point : layer.points)
				can_points.push_back(convert(point.position));
			std::vector<trimesh::vec3> seam_points;
			for (const SeamPerimeter& perimeter : layer.perimeters)
				seam_points.push_back(convert(layer.points[perimeter.seam_index].position));

			debugger->visual_color_points(boost::str(boost::format("seam_candi_%d") % i), can_points, trimesh::vec4(0.0f, 1.0f, 0.0f, 1.0f), 3.0f);
			debugger->visual_color_points(boost::str(boost::format("seam_seam_%d") % i), seam_points, trimesh::vec4(1.0f, 0.0f, 0.0f, 1.0f), 10.0f);
		}

#endif
	}
}