#include "baselineorcinput.hpp"
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
//#define CEREAL_FUTURE_EXPERIMENTAL
#include <cereal/archives/adapters.hpp>

#include "libslic3r/Model.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Slicing.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PrintBase.hpp"
#include "nlohmann/json.hpp"
#include <boost/nowide/fstream.hpp>
#include "GCode/ThumbnailData.hpp"
#include "libslic3r/Semver.hpp"

using namespace cxbaseline;

void BaseLineLogger::PushKey(const std::string& key)
{
    m_key_group.emplace_back(key);
}

void BaseLineLogger::PopKey()
{
    m_key_group.pop_back();
}

void BaseLineLogger::LogError(const std::string& msg)
{
    std::string key_msg;
    m_error_msg = "";
    for (int i = 0; i < m_key_group.size(); ++i)
    {
        key_msg += m_key_group[i];
        if (i < m_key_group.size() - 1)
            key_msg += "->";
    }

    std::cout << "error:" << key_msg << msg << std::endl;
    m_error_msg += "error:" + key_msg + "\n";
}
std::string BaselineOrcaFileUtils::CreateCompareEorrorFile(const std::string& root_dir, const std::string& name, const bool error)
{
    if (!_IsDirExist(root_dir) && !_CreateDir(root_dir))
        return std::string();
    std::filesystem::path file_path(root_dir);
    std::string file_path_name = (file_path /= name).string();
    if (_IsFileExist(file_path_name))
    {
        _RemoveFile(file_path_name);
    }
    if(error)
        return std::string();
    if (!_CreateFile(file_path_name))
        return std::string();
    return file_path_name;
}
std::string BaselineOrcaFileUtils::CreateBaselineFile(const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group)
{
    if (!_IsDirExist(root_dir))
        return std::string();

    std::string dir = _ComposeDir(root_dir, module_group);
    if (!_IsDirExist(dir) && !_CreateDir(dir))
        return std::string();

    std::string file_path = _ComposeBaselinePath(dir, name, _GetBaselineInitVersion());
    if (_IsFileExist(file_path) || !_CreateFile(file_path))
        return std::string("exist");

    return file_path;
}

std::string BaselineOrcaFileUtils::UpdateBaselineFile(const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group)
{
    std::string dir = _ComposeDir(root_dir, module_group);
    if (!_IsDirExist(dir))
        return std::string();

    int version = _GetBaselineCurrentVersion(dir, name);
    if (version == VER_ERR)
        return std::string();
    int next_version = _GetBaselineNextVersion(version);
    if (version == VER_ERR)
        return std::string();

    std::string file_path = _ComposeBaselinePath(dir, name, next_version);
    //if (_IsFileExist(file_path) || !_CreateFile(file_path))
    //    return std::string();

    return file_path;
}

std::string BaselineOrcaFileUtils::GetBaselineFileLatest(const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group)
{
    std::string dir = _ComposeDir(root_dir, module_group);
    if (!_IsDirExist(dir))
        return std::string();

    int version = _GetBaselineCurrentVersion(dir, name);
    if (version == VER_ERR)
        return std::string();

    return _ComposeBaselinePath(dir, name, version);
}

bool BaselineOrcaFileUtils::RemoveBaselineFileLatest(const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group)
{
    std::string file_path = GetBaselineFileLatest(root_dir, name, module_group);
    _RemoveFile(file_path);

    std::vector<std::string> group = module_group;
    while (!group.empty())
    {
        if (!_RemoveDir(_ComposeDir(root_dir, group)))
            break;
        group.pop_back();
    }

    BLReturnBoolen(true);
}

bool BaselineOrcaFileUtils::RemoveBaselineFileAll(const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group)
{
    std::string dir = _ComposeDir(root_dir, module_group);
    if (!_IsDirExist(dir))
        BLReturnBoolen(false);

    int version = _GetBaselineCurrentVersion(dir, name);
    if (version == VER_ERR)
        BLReturnBoolen(false);

    for (int ver = VER_INIT; ver <= version; ++ver)
    {
        std::string file_path = _ComposeBaselinePath(dir, name, ver);
        _RemoveFile(file_path);
    }

    std::vector<std::string> group = module_group;
    while (!group.empty())
    {
        if (!_RemoveDir(_ComposeDir(root_dir, group)))
            break;
        group.pop_back();
    }

    BLReturnBoolen(true);
}

