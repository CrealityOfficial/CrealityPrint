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

#include "parsed_command.h"
#include <sstream>
#include <iomanip>
#include <stdlib.h>
parsed_command::parsed_command()
{
	
	command.reserve(8);
	gcode.reserve(128);
	comment.reserve(128);
	parameters.reserve(6);
	is_known_command = false;
	is_empty = true;
}

void parsed_command::clear()
{
	
	command.clear();
	gcode.clear();
	comment.clear();
	parameters.clear();
	is_known_command = false;
	is_empty = true;
}

std::string parsed_command::rewrite_gcode_string()
{
	std::stringstream stream;
	
	// add command
	stream << command;
	if (parameters.size() > 0)
	{
		for (unsigned int index = 0; index < parameters.size(); index++)
		{
			parsed_command_parameter p = parameters[index];
			
			stream << " " << p.name;
			switch (p.value_type)
			{
			case 'S':
				stream << p.string_value;
				break;
			case 'F':
				stream << p.double_value << std::fixed << std::setprecision(p.double_precision);
				break;
			case 'U':
				stream << std::setprecision(0) << p.unsigned_long_value;
				break;
			}
		}
	}
	if (comment.size() > 0)
	{
		stream << ";" << comment;
	}
	return stream.str();
}

std::string parsed_command::to_string()
{
	if (comment.size() > 0)
	{
		return gcode + ";" + comment;
	}
	return gcode;
}

