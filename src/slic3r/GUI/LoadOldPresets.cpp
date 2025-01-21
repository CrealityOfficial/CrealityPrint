#include "LoadOldPresets.hpp"

#include <fstream>
#include <algorithm>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"

#include <iostream>
#include <fstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <miniz/miniz.h>

namespace Slic3r { 
namespace GUI {
LoadOldPresets::LoadOldPresets() {}
LoadOldPresets::~LoadOldPresets() {}

void LoadOldPresets::setOverrideConfirmCb(std::function<int(const std::list<Slic3r::PresetBundle::STOverrideConfirmFile*>&)> overrideConfirmCb)
{
    m_overrideConfirmCb = overrideConfirmCb;
}

bool LoadOldPresets::loadPresets(const std::string& fileName)
{
    bool bRet = false;
    do {
        if (!boost::iends_with(fileName, ".zip")) {
            setLastError("1", "not zip file");
            break;
        }

        createTmpDir();

        fs::path              filePath = boost::filesystem::path(fileName).make_preferred();
        std::string           file     = filePath.string();
        std::vector<FileData> vtFileName;
        getSubFileName(file, vtFileName);
        if (!m_bIsOldPresets) {
            setLastError("2", "not old presets");
            break;
        }
        //  判断预设值是否存在
        PresetBundle::STOverrideConfirmFile stOverrideConfirmFile;
        stOverrideConfirmFile.fileName = filePath.filename().stem().string();
        for (auto item : vtFileName) {
            if (isPresetExist(item) == 1) {
                if (item.type == DefJson) {
                    if (item.isSystemPreset)
                        continue;
                    stOverrideConfirmFile.lstPrinterPreset.push_back(item.fileName);
                } else if (item.type == Filament) {
                    stOverrideConfirmFile.lstFilamentPreset.push_back(item.fileName);
                } else if (item.type == Process) {
                    stOverrideConfirmFile.lstProcessPreset.push_back(item.fileName);
                }
            }
        }
        m_override = 1; // 1-覆盖 4-创建副本
        if (stOverrideConfirmFile.lstPrinterPreset.size() != 0 || stOverrideConfirmFile.lstFilamentPreset.size() != 0 ||
            stOverrideConfirmFile.lstProcessPreset.size() != 0) {
            if (m_overrideConfirmCb) {
                std::list<PresetBundle::STOverrideConfirmFile*> lstOverrideConfirmFile;
                lstOverrideConfirmFile.push_back(&stOverrideConfirmFile);
                m_override = m_overrideConfirmCb(lstOverrideConfirmFile);
            }
        }

        int errFileCout = 0;
        m_printerData   = PrinterData();
        //  导入打印机预设
        for (auto item : vtFileName) {
            if (item.type == DefJson) {
                if (item.isSystemPreset) {
                    m_printerData.printerInherits = item.inheritsName;
                    bRet = true;
                    continue;
                }
                bRet = doDefJson(item);
            }
        }
        if (!bRet) {
            setLastError("3", "load def json fail");
            break;
        }
        // 导入耗材、工艺预设
        for (auto item : vtFileName) {
            if (item.type == DefJson) {
                continue;
            }
            if (item.type == Filament) {
                bRet = doFilamentJson(item);
            } else if (item.type == Process) {
                bRet = doProcessJson(item);
            }
            if (!bRet) {
                errFileCout++;
            }
        }

        //  重新加载预设文件
        auto* app_config = GUI::wxGetApp().app_config;
        GUI::wxGetApp().preset_bundle->load_presets(*app_config, ForwardCompatibilitySubstitutionRule::EnableSilentDisableSystem);
        GUI::wxGetApp().load_current_presets();
        GUI::wxGetApp().plater()->set_bed_shape();

    } while (0);
    return bRet;
}

void LoadOldPresets::createTmpDir()
{
    boost::system::error_code ec;
    // create user folder
    fs::path user_folder(data_dir() + "/" + PRESET_USER_DIR);
    if (!fs::exists(user_folder))
        fs::create_directory(user_folder, ec);
    if (ec)
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " create directory failed: " << ec.message();
    // create default folder
    fs::path default_folder(user_folder / DEFAULT_USER_FOLDER_NAME);
    if (!fs::exists(default_folder))
        fs::create_directory(default_folder, ec);
    if (ec)
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " create directory failed: " << ec.message();
    // create temp folder
    // std::string user_default_temp_dir = data_dir() + "/" + PRESET_USER_DIR + "/" + DEFAULT_USER_FOLDER_NAME + "/" + "temp";
    fs::path temp_folder(default_folder / "temp");
    m_temp_folder                     = temp_folder;
    std::string user_default_temp_dir = temp_folder.make_preferred().string();
    if (fs::exists(temp_folder))
        fs::remove_all(temp_folder);
    fs::create_directory(temp_folder, ec);
    if (ec)
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " create directory failed: " << ec.message();
}

void LoadOldPresets::setLastError(const std::string& code, const std::string& msg)
{
    m_lastError         = LastError();
    m_lastError.code    = code;
    m_lastError.message = msg;
}

void LoadOldPresets::getSubFileName(const std::string& zipFileName, std::vector<FileData>& vtSubFileName)
{
    mz_zip_archive zip_archive;
    mz_zip_zero_struct(&zip_archive);
    mz_bool status;

    FILE* zipFile = boost::nowide::fopen(zipFileName.c_str(), "rb");
    status        = mz_zip_reader_init_cfile(&zip_archive, zipFile, 0, MZ_ZIP_FLAG_CASE_SENSITIVE | MZ_ZIP_FLAG_IGNORE_PATH);
    if (MZ_FALSE == status) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Failed to initialize reader ZIP archive";
        return;
    } else {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Success to initialize reader ZIP archive";
    }

