#ifndef COMPARE_PROJECT_SETTINGS_H
#define COMPARE_PROJECT_SETTINGS_H
#include "CompareBase.h"
#include "nlohmann/json.hpp"
using namespace nlohmann;

class CompareProjectSettings : public CompareBase
{
public:
    CompareProjectSettings() = default;
    CompareProjectSettings(CompareOptions compare_option) : CompareBase(compare_option) {}
    ~CompareProjectSettings() = default;

    void compare_files(const std::string& file1, const std::string& file2) override;
    void compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2) override;

private:
    bool diff_objects(json const& in, json& out, json const& base);
};
#endif // !COMPARE_PROJECT_SETTINGS_H
