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

#ifndef PARSED_COMMAND_PARAMETER_H
#define PARSED_COMMAND_PARAMETER_H
#include <string>
struct parsed_command_parameter
{
public:
	parsed_command_parameter();
	~parsed_command_parameter();
	parsed_command_parameter(std::string name, double value, unsigned char precision);
	parsed_command_parameter(std::string name, std::string value);
	parsed_command_parameter(std::string name, unsigned long value);
	std::string name;
	unsigned char value_type;
	double double_value;
	unsigned char double_precision;
	unsigned long unsigned_long_value;
	std::string string_value;
};

#endif
