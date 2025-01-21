#ifndef COMPARE_MODEL_SETTINGS_H
#define COMPARE_MODEL_SETTINGS_H
#include "CompareBase.h"

class CompareModelSettings : public CompareBase
{
public:
    CompareModelSettings() = default;
    CompareModelSettings(CompareOptions compare_option) : CompareBase(compare_option) {}
    ~CompareModelSettings() = default;

    void compare_files(const std::string& file1, const std::string& file2) override;
    void compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2) override;
};
#endif // !COMPARE_MODEL_SETTINGS_H
