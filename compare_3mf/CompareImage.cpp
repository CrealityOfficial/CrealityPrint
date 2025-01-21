#include "CompareImage.h"
#include <filesystem>
#include <fstream>
#include <libslic3r/PNGReadWrite.hpp>

CompareImage::CompareImage() : CompareBase(CompareOptions::COMPARE_OPTION_IMAGESIZE) {}

CompareImage::CompareImage(CompareOptions compare_option) : CompareBase(compare_option) {}

void CompareImage::compare_files(const std::string& file1, const std::string& file2)
{
    if (!std::filesystem::exists(file1) || !std::filesystem::exists(file2)) {
        return;
    }
    switch (compare_option_) {
    case CompareOptions::COMPARE_OPTION_NONE: {
        set_compare_result(true);
        break;
    }
    case CompareOptions::COMPARE_OPTION_IMAGESIZE: {
        auto size1 = get_image_size(file1);
        auto size2 = get_image_size(file2);
        if (size1.first == 0 || size2.first == 0) {
            set_compare_result(false);
            set_compare_result_str("Invalid image file");
        } else if (size1.first == size2.first && size1.second == size2.second) {
            set_compare_result(true);
        } else {
            set_compare_result(false);
            set_compare_result_str("Image size mismatch");
        }
        break;
    }
    default: {
        set_compare_result(false);
        set_compare_result_str("Invalid compare option");
        break;
    }
    }
}

void CompareImage::compare_buffer(const char* buffer1, size_t size1, const char* buffer2, size_t size2)
{
    switch (compare_option_) {
    case CompareOptions::COMPARE_OPTION_NONE: {
        set_compare_result(true);
        break;
    }
    case CompareOptions::COMPARE_OPTION_IMAGESIZE: {
        auto png_size1 = get_image_size(buffer1, size1);
        auto png_size2 = get_image_size(buffer2, size2);
        if (png_size1.first == 0 || png_size2.first == 0) {
            set_compare_result(false);
            set_compare_result_str("Invalid image file");
        } else if (png_size1.first == png_size2.first && png_size1.second == png_size2.second) {
            set_compare_result(true);
        } else {
            set_compare_result(false);
            set_compare_result_str("Image size mismatch");
        }
        break;
    }
    default: {
        set_compare_result(false);
        set_compare_result_str("Invalid compare option");
        break;
    }
    }
}

std::pair<size_t, size_t> CompareImage::get_image_size(const std::string& file)
{
    const std::size_t& size = std::filesystem::file_size(file);
    std::string        png_buffer(size, '\0');
    png_buffer.reserve(size);
    std::ifstream ifs(file, std::ios::binary);
    ifs.read(png_buffer.data(), file.size());
    ifs.close();

    return get_image_size(png_buffer.data(), size);
}

std::pair<size_t, size_t> CompareImage::get_image_size(const char* buffer, size_t size)
{
    Slic3r::png::ImageColorscale img;
    Slic3r::png::ReadBuf         rb{buffer, size};

    // for now we only support png
    if (!Slic3r::png::decode_colored_png(rb, img)) {
        return std::make_pair(0, 0);
    }
    return std::make_pair(img.cols, img.rows);
}
