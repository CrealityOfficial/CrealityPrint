#include "gwrapper.h"
#include "settings/Settings.h"

namespace cura52
{
	GroupParamWrapper::GroupParamWrapper()
	{

	}

	void GroupParamWrapper::initialize(Settings* settings)
	{
		if (!settings)
			return;

		wireframe_enabled = settings->get<bool>("wireframe_enabled");

		layer_height_0 = settings->get<coord_t>("layer_height_0");
		layer_height = settings->get<coord_t>("layer_height");

		adhesion_type = settings->get<EPlatformAdhesion>("adhesion_type");
	}
} //namespace cura52


