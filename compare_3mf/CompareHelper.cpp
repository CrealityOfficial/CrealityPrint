#include "CompareHelper.h"
#include "CompareGcode.h"
#include "CompareImage.h"
#include "CompareModel.h"
#include "Constant3mf.h"
#include <boost/algorithm/string/predicate.hpp>

CompareBase* CompareHelper::create_compare_instance(const std::string& file)
{
    if (is_gcode_file(file))
		return new CompareGcode();
    else if (is_image_file(file))
		return new CompareImage();
    else if (is_model_file(file))
		return new CompareModel();
	return nullptr;
}

bool CompareHelper::is_gcode_file(const std::string& file) { return boost::algorithm::iends_with(file, ".gcode"); }

bool CompareHelper::is_image_file(const std::string& file) { return boost::algorithm::iends_with(file, ".png"); }

bool CompareHelper::is_model_file(const std::string& file) { return boost::algorithm::iends_with(file, ".model"); }

bool CompareHelper::is_model_settings_file(const std::string& file) { return file == BBS_MODEL_CONFIG_FILE; }

bool CompareHelper::is_project_settings_file(const std::string& file) { return file == BBS_PROJECT_CONFIG_FILE; }

bool CompareHelper::is_slice_info_file(const std::string& file) { return file == SLICE_INFO_CONFIG_FILE; }

bool CompareHelper::is_custom_gcode_file(const std::string& file) { return file == CUSTOM_GCODE_PER_PRINT_Z_FILE; }

bool CompareHelper::is_layer_height_file(const std::string& file) { return file == BBS_LAYER_HEIGHTS_PROFILE_FILE; }
