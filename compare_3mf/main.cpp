#include "AppConfig.h"
#include "CompareGcode.h"
#include "CompareImage.h"
#include "CompareModel.h"
#include "CompareCustomGcode.h"
#include "CompareLayerHeight.h"
#include "CompareModelSettings.h"
#include "CompareProjectSettings.h"
#include "CompareSliceInfo.h"
#include "CompareHelper.h"
#include <filesystem>

#include "miniz/miniz.h"
#include <boost/filesystem/operations.hpp>
#include <libslic3r/miniz_extension.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "libslic3r/Model.hpp"
#include "libslic3r/Format/3mf.hpp"
#include "libslic3r/Format/STL.hpp"

using namespace Slic3r;

CompareBase* create_compare(const std::string& type)
{
    if (CompareHelper::is_gcode_file(type)) {
        return new CompareGcode();
    } else if (CompareHelper::is_image_file(type)) {
        return new CompareImage();
    } else if (CompareHelper::is_model_file(type)) {
        return new CompareModel();
    } else if (CompareHelper::is_custom_gcode_file(type)) {
        return new CompareCustomGcode();
    } else if (CompareHelper::is_layer_height_file(type)) {
        return new CompareLayerHeight();
    } else if (CompareHelper::is_model_settings_file(type)) {
        return new CompareModelSettings();
    } else if (CompareHelper::is_project_settings_file(type)) {
        return new CompareProjectSettings();
    } else if (CompareHelper::is_slice_info_file(type)) {
        return new CompareSliceInfo();
    }
    return nullptr;
}

int main()
{
    mz_zip_archive        archive_in;
    mz_zip_archive        archive_out;
    std::filesystem::path path_input("D:\\baseline_3mf\\");
    std::filesystem::path path_output("D:\\output_3mf\\");
    for (const auto& entry : std::filesystem::directory_iterator(path_input)) {
        if (entry.is_regular_file()) {
            auto file_name = entry.path().filename().string();
            if (entry.path().extension() != ".3mf") {
                continue;
            }
            mz_zip_zero_struct(&archive_in);
            if (!open_zip_reader(&archive_in, entry.path().u8string().c_str())) {
                continue;
            }

            auto file_name_out = path_output.append(file_name).u8string();
            mz_zip_zero_struct(&archive_out);
            if (!open_zip_reader(&archive_out, file_name_out.c_str())) {
                continue;
            }

            std::unordered_set<std::string> files_input;
            std::vector<std::string>        files_only_input;
            std::unordered_set<std::string> files_output;
            std::vector<std::string>        files_only_output;
            std::vector<std::string>        files_in_common;

            mz_uint                  num_entries = mz_zip_reader_get_num_files(&archive_in);
            mz_zip_archive_file_stat stat;
            for (mz_uint i = 0; i < num_entries; ++i) {
                if (mz_zip_reader_file_stat(&archive_in, i, &stat)) {
                    std::string name(stat.m_filename);
                    std::replace(name.begin(), name.end(), '\\', '/');
                    files_input.emplace(name);
                }
            }

            num_entries = mz_zip_reader_get_num_files(&archive_in);
            for (mz_uint i = 0; i < num_entries; ++i) {
                if (mz_zip_reader_file_stat(&archive_out, i, &stat)) {
                    std::string name(stat.m_filename);
                    std::replace(name.begin(), name.end(), '\\', '/');
                    files_output.emplace(name);
                    if (files_input.count(name) == 0) {
                        files_only_output.emplace_back(name);
                    } else {
                        files_in_common.emplace_back(name);
                    }
                }
            }

            for (const auto& item : files_input) {
                if (files_output.count(item) == 0) {
                    files_only_input.emplace_back(item);
                }
            }

            // compare model files
            // too complicated to implement text comparison
            // compare geometries of 3mf files here, other informations are compared in the below loop
            Model              input_model;
            DynamicPrintConfig input_config;
            Semver             file_version;

            PlateDataPtrs             plate_data;
            En3mfType                 en_3mf_file_type = En3mfType::From_BBS;
            ConfigSubstitutionContext config_substitutions{ForwardCompatibilitySubstitutionRule::Enable};
            std::vector<Preset*>      project_presets;
            input_model = Slic3r::Model::read_from_archive(entry.path().u8string().c_str(), &input_config, &config_substitutions,
                                                           en_3mf_file_type, LoadStrategy::LoadModel, &plate_data, &project_presets,
                                                           &file_version);

            Model                     output_model;
            DynamicPrintConfig        output_config;
            ConfigSubstitutionContext output_ctxt{ForwardCompatibilitySubstitutionRule::Disable};

            output_model = Slic3r::Model::read_from_archive(file_name_out, &output_config, &config_substitutions, en_3mf_file_type,
                                                            LoadStrategy::LoadModel, &plate_data, &project_presets, &file_version);
            if (input_model.objects.size() != output_model.objects.size()) {
                continue;
            }
            for (size_t i = 0; i < input_model.objects.size(); i++) {
                const auto&  input_object  = input_model.objects[i];
                const auto&  output_object = output_model.objects[i];
                TriangleMesh src_mesh      = input_object->mesh();
                TriangleMesh dst_mesh      = output_object->mesh();

                if (!input_object->origin_translation.isApprox(output_object->origin_translation)) {
                    continue;
                }

                bool res = src_mesh.its.vertices.size() == dst_mesh.its.vertices.size();
                if (res) {
                    for (size_t i = 0; i < dst_mesh.its.vertices.size(); ++i) {
                        res &= dst_mesh.its.vertices[i].isApprox(src_mesh.its.vertices[i]);
                    }
                }
                if (!res) {
                    continue;
                }
            }

            // compare files
            for (const auto& item : files_in_common) {
                mz_uint index_input  = mz_zip_reader_locate_file(&archive_in, item.c_str(), nullptr, 0);
                mz_uint index_output = mz_zip_reader_locate_file(&archive_out, item.c_str(), nullptr, 0);
                if (index_input == MZ_PARAM_ERROR || index_output == MZ_PARAM_ERROR) {
                    continue;
                }

                mz_zip_archive_file_stat stat_input;
                mz_zip_archive_file_stat stat_output;
                if (!mz_zip_reader_file_stat(&archive_in, index_input, &stat_input) ||
                    !mz_zip_reader_file_stat(&archive_out, index_output, &stat_output)) {
                    continue;
                }

                if (stat_input.m_uncomp_size != stat_output.m_uncomp_size) {
                    continue;
                }

                std::vector<unsigned char> buffer_input(stat_input.m_uncomp_size);
                std::vector<unsigned char> buffer_output(stat_output.m_uncomp_size);
                if (!mz_zip_reader_extract_to_mem(&archive_in, index_input, buffer_input.data(), buffer_input.size(), 0) ||
                    !mz_zip_reader_extract_to_mem(&archive_out, index_output, buffer_output.data(), buffer_output.size(), 0)) {
                    continue;
                }

                if (stat_input.m_uncomp_size != stat_output.m_uncomp_size) {
                    continue;
                }

                if (stat_input.m_uncomp_size == 0) {
                    continue;
                }
                std::unique_ptr<CompareBase> compare = std::unique_ptr<CompareBase>(create_compare(item));
                if (compare) {
                    compare->compare_buffer(reinterpret_cast<const char*>(buffer_input.data()), buffer_input.size(),
                                            reinterpret_cast<const char*>(buffer_output.data()), buffer_output.size());
                    if (!compare->get_compare_result()) {
                        continue;
                    }
                }
            }
        }
    }

    return 0;
}
