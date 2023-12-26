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
#if _MSC_VER > 1200
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include "logger.h"
logger::logger(std::vector<std::string> names, std::vector<int> levels) {
	// set to true by default, but can be changed by inheritance to support mandatory innitialization (for python or other integrations)
	loggers_created_ = true;
	num_loggers_ = static_cast<int>(names.size());
	logger_names_ = new std::string[static_cast<int>(num_loggers_)];
	logger_levels_ = new int[static_cast<int>(num_loggers_)];
	// this is slow due to the vectors, but it is trivial.  Could switch to an iterator
	
	for (int index = 0; index < num_loggers_; index++)
	{
		logger_names_[index] = names[index];
		logger_levels_[index] = levels[index];
	}
	set_log_level_by_value((int)log_levels::NOSET);
	
}

logger::~logger() {
	delete[] logger_names_;
	delete[] logger_levels_;
}

void logger::set_log_level_by_value(const int logger_type, const int level_value)
{
	logger_levels_[logger_type] = get_log_level_for_value(level_value);
}
void logger::set_log_level_by_value(const int level_value)
{
	int log_level = get_log_level_for_value(level_value);
	for (int type_index = 0; type_index < num_loggers_; type_index++)
	{
		logger_levels_[type_index] = log_level;
	}
}

void logger::set_log_level(const int logger_type, log_levels log_level)
{
	logger_levels_[logger_type] = (int)log_level;
}
std::string logger::get_log_level_name(std::string logger_name)
{
	std::string log_level_name = "UNKNOWN";
	for (int type_index = 0; type_index < num_loggers_; type_index++)
	{
		if (logger_names_[type_index] == logger_name)
		{
			log_level_name = log_level_names[logger_levels_[type_index]];
			break;
		}
	}
	return log_level_name;
}

void logger::set_log_level(log_levels log_level)
{
	for (int type_index = 0; type_index < num_loggers_; type_index++)
	{
		logger_levels_[type_index] = (int)log_level;
	}
}

int logger::get_log_level_value(log_levels log_level)
{
	return log_level_values[(int)log_level];
}
int logger::get_log_level_for_value(int log_level_value)
{
	for (int log_level = 0; log_level < LOG_LEVEL_COUNT; log_level++)
	{
		if (log_level_values[log_level] == log_level_value)
			return log_level;
	}
	return 0;
}
bool logger::is_log_level_enabled(const int logger_type, log_levels log_level)
{
	return logger_levels_[logger_type] <= (int)log_level;
}

void logger::create_log_message(const int logger_type, log_levels log_level, const std::string& message, std::string& output)
{
	// example message
	// 2020-04-20 21:36:59,414 - arc_welder.__init__ - INFO - MESSAGE_GOES_HERE

	// Create the time string in YYYY-MM-DD HH:MM:SS.ms format
	logger::get_timestamp(output);
	// Add a spacer
	output.append(" - ");
	// Add the logger name
	output.append(logger_names_[logger_type]);
	// add a spacer
	output.append(" - ");
	// add the log level name
	output.append(log_level_names[(int)log_level]);
	// add a spacer
	output.append(" - ");
	// add the message
	output.append(message);
}

void logger::log_exception(const int logger_type, const std::string& message)
{
	log(logger_type, log_levels::ERROR, message, true);
}

void logger::log(const int logger_type, log_levels log_level, const std::string& message)
{
	log(logger_type, log_level, message, false);
}

void logger::log(const int logger_type, log_levels log_level, const std::string& message, bool is_exception)
{
	// Make sure the loggers have been initialized
	if (!loggers_created_)
		return;
	// Make sure the current logger is enabled for the log_level
	if (!is_log_level_enabled(logger_type, log_level))
		return;

	// create the log message
	std::string output;
	create_log_message(logger_type, log_level, message, output);

	// write the log
	if (is_exception)
		std::cerr << output << std::endl;
	else
		std::cout << output << std::endl;
	std::cout.flush();
	
}

void logger::get_timestamp(std::string &timestamp)
{
	std::time_t rawtime;
	std::tm* timeinfo;
	char buffer[80];

	std::time(&rawtime);
	timeinfo = std::localtime(&rawtime);
	std::strftime(buffer, 80, "%Y-%m-%d %H:%M:%S.", timeinfo);
	
	timestamp = buffer;
	clock_t t = std::clock();
	int ms = static_cast<int>((t / CLOCKS_PER_MS)) % 1000;

	std::string s_miliseconds;
	sprintf(buffer, "%d", ms) ;// std::to_string(ms);
	s_miliseconds = buffer;
	timestamp.append(std::string(3 - s_miliseconds.length(), '0') + s_miliseconds);
	
}

/*

Severity	Code	Description	Project	File	Line	Suppression State



std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
	const time_t now_time = std::chrono::system_clock::to_time_t(now);
	struct tm  tstruct;
	char buf[25];
	tstruct = *localtime(&now_time);
	// DOESN'T WORK WITH ALL COMPILERS...
	//localtime_s(&tstruct, &now_time);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S.", &tstruct);
	output = buf;
	std::string s_miliseconds = std::to_string(ms.count());
	// Add the milliseconds, padded with 0s, to the output
	output.append(std::string(3 - s_miliseconds.length(), '0') + s_miliseconds);

*/