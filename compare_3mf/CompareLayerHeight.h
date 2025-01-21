#ifndef COMPARE_LAYER_HEIGHT_H
#define COMPARE_LAYER_HEIGHT_H
#include "CompareBase.h"

class CompareLayerHeight : public CompareBase
{
public:
    CompareLayerHeight() = default;
    CompareLayerHeight(CompareOptions compare_option) : CompareBase(compare_option) {}
    ~CompareLayerHeight() = default;

    void compare_files(const std::string& file1, const std::string& file2) override;
    void compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2) override;
};
#endif // !COMPARE_LAYER_HEIGHT_H
