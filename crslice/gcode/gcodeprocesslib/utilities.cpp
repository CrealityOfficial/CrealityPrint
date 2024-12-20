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

#include "utilities.h"

namespace utilities {
	// Box Drawing Consts
	// String Consts
	// Note:  these ascii replacement characters must NOT appear in the text unless they will be replaced with box characters.
	const char box_drawing::table_elements_replacement[8] = {char(128),char(129),char(130),char(131),char(132),char(133),char(134),char(135) };
	//enum BoxElementEnum { HORIZONTAL = 0, VERTICAL = 1, UPPER_LEFT = 2, UPPER_RIGHT = 3, MIDDLE_LEFT = 4, MIDDLE_RIGHT = 5, LOWER_LEFT = 6, LOWER_RIGHT = 7 };
	const std::string box_drawing::table_elements_ascii[8] = {"-","|","+" ,"+" ,"+" ,"+" ,"+" ,"+" };
	const std::string box_drawing::table_elements_utf8[8] = { "\u2500","\u2502","\u250C" ,"\u2510" ,"\u251C" ,"\u2524" ,"\u2514" ,"\u2518" };
	const std::string box_drawing::table_elements_html[8] = { "&#9472","&#9474", "&#9484","&#9488","&#9500","&#9508","&#9492","&#9496" };
	
	box_drawing::box_drawing()
	{
		set_box_type(BoxEncodingEnum::ASCII);

		width_ = 100;
	}
	box_drawing::box_drawing(BoxEncodingEnum encoding, int width)
	{
		set_box_type(encoding);
		width_ = width;
	}

	void box_drawing::set_box_type(BoxEncodingEnum encoding)
	{
		box_encoding_ = encoding;
		for (int index = 0; index < 8; index++)
		{
			switch (box_encoding_)
			{
			case  BoxEncodingEnum::ASCII:
				table_elements_[index] = table_elements_ascii[index];
				break;
			case BoxEncodingEnum::UTF8:
				table_elements_[index] = table_elements_utf8[index];
				break;
			case BoxEncodingEnum::HTML:
				table_elements_[index] = table_elements_html[index];
				break;
			default:
				table_elements_[index] = table_elements_ascii[index];
			}
			
		}
	}

	void box_drawing::top(std::stringstream& stream)
	{
		stream << get_box_replacement_element(BoxElementEnum::UPPER_LEFT) << std::setw(width_) << std::setfill(get_box_replacement_element(BoxElementEnum::HORIZONTAL)) << "" << get_box_replacement_element(BoxElementEnum::UPPER_RIGHT) << "\n" << std::setfill(' ');
	}

	void box_drawing::row(std::stringstream& stream, std::string line)
	{
		stream << get_box_replacement_element(BoxElementEnum::VERTICAL) << line << get_box_replacement_element(BoxElementEnum::VERTICAL) << "\n" << std::setfill(' ');
	}

	void box_drawing::middle(std::stringstream& stream)
	{
		stream << get_box_replacement_element(BoxElementEnum::MIDDLE_LEFT) << std::setw(width_) << std::setfill(get_box_replacement_element(BoxElementEnum::HORIZONTAL)) << "" << get_box_replacement_element(BoxElementEnum::MIDDLE_RIGHT) << "\n" << std::setfill(' ');
	}
	void box_drawing::bottom(std::stringstream& stream)
	{
		stream << get_box_replacement_element(BoxElementEnum::LOWER_LEFT) << std::setw(width_) << std::setfill(get_box_replacement_element(BoxElementEnum::HORIZONTAL)) << "" << get_box_replacement_element(BoxElementEnum::LOWER_RIGHT) << "\n" << std::setfill(' ');
	}
	
	char box_drawing::get_box_replacement_element(BoxElementEnum element)
	{
		return table_elements_replacement[(int)element];
	}