bool BaselineOrcaFileUtils::_IsDirExist(const std::string& path)
{
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

bool BaselineOrcaFileUtils::_IsFileExist(const std::string& path)
{
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

bool BaselineOrcaFileUtils::_CreateDir(const std::string& path)
{
    if (_IsDirExist(path))
        BLReturnBoolen(false);
    return std::filesystem::create_directories(path);
}

bool BaselineOrcaFileUtils::_CreateFile(const std::string& path)
{
    std::filesystem::path file_path(path);
    if (!_IsDirExist(file_path.parent_path().string()))
    {
        if (std::filesystem::create_directories(file_path.parent_path()))
            BLReturnBoolen(false);
    }
    if (_IsFileExist(path))
        BLReturnBoolen(false);
    {
        std::ofstream file(path);
    }
    if (!_IsFileExist(path))
        BLReturnBoolen(false);
    BLReturnBoolen(true);
}

bool BaselineOrcaFileUtils::_RemoveDir(const std::string& path)
{
    if (!_IsDirExist(path))
        BLReturnBoolen(false);

    return std::filesystem::is_empty(path) ? std::filesystem::remove(path) : false;
}

bool BaselineOrcaFileUtils::_RemoveFile(const std::string& path)
{
    if (!_IsFileExist(path))
        BLReturnBoolen(false);

    return std::filesystem::remove(path);
}

std::string BaselineOrcaFileUtils::_ComposeDir(const std::string& root_dir, const std::string& module)
{
    std::filesystem::path file_path(root_dir);
    return (file_path /= module).string();
}

std::string BaselineOrcaFileUtils::_ComposeDir(const std::string& root_dir, const std::vector<std::string>& module_group)
{
    std::filesystem::path file_path(root_dir);
    for (auto& module : module_group)
    {
        file_path.append(module);
    }
    return file_path.string();
}

int BaselineOrcaFileUtils::_GetBaselineInitVersion()
{
    return VER_INIT;
}

int BaselineOrcaFileUtils::_GetBaselineNextVersion(int current_version)
{
    return VER_INIT;
    if (current_version == VER_ERR || current_version < VER_INIT)
        return VER_ERR;

    int next_version = current_version + 1;
    if (next_version == VER_ERR || next_version < VER_INIT)
        return VER_ERR;

    return next_version;
}

int BaselineOrcaFileUtils::_GetBaselineCurrentVersion(const std::string& dir, const std::string name)
{
    return VER_INIT;
    std::vector<int> version_group;
    for (auto const& dir_entry : std::filesystem::directory_iterator{ dir })
    {
        auto& path = dir_entry.path();
        if (!path.has_extension() || path.extension() != EXT)
            continue;

        std::filesystem::path filename{ path.stem() };
        if (!filename.has_stem() || filename.stem() != name)
            continue;

        if (!filename.has_extension())
            continue;

        std::string ver_str = filename.extension().string().substr(1);
        if (ver_str.size() != VER_SIZE)
            continue;

        int version = VER_ERR;
        try
        {
            version = std::stoi(ver_str);
        }
        catch (...)
        {
            return version;
        }

        version_group.emplace_back(version);
    }

    std::sort(version_group.begin(), version_group.end());
    for (int i = 0; i < version_group.size(); ++i)
    {
        if (version_group[i] - VER_INIT != i)
            return VER_ERR;
    }

    return version_group.empty() ? VER_ERR : version_group.back();
}

std::string BaselineOrcaFileUtils::_ComposeBaselinePath(const std::string& dir, const std::string name, int version)
{
    std::string ver = std::to_string(version);
    std::string ver_ext = "." + std::string(VER_SIZE - std::min(VER_SIZE, (int)ver.length()), '0') + ver;
    std::filesystem::path file_path(dir);
    return (file_path /= name + ver_ext + EXT).string();
}

bool BaselineOrcaFileHelper::CreateBaselineFile(nlohmann::json& json, const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group)
{
    std::string path = BaselineOrcaFileUtils::CreateBaselineFile(root_dir, name, module_group);
    if (path.empty())
        BLReturnBoolen(false);
    if(path == "exist")
	{
		BLReturnBoolen(true);
	}   
    //boost::nowide::ofstream stream;
    //stream.open(path.c_str(), std::ios::out | std::ios::trunc);
    //stream << std::setw(4) << json << std::endl;

    std::ofstream  outstream(path);
    if (outstream.is_open())
    {
        outstream << std::setw(4) << json << std::endl;
    }
    BLReturnBoolen(true);
}

bool BaselineOrcaFileHelper::UpdateBaselineFile(nlohmann::json& json, const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group)
{
    std::string path = BaselineOrcaFileUtils::UpdateBaselineFile(root_dir, name, module_group);
    if (path.empty())
        BLReturnBoolen(false);

    //boost::nowide::ofstream stream;
    //stream.open(path.c_str(), std::ios::out | std::ios::trunc);
    //stream << std::setw(4) << json << std::endl;
    //BLReturnBoolen(true);

    std::ofstream  outstream(path);
    if (outstream.is_open())
    {
        outstream << std::setw(4) << json << std::endl;
    }
    BLReturnBoolen(true);
}

bool BaselineOrcaFileHelper::ReadBaselineFileLatest(nlohmann::json& root, const std::string& root_dir, const std::string& name, const std::vector<std::string>& module_group)
{
    using namespace nlohmann;

    std::string file_path = BaselineOrcaFileUtils::GetBaselineFileLatest(root_dir, name, module_group);
    if (file_path.empty())
        BLReturnBoolen(false);

    std::ifstream in_file(file_path);
    root = json::parse(in_file);
    BLReturnBoolen(true);
}

BaselineOrcaInput::BaselineOrcaInput(const std::string& name, const std::vector<std::string>& modules)
    : Baseline(name, modules)
{

}
bool BaselineOrcaInput::Generate()
{
    using namespace nlohmann;

    //  build json
    json root = json::object();
    _GenerateBaseline(root);

    std::string dir = BaseLineUtils::GetRootDirectory();
    std::string name = GetName();
    bool ret = BaselineOrcaFileHelper::CreateBaselineFile(root, dir, name, {});
    if (!ret)
    {
        std::string message = "error:baseline file generate failed!";
        writeResultData(message, false);
    }
    
    BLReturnBoolen(ret);
}
bool BaselineOrcaInput::Compare()
{
    using namespace nlohmann;

    //  build json
    json root = json::object();
    _GenerateBaseline(root);

    // read json file
    std::string dir = BaseLineUtils::GetRootDirectory();
    std::string name = GetName();
    json root_baseline = json::object();
    BaselineOrcaFileHelper::ReadBaselineFileLatest(root_baseline, dir, name, {});

    // compare json
    BLReturnBoolen(_CompareBaseline(root_baseline));
}
bool BaselineOrcaInput::Update()
{
    using namespace nlohmann;

    //  build json
    json root = json::object();
    _GenerateBaseline(root);

    // update version
    std::string dir = BaseLineUtils::GetRootDirectory();
    std::string name = GetName();
    bool ret = BaselineOrcaFileHelper::UpdateBaselineFile(root, dir, name, {});
    if (!ret)
    {
        std::string message = "error:baseline file update failed!";
        writeResultData(message, false);
    }
    BLReturnBoolen(ret);
}

void BaselineOrcaInput::Add(const Slic3r::Model* model)
{
    m_elem_model = model;
}

void BaselineOrcaInput::Add(const Slic3r::DynamicPrintConfig* config)
{
    m_elem_dynamic_print_config = config;
}

void BaselineOrcaInput::Add(const TempParamater* param)
{
    m_elem_temp_param = param;
}

void BaselineOrcaInput::Add(const Slic3r::Calib_Params* param)
{
    m_elem_calib_param = param;
}

void BaselineOrcaInput::Add(const Slic3r::ThumbnailsList* thumbnailData)
{
    m_elem_thumbnail = thumbnailData;
}
void BaselineOrcaInput::_GenerateBaseline(nlohmann::json& json_root)
{
    nlohmann::json childer_json;
    _BuildObject(childer_json[BLName_Val(model)], m_elem_model, _BuildEntityModel);

    _BuildBlockDynamicPrintConfig(childer_json[BLName_Val(dynamic_print_config)], *m_elem_dynamic_print_config);

    //_BuildBlockTempParam(childer_json[BLName_Val(temp_param)], *m_elem_temp_param);

    //_BuildBlockCalibParam(childer_json[BLName_Val(calib_param)], *m_elem_calib_param);

    //_BuildBlockThumbnail(childer_json[BLName_Val(thumbnail)], *m_elem_thumbnail);

    _BuildObject(json_root[BLName_Val(slicer)], childer_json,
        [](nlohmann::json& json_param, const nlohmann::json& json)
        {
            json_param = json;
        });
}
void BaselineOrcaInput::writeResultData(const std::string& message, bool cache)
{
    std::string dir = BaseLineUtils::GetCompareDirectory();
    std::string name = "result.errtxt";
    std::string path = BaselineOrcaFileUtils::CreateCompareEorrorFile(dir, name,false);
    if (!path.empty())
    {
        std::ofstream  outstream(path);
        if (outstream.is_open())
        {
            outstream << std::setw(4) << message << std::endl;
        }
        //  build json
        if (cache)
        {
            nlohmann::json newroot = nlohmann::json::object();
            std::string newname = GetName() + "_compare";
            _GenerateBaseline(newroot);
            BaselineOrcaFileHelper::CreateBaselineFile(newroot, dir, newname, {});
        }
        
    }
}
bool BaselineOrcaInput::_CompareBaseline(const nlohmann::json& json_root)
{
    BaseLineLogger logger;

    bool err = true;
    std::string err_text = "";
    if (json_root.empty())
    {
        err_text = "error:baseline is no find or cxbl is empty\n";
        writeResultData(err_text);
        BLReturnBoolen(false);
    }

    auto iter = json_root.find(BLName_Val(slicer));
    if (iter == json_root.end())
    {
        err_text = "error:slice object ont find\n";

        writeResultData(err_text);
        BLReturnBoolen(false);
    } 
    const nlohmann::json& childer_json = json_root.find(BLName_Val(slicer)).value();

    err_text += "CompareObject:\n";
    logger.m_error_msg = "";
    err &= _CompareObject(childer_json, BLName_Val(model), m_elem_model, logger, _CompareEntityModel);
    err_text += logger.ErrorMsg();

    err_text += "_CompareBlockDynamicPrintConfig:\n";
    logger.m_error_msg = "";
    err &= _CompareBlockDynamicPrintConfig(childer_json, BLName_Val(dynamic_print_config), *m_elem_dynamic_print_config, logger);
    err_text += logger.ErrorMsg() ;

    //err_text += "_CompareBlockTempParam:\n";
    //logger.m_error_msg = "";
    //err &= _CompareBlockTempParam(childer_json, BLName_Val(temp_param), *m_elem_temp_param, logger);
    //err_text += logger.ErrorMsg();

    //err_text += "_CompareBlockCalibParam:\n";
    //logger.m_error_msg = "";
    //err &= _CompareBlockCalibParam(childer_json, BLName_Val(calib_param), *m_elem_calib_param, logger);
    //err_text += logger.ErrorMsg();

    //err_text += "_CompareBlockThumbnail:\n";
    //logger.m_error_msg = "";
    //err &= _CompareBlockThumbnail(childer_json, BLName_Val(thumbnail), *m_elem_thumbnail, logger);
    //err_text += logger.ErrorMsg();
    if(!err)
        writeResultData(err_text, true);
    BLReturnBoolen(err);
    
}

void BaselineOrcaInput::_BuildEntityModel(nlohmann::json& json_model, const Slic3r::Model& model)
{
    // ModelMaterialMap    materials;
    _BuildObjectMap(json_model[BLName_Val(material)], model.materials, 
        [](nlohmann::json& json_object, const Slic3r::ModelMaterial& material)
        {
            // t_model_material_attributes attributes
            _BuildObjectMap(json_object[BLName_Val(attribute)], material.attributes);
            // ModelConfigObject config
            _BuildEntityModelConfigObject(json_object[BLName_Val(config)], material.config);
            // Model *m_model
            json_object[BLName_Val(model)] = "TODO";
        });
    // ModelObjectPtrs     objects;
    _BuildObjectGroup(json_model[BLName_Val(object)], model.objects, _BuildEntityModelObject);
}

bool BaselineOrcaInput::_CompareEntityModel(const nlohmann::json& json_model, const Slic3r::Model& model, BaseLineLogger& log)
{
    bool err = true;

    // ModelMaterialMap    materials;
    err &= _CompareObjectMap(json_model, BLName_Val(material), model.materials, log, 
        [](const nlohmann::json& json_object, const Slic3r::ModelMaterial& material, BaseLineLogger& log)
        {
            bool err = true;
            // t_model_material_attributes attributes
            err &= _CompareObjectMap(json_object, BLName_Val(attribute), material.attributes, log);
            // ModelConfigObject config
            err &= _CompareObject(json_object, BLName_Val(config), material.config, log, _CompareEntityModelConfigObject);
            // Model *m_model
            err &= _CompareObject(json_object, BLName_Val(model), "TODO", log);

            return err;
        });
    // ModelObjectPtrs     objects;
    err &= _CompareObjectGroup(json_model, BLName_Val(object), model.objects, log, _CompareEntityModelObject);
    BLReturnBoolen(err);
}

void BaselineOrcaInput::_BuildEntityModelConfig(nlohmann::json& json_config, const Slic3r::ModelConfig& config)
{
    _BuildBlockDynamicPrintConfig(json_config, config.get());
}

bool BaselineOrcaInput::_CompareEntityModelConfig(const nlohmann::json& json_config, const Slic3r::ModelConfig& config, BaseLineLogger& log)
{
    BLReturnBoolen(_CompareEntityDynamicPrintConfig(json_config, config.get(), log));
}

void BaselineOrcaInput::_BuildEntityModelConfigObject(nlohmann::json& json_config, const Slic3r::ModelConfigObject& config)
{
    _BuildBlockDynamicPrintConfig(json_config, config.get());
}

bool BaselineOrcaInput::_CompareEntityModelConfigObject(const nlohmann::json& json_config, const Slic3r::ModelConfigObject& config, BaseLineLogger& log)
{
    BLReturnBoolen(_CompareEntityDynamicPrintConfig(json_config, config.get(), log));
}

void BaselineOrcaInput::_BuildBlockDynamicPrintConfig(nlohmann::json& json_config, const Slic3r::DynamicPrintConfig& config)
{
    std::map<std::string, const Slic3r::ConfigOption*> options;
    for (auto& key : config.keys())
    {
        if (key == "software_version" ||
            key == "filament_settings_id" ||
            key == "print_settings_id" ||
            key == "printer_settings_id")
        {
            continue;
        }   
        options[key] = config.optptr(key);
    }
    _BuildObjectMap(json_config, options,
        [](nlohmann::json& json_object, const Slic3r::ConfigOption& option)
        {
            json_object = option.serialize();
        });
}

bool BaselineOrcaInput::_CompareBlockDynamicPrintConfig(const nlohmann::json& json_config, const std::string& name, const Slic3r::DynamicPrintConfig& config, BaseLineLogger& log)
{
    std::map<std::string, const Slic3r::ConfigOption*> options;
    for (auto& key : config.keys())
    {
        if (key == "software_version" ||
            key == "filament_settings_id" ||
            key == "print_settings_id" ||
            key == "printer_settings_id")
        {
            continue;
        }  
        options[key] = config.optptr(key);
    }
    BLReturnBoolen(_CompareObjectMap(json_config, name, options, log,
        [](const nlohmann::json& json_option, const Slic3r::ConfigOption& option, BaseLineLogger& log)
        {
            BLReturnBoolen(_CompareEntity(json_option, option.serialize(), log));
        }));
}

bool BaselineOrcaInput::_CompareEntityDynamicPrintConfig(const nlohmann::json& json_config, const Slic3r::DynamicPrintConfig& config, BaseLineLogger& log)
{
    std::map<std::string, const Slic3r::ConfigOption*> options;
    for (auto& key : config.keys())
    {
        if (key == "software_version" ||
            key == "filament_settings_id" ||
            key == "print_settings_id" ||
            key == "printer_settings_id")
        {
            continue;
        }  
        options[key] = config.optptr(key);
    }
    BLReturnBoolen(_CompareEntityMap(json_config, options, log,
        [](const nlohmann::json& json_option, const Slic3r::ConfigOption& option, BaseLineLogger& log)
        {
            BLReturnBoolen(_CompareEntity(json_option, option.serialize(), log));
        }));
}

void BaselineOrcaInput::_BuildBlockTempParam(nlohmann::json& json_param, const TempParamater& param)
{
    _BuildObject(json_param, param, 
        [](nlohmann::json& json_param, const TempParamater& param)
        {
            json_param[BLName_Val(bbl_priner)] = param.is_bbl_printer;
            json_param[BLName_Val(plate_index)] = param.plate_index;
            _BuildEntityVec3d(json_param[BLName_Val(plate_origin)], param.plate_origin);
            json_param[BLName_Val(extruder_count)] = param.extruderCount;
        });
}

bool BaselineOrcaInput::_CompareBlockTempParam(const nlohmann::json& json_object, const std::string& name, const TempParamater& param, BaseLineLogger& log)
{
    BLReturnBoolen(_CompareObject(json_object, name, param, log,
        [](const nlohmann::json& json_param, const TempParamater& param, BaseLineLogger& log)
        {
            bool err = true;
            err &= _CompareObject(json_param, BLName_Val(bbl_priner), param.is_bbl_printer, log);
            err &= _CompareObject(json_param, BLName_Val(plate_index), param.plate_index, log);
            err &= _CompareObject(json_param, BLName_Val(plate_origin), param.plate_origin, log, _CompareEntityVec3d);
            err &= _CompareObject(json_param, BLName_Val(extruder_count), param.extruderCount, log);

            BLReturnBoolen(err);
        }));
}

void BaselineOrcaInput::_BuildBlockCalibParam(nlohmann::json& json_param, const Slic3r::Calib_Params& param)
{
    _BuildObject(json_param, param, 
        [](nlohmann::json& json_param, const Slic3r::Calib_Params& param)
        {
            json_param[BLName_Val(start)] = param.start;
            json_param[BLName_Val(end)] = param.end;
            json_param[BLName_Val(step)] = param.step;
            json_param[BLName_Val(print_numbers)] = param.print_numbers;
            json_param[BLName_Val(CalibMode)] = param.mode;
        });
}

bool BaselineOrcaInput::_CompareBlockCalibParam(const nlohmann::json& json_object, const std::string& name, const Slic3r::Calib_Params& param, BaseLineLogger& log)
{
    BLReturnBoolen(_CompareObject(json_object, name, param, log,
        [](const nlohmann::json& json_param, const Slic3r::Calib_Params& param, BaseLineLogger& log)
        {
            bool err = true;
            err &= _CompareObject(json_param, BLName_Val(start), param.start, log);
            err &= _CompareObject(json_param, BLName_Val(end), param.start, log);
            err &= _CompareObject(json_param, BLName_Val(step), param.step, log);
            err &= _CompareObject(json_param, BLName_Val(print_numbers), param.print_numbers, log);
            err &= _CompareObject(json_param, BLName_Val(CalibMode), param.mode, log);

            BLReturnBoolen(err);
        }));
}

void BaselineOrcaInput::_BuildBlockThumbnail(nlohmann::json& json_thumbnailList, const Slic3r::ThumbnailsList& thumbnailList)
{
    _BuildObjectGroup(json_thumbnailList, thumbnailList,
        [](nlohmann::json& json_thumbnail, const Slic3r::ThumbnailData& thumbnail)
        {
            json_thumbnail[BLName_Val(width)] = thumbnail.width;
            json_thumbnail[BLName_Val(height)] = thumbnail.height;
            json_thumbnail[BLName_Val(pixels)] = "TODO";
        });
}

bool BaselineOrcaInput::_CompareBlockThumbnail(const nlohmann::json& json_thumbnailList, const std::string& name, const Slic3r::ThumbnailsList& thumbnailList, BaseLineLogger& log)
{
    BLReturnBoolen(_CompareObjectGroup(json_thumbnailList, name, thumbnailList, log,
        [](const nlohmann::json& json_thumbnail, const Slic3r::ThumbnailData& thumbnail, BaseLineLogger& log)
        {
            bool err = true;
            err &= _CompareObject(json_thumbnail, BLName_Val(width), thumbnail.width, log);
            err &= _CompareObject(json_thumbnail, BLName_Val(height), thumbnail.height, log);
            err &= _CompareObject(json_thumbnail, BLName_Val(pixels), "TODO", log);
            BLReturnBoolen(err);
        }));
}

void BaselineOrcaInput::_BuildEntityModelObject(nlohmann::json& json_object, const Slic3r::ModelObject& object)
{
    using namespace nlohmann;
    // std::string             name
    json_object[BLName_Val(name)] = object.name;
    // std::string             module_name
    json_object[BLName_Val(module_name)] = object.module_name;
    // std::string             input_file
    _BuildEntityPath(json_object[BLName_Val(input_file)], ""/*object.input_file*/);
    // ModelInstancePtrs       instances
    _BuildObjectGroup(json_object[BLName_Val(instance)], object.instances, _BuildEntityModelInstance);
    // ModelVolumePtrs         volumes
    _BuildObjectGroup(json_object[BLName_Val(volume)], object.volumes, _BuildEntityModelVolume);
    // ModelConfigObject         config
    _BuildEntityModelConfigObject(json_object[BLName_Val(config)], object.config);
    // t_layer_config_ranges   layer_config_ranges
    _BuildObjectMap(json_object[BLName_Val(layer_config_ranges)], object.layer_config_ranges, _BuildEntityModelConfig);
    // LayerHeightProfile      layer_height_profile
    _BuildObjectGroup(json_object[BLName_Val(layer_height_profile)], object.layer_height_profile.get());
    // bool                    printable
    json_object[BLName_Val(printable)] = object.printable;
    // sla::SupportPoints      sla_support_points
    _BuildObjectGroup(json_object[BLName_Val(sla_support_points)], object.sla_support_points,
        [](nlohmann::json& json_support_point, const Slic3r::sla::SupportPoint& support_point)
        {
            // Vec3f pos
            _BuildEntityVec3f(json_support_point[BLName_Val(pos)], support_point.pos);
            // float head_front_radius
            json_support_point[BLName_Val(head_front_radius)] = support_point.head_front_radius;
            // bool  is_new_island
            json_support_point[BLName_Val(is_new_island)] = support_point.is_new_island;
        });
    // sla::PointsStatus       sla_points_status
    json_object[BLName_Val(sla_points_status)] = object.sla_points_status;
    // sla::DrainHoles         sla_drain_holes
    _BuildObjectGroup(json_object[BLName_Val(sla_drain_holes)], object.sla_drain_holes,
        [](nlohmann::json& json_hole, const Slic3r::sla::DrainHole& hole)
        {
            // Vec3f pos
            _BuildEntityVec3f(json_hole[BLName_Val(pos)], hole.pos);
            // Vec3f normal
            _BuildEntityVec3f(json_hole[BLName_Val(normal)], hole.normal);
            // float radius
            json_hole[BLName_Val(radius)] = hole.radius;
            // float height
            json_hole[BLName_Val(height)] = hole.height;
            // bool  failed
            json_hole[BLName_Val(failed)] = hole.failed;
            // size_t steps
            json_hole[BLName_Val(steps)] = hole.steps;
        });
    // CutConnectors           cut_connectors
    _BuildObjectGroup(json_object[BLName_Val(cut_connectors)], object.cut_connectors,
        [](nlohmann::json& json_cut_connector, const Slic3r::CutConnector& cut_connector)
        {
            // Vec3d pos
            _BuildEntityVec3d(json_cut_connector[BLName_Val(position)], cut_connector.pos);
            // Transform3d rotation_m
            _BuildEntityTranform3d(json_cut_connector[BLName_Val(rotation)], cut_connector.rotation_m);
            // float radius
            json_cut_connector[BLName_Val(radius)] = cut_connector.radius;
            // float height
            json_cut_connector[BLName_Val(height)] = cut_connector.height;
            // float radius_tolerance
            json_cut_connector[BLName_Val(radius_tolerance)] = cut_connector.radius_tolerance;
            // float height_tolerance
            json_cut_connector[BLName_Val(height_tolerance)] = cut_connector.height_tolerance;
            // float z_angle
            json_cut_connector[BLName_Val(z_angle)] = cut_connector.z_angle;
            // CutConnectorAttributes attribs
            // CutConnectorType    type
            json_cut_connector[BLName_Val(attribs)][BLName_Val(type)] = cut_connector.attribs.type;
            // CutConnectorStyle   style
            json_cut_connector[BLName_Val(attribs)][BLName_Val(style)] = cut_connector.attribs.style;
            // CutConnectorShape   shape
            json_cut_connector[BLName_Val(attribs)][BLName_Val(shape)] = cut_connector.attribs.shape;
        });
    // CutObjectBase           cut_id
    // size_t m_check_sum
    json_object[BLName_Val(cut_id)][BLName_Val(check_sum)] = object.cut_id.check_sum();
    // size_t m_connectors_cnt
    json_object[BLName_Val(cut_id)][BLName_Val(connectors_cnt)] = object.cut_id.connectors_cnt();
    // Vec3d                   origin_translation
    _BuildEntityVec3d(json_object[BLName_Val(origin_translation)], object.origin_translation);
    // std::vector<ObjectID>   volume_ids
    _BuildObjectGroup(json_object[BLName_Val(volume_ids)], object.volume_ids,
        [](nlohmann::json& json_volume_id, const Slic3r::ObjectID& volume_id)
        {
            json_volume_id = volume_id.id;
        });
}

bool BaselineOrcaInput::_CompareEntityModelObject(const nlohmann::json& json_object, const Slic3r::ModelObject& object, BaseLineLogger& log)
{
    bool err = true;

    // std::string             name
    err &= _CompareObject(json_object, BLName_Val(name), object.name.c_str(), log);
    // std::string             module_name
    err &= _CompareObject(json_object, BLName_Val(module_name),object.module_name, log);
    // std::string             input_file
    err &= _CompareObjectPath(json_object, BLName_Val(input_file), ""/*object.input_file*/, log);
    // ModelInstancePtrs       instances
    err &= _CompareObjectGroup(json_object, BLName_Val(instance), object.instances, log, _CompareEntityModelInstance);
    // ModelVolumePtrs         volumes
    err &= _CompareObjectGroup(json_object, BLName_Val(volume), object.volumes, log, _CompareEntityModelVolume);
    // ModelConfigObject         config
    err &= _CompareObject(json_object, BLName_Val(config), object.config, log, _CompareEntityModelConfigObject);
    // t_layer_config_ranges   layer_config_ranges
    err &= _CompareObjectMap(json_object, BLName_Val(layer_config_ranges), object.layer_config_ranges, log, _CompareEntityModelConfig);
    // LayerHeightProfile      layer_height_profilel
    err &= _CompareObjectGroup(json_object, BLName_Val(layer_height_profile), object.layer_height_profile.get(), log);
    // bool                    printable
    err &= _CompareObject(json_object, BLName_Val(printable), object.printable, log);
    // sla::SupportPoints      sla_support_points
    err &= _CompareObjectGroup(json_object, BLName_Val(sla_support_points), object.sla_support_points, log,
        [](const nlohmann::json& json_object, const Slic3r::sla::SupportPoint& point, BaseLineLogger& log)
        {
            bool err = true;
            // Vec3f pos
            err &= _CompareObject(json_object, BLName_Val(pos), point.pos, log, _CompareEntityVec3f);
            // float head_front_radius
            err &= _CompareObject(json_object, BLName_Val(head_front_radius), point.head_front_radius, log);
            // bool  is_new_island
            err &= _CompareObject(json_object, BLName_Val(is_new_island), point.is_new_island, log);

            BLReturnBoolen(err);
        });
    // sla::PointsStatus       sla_points_status
    err &= _CompareObject(json_object, BLName_Val(sla_points_status), object.sla_points_status, log);
    // sla::DrainHoles         sla_drain_holes
    err &= _CompareObjectGroup(json_object, BLName_Val(sla_drain_holes), object.sla_drain_holes, log,
        [](const nlohmann::json& json_object, const Slic3r::sla::DrainHole& hole, BaseLineLogger& log)
        {
            bool err = true;
            // Vec3f pos
            err &= _CompareObject(json_object, BLName_Val(pos), hole.pos, log, _CompareEntityVec3f);
            // Vec3f normal
            err &= _CompareObject(json_object, BLName_Val(normal), hole.normal, log, _CompareEntityVec3f);
            // Vec3f radius
            err &= _CompareObject(json_object, BLName_Val(radius), hole.radius, log);
            // float height
            err &= _CompareObject(json_object, BLName_Val(height), hole.height, log);
            // bool  failed
            err &= _CompareObject(json_object, BLName_Val(failed), hole.failed, log);
            // size_t steps
            err &= _CompareObject(json_object, BLName_Val(steps), hole.steps, log);

            BLReturnBoolen(err);
        });
    // CutConnectors           cut_connectors
    err &= _CompareObjectGroup(json_object, BLName_Val(cut_connectors), object.cut_connectors, log,
        [](const nlohmann::json& json_object, const Slic3r::CutConnector& connector, BaseLineLogger& log)
        {
            bool err = true;
            // Vec3d pos
            err &= _CompareObject(json_object, BLName_Val(pos), connector.pos, log, _CompareEntityVec3d);
            // Transform3d rotation_m
            err &= _CompareObject(json_object, BLName_Val(rotation), connector.rotation_m, log, _CompareEntityTranform3d);
            // float radius
            err &= _CompareObject(json_object, BLName_Val(radius), connector.radius, log);
            // float height
            err &= _CompareObject(json_object, BLName_Val(height), connector.height, log);
            // float radius_tolerance
            err &= _CompareObject(json_object, BLName_Val(radius_tolerance), connector.radius_tolerance, log);
            // float height_tolerance
            err &= _CompareObject(json_object, BLName_Val(height_tolerance), connector.height_tolerance, log);
            // float z_angle
            err &= _CompareObject(json_object, BLName_Val(z_angle), connector.z_angle, log);
            // CutConnectorAttributes attribs
            err &= _CompareObject(json_object, BLName_Val(attribs), connector.attribs, log,
                [](const nlohmann::json& json_object, const Slic3r::CutConnectorAttributes& attribs, BaseLineLogger& log)
                {
                    bool err = true;
                    // CutConnectorType    type
                    err &= _CompareObject(json_object, BLName_Val(type), attribs.type, log);
                    // CutConnectorStyle   style
                    err &= _CompareObject(json_object, BLName_Val(style), attribs.style, log);
                    // CutConnectorShape   shape
                    err &= _CompareObject(json_object, BLName_Val(shape), attribs.shape, log);

                    BLReturnBoolen(err);
                });
            BLReturnBoolen(err);
        });
    // CutObjectBase           cut_id
    err &= _CompareObject(json_object, BLName_Val(cut_id), object.cut_id, log,
        [](const nlohmann::json& json_object, const Slic3r::CutObjectBase& cut_id, BaseLineLogger& log)
        {
            bool err = true;
            // size_t m_check_sum
            err &= _CompareObject(json_object, BLName_Val(check_sum), cut_id.check_sum(), log);
            // size_t m_connectors_cnt
            err &= _CompareObject(json_object, BLName_Val(connectors_cnt), cut_id.connectors_cnt(), log);

            BLReturnBoolen(err);
        });
    // Vec3d                   origin_translation
    err &= _CompareObject(json_object, BLName_Val(origin_translation), object.origin_translation, log, _CompareEntityVec3d);
    // std::vector<ObjectID>   volume_ids
    err &= _CompareObjectGroup(json_object, BLName_Val(volume_ids), object.volume_ids, log,
        [](const nlohmann::json& json_object, const Slic3r::ObjectID& volume_id, BaseLineLogger& log)
        {
            BLReturnBoolen(_CompareEntity(json_object, volume_id.id, log));
        });

    BLReturnBoolen(err);
}

void BaselineOrcaInput::_BuildEntityPath(nlohmann::json& json_object, const std::string& file)
{
    json_object = "";
    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << boost::format(", file warning: %1%\n") % file;
    if (file.size() > 0) 
    {
        json_object = std::filesystem::path(file).filename().string();
    }
}

bool BaselineOrcaInput::_CompareEntityPath(const nlohmann::json& json_object, const std::string& file, BaseLineLogger& log)
{
    std::string filename;
    if (file.size() > 0)
    {
        filename = std::filesystem::path(file).filename().string();
        BLReturnBoolen(false);
    }

    if (json_object.type() != nlohmann::json::value_t::string)
    {
        log.LogError("The baseline value is NOT file while the object value is. ");
        BLReturnBoolen(false);
    }

    const std::string& baseline = json_object.template get<std::string>();
    if (baseline != filename)
    {
        log.LogError("The baseline value is " + baseline + " while the object value is " + file);
        BLReturnBoolen(false);
    }

    BLReturnBoolen(true);
}

void BaselineOrcaInput::_BuildEntityModelInstance(nlohmann::json& json_instance, const Slic3r::ModelInstance& instance)
{
    // Geometry::Transformation m_transformation
    _BuildEntityTransformation(json_instance[BLName_Val(transformation)], instance.get_transformation());
    // Geometry::Transformation m_assemble_transformation;
    _BuildEntityTransformation(json_instance[BLName_Val(assemble_transform)], instance.get_assemble_transformation());
    // Vec3d m_offset_to_assembly
    _BuildEntityVec3d(json_instance[BLName_Val(offset_to_assembly)], instance.get_offset_to_assembly());
    // bool m_assemble_initialized
    json_instance[BLName_Val(assemble_initialized)] = const_cast<Slic3r::ModelInstance&>(instance).is_assemble_initialized();
    // ModelInstanceEPrintVolumeState print_volume_state
    json_instance[BLName_Val(print_volume_state)] = instance.print_volume_state;
    // bool printable
    json_instance[BLName_Val(printable)] = instance.printable;
    // bool use_loaded_id_for_label
    json_instance[BLName_Val(use_loaded_id_for_label)] = instance.use_loaded_id_for_label;
    // int arrange_order
    json_instance[BLName_Val(arrange_order)] = instance.arrange_order;
    // size_t loaded_id
    json_instance[BLName_Val(loaded_id)] = instance.loaded_id;
}

bool BaselineOrcaInput::_CompareEntityModelInstance(const nlohmann::json& json_instance, const Slic3r::ModelInstance& instance, BaseLineLogger& log)
{
    bool err = true;
    // Geometry::Transformation m_transformation
    err &= _CompareObject(json_instance, BLName_Val(transformation), instance.get_transformation(), log, _CompareEntityTransformation);
    // Geometry::Transformation m_assemble_transformation;
    err &= _CompareObject(json_instance, BLName_Val(assemble_transform), instance.get_assemble_transformation(), log, _CompareEntityTransformation);
    // Vec3d m_offset_to_assembly
    err &= _CompareObject(json_instance, BLName_Val(offset_to_assembly), instance.get_offset_to_assembly(), log, _CompareEntityVec3d);
    // bool m_assemble_initialized
    err &= _CompareObject(json_instance, BLName_Val(assemble_initialized), const_cast<Slic3r::ModelInstance&>(instance).is_assemble_initialized(), log);
    // ModelInstanceEPrintVolumeState print_volume_state
    err &= _CompareObject(json_instance, BLName_Val(print_volume_state), instance.print_volume_state, log);
    // bool printable
    err &= _CompareObject(json_instance, BLName_Val(printable), instance.printable, log);
    // bool use_loaded_id_for_label
    err &= _CompareObject(json_instance, BLName_Val(use_loaded_id_for_label), instance.use_loaded_id_for_label, log);
    // int arrange_order
    err &= _CompareObject(json_instance, BLName_Val(arrange_order), instance.arrange_order, log);
    // size_t loaded_id
    err &= _CompareObject(json_instance, BLName_Val(loaded_id), instance.loaded_id, log);

    BLReturnBoolen(err);
}

void BaselineOrcaInput::_BuildEntityTransformation(nlohmann::json& json_object, const Slic3r::Geometry::Transformation& transform)
{
    _BuildEntityTranform3d(json_object, transform.get_matrix());
}

bool BaselineOrcaInput::_CompareEntityTransformation(const nlohmann::json& json_object, const Slic3r::Geometry::Transformation& transform, BaseLineLogger& log)
{
    BLReturnBoolen(_CompareEntityTranform3d(json_object, transform.get_matrix(), log));
}

void BaselineOrcaInput::_BuildEntityTranform3d(nlohmann::json& json_object, const Slic3r::Transform3d& transform)
{
    json_object = { transform(0,0), transform(0,1),transform(0,2),
                    transform(1,0), transform(1,1),transform(1,2),
                    transform(2,0), transform(2,1),transform(2,2) };
}

bool BaselineOrcaInput::_CompareEntityTranform3d(const nlohmann::json& json_object, const Slic3r::Transform3d& transform, BaseLineLogger& log)
{
    if (json_object.size() != 9)
    {
        log.LogError("The baseline does NOT Transform3d while the object is.");
        BLReturnBoolen(false);
    }

    if(!_CompareValue((double)json_object.at(0), transform(0, 0))
    || !_CompareValue((double)json_object.at(1), transform(0, 1))
    || !_CompareValue((double)json_object.at(2), transform(0, 2))
    || !_CompareValue((double)json_object.at(3), transform(1, 0))
    || !_CompareValue((double)json_object.at(4), transform(1, 1))
    || !_CompareValue((double)json_object.at(5), transform(1, 2))
    || !_CompareValue((double)json_object.at(6), transform(2, 0))
    || !_CompareValue((double)json_object.at(7), transform(2, 1))
    || !_CompareValue((double)json_object.at(8), transform(2, 2)))
    {
        log.LogError("The baseline transform is " 
            "M(0, 0) = " + std::to_string((double)json_object.at(0)) + "M(0, 1) = " + std::to_string((double)json_object.at(1)) + "M(0, 2) = " + std::to_string((double)json_object.at(2)) +
            "M(1, 0) = " + std::to_string((double)json_object.at(3)) + "M(1, 1) = " + std::to_string((double)json_object.at(4)) + "M(1, 2) = " + std::to_string((double)json_object.at(5)) +
            "M(2, 0) = " + std::to_string((double)json_object.at(6)) + "M(2, 1) = " + std::to_string((double)json_object.at(7)) + "M(2, 2) = " + std::to_string((double)json_object.at(8)) +
            "while object transform is "
            "M(0, 0) = " + std::to_string(transform(0, 0)) + "M(0, 1) = " + std::to_string(transform(0, 1)) + "M(0, 2) = " + std::to_string(transform(0, 2)) +
            "M(1, 0) = " + std::to_string(transform(1, 0)) + "M(1, 1) = " + std::to_string(transform(1, 1)) + "M(1, 2) = " + std::to_string(transform(1, 2)) +
            "M(2, 0) = " + std::to_string(transform(2, 0)) + "M(2, 1) = " + std::to_string(transform(2, 1)) + "M(2, 2) = " + std::to_string(transform(2, 2)));

        BLReturnBoolen(false);
    }

    BLReturnBoolen(true);
}

void BaselineOrcaInput::_BuildEntityPolygon(nlohmann::json& json_object, const Slic3r::Polygon& polygon)
{
    _BuildObjectGroup(json_object[BLName_Val(points)], polygon.points, _BuildEntityVec2crd);
}

bool BaselineOrcaInput::_CompareEntityPolygon(const nlohmann::json& json_object, const Slic3r::Polygon& polygon, BaseLineLogger& log)
{
    BLReturnBoolen(_CompareObjectGroup(json_object, BLName_Val(points), polygon.points, log, _CompareEntityVec2crd));
}

void BaselineOrcaInput::_BuildEntityExPolygon(nlohmann::json& json_object, const Slic3r::ExPolygon& expolygon)
{
    // Polygon  contour
    _BuildEntityPolygon(json_object[BLName_Val(contour)], expolygon.contour);
    // Polygons holes
    _BuildObjectGroup(json_object[BLName_Val(holes)], expolygon.holes, _BuildEntityPolygon);
}

bool BaselineOrcaInput::_CompareEntityExPolygon(const nlohmann::json& json_object, const Slic3r::ExPolygon& expolygon, BaseLineLogger& log)
{
    bool err = true;
    // Polygon  contour
    err &= _CompareObject(json_object, BLName_Val(contour), expolygon.contour, log, _CompareEntityPolygon);
    // Polygons holes
    err &= _CompareObjectGroup(json_object, BLName_Val(holes), expolygon.holes, log, _CompareEntityPolygon);

    BLReturnBoolen(err);
}

void BaselineOrcaInput::_BuildEntityVtxIdx(nlohmann::json& json_object, const stl_triangle_vertex_indices& vec)
{
    json_object = { vec.x(), vec.y(), vec.z() };
}

bool BaselineOrcaInput::_CompareEntityVtxIdx(const nlohmann::json& json_object, const stl_triangle_vertex_indices& vec, BaseLineLogger& log)
{
    if (!json_object.is_array() && json_object.size() != 3)
    {
        log.LogError("The baseline type is NOT Vec3d. ");
        BLReturnBoolen(false);
    }

    int x = json_object.at(0);
    int y = json_object.at(1);
    int z = json_object.at(2);
    if (!_CompareValue(x, vec.x()) || !_CompareValue(y, vec.y()) || !_CompareValue(z, vec.z()))
    {
        log.LogError("The baseline value is ["
            + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) +
            " ] while the object value is ["
            + std::to_string(vec.x()) + ", " + std::to_string(vec.y()) + ", " + std::to_string(vec.z())
            + "]");

        BLReturnBoolen(false);
    }

    BLReturnBoolen(true);
}

void BaselineOrcaInput::_BuildEntityVec2d(nlohmann::json& json_object, const Slic3r::Vec2d& vec)
{
    json_object = { vec.x(), vec.y() };
}

bool BaselineOrcaInput::_CompareEntityVec2d(const nlohmann::json& json_object, const Slic3r::Vec2d& vec, BaseLineLogger& log)
{
    if (!json_object.is_array() && json_object.size() != 2)
    {
        log.LogError("The baseline type is NOT Vec2d. ");
        BLReturnBoolen(false);
    }

    double x = json_object.at(0);
    double y = json_object.at(1);
    if (!_CompareValue(x, vec.x()) || !_CompareValue(y, vec.y()))
    {
        log.LogError("The baseline value is [ "
            + std::to_string(x) + ", " + std::to_string(y) +
            " ] while the object value is ["
            + std::to_string(vec.x()) + ", " + std::to_string(vec.y())
            + ")");

        BLReturnBoolen(false);
    }

    BLReturnBoolen(true);
}

void BaselineOrcaInput::_BuildEntityVec2crd(nlohmann::json& json_object, const Slic3r::Vec2crd& vec)
{
    json_object = { vec.x(), vec.y() };
}

bool BaselineOrcaInput::_CompareEntityVec2crd(const nlohmann::json& json_object, const Slic3r::Vec2crd& vec, BaseLineLogger& log)
{
    if (!json_object.is_array() && json_object.size() != 2)
    {
        log.LogError("The baseline type is NOT Vec2crd. ");
        BLReturnBoolen(false);
    }

    int32_t x = json_object.at(0);
    int32_t y = json_object.at(1);
    if (!_CompareValue(x, vec.x()) || !_CompareValue(y, vec.y()))
    {
        log.LogError("The baseline value is [ "
            + std::to_string(x) + ", " + std::to_string(y) +
            " ] while the object value is ["
            + std::to_string(vec.x()) + ", " + std::to_string(vec.y())
            + ")");

        BLReturnBoolen(false);
    }

    BLReturnBoolen(true);
}

void BaselineOrcaInput::_BuildEntityVec3d(nlohmann::json& json_object, const Slic3r::Vec3d& vec)
{
    json_object = { vec.x(), vec.y(), vec.z() };
}

bool BaselineOrcaInput::_CompareEntityVec3d(const nlohmann::json& json_object, const Slic3r::Vec3d& vec, BaseLineLogger& log)
{
    if (!json_object.is_array() && json_object.size() != 3)
    {
        log.LogError("The baseline type is NOT Vec3d. ");
        BLReturnBoolen(false);
    }

    double x = json_object.at(0);
    double y = json_object.at(1);
    double z = json_object.at(2);
    if (!_CompareValue(x, vec.x()) || !_CompareValue(y, vec.y()) || !_CompareValue(z, vec.z()))
    {
        log.LogError("The baseline value is ["
            + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) +
            " ] while the object value is ["
            + std::to_string(vec.x()) + ", " + std::to_string(vec.y()) + ", " + std::to_string(vec.z())
            + "]");

        BLReturnBoolen(false);
    }

    BLReturnBoolen(true);
}

