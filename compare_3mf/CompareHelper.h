#ifndef COMPARE_HELPER_H
#define COMPARE_HELPER_H
#include <string>

class CompareBase;

class CompareHelper
{
public:
    CompareHelper()  = default;
    ~CompareHelper() = default;

public:
    static CompareBase* create_compare_instance(const std::string& file);
    static bool         is_gcode_file(const std::string& file);
    static bool         is_image_file(const std::string& file);
    static bool         is_model_file(const std::string& file);
    static bool is_model_settings_file(const std::string& file);
    static bool is_project_settings_file(const std::string& file);
    static bool is_slice_info_file(const std::string& file);
    static bool is_custom_gcode_file(const std::string& file);
    static bool is_layer_height_file(const std::string& file);
};

#endif // !COMPARE_HELPER_H
