#include "crslice/gcode/pressureEquity.h"
#include "PressureEqualizer.h"

namespace crslice
{
	void pressureE(const std::vector<std::string>& inputGcodes, std::vector<std::string>& outputGcodes,
		double filament_diameter, float extrusion_rate, float segment_length, bool use_relative_e_distances,
		PEDebugger* debugger)
	{
		if (inputGcodes.size() == 0)
		{
			return;
		}

		Slic3r::PressureEqualizer eq(filament_diameter, extrusion_rate, segment_length, use_relative_e_distances);
		eq.setDebugger(debugger);

		Slic3r::LayerResult Lr;

		for (int i = 0; i < inputGcodes.size(); i++)
		{
			Lr.gcode = inputGcodes.at(i);
			Slic3r::LayerResult getLayerResult = eq.process_layer(Lr);
			if (!getLayerResult.gcode.empty())
				outputGcodes.push_back(getLayerResult.gcode);
		}
		outputGcodes.push_back(inputGcodes.at(inputGcodes.size() - 1));
	}
}