void BaselineOrcaInput::_BuildEntityVec3f(nlohmann::json& json_object, const Slic3r::Vec3f& vec)
{
    json_object = { vec.x(), vec.y(), vec.z() };
}

bool BaselineOrcaInput::_CompareEntityVec3f(const nlohmann::json& json_object, const Slic3r::Vec3f& vec, BaseLineLogger& log)
{
    if (!json_object.is_array() && json_object.size() != 3)
    {
        log.LogError("The baseline type is NOT Vec3f. ");
        BLReturnBoolen(false);
    }

    float x = json_object.at(0);
    float y = json_object.at(1);
    float z = json_object.at(2);
    if (!_CompareValue(x, vec.x()) || !_CompareValue(y, vec.y()) || !_CompareValue(z, vec.z()))
    {
        log.LogError("The baseline value is["
            + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) +
            " ] while the object value is ["
            + std::to_string(vec.x()) + ", " + std::to_string(vec.y()) + ", " + std::to_string(vec.z())
            + "]");

        BLReturnBoolen(false);
    }

    BLReturnBoolen(true);
}

void BaselineOrcaInput::_BuildEntityTextConfig(nlohmann::json& json_object, const Slic3r::TextConfiguration& text_config)
{
    using namespace nlohmann;

    json text_config_style = json_object[BLName_Val(style)];
    // std::string name
    text_config_style[BLName_Val(name)] = text_config.style.name;
    // std::string path
    _BuildEntityPath(text_config_style[BLName_Val(path)], text_config.style.path);
    // Type type
    text_config_style[BLName_Val(type)] = text_config.style.type;
    // FontProp prop
    // std::optional<int> char_gap
    json text_config_style_prop = text_config_style[BLName_Val(prop)];
    _BuildObjectOptional(text_config_style_prop[BLName_Val(char_gap)], text_config.style.prop.char_gap);
    // std::optional<int> line_gap
    _BuildObjectOptional(text_config_style_prop[BLName_Val(line_gap)], text_config.style.prop.line_gap);
    // std::optional<float> boldness
    _BuildObjectOptional(text_config_style_prop[BLName_Val(boldness)], text_config.style.prop.boldness);
    // std::optional<float> skew
    _BuildObjectOptional(text_config_style_prop[BLName_Val(skew)], text_config.style.prop.skew);
    // std::optional<unsigned int> collection_number
    _BuildObjectOptional(text_config_style_prop[BLName_Val(collection_number)], text_config.style.prop.collection_number);
    // bool per_glyph
    text_config_style_prop[BLName_Val(per_glyph)] = text_config.style.prop.per_glyph;
    // Align align
    text_config_style_prop[BLName_Val(align)]
        = { text_config.style.prop.align.first,
            text_config.style.prop.align.second };
    // float size_in_mm
    text_config_style_prop[BLName_Val(size_in_mm)] = text_config.style.prop.size_in_mm;
    // std::optional<std::string> family
    _BuildObjectOptional(text_config_style_prop[BLName_Val(family)], text_config.style.prop.family);
    // std::optional<std::string> face_name
    _BuildObjectOptional(text_config_style_prop[BLName_Val(face_name)], text_config.style.prop.face_name);
    // std::optional<std::string> style
    _BuildObjectOptional(text_config_style_prop[BLName_Val(style)], text_config.style.prop.style);
    // std::optional<std::string> weight
    _BuildObjectOptional(text_config_style_prop[BLName_Val(weight)], text_config.style.prop.weight);
    // std::string text
    text_config_style[BLName_Val(text)] = text_config.text;
}

