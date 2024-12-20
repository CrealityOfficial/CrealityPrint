#ifndef CRSLICE_CACHE_SLICE_DEBUG_H_2
#define CRSLICE_CACHE_SLICE_DEBUG_H_2
#include "crslice2/cacheslice.h"

namespace ccglobal
{
	class VisualDebugger;
}

namespace crslice2
{
	class CacheSliceImpl;
	class CRSLICE2_API CacheSlice
	{
	public:
		CacheSlice();
		~CacheSlice();

		void initialize(const std::string& directory); // used for bench

		bool slice(const CacheSliceParam& param, ccglobal::Tracer* tracer = nullptr);

		void volume_slices(int index, ccglobal::VisualDebugger* debugger);
		void raw_surfaces(int index, ccglobal::VisualDebugger* debugger);
		void concial_overhang(int index, ccglobal::VisualDebugger* debugger);
		void hole_to_polyhole(int index, ccglobal::VisualDebugger* debugger);
		void perimeters(int index, ccglobal::VisualDebugger* debugger);
		void curled_extrusions(int index, ccglobal::VisualDebugger* debugger);
		void surface_type(int index, ccglobal::VisualDebugger* debugger);
		void infill_surface_type(int index, int step, ccglobal::VisualDebugger* debugger);
		void infills(int index, ccglobal::VisualDebugger* debugger);
		void seam(int index, ccglobal::VisualDebugger* debugger);
	protected:
		CacheSliceImpl* m_impl;
	};
}
#endif  // CRSLICE_CACHE_SLICE_DEBUG_H_2