    // Extract Files
    int num_files = mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < num_files; i++) {
        mz_zip_archive_file_stat file_stat;
        status = mz_zip_reader_file_stat(&zip_archive, i, &file_stat);
        if (status) {
            std::string file_name = into_u8(file_stat.m_filename);
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Form zip file: " << zipFileName << ". Read file name: " << file_stat.m_filename;
            if (file_stat.m_is_directory) {
                boost::system::error_code ec;
                fs::create_directory(m_temp_folder / file_name, ec);
            } else {
                //size_t index = file_name.find_last_of('/');
                //if (std::string::npos != index) 
                {
                   // file_name = file_name.substr(index + 1);
                    // create target file path
                    fs::path target_file_path = boost::filesystem::path(m_temp_folder / file_name).make_preferred();
                   std::string targetFileName   = target_file_path.string();

                    status = mz_zip_reader_extract_to_file(&zip_archive, i, encode_path(targetFileName.c_str()).c_str(),
                                                           MZ_ZIP_FLAG_CASE_SENSITIVE);
                    // target file is opened
                    if (MZ_FALSE == status) {
                        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << " Failed to open target file: " << targetFileName;
                    } else {
                        FileData fileData;
                        fileData.name = targetFileName;
                        fileData.fileName = target_file_path.filename().stem().string();
                        int pos           = targetFileName.find(".def.json");
                        if (pos != std::string::npos) {
                            int pos2          = fileData.fileName.find(".def");
                            fileData.fileName = fileData.fileName.substr(0, pos2);
                            fileData.inheritsName = getInherits(fileData.fileName);
                            m_printerData.printerInherits = fileData.inheritsName;

                            PresetCollection* collection = &GUI::wxGetApp().preset_bundle->printers;
                            Preset*           p          = collection->find_preset(fileData.inheritsName, false);
                            if (p) {
                                if (p->is_default || p->is_system) {
                                    fileData.inheritsName = fileData.inheritsName;
                                    fileData.isSystemPreset = true;
                                }
                            } 
                            fileData.type = DefJson;
                            m_bIsOldPresets = true;
                        } else if (target_file_path.parent_path() == m_temp_folder.string() + "\\materials") {
                            fileData.type = Filament;
                        } else if (target_file_path.parent_path() == m_temp_folder.string() + "\\processes") {
                            fileData.type = Process;
                        }
                        vtSubFileName.push_back(std::move(fileData));
                    }
                }
            }
        }
    }
    fclose(zipFile);
}

