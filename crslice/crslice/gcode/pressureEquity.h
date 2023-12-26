#ifndef CRCOMMON_PRESSUREEQUITY_1690769853657_H
#define CRCOMMON_PRESSUREEQUITY_1690769853657_H
#include "crslice/interface.h"
#include "trimesh2/Vec.h"
#include <string>
#include <vector>

namespace crslice
{
	struct GCodeLine
	{
		trimesh::vec3 start;
		trimesh::vec3 end;

		float currentE;
		float startE;
		float endE;
	};

	class PEDebugger
	{
	public:
		virtual ~PEDebugger() {}

		virtual void onStartLayer(int index) = 0;
		virtual void onLine(const GCodeLine& line) = 0;
	};

	CRSLICE_API void pressureE(const std::vector<std::string>& inputGcodes, std::vector<std::string>& outputGcodes,
		double filament_diameter, float extrusion_rate, float segment_length, bool use_relative_e_distances,
		PEDebugger* debugger = nullptr);
}

#endif