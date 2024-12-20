// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef MWRAPPER_SETTINGS_SETTINGS_H
#define MWRAPPER_SETTINGS_SETTINGS_H
#include <string>
#include "types/Temperature.h"

namespace cura52
{
	class Settings;
	class MeshParamWrapper
	{
	public:
		MeshParamWrapper();
		~MeshParamWrapper() = default;

		void initialize(Settings* settings);
	};
} //namespace cura52

#endif //MWRAPPER_SETTINGS_SETTINGS_H

