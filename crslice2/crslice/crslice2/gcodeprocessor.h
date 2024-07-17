#ifndef GCODE_PROCESSOR_H_2
#define GCODE_PROCESSOR_H_2
#include "crslice2/interface.h"
#include <string>
#include <vector>

namespace crslice2
{
    CRSLICE2_API void process_file(const std::string& file, std::vector<std::vector<std::pair<int, float>>>& times);
}
#endif  // GCODE_PROCESSOR_H_2
