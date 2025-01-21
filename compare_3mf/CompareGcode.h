#ifndef COMPARE_GCODE_H
#define COMPARE_GCODE_H

#include "CompareBase.h"

class CompareGcode : public CompareBase
{
public:
	CompareGcode() = default;
	~CompareGcode() = default;

public:
	void compare_files(const std::string& file1, const std::string& file2) override;
	void compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2) override;
};

#endif // !COMPARE_GCODE_H