bool BaselineOrcaInput::_CompareEntityTextConfig(const nlohmann::json& json_object, const Slic3r::TextConfiguration& text_config, BaseLineLogger& log)
{
    bool err = true;

    // EmbossStyle style
    err &= _CompareObject(json_object, BLName_Val(style), text_config.style, log,
        [](const nlohmann::json& json_object, const Slic3r::EmbossStyle& style, BaseLineLogger& log)
        {
            bool err = true;
            // std::string name
            err &= _CompareObject(json_object, BLName_Val(name), style.name, log);
            // std::string path
            err &= _CompareObjectPath(json_object, BLName_Val(path), style.path, log);
            // Type type
            err &= _CompareObject(json_object, BLName_Val(type), style.type, log);
            // FontProp prop
            err &= _CompareObject(json_object, BLName_Val(prop), style.prop, log,
                [](const nlohmann::json& json_object, const Slic3r::FontProp& property, BaseLineLogger& log)
                {
                    bool err = true;
                    // std::optional<int> char_gap
                    err &= _CompareObjectOptional(json_object, BLName_Val(char_gap), property.char_gap, log);
                    // std::optional<int> line_gap
                    err &= _CompareObjectOptional(json_object, BLName_Val(line_gap), property.line_gap, log);
                    // std::optional<float> boldness
                    err &= _CompareObjectOptional(json_object, BLName_Val(boldness), property.boldness, log);
                    // std::optional<float> skew
                    err &= _CompareObjectOptional(json_object, BLName_Val(skew), property.skew, log);
                    // std::optional<unsigned int> collection_number
                    err &= _CompareObjectOptional(json_object, BLName_Val(collection_number), property.collection_number, log);
                    // bool per_glyph
                    err &= _CompareObject(json_object, BLName_Val(per_glyph), property.per_glyph, log);
                    // Align align
                    std::vector<int> align = {
                        std::underlying_type_t<Slic3r::FontProp::HorizontalAlign>(property.align.first),
                        std::underlying_type_t<Slic3r::FontProp::VerticalAlign>(property.align.second) };
                    err &= _CompareObjectGroup(json_object, BLName_Val(align), align, log);
                    // float size_in_mm
                    err &= _CompareObject(json_object, BLName_Val(size_in_mm), property.size_in_mm, log);
                    // std::optional<std::string> family
                    err &= _CompareObjectOptional(json_object, BLName_Val(family), property.family, log);
                    // std::optional<std::string> face_name
                    err &= _CompareObjectOptional(json_object, BLName_Val(face_name), property.face_name, log);
                    // std::optional<std::string> style
                    err &= _CompareObjectOptional(json_object, BLName_Val(style), property.style, log);
                    // std::optional<std::string> weight
                    err &= _CompareObjectOptional(json_object, BLName_Val(weight), property.weight, log);

                    BLReturnBoolen(err);
                });

            BLReturnBoolen(err);
        });
    // std::string text
    err &= _CompareObject(json_object, BLName_Val(text), text_config.text, log);

    BLReturnBoolen(err);
}

