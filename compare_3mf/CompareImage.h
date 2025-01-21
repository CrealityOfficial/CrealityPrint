#ifndef COMPARE_IMAGE_H
#define COMPARE_IMAGE_H
#include "CompareBase.h"

class CompareImage : public CompareBase
{
public:
    CompareImage();
    CompareImage(CompareOptions compare_option);
    ~CompareImage() = default;
    
    void compare_files(const std::string& file1, const std::string& file2) override;
    void compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2) override;

private:
	std::pair<size_t, size_t> get_image_size(const std::string& file);
    std::pair<size_t, size_t> get_image_size(const char* buffer, size_t size);
};
#endif // !COMPARE_IMAGE_H