	void box_drawing::make_replacements(std::string &box)
	{
		for (int index = 0; index < 8; index++)
		{
			char c = table_elements_replacement[index];
			std::string search;
			search += c;
			box = utilities::replace(box, search, table_elements_[index]);
		}

	}

	


	const std::string WHITESPACE_ = " \n\r\t\f\v";
	const char GUID_RANGE[] = "0123456789abcdef";
	const bool GUID_DASHES[] = { 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0 };

	extern const char PATH_SEPARATOR_ =
#ifdef _WIN32
		'\\';
#else
		'/';
#endif

}

bool utilities::is_zero(double x, double tolerance)
{
	return utilities::abs(x) < tolerance;
}
bool utilities::is_zero(double x)
{
	return utilities::abs(x) < ZERO_TOLERANCE;
}

int utilities::round_up_to_int(double x, double tolerance)
{
	return int(x + tolerance);
}

int utilities::round_up_to_int(double x)
{
	return int(x + ZERO_TOLERANCE);
}

bool utilities::is_equal(double x, double y, double tolerance)
{
	double abs_difference = utilities::abs(x - y);
	return abs_difference < tolerance;
}

bool utilities::is_equal(double x, double y)
{
	double abs_difference = utilities::abs(x - y);
	return abs_difference < ZERO_TOLERANCE;
}

bool utilities::greater_than(double x, double y, double tolerance)
{
	return x > y && !is_equal(x, y, tolerance);
}

bool utilities::greater_than(double x, double y)
{
	return x > y && !is_equal(x, y);
}

bool utilities::greater_than_or_equal(double x, double y, double tolerance)
{
	return x > y || is_equal(x, y, tolerance);
}

bool utilities::greater_than_or_equal(double x, double y)
{
	return x > y || is_equal(x, y);
}

bool utilities::less_than(double x, double y, double tolerance)
{
	return x < y && !is_equal(x, y, tolerance);
}

bool utilities::less_than(double x, double y)
{
	return x < y && !is_equal(x, y);
}

bool utilities::less_than_or_equal(double x, double y, double tolerance)
{
	return x < y || is_equal(x, y, tolerance);
}

bool utilities::less_than_or_equal(double x, double y)
{
	return x < y || is_equal(x, y);
}


double utilities::get_cartesian_distance(double x1, double y1, double x2, double y2)
{
	// Compare the saved points cartesian distance from the current point
	double xdif = x1 - x2;
	double ydif = y1 - y2;
	double dist_squared = xdif * xdif + ydif * ydif;
	return utilities::sqrt(dist_squared);
}

double utilities::get_cartesian_distance(double x1, double y1, double z1, double x2, double y2, double z2)
{
	// Compare the saved points cartesian distance from the current point
	double xdif = x1 - x2;
	double ydif = y1 - y2;
	double zdif = z1 - z2;
	double dist_squared = xdif * xdif + ydif * ydif + zdif * zdif;
	return utilities::sqrt(dist_squared);
}

double utilities::get_arc_distance(double x1, double y1, double z1, double x2, double y2, double z2, double i, double j, double r, bool is_clockwise)
{
	double center_x = x1 - i;
	double center_y = y1 - j;
	double radius = utilities::hypot(i, j);
	double z_dist = z2 - z1;
	double rt_x = x2 - center_x;
	double rt_y = y2 - center_y;
	double angular_travel_total = utilities::atan2(i * rt_y - j * rt_x, i * rt_x + j * rt_y);
	if (angular_travel_total < 0) { angular_travel_total += 2.0 * PI_DOUBLE; }
	// Adjust the angular travel if the direction is clockwise
	if (is_clockwise) { angular_travel_total -= 2.0 * PI_DOUBLE; }
	// Full circle fix.
	if (x1 == x2 && y1 == y2 && angular_travel_total == 0)
	{
		angular_travel_total += 2.0 * PI_DOUBLE;
	}

	// 20200417 - FormerLurker - rename millimeters_of_travel to millimeters_of_travel_arc to better describe what we are
	// calculating here
	return utilities::hypot(angular_travel_total * radius, utilities::abs(z_dist));

}

