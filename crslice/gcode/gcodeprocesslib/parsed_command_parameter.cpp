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

#include "parsed_command_parameter.h"
#include "parsed_command.h"
parsed_command_parameter::parsed_command_parameter()
{
	value_type = 'N';
	name.reserve(1);
	name = "";
	unsigned_long_value = 0;
	double_value = 0;
	double_precision = 0;
	string_value;
	string_value = "";
}

parsed_command_parameter::parsed_command_parameter(const std::string name, double value, unsigned char precision) : name(name), double_value(value), double_precision(precision)
{
	value_type = 'F';
}

parsed_command_parameter::parsed_command_parameter(const std::string name, const std::string value) : name(name), string_value(value), double_precision(0)
{
	value_type = 'S';
}

parsed_command_parameter::parsed_command_parameter(const std::string name, const unsigned long value) : name(name), unsigned_long_value(value), double_precision(0)
{
	value_type = 'U';
}
parsed_command_parameter::~parsed_command_parameter()
{

}
