// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef GWRAPPER_SETTINGS_SETTINGS_H
#define GWRAPPER_SETTINGS_SETTINGS_H
#include <string>
#include "types/Temperature.h"
#include "utils/Coord_t.h"
#include "types/EnumSettings.h"

namespace cura52
{
	class Settings;
	class GroupParamWrapper
	{
	public:
		GroupParamWrapper();
		~GroupParamWrapper() = default;

		void initialize(Settings* settings);
		
		bool get_wireframe_enabled() const {
			return wireframe_enabled;
		}
		inline coord_t get_layer_height_0() const{
			return layer_height_0;
		}
		inline coord_t get_layer_height() const {
			return layer_height;
		}
		inline EPlatformAdhesion get_adhesion_type() const {
			return adhesion_type;
		}
	protected:
		bool wireframe_enabled = false;
		coord_t layer_height_0 = 200;
		coord_t layer_height = 200;

		EPlatformAdhesion adhesion_type = EPlatformAdhesion::AUTOBRIM;
	};
} //namespace cura52

#endif //GWRAPPER_SETTINGS_SETTINGS_H

