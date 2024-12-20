#include "Debugger.hpp"

namespace Slic3r {
	void bench_debug_volume_slices(Print* print, PrintObject* object, const std::vector<VolumeSlices>& slices, const std::vector<float>& slice_zs)
	{
#ifdef BENCH_DEBUG
		print->debugger->volume_slices(print, object, slices, slice_zs);
#endif
	}

	void bench_debug_lslices(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->raw_lslices(print, object);
#endif
	}

	void begin_debug_concial_overhang(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->begin_concial_overhang(print, object);
#endif
	}

	void end_debug_concial_overhang(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->end_concial_overhang(print, object);
#endif
	}

	void begin_debug_hole_to_polyhole(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->begin_hole_to_polyhole(print, object);
#endif
	}

	void end_debug_hole_to_polyhole(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->end_hole_to_polyhole(print, object);
#endif
	}

	void debug_perimeters(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->perimeters(print, object);
#endif
	}

	void debug_estimate_curled_extrusions(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->curled_extrusions(print, object);
#endif
	}

	void debug_surfaces_type(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->surfaces_type(print, object);
#endif
	}

	void debug_infill_surfaces_type(Print* print, PrintObject* object, int step)
	{
#ifdef BENCH_DEBUG
		print->debugger->infill_surfaces_type(print, object, step);
#endif
	}

	void debug_infills(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->infills(print, object);
#endif
	}

	void bench_debug_generate(Print* print, int layer, const std::string& code, bool by_object)
	{
#ifdef BENCH_DEBUG
		print->debugger->gcode("generate", layer, by_object, code);
#endif
	}

	void bench_debug_cooling(Print* print, int layer, const std::string& code, bool by_object)
	{
#ifdef BENCH_DEBUG
		print->debugger->gcode("cooling", layer, by_object, code);
#endif
	}

	void bench_debug_fanmove(Print* print, int layer, const std::string& code, bool by_object)
	{
#ifdef BENCH_DEBUG
		print->debugger->gcode("fanmove", layer, by_object, code);
#endif
	}

	void bench_debug_output(Print* print, int layer, const std::string& code, bool by_object)
	{
#ifdef BENCH_DEBUG
		print->debugger->gcode("output", layer, by_object, code);
#endif
	}

	void bench_debug_seamplacer(Print* print, SeamPlacer* placer)
	{
#ifdef BENCH_DEBUG
		print->debugger->seamplacer(print, placer);
#endif
	}
}
