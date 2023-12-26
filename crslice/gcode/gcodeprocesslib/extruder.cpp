////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gcode Processor Library
//
// Tools for parsing gcode and calculating printer state from parsed gcode commands.
//
// Copyright(C) 2021 - Brad Hochgesang
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This program is free software : you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU Affero General Public License for more details.
//
//
// You can contact the author at the following email address: 
// FormerLurker@pm.me
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "extruder.h"
#include <iostream>

extruder::extruder()
{
	x_firmware_offset = 0;
	y_firmware_offset = 0;
	z_firmware_offset = 0;
	e = 0;
	e_offset = 0;
	e_relative = 0;
	extrusion_length = 0;
	extrusion_length_total = 0;
	retraction_length = 0;
	deretraction_length = 0;
	is_extruding_start = false;
	is_extruding = false;
	is_primed = false;
	is_retracting_start = false;
	is_retracting = false;
	is_retracted = false;
	is_partially_retracted = false;
	is_deretracting_start = false;
	is_deretracting = false;
	is_deretracted = false;
}

double extruder::get_offset_e() const
{
	return e - e_offset;
}
