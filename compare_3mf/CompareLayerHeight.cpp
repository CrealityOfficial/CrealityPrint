#include "CompareLayerHeight.h"
#include <sstream>
#include <boost/algorithm/string.hpp>

void CompareLayerHeight::compare_files(const std::string& file1, const std::string& file2) {}

void CompareLayerHeight::compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2)
{
    std::vector<std::string> line_result1;
    std::vector<std::string> line_result2;
    auto                     get_info = [](const char* buffer, size_t size, std::vector<std::string>& line_result) {
        std::string        buffer_str(buffer, size);
        std::istringstream iss(buffer_str);
        std::string        line;
        while (std::getline(iss, line)) {
            std::istringstream iss_line(line);
            boost::algorithm::split(line_result, line, boost::is_any_of(";"));
            if (!line_result.empty()) {
                auto& result_first = line_result[0];
                auto  pos          = result_first.find("|");
                if (pos != std::string::npos) {
                    result_first = result_first.substr(pos + 1);
                }
            }
        }
    };
    get_info(buffer1, size1, line_result1);
    get_info(buffer2, size2, line_result2);

    if (line_result1.size() != line_result2.size())
        return;

    const float epsilon = 1e-6f;
    for (size_t i = 0; i < line_result1.size(); ++i) {
        if (std::fabs(std::atof(line_result1[i].c_str()) - std::atof(line_result2[i].c_str())) > epsilon)
            return;
    }
}