std::string utilities::to_string(double value)
{
	std::ostringstream os;
	os << value;
	return os.str();
}

std::string utilities::to_string(int value)
{
	std::ostringstream os;
	os << value;
	return os.str();
}

std::string utilities::ltrim(const std::string& s)
{
	size_t start = s.find_first_not_of(WHITESPACE_);
	return (start == std::string::npos) ? "" : s.substr(start);
}

std::string utilities::rtrim(const std::string& s)
{
	size_t end = s.find_last_not_of(WHITESPACE_);
	return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string utilities::trim(const std::string& s)
{
	return rtrim(ltrim(s));
}

std::string utilities::join(const std::string* strings, size_t length, std::string sep)
{
	std::string output;
	for (int i = 0; i < length; i++)
	{
		if (i > 0)
		{
			output += sep;
		}
		output += strings[i];
	}
	return output;
}

std::string utilities::join(const std::vector<std::string> strings, std::string sep)
{
	std::string output;

	for (std::vector<std::string>::const_iterator p = strings.begin();
		p != strings.end(); ++p) {
		output += *p;
		if (p != strings.end() - 1)
			output += sep;
	}
	return output;
}
/* Might need this later
bool utilities::contains(const std::string source, const std::string substring)
{
	return source.find(substring, 0) != std::string::npos;
}*/

std::istream& utilities::safe_get_line(std::istream& is, std::string& t)
{
	t.clear();
	// The characters in the stream are read one-by-one using a std::streambuf.
	// That is faster than reading them one-by-one using the std::istream.
	// Code that uses streambuf this way must be guarded by a sentry object.
	// The sentry object performs various tasks,
	// such as thread synchronization and updating the stream state.

	std::istream::sentry se(is, true);
	std::streambuf* sb = is.rdbuf();

	for (;;) {
		const int c = sb->sbumpc();
		switch (c) {
		case '\n':
			return is;
		case '\r':
			if (sb->sgetc() == '\n')
				sb->sbumpc();
			return is;
		case EOF:
			// Also handle the case when the last line has no line ending
			if (t.empty())
				is.setstate(std::ios::eofbit);
			return is;
		default:
			t += static_cast<char>(c);
		}
	}
}

std::string utilities::center(std::string input, int width)
{
	int input_width = (int)input.length();
	int difference = width - input_width;
	if (difference < 1)
	{
		return input;
	}
	int left_padding = difference / 2;
	int right_padding = width - left_padding - input_width;
	return std::string(left_padding, ' ') + input + std::string(right_padding, ' ');
}

double utilities::get_percent_change(int v1, int v2)
{
	if (v1 != 0)
	{
		return (((double)v2 - (double)v1) / (double)v1) * 100.0;
	}
	return 0;
}

double utilities::get_percent_change(double v1, double v2)
{
	if (v1 != 0)
	{
		return ((v2 - v1) / v1);
	}
	return 0;
}

std::string utilities::get_percent_change_string(int v1, int v2, int precision)
{
	std::stringstream format_stream;
	format_stream.str(std::string());
	std::string percent_change_string;
	if (v1 == 0)
	{
		if (v2 > 0)
		{
			format_stream << "INF";
		}
		else
		{
			format_stream << std::fixed << std::setprecision(1) << 0.0 << "%";
		}
	}
	else
	{
		double percent_change = get_percent_change(v1, v2);
		format_stream << std::fixed << std::setprecision(precision) << percent_change << "%";
	}
	return format_stream.str();
}

int utilities::get_num_digits(int x)
{
	x = utilities::abs(x);
	return (x < 10 ? 1 :
		(x < 100 ? 2 :
			(x < 1000 ? 3 :
				(x < 10000 ? 4 :
					(x < 100000 ? 5 :
						(x < 1000000 ? 6 :
							(x < 10000000 ? 7 :
								(x < 100000000 ? 8 :
									(x < 1000000000 ? 9 :
										(x < 10000000000 ? 10 : -1))))))))));
}

int utilities::get_num_digits(double x, int precision)
{
	return get_num_digits(
		(int)utilities::ceil(x * utilities::pow(10, precision) - .4999999999999)
		/ utilities::pow(10, precision)
	);
}

int utilities::get_num_digits(double x)
{
	return get_num_digits((int)x);
}

// Nice utility function found here: https://stackoverflow.com/questions/8520560/get-a-file-name-from-a-path
std::vector<std::string> utilities::splitpath(const std::string& str)
{
	std::vector<std::string> result;

	char const* pch = str.c_str();
	char const* start = pch;
	for (; *pch; ++pch)
	{
		if (*pch == PATH_SEPARATOR_)
		{
			if (start != pch)
			{
				std::string str(start, pch);
				result.push_back(str);
			}
			else
			{
				result.push_back("");
			}
			start = pch + 1;
		}
	}
	result.push_back(start);

	return result;
}

bool utilities::get_file_path(const std::string& file_path, std::string& path)
{
	std::vector<std::string> file_parts = splitpath(file_path);
	if (file_parts.size() == 0)
		return false;
	for (int index = 0; index < file_parts.size() - 1; index++)
	{
		path += file_parts[index];
		path += PATH_SEPARATOR_;
	}
	return true;
}

std::string utilities::create_uuid() {
	std::string res;
	for (int i = 0; i < 16; i++) {
		if (GUID_DASHES[i]) res += "-";
		res += GUID_RANGE[(int)(rand() % 16)];
		res += GUID_RANGE[(int)(rand() % 16)];
	}
	return res;
}

bool utilities::get_temp_file_path_for_file(const std::string& file_path, std::string& temp_file_path)
{
	temp_file_path = "";
	if (!get_file_path(file_path, temp_file_path))
	{
		return false;
	}
	temp_file_path = temp_file_path;
	temp_file_path += create_uuid();
	temp_file_path += ".tmp";
	return true;
}

bool utilities::does_file_exist(const std::string& file_path)
{
	FILE* file;
	if (file = fopen(file_path.c_str(), "r")) {
		fclose(file);
		return true;
	}
	return false;
}

double utilities::hypot(double x, double y)
{
	if (x < 0) x = -x;
	if (y < 0) y = -y;
	if (x < y) {
		double tmp = x;
		x = y; y = tmp;
	}
	if (y == 0.0) return x;
	y /= x;
	return x * utilities::sqrt(1.0 + y * y);
}

float utilities::hypotf(float x, float y)
{
	if (x < 0.0f) x = -x;
	if (y < 0.0f) y = -y;
	if (x < y) {
		float tmp = x;
		x = y; y = tmp;
	}
	if (y == 0.0f) return x;
	y /= x;
	return x * utilities::sqrtf(1.0f + y * y);
}

double utilities::atan2(double y, double x)
{
	return std::atan2(y, x);
}

float utilities::atan2f(float y, float x)
{
	return std::atan2(y, x);
}

double utilities::floor(double x)
{
	return std::floor(x);
}

float utilities::floorf(float x)
{
	return std::floor(x);
}

double utilities::ceil(double x)
{
	return std::ceil(x);
}

float utilities::ceilf(float x)
{
	return std::ceil(x);
}

double utilities::cos(double x)
{
	return std::cos(x);
}

float utilities::cosf(float x)
{
	return std::cos(x);
}

double utilities::sin(double x)
{
	return std::sin(x);
}

float utilities::sinf(float x)
{
	return std::sin(x);
}

double utilities::abs(double x)
{
	return std::abs(x);
}

int utilities::abs(int x)
{
	return std::abs(x);
}

float utilities::absf(float x)
{
	return std::abs(x);
}

double utilities::fabs(double x)
{
	return std::fabs(x);
}

float utilities::fabsf(float x)
{
	return std::fabs(x);
}

double utilities::sqrt(double x)
{
	return std::sqrt(x);
}

float utilities::sqrtf(float x)
{
	return std::sqrt(x);
}

double utilities::pow(int e, double x)
{
	return std::pow(e, x);
}

double utilities::min(double x, double y)
{
	return std::min(x, y);
}

float utilities::minf(float x, float y)
{
	return std::min(x, y);
}

double utilities::max(double x, double y)
{
	return std::max(x, y);
}

float utilities::maxf(float x, float y)
{
	return std::max(x, y);
}

double utilities::radians(double x)
{
	return (x * PI_DOUBLE) / 180.0;
}

float utilities::radiansf(float x)
{
	return (x * PI_FLOAT) / 180.0f;
}

double utilities::sq(double x)
{
	return x * x;
}

float utilities::sqf(float x)
{
	return x * x;
}

bool utilities::within(double value, double min, double max)
{
	return ((value) >= (min) && (value) <= (max));
}

bool utilities::withinf(float value, float min, float max)
{
	return ((value) >= (min) && (value) <= (max));
}

double utilities::constrain(double value, double arg_min, double arg_max)
{
	return ((value) < (arg_min) ? (arg_min) : ((value) > (arg_max) ? (arg_max) : (value)));
}

float utilities::constrainf(float value, float arg_min, float arg_max)
{
	return ((value) < (arg_min) ? (arg_min) : ((value) > (arg_max) ? (arg_max) : (value)));
}

double utilities::reciprocal(double x)
{
	return 1.0 / x;
}

float utilities::reciprocalf(float x)
{
	return 1.0f / x;
}

void* utilities::memcpy(void* dest, const void* src, size_t n)
{
	return std::memcpy(dest, src, n);
}

std::string utilities::dtos(double x, unsigned char precision)
{
	static char buffer[FPCONV_BUFFER_LENGTH];
	char* p = buffer;
	buffer[fpconv_dtos(x, buffer, precision)] = '\0';
	/* This is code that can be used to compare the output of the
		 modified fpconv_dtos function to the ofstream output
		 Note:  It currently only fails for some checks where the original double does not store
					  perfectly.  In this case I actually think the dtos output is better than ostringstream!
	std::ostringstream stream;
	stream << std::fixed;
	stream << std::setprecision(precision) << x;

	if (std::string(buffer) != stream.str())
	{
		std::cout << std::fixed << "Failed to convert: " << std::setprecision(24) << x << " Precision:" << std::setprecision(0) << static_cast <int> (precision) << " String:" << std::string(buffer) << " Stream:" << stream.str() << std::endl;
	}
	*/
	return buffer;
}
/*
bool case_insensitive_compare_char(char& c1, char& c2)
{
	if (c1 == c2)
		return true;
	else if (std::toupper(c1) == std::toupper(c2))
		return true;
	return false;
}

/*
 * Case Insensitive String Comparision

bool case_insensitive_compare(std::string& str1, std::string& str2)
{
	return ((str1.size() == str2.size()) &&	std::equal(str1.begin(), str1.end(), str2.begin(), &case_insensitive_compare_char));
}

*/

std::string utilities::replace(std::string subject, const std::string& search, const std::string& replace) {
	if (search.length() > 0)
	{
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos) {
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
	}
	return subject;
}

double utilities::rand_range(double min, double max) {
	double f = (double)std::rand() / RAND_MAX;
	return min + f * (max - min);
}

unsigned char utilities::rand_range(unsigned char min, unsigned char max) {
	double f = (double)std::rand() / RAND_MAX;
	return static_cast<unsigned char>(static_cast<double>(min) + f * (static_cast<double>(max) - static_cast<double>(min)));
}

int utilities::rand_range(int min, int max) {
	double f = (double)std::rand() / RAND_MAX;
	return static_cast<int>(static_cast<double>(min) + f * (static_cast<double>(max) - static_cast<double>(min)));
}