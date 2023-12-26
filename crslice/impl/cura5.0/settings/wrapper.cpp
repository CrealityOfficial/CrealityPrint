#include "wrapper.h"
#include "settings/Settings.h"

namespace cura52
{
	SceneParamWrapper::SceneParamWrapper()
	{

	}

	void SceneParamWrapper::initialize(Settings* settings)
	{
		if (!settings)
			return;

		machine_name = settings->get<std::string>("machine_name");
		relative_extrusion = settings->get<bool>("relative_extrusion");
		machine_use_extruder_offset_to_offset_coords = settings->get<bool>("machine_use_extruder_offset_to_offset_coords");
		always_write_active_tool = settings->get<bool>("machine_always_write_active_tool");
		machine_heated_build_volume = settings->get<bool>("machine_heated_build_volume");
		build_volume_temperature = settings->get<Temperature>("build_volume_temperature");

		special_slope_slice_angle = settings->get<double>("special_slope_slice_angle");
		special_slope_slice_axis = settings->get<std::string>("special_slope_slice_axis");
	}
} //namespace cura52