void BaselineOrcaInput::_BuildEntityModelVolume(nlohmann::json& json_volume, const Slic3r::ModelVolume& volume)
{
    using namespace nlohmann;

    // std::string         name
    json_volume[BLName_Val(name)] = volume.name;
    // std::string input_file
    _BuildEntityPath(json_volume[BLName_Val(source)][BLName_Val(input_file)], ""/*volume.source.input_file*/);
    // int object_idx
    json_volume[BLName_Val(source)][BLName_Val(object_idx)] = volume.source.object_idx;
    // int volume_idx
    json_volume[BLName_Val(source)][BLName_Val(volume_idx)] = volume.source.volume_idx;
    // Vec3d mesh_offset
    _BuildEntityVec3d(json_volume[BLName_Val(source)][BLName_Val(mesh_offset)], volume.source.mesh_offset);
    // Geometry::Transformation transform
    _BuildEntityTransformation(json_volume[BLName_Val(source)][BLName_Val(transform)], volume.source.transform);
    // bool is_converted_from_inches
    json_volume[BLName_Val(source)][BLName_Val(is_converted_from_inches)] = volume.source.is_converted_from_inches;
    // bool is_converted_from_meters
    json_volume[BLName_Val(source)][BLName_Val(is_converted_from_meters)] = volume.source.is_converted_from_meters;
    // bool is_from_builtin_objects
    json_volume[BLName_Val(source)][BLName_Val(is_from_builtin_objects)] = volume.source.is_from_builtin_objects;
    // bool                is_from_upper
    json_volume[BLName_Val(cut_info)][BLName_Val(from_upper)] = volume.cut_info.is_from_upper;
    // bool                is_connector
    json_volume[BLName_Val(cut_info)][BLName_Val(is_connector)] = volume.cut_info.is_connector;
    // bool                is_processed
    json_volume[BLName_Val(cut_info)][BLName_Val(is_processed)] = volume.cut_info.is_processed;
    // CutConnectorType    connector_type
    json_volume[BLName_Val(cut_info)][BLName_Val(connector_type)] = volume.cut_info.connector_type;
    // float               radius_tolerance
    json_volume[BLName_Val(cut_info)][BLName_Val(radius_tolerance)] = volume.cut_info.radius_tolerance;
    // float               height_tolerance
    json_volume[BLName_Val(cut_info)][BLName_Val(height_tolerance)] = volume.cut_info.height_tolerance;
    // ModelConfigObject    config
    _BuildEntityModelConfigObject(json_volume[BLName_Val(config)], volume.config);
    // FacetsAnnotation    supported_facets
    _BuildEntityFacetsAnnotation(json_volume[BLName_Val(supported_facets)], volume.supported_facets);
    // FacetsAnnotation    seam_facets
    _BuildEntityFacetsAnnotation(json_volume[BLName_Val(seam_facets)], volume.seam_facets);
    // FacetsAnnotation    mmu_segmentation_facets
    _BuildEntityFacetsAnnotation(json_volume[BLName_Val(mmu_segmentation_facets)], volume.mmu_segmentation_facets);
    // std::vector<int> mmuseg_extruders
    _BuildObjectGroup(json_volume[BLName_Val(mmuseg_extruders)], volume.mmuseg_extruders);
    // FacetsAnnotation    exterior_facets
    _BuildEntityFacetsAnnotation(json_volume[BLName_Val(exterior_facets)], volume.exterior_facets);
    // std::optional<TextConfiguration> text_configuration
    _BuildObjectOptional(json_volume[BLName_Val(text_configuration)], volume.text_configuration, _BuildEntityTextConfig);
    // std::optional<EmbossShape> emboss_shape
    _BuildObjectOptional(json_volume[BLName_Val(emboss_shape)], volume.emboss_shape, _BuildEntityEmbossShape);
    // ModelObject*                        object
    json_volume[BLName_Val(object)] = "TODO";
    // std::shared_ptr<const TriangleMesh> m_mesh
    _BuildObjectSharedPtr(json_volume[BLName_Val(mesh)], volume.mesh_ptr(), _BuildEntityMesh);
    // ModelVolumeType                     m_type
    json_volume[BLName_Val(type)] = volume.type();
    // t_model_material_id                 m_material_id
    json_volume[BLName_Val(material_id)] = volume.material_id();
    // std::shared_ptr<const TriangleMesh> m_convex_hull
    _BuildObjectSharedPtr(json_volume[BLName_Val(convex_hull)], volume.get_convex_hull_shared_ptr(), _BuildEntityMesh);
    // mutable polygon                     m_convex_hull_2d
    json_volume[BLName_Val(m_convex_hull_2d)] = "TODO";
    // mutable transform3d                 m_cached_trans_matrix
    json_volume[BLName_Val(cached_trans_matrix)] = "TODO";
    // mutable polygon                     m_cached_2d_polygon
    json_volume[BLName_Val(cached_2d_polygon)] = "TODO";
    // Geometry::Transformation            m_transformation
    _BuildEntityTransformation(json_volume[BLName_Val(transformation)], volume.get_transformation());
}

