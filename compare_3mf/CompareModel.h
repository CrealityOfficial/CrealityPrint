#ifndef COMPARE_MODEL_H
#define COMPARE_MODEL_H
#include "CompareBase.h"

class CompareModel : public CompareBase
{
public:
    CompareModel() = default;
    CompareModel(CompareOptions compare_option) : CompareBase(compare_option) {}
    ~CompareModel() = default;

    void compare_files(const std::string& file1, const std::string& file2) override;
    void compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2) override;
};
#endif // !COMPARE_MODEL_H
