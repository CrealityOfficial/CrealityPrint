// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef EWRAPPER_SETTINGS_SETTINGS_H
#define EWRAPPER_SETTINGS_SETTINGS_H
#include <string>
#include "types/Temperature.h"

namespace cura52
{
	class Settings;
	class ExtruderParamWrapper
	{
	public:
		ExtruderParamWrapper();
		~ExtruderParamWrapper() = default;

		void initialize(Settings* settings);
	};
} //namespace cura52

#endif //SETTINGS_SETTINGS_H