bool BaselineOrcaInput::_CompareEntityModelVolume(const nlohmann::json& json_object, const Slic3r::ModelVolume& volume, BaseLineLogger& log)
{
    bool err = true;

    // std::string         name
    err &= _CompareObject(json_object, BLName_Val(name), volume.name, log);
    // Source              source
    err &= _CompareObject(json_object, BLName_Val(source), volume.source, log,
        [](const nlohmann::json& json_object, const Slic3r::ModelVolume::Source& source, BaseLineLogger& log)
        {
            bool err = true;
            // std::string input_file
            err &= _CompareObjectPath(json_object, BLName_Val(input_file),"" /*source.input_file*/, log);
            // int object_idx
            err &= _CompareObject(json_object, BLName_Val(object_idx), source.object_idx, log);
            // int volume_idx
            err &= _CompareObject(json_object, BLName_Val(volume_idx), source.volume_idx, log);
            // Vec3d mesh_offset
            err &= _CompareObject(json_object, BLName_Val(mesh_offset), source.mesh_offset, log, _CompareEntityVec3d);
            // Geometry::Transformation transform
            err &= _CompareObject(json_object, BLName_Val(transform), source.transform, log, _CompareEntityTransformation);
            // bool is_converted_from_inches
            err &= _CompareObject(json_object, BLName_Val(is_converted_from_inches), source.is_converted_from_inches, log);
            // bool is_converted_from_meters
            err &= _CompareObject(json_object, BLName_Val(is_converted_from_meters), source.is_converted_from_meters, log);
            // bool is_from_builtin_objects
            err &= _CompareObject(json_object, BLName_Val(is_from_builtin_objects), source.is_from_builtin_objects, log);

            BLReturnBoolen(err);
        });
    // CutInfo             cut_info;
    err &= _CompareObject(json_object, BLName_Val(cut_info), volume.cut_info, log,
        [](const nlohmann::json& json_object, const Slic3r::ModelVolume::CutInfo& cut_info, BaseLineLogger& log)
        {
            bool err = true;
            // bool                is_from_upper
            err &= _CompareObject(json_object, BLName_Val(from_upper), cut_info.is_from_upper, log);
            // bool                is_connector
            err &= _CompareObject(json_object, BLName_Val(is_connector), cut_info.is_connector, log);
            // bool                is_processed
            err &= _CompareObject(json_object, BLName_Val(is_processed), cut_info.is_processed, log);
            // CutConnectorType    connector_type
            err &= _CompareObject(json_object, BLName_Val(connector_type), cut_info.connector_type, log);
            // float               radius_tolerance
            err &= _CompareObject(json_object, BLName_Val(radius_tolerance), cut_info.radius_tolerance, log);
            // float               height_tolerance
            err &= _CompareObject(json_object, BLName_Val(height_tolerance), cut_info.height_tolerance, log);

            BLReturnBoolen(err);
        });
    // ModelConfigObject    config
    err &= _CompareObject(json_object, BLName_Val(config), volume.config, log, _CompareEntityModelConfigObject);
    // FacetsAnnotation    supported_facets
    err &= _CompareObject(json_object, BLName_Val(supported_facets), volume.supported_facets, log, _CompareEntityFacetsAnnotation);
    // FacetsAnnotation    seam_facets
    err &= _CompareObject(json_object, BLName_Val(seam_facets), volume.seam_facets, log, _CompareEntityFacetsAnnotation);
    // FacetsAnnotation    mmu_segmentation_facets
    // todo :lisugui
    //err &= _CompareObject(json_object, BLName_Val(mmu_segmentation_facets), volume.mmu_segmentation_facets, log, _CompareEntityFacetsAnnotation);
    
    // std::vector<int> mmuseg_extruders
    err &= _CompareObjectGroup(json_object, BLName_Val(mmuseg_extruders), volume.mmuseg_extruders, log);
    // FacetsAnnotation    exterior_facets
    err &= _CompareObject(json_object, BLName_Val(exterior_facets), volume.exterior_facets, log, _CompareEntityFacetsAnnotation);
    // todo :lisugui
    //err &= _CompareObjectOptional(json_object, BLName_Val(text_configuration), volume.text_configuration, log, _CompareEntityTextConfig);
    //err &= _CompareObjectOptional(json_object, BLName_Val(emboss_shape), volume.emboss_shape, log, _CompareEntityEmbossShape);
    // modelobject*                        object
    err &= _CompareObject(json_object, BLName_Val(object), "TODO", log);
    // std::shared_ptr<const TriangleMesh> m_mesh
    err &= _CompareObjectSharedPtr(json_object, BLName_Val(mesh), volume.mesh_ptr(), log, _CompareEntityMesh);
    // ModelVolumeType                     m_type
    err &= _CompareObject(json_object, BLName_Val(type), volume.type(), log);
    // t_model_material_id                 m_material_id
    err &= _CompareObject(json_object, BLName_Val(material_id), volume.material_id(), log);
    // std::shared_ptr<const TriangleMesh> m_convex_hull
    err &= _CompareObjectSharedPtr(json_object, BLName_Val(convex_hull), volume.get_convex_hull_shared_ptr(), log, _CompareEntityMesh);
    // mutable polygon                     m_convex_hull_2d
    err &= _CompareObject(json_object, BLName_Val(m_convex_hull_2d), "TODO", log);
    // mutable transform3d                 m_cached_trans_matrix
    err &= _CompareObject(json_object, BLName_Val(cached_trans_matrix), "TODO", log);
    // mutable polygon                     m_cached_2d_polygon
    err &= _CompareObject(json_object, BLName_Val(cached_2d_polygon), "TODO", log);
    // Geometry::Transformation            m_transformation
    err &= _CompareObject(json_object, BLName_Val(transformation), volume.get_transformation(), log, _CompareEntityTransformation);

    BLReturnBoolen(err);
}

void BaselineOrcaInput::_BuildEntityFacetsAnnotation(nlohmann::json& json_object_group, const Slic3r::FacetsAnnotation& facets)
{
    auto& support_facets = facets.get_data();
    std::vector<int> support_facet_group;
    for (int i = 0; i < support_facets.first.size(); ++i)
    {
        support_facet_group.emplace_back(support_facets.first[i].first);
        support_facet_group.emplace_back(support_facets.first[i].second);
        support_facet_group.emplace_back(support_facets.second[i] ? 1 : 0);
    }
    _BuildObjectGroup(json_object_group, support_facet_group);
}

bool BaselineOrcaInput::_CompareEntityFacetsAnnotation(const nlohmann::json& json_object, const Slic3r::FacetsAnnotation& facets, BaseLineLogger& log)
{
    using namespace nlohmann;
    
    if (!json_object.is_array())
    {
        log.LogError("The baseline is NOT an array while the group is");
        BLReturnBoolen(false);
    }

    auto& support_facets = facets.get_data();
    uint64_t size = support_facets.first.size();
    uint64_t json_size = json_object.size(); 
    if (json_object.size() != support_facets.first.size() * 3)
    {
        log.LogError("The baseline support_facets size is " + std::to_string(json_object.size()) + " while the group size is " + std::to_string(support_facets.first.size()));
        BLReturnBoolen(false);
    }

    bool err = true;
    for (int i = 0; i < support_facets.first.size(); ++i)
    {
        int value0 = json_object.at(i).at(0);
        int value1 = json_object.at(i).at(1);
        bool value2 = json_object.at(i).at(2);
        if (_CompareValue(value0, support_facets.first[i].first)
         || _CompareValue(value1, support_facets.first[i].second)
         || _CompareValue(value2, support_facets.second[i]))
        {
            log.LogError("The baseline support_facet is " +
                std::to_string(value0) + ", " + std::to_string(value1) + ", " + std::to_string(value2) +
                " while the group size is " +
                std::to_string(support_facets.first[i].first) + ", " + std::to_string(support_facets.first[i].second) + ", " + std::to_string(support_facets.second[i]));
            err &= false;
        }
    }

    BLReturnBoolen(err);
}

void BaselineOrcaInput::_BuildEntityEmbossShape(nlohmann::json& json_object, const Slic3r::EmbossShape& emboss_shape)
{
    using namespace nlohmann;

    // expolygonswithids shapes_with_ids
    _BuildObjectGroup(json_object[BLName_Val(shapes_with_ids)], emboss_shape.shapes_with_ids,
        [](nlohmann::json& json_shape, const Slic3r::ExPolygonsWithId& shape)
        {
            // unsigned id
            json_shape[BLName_Val(id)] = shape.id;
            // expolygons expoly
            _BuildObjectGroup(json_shape[BLName_Val(expolygon)], shape.expoly, _BuildEntityExPolygon);
            // bool is_healed
            json_shape[BLName_Val(is_healed)] = shape.is_healed;
        });

    // HealedExPolygons final_shape
    {
        // ExPolygons expolygons
        _BuildObjectGroup(json_object[BLName_Val(final_shape)][BLName_Val(expolygon)], emboss_shape.final_shape.expolygons, _BuildEntityExPolygon);
        // bool is_healed
        json_object[BLName_Val(final_shape)][BLName_Val(is_healed)] = emboss_shape.final_shape.is_healed;
    }
    // double scale
    json_object[BLName_Val(scale)] = emboss_shape.scale;
    // embossprojection projection
    {
        // double depth
        json_object[BLName_Val(projection)][BLName_Val(depth)] = emboss_shape.projection.depth;
        // bool use_surface
        json_object[BLName_Val(projection)][BLName_Val(use_surface)] = emboss_shape.projection.use_surface;
    }
    // std::optional<Slic3r::Transform3d> fix_3mf_tr
    _BuildObjectOptional(json_object[BLName_Val(fix_3mf_tr)], emboss_shape.fix_3mf_tr, _BuildEntityTranform3d);
    // std::optional<SvgFile> svg_file
    _BuildObjectOptional(json_object[BLName_Val(svg_file)], emboss_shape.svg_file, _BuildEntitySvgFile);
}

