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

#ifndef UTILITIES_H
#define UTILITIES_H
#include <string>
#include <vector>
#include <set>
#include <cmath>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include "fpconv.h"

#define FPCONV_BUFFER_LENGTH 25
// Had to increase the zero tolerance because prusa slicer doesn't always 
// retract enough while wiping.
#define ZERO_TOLERANCE 0.000005
#define PI_DOUBLE 3.14159265358979323846264338327950288
#define PI_FLOAT 3.14159265358979323846264338327950288f

namespace utilities{
	extern const std::string WHITESPACE_;
	extern const char GUID_RANGE[];
	extern const bool GUID_DASHES[];
	extern const char PATH_SEPARATOR_;
	bool is_zero(double x, double tolerance);
	bool is_zero(double x);

	int round_up_to_int(double x, double tolerance);
	int round_up_to_int(double x);

	bool is_equal(double x, double y, double tolerance);

	bool is_equal(double x, double y);

	bool greater_than(double x, double y, double tolerance);

	bool greater_than(double x, double y);

	bool greater_than_or_equal(double x, double y, double tolerance);
	bool greater_than_or_equal(double x, double y);

	bool less_than(double x, double y, double tolerance);
	bool less_than(double x, double y);

	bool less_than_or_equal(double x, double y, double tolerance);

	bool less_than_or_equal(double x, double y);

	double get_cartesian_distance(double x1, double y1, double x2, double y2);

	double get_cartesian_distance(double x1, double y1, double z1, double x2, double y2, double z2);

	double get_arc_distance(double x1, double y1, double z1, double x2, double y2, double z2, double i, double j, double r, bool is_clockwise);
	std::string to_string(double value);

	std::string to_string(int value);
	std::string ltrim(const std::string& s);

	std::string rtrim(const std::string& s);

	std::string trim(const std::string& s);
	std::string join(const std::string* strings, size_t length, std::string sep);

	std::string join(const std::vector<std::string> strings, std::string sep);

	// bool contains(const std::string source, const std::string substring); // Might need this later
	std::istream& safe_get_line(std::istream& is, std::string& t);

	std::string center(std::string input, int width);
	double get_percent_change(int v1, int v2);
	double get_percent_change(double v1, double v2);
	std::string get_percent_change_string(int v1, int v2, int precision);

	int get_num_digits(int x);
	int get_num_digits(double x, int precision);

	int get_num_digits(double x);
	// Nice utility function found here: https://stackoverflow.com/questions/8520560/get-a-file-name-from-a-path
	std::vector<std::string> splitpath(const std::string& str);

	bool get_file_path(const std::string& file_path, std::string& path);

	std::string create_uuid();

	bool does_file_exist(const std::string& file_path);

	bool get_temp_file_path_for_file(const std::string& file_path, std::string& temp_file_path);

	double hypot(double x, double y);
	
	float hypotf(float x, float y);

	double atan2(double y, double x);

	float atan2f(float y, float x);

	double floor(double x);

	float floorf(float x);

	double ceil(double x);
	
	float ceilf(float x);

	double cos(double x);

	float cosf(float x);

	double sin(double x);

	float sinf(float x);

	double abs(double x);

	int abs(int x);

	float absf(float x);

	double fabs(double x);

	float fabsf(float x);

	double sqrt(double x);

	float sqrtf(float x);

	double pow(int e, double x);


	double min(double x, double y);
	
	float minf(float x, float y);
	
	double max(double x, double y);
	
	float maxf(float x, float y);

	double radians(double x);
	
	float radiansf(float x);

	double sq(double x);

	float sqf(float x);

	bool within(double n, double l, double h);

	bool withinf(float n, float l, float h);

	double constrain(double value, double arg_min, double arg_max);

	float constrainf(float value, float arg_min, float arg_max);

	double reciprocal(double x);

	float reciprocalf(float x);

	void* memcpy(void* dest, const void* src, size_t n);

	std::string dtos(double x, unsigned char precision);
	
	std::string replace(std::string subject, const std::string& search, const std::string& replace);

	double rand_range(double min, double max);
	unsigned char rand_range(unsigned char min, unsigned char max);
	int rand_range(int min, int max);

	
	
	class box_drawing {
	
	public:
		enum BoxElementEnum { HORIZONTAL = 0, VERTICAL = 1, UPPER_LEFT = 2, UPPER_RIGHT = 3, MIDDLE_LEFT = 4, MIDDLE_RIGHT = 5, LOWER_LEFT = 6, LOWER_RIGHT = 7 };
		enum BoxEncodingEnum {ASCII=0, UTF8=1, HTML=2};
		box_drawing();
		box_drawing(BoxEncodingEnum encoding, int width);
		static const char table_elements_replacement[8];
		static const std::string table_elements_ascii[8];
		static const std::string table_elements_utf8[8];
		static const std::string table_elements_html[8];
		char get_box_replacement_element(BoxElementEnum element);
		void top(std::stringstream& stream);
	    void row(std::stringstream& stream, std::string line);
		void middle(std::stringstream& stream);
		void bottom(std::stringstream& stream);
		void set_box_type(BoxEncodingEnum encoding);
		void make_replacements(std::string &box);

		private:
			std::string table_elements_[8];
			BoxEncodingEnum box_encoding_;
			std::string output_;
			std::stringstream output_stream_;
			int width_;
			
	};

}
#endif