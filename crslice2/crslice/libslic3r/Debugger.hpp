#ifndef slic3r_Debugger_hpp_
#define slic3r_Debugger_hpp_

#include "libslic3r/Print.hpp"

namespace Slic3r {

	class Debugger
	{
	public:
		virtual ~Debugger() {}

		virtual void volume_slices(Print* print, PrintObject* object, const std::vector<VolumeSlices>& slices, const std::vector<float>& slice_zs) = 0;
		virtual void raw_lslices(Print* print, PrintObject* object) = 0;

		virtual void begin_concial_overhang(Print* print, PrintObject* object) = 0;
		virtual void end_concial_overhang(Print* print, PrintObject* object) = 0;

		virtual void begin_hole_to_polyhole(Print* print, PrintObject* object) = 0;
		virtual void end_hole_to_polyhole(Print* print, PrintObject* object) = 0;

		virtual void perimeters(Print* print, PrintObject* object) = 0;
		virtual void curled_extrusions(Print* print, PrintObject* object) = 0;

		virtual void surfaces_type(Print* print, PrintObject* object) = 0;
		virtual void infill_surfaces_type(Print* print, PrintObject* object, int step) = 0;

		virtual void infills(Print* print, PrintObject* object) = 0;

		virtual void seamplacer(Print* print, SeamPlacer* placer) = 0;
		virtual void gcode(const std::string& name, int layer, bool by_object, const std::string& gcode) = 0;
	};
}

#endif