bool BaselineOrcaInput::_CompareEntityEmbossShape(const nlohmann::json& json_object, const Slic3r::EmbossShape& emboss_shape, BaseLineLogger& log)
{
    bool err = true;
    // expolygonswithids shapes_with_ids
    err &= _CompareObjectGroup(json_object, BLName_Val(shapes_with_ids), emboss_shape.shapes_with_ids, log,
        [](const nlohmann::json& json_object, const Slic3r::ExPolygonsWithId& shape, BaseLineLogger& log)
        {
            bool err = true;
            // unsigned id
            err &= _CompareObject(json_object, BLName_Val(id), shape.id, log);
            // expolygons expoly
            err &= _CompareObjectGroup(json_object, BLName_Val(expolygon), shape.expoly, log, _CompareEntityExPolygon);
            // bool is_healed
            err &= _CompareObject(json_object, BLName_Val(is_healed), shape.is_healed, log);

            BLReturnBoolen(err);
        });
    // HealedExPolygons final_shape
    err &= _CompareObject(json_object, BLName_Val(final_shape), emboss_shape.final_shape, log,
        [](const nlohmann::json& json_object, const Slic3r::HealedExPolygons& shape, BaseLineLogger& log)
        {
            bool err = true;
            // ExPolygons expolygons
            err &= _CompareObjectGroup(json_object, BLName_Val(expolygons), shape.expolygons, log, _CompareEntityExPolygon);
            // bool is_healed
            err &= _CompareObject(json_object, BLName_Val(is_healed), shape.is_healed, log);

            BLReturnBoolen(err);
        });

    // double scale
    err &= _CompareObject(json_object, BLName_Val(scale), emboss_shape.scale, log);
    // embossprojection projection
    err &= _CompareObject(json_object, BLName_Val(final_shape), emboss_shape.projection, log,
        [](const nlohmann::json& json_object, const Slic3r::EmbossProjection& projection, BaseLineLogger& log)
        {
            bool err = true;
            // double depth
            err &= _CompareObject(json_object, BLName_Val(depth), projection.depth, log);
            // bool use_surface
            err &= _CompareObject(json_object, BLName_Val(use_surface), projection.use_surface, log);
            BLReturnBoolen(err);
        });
    // std::optional<Slic3r::transform3d> fix_3mf_tr
    err &= _CompareObjectOptional(json_object, BLName_Val(fix_3mf_tr), emboss_shape.fix_3mf_tr, log, _CompareEntityTranform3d);
    // std::optional<SvgFile> svg_file
    err &= _CompareObjectOptional(json_object, BLName_Val(svg_file), emboss_shape.svg_file, log, _CompareEntitySvgFile);

    BLReturnBoolen(err);
}

void BaselineOrcaInput::_BuildEntitySvgFile(nlohmann::json& json_object, const Slic3r::EmbossShape::SvgFile& svg_file)
{
    using namespace nlohmann;

    // std::string path
    _BuildEntityPath(json_object[BLName_Val(path)], svg_file.path);
    // std::string path_in_3mf
    _BuildEntityPath(json_object[BLName_Val(path_in_3mf)], svg_file.path_in_3mf);
    // std::shared_ptr<NSVGimage> image
    _BuildObjectSharedPtr(json_object[BLName_Val(nsvgimage)], svg_file.image, _BuildEntityImage);
    // std::shared_ptr<std::string> file_data
    _BuildObjectSharedPtr(json_object[BLName_Val(file_data)], svg_file.file_data, _BuildEntityPath);
}

bool BaselineOrcaInput::_CompareEntitySvgFile(const nlohmann::json& json_object, const Slic3r::EmbossShape::SvgFile& svg_file, BaseLineLogger& log)
{
    bool err = true;
    // std::string path
    err &= _CompareObjectPath(json_object, BLName_Val(path), svg_file.path, log);
    // std::string path_in_3mf
    err &= _CompareObjectPath(json_object, BLName_Val(path_in_3mf), svg_file.path_in_3mf, log);
    // std::shared_ptr<NSVGimage> image
    err &= _CompareObjectSharedPtr(json_object, BLName_Val(image), svg_file.image, log, _CompareEntityImage);

    BLReturnBoolen(err);
}

void BaselineOrcaInput::_BuildEntityImage(nlohmann::json& json_object, const NSVGimage& image)
{
    using namespace nlohmann;

    // float width
    json_object[BLName_Val(width)] = image.width;
    // float height
    json_object[BLName_Val(width)] = image.height;
    // nsvgshape* shapes
    _BuildObjectGroup(json_object[BLName_Val(shape)], image.shapes,
        [](nlohmann::json& json_shape, const NSVGshape& shape)
        {
            // char id[64]
            json_shape[BLName_Val(id)] = shape.id;
            // NSVGpaint fill
            _BuildEntityPaint(json_shape[BLName_Val(fill)], shape.fill);
            // NSVGpaint stroke
            _BuildEntityPaint(json_shape[BLName_Val(stroke)], shape.stroke);
            // float opacity
            json_shape[BLName_Val(opacity)] = shape.opacity;
            // float strokeWidth
            json_shape[BLName_Val(strokeWidth)] = shape.strokeWidth;
            // float strokeDashOffset
            json_shape[BLName_Val(strokeDashOffset)] = shape.strokeDashOffset;
            // float strokeDashArray[8]
            const float* strokeDashArray = shape.strokeDashArray;
            json_shape[BLName_Val(strokeDashArray)] = 
            { 
                strokeDashArray[0],strokeDashArray[1],strokeDashArray[2],
                strokeDashArray[3],strokeDashArray[4],strokeDashArray[5] 
            };
            // float strokeDashCount
            json_shape[BLName_Val(strokeDashCount)] = shape.strokeDashCount;
            // float strokeLineCap
            json_shape[BLName_Val(strokeLineCap)] = shape.strokeLineCap;
            // float miterLimit
            json_shape[BLName_Val(miterLimit)] = shape.miterLimit;
            // float fillRule
            json_shape[BLName_Val(fillRule)] = shape.fillRule;
            // float flags
            json_shape[BLName_Val(flags)] = shape.flags;
            // float bounds[4]
            json_shape[BLName_Val(bounds)] = { shape.bounds[0],shape.bounds[1],shape.bounds[2], shape.bounds[3] };
            // NSVGpath* paths
            _BuildObjectGroup(json_shape[BLName_Val(paths)], shape.paths,
                [](nlohmann::json& json_path, const NSVGpath& path)
                {
                    // float* pts
                    json json_point_group = (json_path[BLName_Val(pts)] = json::array());
                    for (int i = 0; i < path.npts; ++i)
                    {
                        json_point_group.push_back(path.pts[i * 2]);
                        json_point_group.push_back(path.pts[i * 2 + 1]);
                    }
                    // int npts
                    json_path[BLName_Val(npts)] = path.npts;
                    // char closed
                    json_path[BLName_Val(closed)] = path.closed;
                    // float bounds[4]
                    json_path[BLName_Val(bounds)] = { path.bounds[0], path.bounds[1], path.bounds[2], path.bounds[3] };

                    return path.next;
                });

            return shape.next;
        });
}

bool BaselineOrcaInput::_CompareEntityImage(const nlohmann::json& json_object, const NSVGimage& image, BaseLineLogger& log)
{
    bool err = true;
    // float width
    err &= _CompareObject(json_object, BLName_Val(width), image.width, log);
    // float height
    err &= _CompareObject(json_object, BLName_Val(height), image.height, log);
    // NSVGpaint* shapes
    err &= _CompareObjectGroupEx(json_object, BLName_Val(shape), image.shapes, log, 
        [](const nlohmann::json& json_object, const NSVGshape& shape, BaseLineLogger& log)
        {
            bool err = true;
            // char id[64]
            err &= _CompareObject(json_object, BLName_Val(id), shape.id, log);
            // NSVGpaint fill
            err &= _CompareObject(json_object, BLName_Val(fill), shape.fill, log, _CompareEntityPaint);
            // NSVGpaint stroke
            err &= _CompareObject(json_object, BLName_Val(stroke), shape.stroke, log, _CompareEntityPaint);
            // float opacity
            err &= _CompareObject(json_object, BLName_Val(opacity), shape.opacity, log);
            // float strokeWidth
            err &= _CompareObject(json_object, BLName_Val(strokeWidth), shape.strokeWidth, log);
            // float strokeDashOffset
            err &= _CompareObject(json_object, BLName_Val(strokeDashOffset), shape.strokeDashOffset, log);
            // float strokeDashArray[8]
            std::vector<float> strokeDashArray = 
            {
                shape.strokeDashArray[0],shape.strokeDashArray[1],shape.strokeDashArray[2],shape.strokeDashArray[3],
                shape.strokeDashArray[4],shape.strokeDashArray[5],shape.strokeDashArray[6],shape.strokeDashArray[7]
            };
            err &= _CompareObjectGroup(json_object, BLName_Val(strokeDashArray), strokeDashArray, log);
            // float strokeDashCount
            err &= _CompareObject(json_object, BLName_Val(strokeDashCount), shape.strokeDashCount, log);
            // float strokeLineCap
            err &= _CompareObject(json_object, BLName_Val(strokeLineCap), shape.strokeLineCap, log);
            // float miterLimit
            err &= _CompareObject(json_object, BLName_Val(miterLimit), shape.miterLimit, log);
            // float fillRule
            err &= _CompareObject(json_object, BLName_Val(fillRule), shape.fillRule, log);
            // float flags
            err &= _CompareObject(json_object, BLName_Val(flags), shape.flags, log);
            // float bounds[4]
            std::vector<float> bounds = {shape.bounds[0],shape.bounds[1],shape.bounds[2],shape.bounds[3]};
            err &= _CompareObjectGroup(json_object, BLName_Val(bounds), bounds, log);
            // NSVGpath* paths
            err &= _CompareObjectGroupEx(json_object, BLName_Val(paths), shape.paths, log,
                [](const nlohmann::json& json_object, const NSVGpath& path, BaseLineLogger& log)
                {
                    bool err = true;
                    // float* pts
                    err &= _CompareObjectGroupEx(json_object, BLName_Val(pts), path.pts, path.npts * 2, log);
                    // int npts
                    err &= _CompareObject(json_object, BLName_Val(npts), path.npts, log);
                    // char closed
                    err &= _CompareObject(json_object, BLName_Val(closed), path.closed, log);
                    // float bounds[4]
                    std::vector<float> bounds = { path.bounds[0],path.bounds[1],path.bounds[2],path.bounds[3] };
                    err &= _CompareObjectGroup(json_object, BLName_Val(bounds), bounds, log);

                    BLReturnBoolen(err);
                },
                [](const NSVGpath& path)
                {
                    return path.next;
                });
            BLReturnBoolen(err);
        },
        [](const NSVGshape& shape)
        {
            return shape.next;
        });

    BLReturnBoolen(err);
}