bool LoadOldPresets::loadMachineList()
{
    bool bRet = false;
    do {
        m_jsonMachineList = json();
        fs::path pathMachineList(data_dir() + "/" + PRESET_SYSTEM_DIR + "/Creality/machineList.json");
        try {
            std::ifstream file(pathMachineList.string());
            file.imbue(std::locale("en_US.UTF-8"));
            if (!file.is_open()) {
                // 获取错误码
                std::error_code error_code = std::make_error_code(std::errc::no_such_file_or_directory);
                setLastError(std::to_string(error_code.value()), error_code.message());
                break;
            }
            // file >> json;
            file >> m_jsonMachineList;
            file.close();
            bRet = true;
        } catch (...) {
            setLastError("10", "open json crach");
            break;
        }
    } while (0);
    return bRet;
}
bool LoadOldPresets::loadSystemFilamentFile() { 
    bool bRet = false;
    do {
        m_vtSystemFilamentFile.clear();

        fs::path dir_path(data_dir() + "/" + PRESET_SYSTEM_DIR + "/Creality/filament");

        // 检查路径是否存在且为目录
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
            break;
        }

        // 遍历目录
        for (auto& entry : fs::directory_iterator(dir_path)) {
            m_vtSystemFilamentFile.push_back(entry.path().filename().string());
        }
    } while (0);
    return bRet;
}
bool LoadOldPresets::loadSystemProcessFile()
{
    bool bRet = false;
    do {
        m_vtSystemProcessFile.clear();

        fs::path dir_path(data_dir() + "/" + PRESET_SYSTEM_DIR + "/Creality/process");

        // 检查路径是否存在且为目录
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
            break;
        }

        // 遍历目录
        for (auto& entry : fs::directory_iterator(dir_path)) {
            m_vtSystemProcessFile.push_back(entry.path().filename().string());
        }
    } while (0);
    return bRet;
}
std::string LoadOldPresets::getPrinterName(const std::string& printerIntName) { 
    std::string name;

    if (printerIntName.empty())
        return name;

    if (m_jsonMachineList.empty()) {
        loadMachineList();
    }
    auto& printerList = m_jsonMachineList["printerList"];
    if (printerList.is_array()) {

        int size = printerList.size();
        for (int i = 0; i < size; ++i) {
            auto& printerIntNameRef = printerList[i]["printerIntName"];
            if (printerIntNameRef.is_string() && printerIntNameRef.get<std::string>() == printerIntName) {
                auto& printerNameRef = printerList[i]["name"];
                if (printerNameRef.is_string()) {
                    name = printerNameRef.get<std::string>();
                }
                break;
            }
        }
    }
    return name; 
}

void LoadOldPresets::splitPrinterPresetFileName(const std::string& printerPresetFileName, std::string& printerName, std::string& nozzle) {
    size_t pos  = printerPresetFileName.rfind("-");
    printerName = printerPresetFileName.substr(0, pos);
    nozzle      = printerPresetFileName.substr(pos + 1, printerPresetFileName.length() - pos - 1);
}

std::string LoadOldPresets::getInherits(const std::string& inherits)
{
    std::string ssInherits;
    size_t      pos        = inherits.rfind("-");
    std::string printer    = inherits.substr(0, pos);
    std::string nozzle     = inherits.substr(pos + 1, inherits.length() - pos - 1);
    m_printerData.nozzle   = nozzle;
    std::string printerName = getPrinterName(printer);
    if (printerName.empty())
        return "";
    ssInherits             = "Creality " + printerName + " " + nozzle + " nozzle";
    return ssInherits;
}

std::string LoadOldPresets::getFilamentInherits(const std::string& inherits)
{
    //std::string filamentType = inherits; 
    //std::transform(filamentType.begin(), filamentType.end(), filamentType.begin(), tolower);
    return getFilamentInheritsFileName(inherits);
}

std::string LoadOldPresets::getProcessInherits(const std::string& layerHeight) { 
    string ssLayerHeight = layerHeight;
    if (layerHeight.length() == 3 && layerHeight.c_str()[1] == '.') {
        ssLayerHeight = layerHeight + "0";
    }
    return getProcessInheritsFileName(ssLayerHeight);
}

std::string LoadOldPresets::getFilamentInheritsFileName(const std::string& inheritsType) { 
    if (m_vtSystemFilamentFile.empty()) {
        loadSystemFilamentFile();
    }
    std::vector<std::string> filamentPrefix = {"Generic ", "Hyper ", "HP-", "HP Ultra", "Ender-", "ENDER FAST ", "CR-"};
    for (auto item : filamentPrefix) {
        std::string filamentIneritsFileNameNoSuffix = item + inheritsType + " @" + m_printerData.printerInherits;
        std::string filamentIneritsFileName         = filamentIneritsFileNameNoSuffix + ".json";
        if (std::find(m_vtSystemFilamentFile.begin(), m_vtSystemFilamentFile.end(), filamentIneritsFileName) !=
            m_vtSystemFilamentFile.end()) {
            return filamentIneritsFileNameNoSuffix;
        }
    }
    return "";
}

std::string LoadOldPresets::getProcessInheritsFileName(const std::string& inheritsLayerHeight)
{
    if (m_vtSystemProcessFile.empty()) {
        loadSystemProcessFile();
    }
    std::vector<std::string> processSuffix = {" Standard", " Hight Quality", " Fine", " Extra Fine", " HueForge"};
    for (auto item : processSuffix) {
        std::string processIneritsFileNameNoSuffix = inheritsLayerHeight + "mm" + item + " @" + m_printerData.printerInherits;
        std::string processIneritsFileName         = processIneritsFileNameNoSuffix + ".json";
        if (std::find(m_vtSystemProcessFile.begin(), m_vtSystemProcessFile.end(), processIneritsFileName) != m_vtSystemProcessFile.end()) {
            return processIneritsFileNameNoSuffix;
        }
    }
    return "";
}

bool LoadOldPresets::doDefJson(const FileData& fileData)
{
    bool bRet = false;
    do {
        json jsonIn  = json();
        json jsonOut = json();
        try {
            std::ifstream file(from_u8(fileData.name).ToStdString());
            file.imbue(std::locale("en_US.UTF-8"));
            if (!file.is_open()) {
                // 获取错误码
                std::error_code error_code = std::make_error_code(std::errc::no_such_file_or_directory);
                setLastError(std::to_string(error_code.value()), error_code.message());
                break;
            }
            // file >> json;
            file >> jsonIn;
            file.close();

        } catch (...) {
            setLastError("10", "open json crach");
            break;
        }
        if (jsonIn.contains("printer")) {
            jsonIn["printer"].erase("inherits");
        }
        fs::path    path(fileData.name);
        std::string outputName = path.filename().string();
        outputName.erase(outputName.find(".def.json"), 9);
        std::string printerName = "";
        std::string nozzle      = "";
        splitPrinterPresetFileName(outputName, printerName, nozzle);
        std::string inherits = "";
        if (jsonIn.contains("metadata") && jsonIn["metadata"].contains("inherits_from")) {
            inherits = getInherits(jsonIn["metadata"]["inherits_from"]);

            if (!fileData.inheritsName.empty()) {
                outputName = fileData.inheritsName;
            } else {
                int dotpos = outputName.rfind("-");
                if (dotpos != std::string::npos) {
                    outputName = outputName.substr(0, dotpos);
                }
            }
            m_printerData.printerInherits  = inherits;
            jsonOut["base_id"]             = "";
            jsonOut["from"]                = "User";
            jsonOut["inherits"]            = inherits;
            jsonOut["is_custom_defined"]   = "0";
            jsonOut["name"]                = outputName;
            jsonOut["printer_settings_id"] = outputName;
            jsonOut["version"]             = "3.0.0.0";
            if (jsonIn.contains("printer")) {
                jsonOut.merge_patch(jsonIn["printer"]);
            }
            if (jsonIn["extruders"].is_array() && jsonIn["extruders"].size() > 0) {
                jsonOut.merge_patch(jsonIn["extruders"][0]["engine_data"]);
            }

            std::string tmpOutputName = getTmpOutputName(fileData.name);

            if (fs::exists(tmpOutputName)) {
                fs::remove(tmpOutputName);
            }
            boost::nowide::ofstream c1;
            c1.open(tmpOutputName, std::ios::out | std::ios::trunc);
            c1 << std::setw(4) << jsonOut << std::endl;
            c1.close();

            DynamicPrintConfig                 config;
            std::map<std::string, std::string> key_values;
            std::string                        reason;
            config.load_from_json(tmpOutputName, ForwardCompatibilitySubstitutionRule::Enable, key_values, reason);

            DynamicPrintConfig new_config;
            PresetCollection*  collection = nullptr;
            collection                    = &GUI::wxGetApp().preset_bundle->printers;
            Preset*       inherit_preset  = nullptr;
            ConfigOption* inherits_config = config.option(BBL_JSON_KEY_INHERITS);
            std::string   inherits_value;
            if (inherits_config) {
                ConfigOptionString* option_str = dynamic_cast<ConfigOptionString*>(inherits_config);
                inherits_value                 = option_str->value;
                inherit_preset                 = collection->find_preset(inherits_value, false, true);
            }
            if (inherit_preset) {
                new_config = inherit_preset->config;
            } else {
                break;
            }
            t_config_option_keys diffKeys = new_config.diff(config);

            if (m_override == 4) {
                std::string new_name   = "";
                Preset*     new_preset = nullptr;
                for (int i = 1; i < 100; ++i) {
                    new_name   = outputName + "(" + std::to_string(i) + ")";
                    new_preset = collection->find_preset(new_name, false);
                    if (new_preset == nullptr)
                        break;
                }
                if (!new_preset) {
                    outputName = new_name;
                }
            }

            //  重新组合
            jsonOut                        = json();
            jsonOut["base_id"]             = inherit_preset->base_id;
            jsonOut["from"]                = "User";
            jsonOut["inherits"]            = inherits;
            jsonOut["is_custom_defined"]   = "0";
            jsonOut["name"]                = outputName;
            jsonOut["printer_settings_id"] = outputName;
            jsonOut["version"]             = "3.0.0.0";
            for (auto item : diffKeys) {
                if (item == BBL_JSON_KEY_INHERITS || item == "printer_model" || item == "default_print_profile" ||
                    item == "printer_variant" || item == "support_multi_bed_types")
                    continue;
                if (jsonIn.contains("printer") && !jsonIn["printer"][item].is_null()) {
                    jsonOut[item] = jsonIn["printer"][item];
                }
                if (jsonIn["extruders"].is_array() && jsonIn["extruders"].size() > 0 &&
                    !jsonIn["extruders"][0]["engine_data"][item].is_null()) {
                    jsonOut[item] = jsonIn["extruders"][0]["engine_data"][item];
                }
            }
        } else {
            std::string printerName = "";
            std::string nozzle      = "";
            splitPrinterPresetFileName(outputName, printerName, nozzle);
            int dotpos = outputName.rfind("-");
            if (dotpos != std::string::npos) {
                outputName = outputName.substr(0, dotpos);
            }

            jsonOut["base_id"]             = "";
            jsonOut["from"]                = "User";
            jsonOut["inherits"]            = "";
            jsonOut["is_custom_defined"]   = "0";
            jsonOut["name"]                = outputName;
            jsonOut["printer_settings_id"] = outputName;
            jsonOut["version"]             = "3.0.0.0";
            jsonOut["family"]              = "Creality";
            if (jsonIn.contains("engine_data")) {
                if (jsonIn["engine_data"].contains("inherits")) {
                    jsonIn["engine_data"].erase("inherits");
                }
                jsonOut.merge_patch(jsonIn["engine_data"]);
            }
            if (jsonIn.contains("extruders") && jsonIn["extruders"].is_array() && jsonIn["extruders"][0].contains("engine_data")) {
                jsonOut.merge_patch(jsonIn["extruders"][0]["engine_data"]);
            }
            m_printerData.printerInherits = "fdm_creality_common";
        }

        //  如果单挤出机多材料为0时，强制手动更换丝材为0
        if (jsonOut.contains("single_extruder_multi_material") && 
            jsonOut["single_extruder_multi_material"].is_string() && jsonOut["single_extruder_multi_material"].get<std::string>() == "0") {
            jsonOut["manual_filament_change"] = "0";
        }

        const UserInfo& userInfo = wxGetApp().get_user();
        std::string     outJsonFile;
        if (userInfo.bLogin) {
            outJsonFile =
                fs::path(data_dir()).append("user").append(userInfo.userId).append("machine").append(outputName + ".json").string();
        } else {
            outJsonFile = fs::path(data_dir()).append("user").append("default").append("machine").append(outputName + ".json").string();
        }
        if (fs::exists(outJsonFile)) {
            fs::remove(outJsonFile);
        }
        boost::nowide::ofstream c;
        c.open(outJsonFile, std::ios::out | std::ios::trunc);
        c << std::setw(4) << jsonOut << std::endl;
        c.close();

        //  保存info文件
        std::string outInfoFile;
        if (userInfo.bLogin && !fileData.isSystemPreset) {
            outInfoFile = fs::path(data_dir()).append("user").append(userInfo.userId).append("machine").append(outputName + ".info").string();

            if (fs::exists(outInfoFile)) {
                fs::remove(outInfoFile);
            }

            boost::nowide::ofstream c2;
            c2.open(outInfoFile, std::ios::out | std::ios::trunc);
            c2 << "sync_info = create" << std::endl;
            c2 << "user_id = " << userInfo.userId << std::endl;
            c2 << "setting_id =" << std::endl;
            c2 << "base_id = " << 12345 << std::endl;
            c2 << "updated_time =" << std::endl;
            c2.close();
        }

        bRet = true;
    } while (0);
    return bRet;
}
bool LoadOldPresets::doFilamentJson(const FileData& fileData)
{
    bool bRet = false;
    do {
        json jsonIn  = json();
        json jsonOut = json();
        try {
            std::ifstream file(from_u8(fileData.name).ToStdString());
            file.imbue(std::locale("en_US.UTF-8"));
            if (!file.is_open()) {
                // 获取错误码
                std::error_code error_code = std::make_error_code(std::errc::no_such_file_or_directory);
                setLastError(std::to_string(error_code.value()), error_code.message());
                break;
            }
            // file >> json;
            file >> jsonIn;
            file.close();

        } catch (...) {
            setLastError("10", "open json crach");
            break;
        }
        fs::path    path(fileData.name);
        std::string outputName = path.filename().string();
        outputName.erase(outputName.find(".json"), 5);
        int dotpos                   = outputName.rfind("-");
        if (dotpos != std::string::npos) {
            outputName = outputName.substr(0, dotpos);
        }
        outputName                   = outputName + " @" + getCompatiblePrinters(fileData);
        if (jsonIn.contains("engine_data") && jsonIn["engine_data"].contains("filament_type")) {
            m_printerData.filamentType = jsonIn["engine_data"]["filament_type"];
        }
        std::string       inherits   = getFilamentInherits(m_printerData.filamentType);
        PresetCollection* collection = nullptr;
        collection                   = &GUI::wxGetApp().preset_bundle->filaments;

        if (!inherits.empty()) {
            jsonIn["engine_data"].erase("inherits");
            jsonIn["engine_data"].erase("compatible_printers");
            jsonOut.merge_patch(jsonIn["engine_data"]);
            jsonOut["base_id"]              = "";
            jsonOut["from"]                 = "User";
            jsonOut["inherits"]             = inherits;
            jsonOut["is_custom_defined"]    = "0";
            jsonOut["name"]                 = outputName;
            jsonOut["filament_settings_id"] = outputName;
            jsonOut["instantiation"]        = "true";
            jsonOut["version"]              = "3.0.0.0";

            std::string tmpOutputName = getTmpOutputName(fileData.name);

            if (fs::exists(tmpOutputName)) {
                fs::remove(tmpOutputName);
            }
            boost::nowide::ofstream c1;
            c1.open(tmpOutputName, std::ios::out | std::ios::trunc);
            c1 << std::setw(4) << jsonOut << std::endl;
            c1.close();

            DynamicPrintConfig                 config;
            std::map<std::string, std::string> key_values;
            std::string                        reason;
            config.load_from_json(tmpOutputName, ForwardCompatibilitySubstitutionRule::Enable, key_values, reason);

            DynamicPrintConfig new_config;
            Preset*       inherit_preset  = nullptr;
            ConfigOption* inherits_config = config.option(BBL_JSON_KEY_INHERITS);
            std::string   inherits_value;
            if (inherits_config) {
                ConfigOptionString* option_str = dynamic_cast<ConfigOptionString*>(inherits_config);
                inherits_value                 = option_str->value;
                inherit_preset                 = collection->find_preset(inherits_value, false, true);
            }
            if (inherit_preset) {
                new_config = inherit_preset->config;
            } else {
                break;
            }
            t_config_option_keys diffKeys = new_config.diff(config);
            jsonOut                        = json();
            for (auto item : diffKeys) {
                if (item == BBL_JSON_KEY_INHERITS || item == "compatible_printers" || item == "default_filament_colour")
                    continue;
                if (!jsonIn["engine_data"][item].is_null()) {
                    jsonOut[item] = jsonIn["engine_data"][item];
                }
            }
            jsonOut["base_id"] = inherit_preset->base_id;

        } else {
            jsonIn["engine_data"].erase("inherits");
            jsonIn["engine_data"].erase("compatible_printers");
            jsonIn["engine_data"].erase("default_filament_colour");
            for (auto item : jsonIn["engine_data"].items()) {
                if (!item.value().is_null()) {
                    jsonOut[item.key()] = item.value();
                }
            }
            jsonOut["base_id"] = "";
        }

        if (m_override == 4) {
            std::string new_name   = "";
            Preset*     new_preset = nullptr;
            for (int i = 1; i < 100; ++i) {
                new_name   = outputName + "(" + std::to_string(i) + ")";
                new_preset = collection->find_preset(new_name, false);
                if (new_preset == nullptr)
                    break;
            }
            if (!new_preset) {
                outputName = new_name;
            }
        }

        std::string compatiblePrinters = getCompatiblePrinters(fileData);
        if (compatiblePrinters.empty())
            break;
        //  重新组合
        jsonOut["from"]                = "User";
        jsonOut["compatible_printers"]     = json::array();
        jsonOut["compatible_printers"][0]  = getCompatiblePrinters(fileData);
        jsonOut["inherits"]             = inherits;
        jsonOut["is_custom_defined"]   = "0";
        jsonOut["name"]                = outputName;
        jsonOut["filament_settings_id"] = outputName;
        jsonOut["instantiation"]           = "true";
        jsonOut["version"]             = "3.0.0.0";

        /*
        for (auto item : diffKeys) {
            if (item == BBL_JSON_KEY_INHERITS || item == "printer_model")
                continue;
            if (!jsonIn["printer"][item].is_null()) {
                jsonOut[item] = jsonIn["printer"][item];
            }
        }*/
        const UserInfo& userInfo = wxGetApp().get_user();
        std::string     outJsonFile;
        if (userInfo.bLogin) {
            outJsonFile = fs::path(data_dir()).append("user").append(userInfo.userId).append("filament").append(outputName + ".json").string();
        } else {
            outJsonFile = fs::path(data_dir()).append("user").append("default").append("filament").append(outputName + ".json").string();
        }
        if (fs::exists(outJsonFile)) {
            fs::remove(outJsonFile);
        }
        boost::nowide::ofstream c;
        c.open(outJsonFile, std::ios::out | std::ios::trunc);
        c << std::setw(4) << jsonOut << std::endl;
        c.close();

        //  保存info文件
        std::string outInfoFile;
        if (userInfo.bLogin) {
            outInfoFile = fs::path(data_dir()).append("user").append(userInfo.userId).append("filament").append(outputName + ".info").string();

            if (fs::exists(outInfoFile)) {
                fs::remove(outInfoFile);
            }

            boost::nowide::ofstream c2;
            c2.open(outInfoFile, std::ios::out | std::ios::trunc);
            c2 << "sync_info = create" << std::endl;
            c2 << "user_id = " << userInfo.userId << std::endl;
            c2 << "setting_id =" << std::endl;
            c2 << "base_id = " << 12345 << std::endl;
            c2 << "updated_time =" << std::endl;
            c2.close();
        }

        bRet = true;
    } while (0);
    return bRet;
}
bool LoadOldPresets::doProcessJson(const FileData& fileData)
{
    bool bRet = false;
    do {
        json jsonIn  = json();
        json jsonOut = json();
        try {
            std::ifstream file(from_u8(fileData.name).ToStdString());
            file.imbue(std::locale("en_US.UTF-8"));
            if (!file.is_open()) {
                // 获取错误码
                std::error_code error_code = std::make_error_code(std::errc::no_such_file_or_directory);
                setLastError(std::to_string(error_code.value()), error_code.message());
                break;
            }
            // file >> json;
            file >> jsonIn;
            file.close();

        } catch (...) {
            setLastError("10", "open json crach");
            break;
        }
        fs::path    path(fileData.name);
        std::string outputName = path.filename().string();
        int      pos        = outputName.find(".json");
        if (pos != -1) {
            outputName.erase(pos, 5);
        }
        std::string layerHeight = "";
        if (jsonIn.contains("engine_data") && jsonIn["engine_data"].contains("layer_height")) {
            layerHeight = jsonIn["engine_data"]["layer_height"];
        }
        std::string       inherits    = getProcessInherits(layerHeight);
        PresetCollection* collection  = nullptr;
        collection                    = &GUI::wxGetApp().preset_bundle->prints;

        if (!inherits.empty()) {
            jsonIn["engine_data"].erase("inherits");
            jsonOut.merge_patch(jsonIn["engine_data"]);
            jsonOut["base_id"]              = "";
            jsonOut["from"]                 = "User";
            jsonOut["inherits"]             = inherits;
            jsonOut["is_custom_defined"]    = "0";
            jsonOut["name"]                 = outputName;
            jsonOut["filament_settings_id"] = outputName;
            jsonOut["instantiation"]        = "true";
            jsonOut["version"]              = "3.0.0.0";

            std::string tmpOutputName = getTmpOutputName(fileData.name);

            if (fs::exists(tmpOutputName)) {
                fs::remove(tmpOutputName);
            }
            boost::nowide::ofstream c1;
            c1.open(tmpOutputName, std::ios::out | std::ios::trunc);
            c1 << std::setw(4) << jsonOut << std::endl;
            c1.close();

            DynamicPrintConfig                 config;
            std::map<std::string, std::string> key_values;
            std::string                        reason;
            config.load_from_json(tmpOutputName, ForwardCompatibilitySubstitutionRule::Enable, key_values, reason);

            DynamicPrintConfig new_config;
            Preset*       inherit_preset  = nullptr;
            ConfigOption* inherits_config = config.option(BBL_JSON_KEY_INHERITS);
            std::string   inherits_value;
            if (inherits_config) {
                ConfigOptionString* option_str = dynamic_cast<ConfigOptionString*>(inherits_config);
                inherits_value                 = option_str->value;
                inherit_preset                 = collection->find_preset(inherits_value, false, true);
            }
            if (inherit_preset) {
                new_config = inherit_preset->config;
            } else {
                break;
            }
            t_config_option_keys diffKeys = new_config.diff(config);
            jsonOut                       = json();
            for (auto item : diffKeys) {
                if (item == BBL_JSON_KEY_INHERITS)
                    continue;
                if (!jsonIn["engine_data"][item].is_null()) {
                    jsonOut[item] = jsonIn["engine_data"][item];
                }
            }
            jsonOut["base_id"] = inherit_preset->base_id;

        } else {
            jsonIn["engine_data"].erase("inherits");
            for (auto item : jsonIn["engine_data"].items()) {
                if (!item.value().is_null()) {
                    jsonOut[item.key()] = item.value();
                }
            }
            jsonOut["base_id"]                = "";
        }

        if (m_override == 4) {
            std::string new_name   = "";
            Preset*     new_preset = nullptr;
            for (int i = 1; i < 100; ++i) {
                new_name   = outputName + "(" + std::to_string(i) + ")";
                new_preset = collection->find_preset(new_name, false);
                if (new_preset == nullptr)
                    break;
            }
            if (!new_preset) {
                outputName = new_name;
            }
        }

        //  重新组合
        jsonOut["from"]                   = "User";
        jsonOut["compatible_printers"]    = json::array();
        jsonOut["compatible_printers"][0] = getCompatiblePrinters(fileData);
        jsonOut["inherits"]               = inherits;
        jsonOut["is_custom_defined"]      = "0";
        jsonOut["name"]                   = outputName;
        jsonOut["print_settings_id"]   = outputName;
        jsonOut["instantiation"]          = "true";
        jsonOut["version"]                = "3.0.0.0";

        /*
        for (auto item : diffKeys) {
            if (item == BBL_JSON_KEY_INHERITS || item == "printer_model")
                continue;
            if (!jsonIn["printer"][item].is_null()) {
                jsonOut[item] = jsonIn["printer"][item];
            }
        }*/
        const UserInfo& userInfo = wxGetApp().get_user();
        std::string     outJsonFile;
        if (userInfo.bLogin) {
            outJsonFile =
                fs::path(data_dir()).append("user").append(userInfo.userId).append("process").append(outputName + ".json").string();
        } else {
            outJsonFile = fs::path(data_dir()).append("user").append("default").append("process").append(outputName + ".json").string();
        }
        if (fs::exists(outJsonFile)) {
            fs::remove(outJsonFile);
        }
        boost::nowide::ofstream c;
        c.open(outJsonFile, std::ios::out | std::ios::trunc);
        c << std::setw(4) << jsonOut << std::endl;
        c.close();

                //  保存info文件
        std::string outInfoFile;
        if (userInfo.bLogin) {
            outInfoFile =
                fs::path(data_dir()).append("user").append(userInfo.userId).append("process").append(outputName + ".info").string();

            if (fs::exists(outInfoFile)) {
                fs::remove(outInfoFile);
            }

            boost::nowide::ofstream c2;
            c2.open(outInfoFile, std::ios::out | std::ios::trunc);
            c2 << "sync_info = create" << std::endl;
            c2 << "user_id = " << userInfo.userId << std::endl;
            c2 << "setting_id =" << std::endl;
            c2 << "base_id = " << 12345 << std::endl;
            c2 << "updated_time =" << std::endl;
            c2.close();
        }

        bRet = true;
    } while (0);
    return bRet;
}
int LoadOldPresets::isPresetExist(const FileData& fileData)
{
    int nRet = 0;
    do {
        PresetCollection* collection = nullptr;
        std::string       presetName = fileData.fileName;
        if (fileData.type == DefJson) {
            if (fileData.isSystemPreset)
                break;
            collection = &GUI::wxGetApp().preset_bundle->printers;
            int pos    = fileData.fileName.rfind("-");
            if (pos != -1) {
                presetName = fileData.fileName.substr(0, pos);
            }
            if (!fileData.inheritsName.empty())
                presetName = fileData.inheritsName;
        } else if (fileData.type == Filament) {
            collection = &GUI::wxGetApp().preset_bundle->filaments;
            int pos    = fileData.fileName.rfind("-");
            if (pos != -1) {
                presetName = fileData.fileName.substr(0, pos);
            }
            if (!getCompatiblePrinters(fileData).empty())
                presetName = presetName + " @" + getCompatiblePrinters(fileData);
        } else if (fileData.type == Process) {
            collection             = &GUI::wxGetApp().preset_bundle->prints;
        }
        if (collection == nullptr) {
            nRet = -1;
            break;
        }
        Preset* p = collection->find_preset(presetName, false);
        if (p) {
            if (p->is_default || p->is_system) {
                nRet = -2;
                break;
            }else {
                nRet = 1;
            }
        } 
    } while (0);
    return nRet;
}
std::string LoadOldPresets::getTmpOutputName(const std::string& fileName) { 
    if (fileName.empty())
        return "";
    return fileName + ".tmp";
}
std::string LoadOldPresets::getCompatiblePrinters(const FileData& fileData) { return m_printerData.printerInherits; }
}} // namespace Slic3r::GUI