void BaselineOrcaInput::_BuildEntityPaint(nlohmann::json& json_object, const NSVGpaint& paint)
{
    using namespace nlohmann;

    // char type
    json_object[BLName_Val(type)] = paint.type;
    // unsigned int color
    if ((NSVGpaintType)paint.type == NSVGpaintType::NSVG_PAINT_COLOR)
    {
        json_object[BLName_Val(color)] = paint.color;
    }
    // NSVGgradient* gradient
    if ((NSVGpaintType)paint.type == NSVGpaintType::NSVG_PAINT_LINEAR_GRADIENT
        || (NSVGpaintType)paint.type == NSVGpaintType::NSVG_PAINT_RADIAL_GRADIENT)
    {
        json json_gradient = json_object[BLName_Val(gradient)];
        // float xform[6]
        json_gradient[BLName_Val(xform)] = { paint.gradient->xform[0],paint.gradient->xform[1],paint.gradient->xform[2],
                                             paint.gradient->xform[3],paint.gradient->xform[4],paint.gradient->xform[5] };
        // char spread
        json_gradient[BLName_Val(spread)] = paint.gradient->spread;
        // float fx, fy
        json_gradient[BLName_Val(fx)] = paint.gradient->fx;
        json_gradient[BLName_Val(fy)] = paint.gradient->fy;
        // int nstops
        json_gradient[BLName_Val(nstops)] = paint.gradient->nstops;
        // nsvggradientstop stops[1]
        _BuildObjectGroup(json_object[BLName_Val(gradient)][BLName_Val(stops)], paint.gradient->stops, paint.gradient->nstops,
            [](nlohmann::json& json_stop, const NSVGgradientStop& stop)
            {
                // unsigned int color
                json_stop[BLName_Val(color)] = stop.color;
                // float offset
                json_stop[BLName_Val(offset)] = stop.offset;
            });
    }
}

bool BaselineOrcaInput::_CompareEntityPaint(const nlohmann::json& json_object, const NSVGpaint& paint, BaseLineLogger& log)
{
    bool err = true;

    // char type
    err &= _CompareObject(json_object, BLName_Val(type), paint.type, log);
    // unsigned int color
    if ((NSVGpaintType)paint.type == NSVGpaintType::NSVG_PAINT_COLOR)
    {
        err &= _CompareObject(json_object, BLName_Val(color), paint.type, log);
    }
    // NSVGgradient* gradient
    if ((NSVGpaintType)paint.type == NSVGpaintType::NSVG_PAINT_LINEAR_GRADIENT
     || (NSVGpaintType)paint.type == NSVGpaintType::NSVG_PAINT_RADIAL_GRADIENT)
    {
        err &= _CompareObject(json_object, BLName_Val(gradient), paint.gradient, log,
            [](const nlohmann::json& json_object, const NSVGgradient& gradient, BaseLineLogger& log)
            {
                bool err = true;
                // float xform[6]
                std::vector<double> xform = { gradient.xform[0], gradient.xform[1], gradient.xform[2],
                                              gradient.xform[3], gradient.xform[4], gradient.xform[5] };
                err &= _CompareObjectGroup(json_object, BLName_Val(xform), xform, log);
                // char spread
                err &= _CompareObject(json_object, BLName_Val(spread), gradient.spread, log);
                // float fx, fy
                err &= _CompareObject(json_object, BLName_Val(fx), gradient.fx, log);
                err &= _CompareObject(json_object, BLName_Val(fy), gradient.fy, log);
                // int nstops
                err &= _CompareObject(json_object, BLName_Val(nstops), gradient.nstops, log);
                // nsvggradientstop stops[1]
                err &= _CompareObjectGroupEx(json_object, BLName_Val(stops), gradient.stops, gradient.nstops, log,
                    [](const nlohmann::json& json_object, const NSVGgradientStop& stop, BaseLineLogger& log)
                    {
                        bool err = true;
                        err &= _CompareObject(json_object, BLName_Val(color), stop.color, log);
                        err &= _CompareObject(json_object, BLName_Val(offset), stop.color, log);
                        BLReturnBoolen(err);
                    });

                BLReturnBoolen(err);
            });
    }

    BLReturnBoolen(err);
}

void BaselineOrcaInput::_BuildEntityMesh(nlohmann::json& json_mesh, const Slic3r::TriangleMesh& mesh)
{
    using namespace nlohmann;

    // TriangleMeshStats m_stats
    const Slic3r::TriangleMeshStats& mesh_stat = mesh.stats();
    json& json_mesh_stats = json_mesh[BLName_Val(stats)];
    // uint32_t      number_of_facets
    json_mesh_stats[BLName_Val(number_of_facets)] = mesh_stat.number_of_facets;
    // stl_vertex    max
    _BuildEntityVec3f(json_mesh_stats[BLName_Val(max)], mesh_stat.max);
    // stl_vertex    min
    _BuildEntityVec3f(json_mesh_stats[BLName_Val(min)], mesh_stat.min);
    // stl_vertex    size
    _BuildEntityVec3f(json_mesh_stats[BLName_Val(size)], mesh_stat.size);
    // float         volume
    json_mesh_stats[BLName_Val(volume)] = mesh_stat.volume;
    // int           number_of_parts
    json_mesh_stats[BLName_Val(number_of_parts)] = mesh_stat.number_of_parts;
    // int           open_edges
    json_mesh_stats[BLName_Val(open_edges)] = mesh_stat.open_edges;
    // RepairedMeshErrors repaired_errors
    {
        json& json_mesh_stats_repaired_error = json_mesh_stats[BLName_Val(repaired_errors)];
        // int           edges_fixed
        json_mesh_stats_repaired_error[BLName_Val(edges_fixed)] = mesh_stat.repaired_errors.edges_fixed;
        // int           degenerate_facets
        json_mesh_stats_repaired_error[BLName_Val(degenerate_facets)] = mesh_stat.repaired_errors.degenerate_facets;
        // int           facets_removed
        json_mesh_stats_repaired_error[BLName_Val(facets_removed)] = mesh_stat.repaired_errors.facets_removed;
        // int           facets_reversed
        json_mesh_stats_repaired_error[BLName_Val(facets_reversed)] = mesh_stat.repaired_errors.facets_reversed;
        // int           backwards_edges
        json_mesh_stats_repaired_error[BLName_Val(backwards_edges)] = mesh_stat.repaired_errors.backwards_edges;
    }
    // vec3d m_init_shift
    _BuildEntityVec3d(json_mesh[BLName_Val(init_shift)], mesh.get_init_shift());
    // indexed_triangle_set its
    {
        json& json_mesh_its = json_mesh[BLName_Val(its)];
        // std::vector<stl_triangle_vertex_indices>    indices
        _BuildObjectGroup(json_mesh_its[BLName_Val(indices)], mesh.its.indices, _BuildEntityVtxIdx);

        // std::vector<stl_vertex>                     vertices
        _BuildObjectGroup(json_mesh_its[BLName_Val(vertices)], mesh.its.vertices, _BuildEntityVec3f);
        // std::vector<FaceProperty>                   properties
        _BuildObjectGroup(json_mesh_its[BLName_Val(properties)], mesh.its.properties,
            [](nlohmann::json& json_property, const FaceProperty& property)
            {
                json_property[BLName_Val(type)] = property.type;
                json_property[BLName_Val(area)] = property.area;
            });
    }
}

bool BaselineOrcaInput::_CompareEntityMesh(const nlohmann::json& json_object, const Slic3r::TriangleMesh& mesh, BaseLineLogger& log)
{
    bool err = true;
    // TriangleMeshStats m_stats
    err &= _CompareObject(json_object, BLName_Val(stats), mesh.stats(), log,
        [](const nlohmann::json& json_object, const Slic3r::TriangleMeshStats& stat, BaseLineLogger& log)
        {
            bool err = true;
            // uint32_t      number_of_facets
            err &= _CompareObject(json_object, BLName_Val(number_of_facets), stat.number_of_facets, log);
            // stl_vertex    max
            err &= _CompareObject(json_object, BLName_Val(max), stat.max, log, _CompareEntityVec3f);
            // stl_vertex    min
            err &= _CompareObject(json_object, BLName_Val(min), stat.min, log, _CompareEntityVec3f);
            // stl_vertex    size
            err &= _CompareObject(json_object, BLName_Val(size), stat.size, log, _CompareEntityVec3f);
            // float         volume
            err &= _CompareObject(json_object, BLName_Val(volume), stat.volume, log);
            // int           number_of_parts
            err &= _CompareObject(json_object, BLName_Val(number_of_parts), stat.number_of_parts, log);
            // int           open_edges
            err &= _CompareObject(json_object, BLName_Val(open_edges), stat.open_edges, log);
            // RepairedMeshErrors repaired_errors
            err &= _CompareObject(json_object, BLName_Val(repaired_errors), stat.repaired_errors, log,
                [](const nlohmann::json& json_object, const Slic3r::RepairedMeshErrors& repaired_errors, BaseLineLogger& log)
                {
                    bool err = true;
                    // int           edges_fixed
                    err &= _CompareObject(json_object, BLName_Val(edges_fixed), repaired_errors.edges_fixed, log);
                    // int           degenerate_facets
                    err &= _CompareObject(json_object, BLName_Val(degenerate_facets), repaired_errors.degenerate_facets, log);
                    // int           facets_removed
                    err &= _CompareObject(json_object, BLName_Val(facets_removed), repaired_errors.facets_removed, log);
                    // int           facets_reversed
                    err &= _CompareObject(json_object, BLName_Val(facets_reversed), repaired_errors.facets_reversed, log);
                    // int           backwards_edges
                    err &= _CompareObject(json_object, BLName_Val(backwards_edges), repaired_errors.backwards_edges, log);

                    BLReturnBoolen(err);
                });

            BLReturnBoolen(err);
        });
    // vec3d m_init_shift
    err &= _CompareObject(json_object, BLName_Val(init_shift), mesh.get_init_shift(), log, _CompareEntityVec3d);
    // indexed_triangle_set its
    err &= _CompareObject(json_object, BLName_Val(its), mesh.its, log,
        [](const nlohmann::json& json_object, const indexed_triangle_set& its, BaseLineLogger& log)
        {
            bool err = true;
            // std::vector<stl_triangle_vertex_indices>    indices
            err &= _CompareObjectGroup(json_object, BLName_Val(indices), its.indices, log, _CompareEntityVtxIdx);
            // std::vector<stl_vertex>                     vertices
            err &= _CompareObjectGroup(json_object, BLName_Val(vertices), its.vertices, log, _CompareEntityVec3f);
            // std::vector<FaceProperty>                   properties
            err &= _CompareObjectGroup(json_object, BLName_Val(properties), its.properties, log,
                [](const nlohmann::json& json_object, const FaceProperty& property, BaseLineLogger& log)
                {
                    bool err = true;
                    err &= _CompareObject(json_object, BLName_Val(type), property.type, log);
                    err &= _CompareObject(json_object, BLName_Val(area), property.area, log);
                    BLReturnBoolen(err);
                });
            BLReturnBoolen(err);
        });

    BLReturnBoolen(err);
}

BaselineOrcaOutput::BaselineOrcaOutput(const std::string& name, const std::vector<std::string>& modules)
    : Baseline(name, modules)
{

}

bool BaselineOrcaOutput::Generate()
{
    BLReturnBoolen(true);
}
bool BaselineOrcaOutput::Compare()
{
    BLReturnBoolen(true);
}
bool BaselineOrcaOutput::Update()
{
    BLReturnBoolen(true);
}

CxRegisterBaseline(BaselineOrcaInput, BaseLineOrcaInputName, {});
CxRegisterBaseline(BaselineOrcaOutput, BaseLineOrcaOutputName, {});