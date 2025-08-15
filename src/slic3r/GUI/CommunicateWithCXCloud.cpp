#include "CommunicateWithCXCloud.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <wx/base64.h>

#include "GUI.hpp"
#include "GUI_App.hpp"
#include "../Utils/Http.hpp"
#define SLIC3R_ALLOW_LIBSLIC3R_I18N_IN_SLIC3R
#include "libslic3r/I18N.hpp"
#undef SLIC3R_ALLOW_LIBSLIC3R_I18N_IN_SLIC3R
#include "slic3r/GUI/I18N.hpp"
#include "libslic3r/Time.hpp"

#include <slic3r/GUI/print_manage/AccountDeviceMgr.hpp>

namespace Slic3r { 
namespace GUI {
CXCloudDataCenter& CXCloudDataCenter::getInstance()
{
    static CXCloudDataCenter instance;
    return instance;
}

std::map<std::string, std::map<std::string, std::string>> CXCloudDataCenter::getUserCloudPresets() {
    std::map<std::string, std::map<std::string, std::string>> mapUserCloudPresets;
    m_mutexUserCloudPresets.lock();
    mapUserCloudPresets = m_mapUserCloudPresets;
    m_mutexUserCloudPresets.unlock();
    return std::move(mapUserCloudPresets);
}

static std::string getPresetValue(const std::map<std::string, std::string>& mapPresetValue) { 
    std::string value;
    auto        iter = mapPresetValue.find("name");
    value += "[name=";
    if (iter != mapPresetValue.end()) {
        value += iter->second;
    }
    iter = mapPresetValue.find("type");
    value += ",type=";
    if (iter != mapPresetValue.end()) {
        value += iter->second;
    }
    iter = mapPresetValue.find("user_id");
    value += ",user_id=";
    if (iter != mapPresetValue.end()) {
        value += iter->second;
    }
    iter = mapPresetValue.find("setting_id");
    value += ",setting_id=";
    if (iter != mapPresetValue.end()) {
        value += iter->second;
    }
    iter = mapPresetValue.find("updated_time");
    value += ",updated_time=";
    if (iter != mapPresetValue.end()) {
        value += iter->second;
    }
    value += "]";
    return value;
}

void CXCloudDataCenter::setUserCloudPresets(const std::string&                        presetName,
                                            const std::string&                        settingID,
                                            const std::map<std::string, std::string>& mapPresetValue)
{
    m_mutexUserCloudPresets.lock();
    auto iter = m_mapUserCloudPresets.find(presetName);
    if (iter == m_mapUserCloudPresets.end()){
        m_mapUserCloudPresets[presetName] = mapPresetValue;
        m_mapSettingID2PresetName[settingID] = presetName;
    } else {
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets CXCloudDataCenter setUserCloudPresets has same preset data.oldData="
                                   << getPresetValue(iter->second) << ",newData=" << getPresetValue(mapPresetValue);
        m_mapUserCloudPresets[presetName]    = mapPresetValue;
        m_mapSettingID2PresetName[settingID] = presetName;
    }
    m_mutexUserCloudPresets.unlock();
}

void CXCloudDataCenter::cleanUserCloudPresets() { 
    m_mutexUserCloudPresets.lock();
    m_mapUserCloudPresets.clear();
    m_mapSettingID2PresetName.clear();
    m_mutexUserCloudPresets.unlock();
}

void CXCloudDataCenter::updateUserCloudPresets(const std::string& presetName, const std::string& settingID, const std::map<std::string, std::string>& mapPresetValue) {
    m_mutexUserCloudPresets.lock();
    auto iter = m_mapUserCloudPresets.find(presetName);
    if (iter != m_mapUserCloudPresets.end()) {
        iter->second = mapPresetValue;
    } else {
        // BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets CXCloudDataCenter setUserCloudPresets has same preset data.oldData="
        //                           << getPresetValue(iter->second) << ",newData=" << getPresetValue(mapPresetValue);
        m_mapUserCloudPresets[presetName]    = mapPresetValue;
        m_mapSettingID2PresetName[settingID] = presetName;
    }
    m_mutexUserCloudPresets.unlock();
}

int CXCloudDataCenter::deleteUserPresetBySettingID(const std::string& settingID) { 
    auto iter = m_mapSettingID2PresetName.find(settingID);
    if (iter != m_mapSettingID2PresetName.end()) {
        if (m_mapUserCloudPresets.find(iter->second) != m_mapUserCloudPresets.end()) {
            m_mapUserCloudPresets.erase(iter->second);
        }
        m_mapSettingID2PresetName.erase(iter->first);
    }
    return 0; 
}

void CXCloudDataCenter::setDownloadConfigToLocalState(ENDownloadConfigState state) { m_enDownloadConfigToLocalState = state; }
ENDownloadConfigState CXCloudDataCenter::getDownloadConfigToLocalState() { return m_enDownloadConfigToLocalState; }

bool CXCloudDataCenter::isUpdateConfigFileTimeout() { 
    long long now = Slic3r::Utils::get_current_time_utc(); 
    long long timediff = now - m_llUpdateConfigFileTimestamp;
    if (timediff >= 5 * 60 * 60) {
        return true;
    }
    return false;
}
void CXCloudDataCenter::resetUpdateConfigFileTime() { m_llUpdateConfigFileTimestamp = Slic3r::Utils::get_current_time_utc(); }

bool CXCloudDataCenter::isNetworkErrorRetryTimeout()
{
    long long now      = Slic3r::Utils::get_current_time_utc();
    long long timediff = now - m_llNetworkErrorRetryTimestamp;
    if (timediff >= 5 * 60 * 60) {
        return true;
    }
    return false;
}

void CXCloudDataCenter::resetNetworkErrorRetryTime() { m_llNetworkErrorRetryTimestamp = Slic3r::Utils::get_current_time_utc(); }

bool CXCloudDataCenter::isNetworkError() { return m_bNetworkError; }

void CXCloudDataCenter::setNetworkError(bool bError) { m_bNetworkError = bError; }

void CXCloudDataCenter::setConfigFileRetInfo(const PreUpdateProfileRetInfo& retInfo) { m_configFileRetInfo = retInfo; }
const PreUpdateProfileRetInfo& CXCloudDataCenter::getConfigFileRetInfo() { return m_configFileRetInfo; }

void CXCloudDataCenter::updateCXCloutLoginInfo(const std::string& userId, const std::string& token)
{
    BOOST_LOG_TRIVIAL(warning) << "CXCloudDataCenter updateCXCloutLoginInfo token=" << token << ",userId=" << userId;
    m_cxCloudLoginInfoMutex.lock();
    m_cxCloudLoginInfo.token  = token;
    m_cxCloudLoginInfo.userId = userId;
    m_cxCloudLoginInfo.tokenValid = true;
    m_cxCloudLoginInfoMutex.unlock();
}
void CXCloudDataCenter::setTokenInvalid(bool bTokenValid/* = false*/)
{
    BOOST_LOG_TRIVIAL(warning) << "CXCloudDataCenter setTokenInvalid " << bTokenValid;
    m_cxCloudLoginInfoMutex.lock();
    m_cxCloudLoginInfo.tokenValid = bTokenValid;
    if (bTokenValid) {
        m_bNetworkError = false;
    }
    m_cxCloudLoginInfoMutex.unlock();
}

bool CXCloudDataCenter::isTokenValid() { 
    bool tokenValid = false;
    m_cxCloudLoginInfoMutex.lock();
    tokenValid = m_cxCloudLoginInfo.tokenValid;
    m_cxCloudLoginInfoMutex.unlock();
    return tokenValid; 
}

bool CXCloudDataCenter::isDownloadPresetFinished()
{
    return m_enDownloadPresetState.load() == ENDownloadPresetState::ENDPS_DOWNLOAD_SUCCESS ||
            m_enDownloadPresetState.load() == ENDownloadPresetState::ENDPS_DOWNLOAD_FAILED;
}

CLocalDataCenter& CLocalDataCenter::getInstance()
{
    static CLocalDataCenter instance;
    return instance;
}

bool CLocalDataCenter::isPrinterPresetExisted(const std::string& presetName)
{
    bool bRet = false;

    PresetCollection* collection = nullptr;
    collection                   = &GUI::wxGetApp().preset_bundle->printers;
    Preset* inherit_preset       = collection->find_preset(presetName, false, true);
    if (inherit_preset) {
        bRet = true;
    }

    return bRet;
}

bool CLocalDataCenter::isFilamentPresetExisted(const std::string& presetName)
{
    bool bRet = false;

    PresetCollection* collection = nullptr;
    collection                   = &GUI::wxGetApp().preset_bundle->filaments;
    Preset* inherit_preset       = collection->find_preset(presetName, false, true);
    if (inherit_preset) {
        bRet = true;
    }

    return bRet;
}

bool CLocalDataCenter::isProcessPresetExisted(const std::string& presetName)
{
    bool bRet = false;

    PresetCollection* collection = nullptr;
    collection                   = &GUI::wxGetApp().preset_bundle->prints;
    Preset* inherit_preset       = collection->find_preset(presetName, false, true);
    if (inherit_preset) {
        bRet = true;
    }

    return bRet;
}

CommunicateWithCXCloud::CommunicateWithCXCloud(){

}
CommunicateWithCXCloud::~CommunicateWithCXCloud() {}

int CommunicateWithCXCloud::getUserProfileList(std::vector<UserProfileListItem>& vtUserProfileListItem)
{ 
    int         nRet                  = -1;
    std::string version               = ""; // preset_bundle->get_vendor_profile_version(PresetBundle::BBL_BUNDLE).to_string();
    std::string version_type          = get_vertion_type();
    std::string base_url              = get_cloud_api_url();
    auto        preupload_profile_url = "/api/cxy/v2/slice/profile/user/list";
    Http::set_extra_headers(wxGetApp().get_extra_header());
    Http http = Http::post(base_url + preupload_profile_url);

    BOOST_LOG_TRIVIAL(warning) << "CommunicateWithCXCloud getUserProfileList [" << base_url << preupload_profile_url << "]";

    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    json               j;
    j["version"] = version;

    http.header("Content-Type", "application/json")
        .header("__CXY_REQUESTID_", to_string(uuid))
        .timeout_connect(5)
        .timeout_max(15)
        .set_post_body(j.dump())
        .on_complete([&](std::string body, unsigned status) {
            if (Slic3r::GUI::wxGetApp().app_config->get("showDebugLog") == "1") {
                BOOST_LOG_TRIVIAL(warning) << "getUserProfileList json=" << body;
            }
            json j = json();
            try {
                j = json::parse(body);
            } catch (nlohmann::detail::parse_error& err) {
                BOOST_LOG_TRIVIAL(error) << "SyncUserPresets CommunicateWithCXCloud getUserProfileList parse body fail";
                nRet = -1;
                return;
            }
            if (j["code"] != 0) {
                if (j["code"].get<int>() == 4) {
                    CXCloudDataCenter::getInstance().setTokenInvalid();
                } else {
                    CXCloudDataCenter::getInstance().setTokenInvalid(true);
                }
                nRet = 1;       // 请求失败
                setLastError(std::to_string(j["code"].get<int>()), "");
                BOOST_LOG_TRIVIAL(error) << "SyncUserPresets CommunicateWithCXCloud getUserProfileList fail.code=" << j["code"];
                return;
            }
            CXCloudDataCenter::getInstance().setTokenInvalid(true);
            int total_count = 0;
            for (auto& file : j["result"]["list"]) {
                UserProfileListItem userProfileListItem;
                userProfileListItem.id      = file.contains("id") ? file["id"].get<std::string>() : "";
                userProfileListItem.lastModifyTime = file.contains("lastModifyTime") ? file["lastModifyTime"].get<long long>() : 0;
                if (file["nozzleDiameter"].is_array()) {
                    for (size_t i = 0; i < file["nozzleDiameter"].size(); ++i) {
                        userProfileListItem.nozzleDiameter.push_back(file["nozzleDiameter"][i].get<std::string>());
                    }
                }
                userProfileListItem.printerIntName = file.contains("printerIntName") ? file["printerIntName"].get<std::string>() : "";
                userProfileListItem.type           = file.contains("type") ? file["type"].get<std::string>() : "";
                userProfileListItem.version        = file.contains("version") ? file["version"].get<std::string>() : "";
                userProfileListItem.zipUrl         = file.contains("zipUrl") ? file["zipUrl"].get<std::string>() : "";
                vtUserProfileListItem.push_back(std::move(userProfileListItem));
            }
            nRet = 0;
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            BOOST_LOG_TRIVIAL(error) << "CommunicateWithCXCloud getUserProfileList fail.err=" << error << ",status=" << status;
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {

        })
        .perform_sync();

    return nRet; 
}

int CommunicateWithCXCloud::downloadUserPreset(const UserProfileListItem& userProfileListItem, std::string& saveJsonFile)
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets downloadUserPreset start...";
    BOOST_LOG_TRIVIAL(warning) << "CommunicateWithCXCloud downloadUserPreset id=" << userProfileListItem.id
        << " type=" << userProfileListItem.type << " name=" << userProfileListItem.printerIntName << " url=" << userProfileListItem.zipUrl;
    
    int  nRet = -1;
    const std::string& url  = userProfileListItem.zipUrl;
    const std::string& version = userProfileListItem.version;
    long long          update_time = userProfileListItem.lastModifyTime;
    const std::string& file_type   = userProfileListItem.type;
    const std::string& setting_id  = userProfileListItem.id;

    const UserInfo& userInfo = wxGetApp().get_user();
    std::string user = userInfo.userId;
    fs::path    user_folder(data_dir() + "/" + PRESET_USER_DIR);
    if (!fs::exists(user_folder))
        fs::create_directory(user_folder);

    std::string dir_user_presets = data_dir() + "/" + PRESET_USER_DIR + "/" + user;
    fs::path    dest_path(user_folder / user);
    if (!fs::exists(dest_path))
        fs::create_directory(dest_path);

    auto http = Http::get(url);
    http.timeout_max(15).on_header_callback([&](std::string header) {
            std::string filename;

            std::regex  r("filename=\"([^\"]*)\"");
            std::smatch match;
            if (std::regex_search(header.cbegin(), header.cend(), match, r)) {
                filename = match.str(1);
            }

            if (!filename.empty()) {
                dest_path = dest_path / filename;
            }
        })
        .on_progress([](Http::Progress progress, bool& cancel) {})
        .on_error([](std::string body, std::string error, unsigned http_status) {
            BOOST_LOG_TRIVIAL(error) << "CommunicateWithCXCloud downloadUserPreset fail.err=" << error << ",status=" << http_status;
        })
        .on_complete([&](std::string body, unsigned /* http_status */) {
            if (Slic3r::GUI::wxGetApp().app_config->get("showDebugLog") == "1") {
                BOOST_LOG_TRIVIAL(warning) << "downloadUserPreset json=" << body;
            }
            try {
                if (userProfileListItem.type == "sync_data") {
                    if (userInfo.bLogin) {
                        saveSyncDataToLocal(userInfo, userProfileListItem, body, saveJsonFile);
                        nRet = 0;
                    }
                    return;
                } else if (userProfileListItem.type == "local_device") {
                    if (userInfo.bLogin) {
                        saveLocalDeviceToLocal(userInfo, userProfileListItem, body, saveJsonFile);
                        nRet = 0;
                    }
                    return;
                }
                json j = json();
                try {
                    j = json::parse(body);
                } catch (nlohmann::detail::parse_error& err) {
                    BOOST_LOG_TRIVIAL(error) << "SyncUserPresets CommunicateWithCXCloud downloadUserPreset parse body fail";
                    nRet = 0;
                    return;
                }
                json                               jsonOut = json();
                std::map<std::string, std::string> inner_map;
                for (auto& element : j.items()) {
                    auto key   = element.key();
                    auto value = element.value();
                    if (value.is_array()) {
                        std::string ssValue = "";
                        for (int i = 0; i < value.size(); ++i) {
                            if (i != 0) {
                                if ("compatible_printers" == key) {
                                    ssValue += ";";
                                    inner_map[key] += ";";
                                } else {
                                    ssValue += ",";
                                    inner_map[key] += ",";
                                }
                            }
                            inner_map[key] += value[i];
                            ssValue += value[i];
                        }
                        if (value.size() != 0) {
                            jsonOut[element.key()] = ssValue;
                        } else {
                            if (inner_map.find(key) != inner_map.end())
                                inner_map[key].pop_back();
                        }
                    } else if (!value.is_null()) {
                        inner_map[key] = value;
                        jsonOut[element.key()] = element.value();
                    }
                }
                inner_map.emplace("type", file_type);
                inner_map.emplace("user_id", user);
                inner_map.emplace("version", version);
                inner_map.emplace("updated_time", std::to_string(update_time));
                inner_map.emplace("setting_id", setting_id);
                if (inner_map.find("base_id") == inner_map.cend()) {
                    inner_map.emplace("base_id", "");
                }

                CXCloudDataCenter::getInstance().setUserCloudPresets(j["name"], setting_id, inner_map);

                std::string outputName = "";
                if (jsonOut.contains("name"))
                    outputName = jsonOut["name"];

                std::string outJsonFile;
                std::string outInfoFile;
                PresetCollection* collection = nullptr;
                if (userInfo.bLogin) {
                    if (file_type == "printer") {
                        fs::path pathMachine = fs::path(data_dir()).append("user").append(userInfo.userId).append("machine");
                        if (j.contains("inherits") && j["inherits"].get<std::string>().empty()) {
                            fs::path pathBase = pathMachine.append("base");
                            if (!fs::exists(pathBase))
                                fs::create_directory(pathBase);
                            outJsonFile = fs::path(pathBase).append(outputName + ".json").string();
                            outInfoFile = fs::path(pathBase).append(outputName + ".info").string();
                        } else {
                            outJsonFile = fs::path(pathMachine).append(outputName + ".json").string();
                            outInfoFile = fs::path(pathMachine).append(outputName + ".info").string();
                        }
                        collection = &GUI::wxGetApp().preset_bundle->printers;                        
                        if (collection->get_selected_preset_name() == j["name"].get<std::string>()) {
                            Preset* p         = collection->find_preset(j["name"].get<std::string>());
                            Preset* pInherits = collection->find_preset(j["inherits"].get<std::string>());
                            if (p != nullptr && pInherits != nullptr) {
                                
                                collection->lock();
                                DynamicPrintConfig dcRemote = pInherits->config;
                                ForwardCompatibilitySubstitutionRule rule = ForwardCompatibilitySubstitutionRule::Enable;
                                dcRemote.load_string_map(inner_map, rule);
                                DynamicPrintConfig       dcLocal       = p->config;
                                std::vector<std::string> dirty_options = p->config.diff(dcRemote);
                                for (auto item : dirty_options) {
                                    collection->get_selected_preset().config.optptr(item)->set(dcRemote.option(item));
                                }
                                for (auto item : dirty_options) {
                                    p->config.optptr(item)->set(dcLocal.option(item));
                                }
                                if (dirty_options.size() > 0) {
                                    collection->get_selected_preset().set_dirty();
                                }
                                collection->unlock();
                            }
                        }
                    } else if (file_type == "materia") {
                        fs::path pathFilament = fs::path(data_dir()).append("user").append(userInfo.userId).append("filament");
                        if (j.contains("inherits") && j["inherits"].get<std::string>().empty()) {
                            fs::path pathBase = pathFilament.append("base");
                            if (!fs::exists(pathBase))
                                fs::create_directory(pathBase);
                            outJsonFile = fs::path(pathBase).append(outputName + ".json").string();
                            outInfoFile = fs::path(pathBase).append(outputName + ".info").string();
                        } else {
                            outJsonFile = fs::path(pathFilament).append(outputName + ".json").string();
                            outInfoFile = fs::path(pathFilament).append(outputName + ".info").string();
                        }
                        collection = &GUI::wxGetApp().preset_bundle->filaments;
                        if (collection->get_selected_preset_name() == j["name"].get<std::string>()) {
                            Preset* p         = collection->find_preset(j["name"].get<std::string>());
                            Preset* pInherits = collection->find_preset(j["inherits"].get<std::string>());
                            if (p != nullptr && pInherits != nullptr) {
                                collection->lock();
                                DynamicPrintConfig dcRemote = pInherits->config;
                                ForwardCompatibilitySubstitutionRule rule = ForwardCompatibilitySubstitutionRule::Enable;
                                dcRemote.load_string_map(inner_map, rule);
                                DynamicPrintConfig       dcLocal       = p->config;
                                std::vector<std::string> dirty_options = p->config.diff(dcRemote);
                                for (auto item : dirty_options) {
                                    collection->get_selected_preset().config.optptr(item)->set(dcRemote.option(item));
                                }
                                for (auto item : dirty_options) {
                                    p->config.optptr(item)->set(dcLocal.option(item));
                                }
                                if (dirty_options.size() > 0) {
                                    collection->get_selected_preset().set_dirty();
                                    CXCloudDataCenter::getInstance().setFilamentPresetDirty(true);
                                }
                                collection->unlock();
                            }
                        }
                    } else if (file_type == "process") {
                        fs::path pathProcess = fs::path(data_dir()).append("user").append(userInfo.userId).append("process");
                        outJsonFile          = fs::path(pathProcess).append(outputName + ".json").string();
                        outInfoFile          = fs::path(pathProcess).append(outputName + ".info").string();
                        collection           = &GUI::wxGetApp().preset_bundle->prints;
                        if (collection->get_selected_preset_name() == j["name"].get<std::string>()) {
                            Preset* p         = collection->find_preset(j["name"].get<std::string>());
                            Preset* pInherits = collection->find_preset(j["inherits"].get<std::string>());
                            if (p != nullptr && pInherits != nullptr) {
                                collection->lock();
                                DynamicPrintConfig dcRemote = pInherits->config;
                                ForwardCompatibilitySubstitutionRule rule = ForwardCompatibilitySubstitutionRule::Enable;
                                dcRemote.load_string_map(inner_map, rule);
                                DynamicPrintConfig       dcLocal       = p->config;
                                std::vector<std::string> dirty_options = p->config.diff(dcRemote);
                                for (auto item : dirty_options) {
                                    collection->get_selected_preset().config.optptr(item)->set(dcRemote.option(item));
                                }
                                for (auto item : dirty_options) {
                                    p->config.optptr(item)->set(dcLocal.option(item));
                                }
                                if (dirty_options.size() > 0) {
                                    collection->get_selected_preset().set_dirty();
                                    CXCloudDataCenter::getInstance().setProcessPresetDirty(true);
                                }
                                collection->unlock();
                            }
                        }
                    }
                } else {
                    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets downloadUserPreset end not login";
                    return;
                }

                //if (fs::exists(outJsonFile)) {
                //    fs::remove(outJsonFile);
                //}
                //boost::nowide::ofstream c;
                //c.open(outJsonFile, std::ios::out | std::ios::trunc);
                //c << std::setw(4) << jsonOut << std::endl;
                //c.close();
                //saveJsonFile = outJsonFile;

                ////  保存info文件
                //if (userInfo.bLogin) {

                //    if (fs::exists(outInfoFile)) {
                //        fs::remove(outInfoFile);
                //    }
                //    boost::nowide::ofstream c2;
                //    c2.open(outInfoFile, std::ios::out | std::ios::trunc);
                //    c2 << "sync_info =" << std::endl;
                //    c2 << "user_id = " << user << std::endl;
                //    c2 << "setting_id =" << setting_id << std::endl;
                //    if (jsonOut.contains("base_id"))
                //        c2 << "base_id = " << jsonOut["base_id"].get<std::string>() << std::endl;
                //    else
                //        c2 << "base_id = " << std::endl;
                //    c2 << "updated_time =" << update_time << std::endl;
                //    c2.close();
                //}

                nRet = 0;
            } catch (const std::exception& e) {
                auto err = e.what();
            }
        })
        .perform_sync();
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets downloadUserPreset end";
    return nRet;
}

int CommunicateWithCXCloud::preUpdateProfile_create(const UploadFileInfo& fileInfo, PreUpdateProfileRetInfo& retInfo)
{
    int nRet = -1;

    std::string base_url = get_cloud_api_url();
    std::string new_setting_id;
    auto        preupload_profile_url = "/api/cxy/v2/slice/profile/user/preUploadProfile";
    Http::set_extra_headers(wxGetApp().get_extra_header());
    Http        http = Http::post(base_url + preupload_profile_url);
    std::string file_type;
    json        j;
    j["md5"]                = get_file_md5(fileInfo.file);
    j["type"]               = fileInfo.type;
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    http.header("Content-Type", "application/json")
        .header("__CXY_REQUESTID_", to_string(uuid))
        .set_post_body(j.dump())
        .on_complete([&](std::string body, unsigned status) {
            CXCloudDataCenter::getInstance().setNetworkError(false);
            json j;
            try {
                j = json::parse(body);
            } catch (nlohmann::detail::parse_error& err) {
                BOOST_LOG_TRIVIAL(error) << "SyncUserPresets CommunicateWithCXCloud preUpdateProfile_create parse body fail";
                return;
            }
            if (j["code"] != 0) {
                if (j["code"].get<int>() == 4) {
                    CXCloudDataCenter::getInstance().setTokenInvalid();
                    BOOST_LOG_TRIVIAL(error) << "SyncUserPresets CommunicateWithCXCloud preUpdateProfile_create fail.code=" << j["code"];
                }
                BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets CommunicateWithCXCloud preUpdateProfile_create fail.code=" << j["code"]
                                           << ";body=" << body;
                return;
            }
            new_setting_id             = j["result"]["id"];
            auto        lastModifyTime = j["result"]["lastModifyTime"];
            auto        uploadPolicy   = j["result"]["uploadPolicy"];
            std::string OSSAccessKeyId = uploadPolicy["OSSAccessKeyId"];
            std::string Signature      = uploadPolicy["Signature"];
            std::string Policy         = uploadPolicy["Policy"];
            std::string Key            = uploadPolicy["Key"];
            std::string Callback       = uploadPolicy["Callback"];
            std::string Host           = uploadPolicy["Host"];
            Http        http           = Http::post(Host);
            http.form_add("OSSAccessKeyId", OSSAccessKeyId)
                .form_add("Signature", Signature)
                .form_add("Policy", Policy)
                .form_add("Key", Key)
                .form_add("Callback", Callback)
                .form_add_file("File", fileInfo.file, fileInfo.name)
                .on_complete([&](std::string body, unsigned status) { auto ifs = fileInfo.file; })
                .on_error([&](std::string body, std::string error, unsigned status) {
                    nRet       = 0;
                    //updated_info = "hold";
                    retInfo.updatedInfo = "hold";
                    BOOST_LOG_TRIVIAL(error) << "CommunicateWithCXCloud preUpdateProfile_create post host fail.err=" << error << ",status=" << status;
                })
                .on_progress([&](Http::Progress progress, bool& cancel) {

                })
                .perform_sync();
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            BOOST_LOG_TRIVIAL(error) << "CommunicateWithCXCloud preUpdateProfile_create fail.err=" << error << ",status=" << status;
            CXCloudDataCenter::getInstance().setNetworkError(true);
            CXCloudDataCenter::getInstance().resetNetworkErrorRetryTime();
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {

        })
        .perform_sync();
    if (!new_setting_id.empty()) {
        nRet                 = 0;
        retInfo.name      = fileInfo.name;
        retInfo.settingId = new_setting_id;
        //auto update_time_str = values_map[BBL_JSON_KEY_UPDATE_TIME];
        //if (!update_time_str.empty())
        //    update_time = std::atoll(update_time_str.c_str());
    }
    return nRet;
}

int CommunicateWithCXCloud::preUpdateProfile_update(const UploadFileInfo& fileInfo, PreUpdateProfileRetInfo& retInfo)
{ 
    int nRet = -1;

    std::string version_type          = get_vertion_type();
    std::string base_url              = get_cloud_api_url();
    auto        preupload_profile_url = "/api/cxy/v2/slice/profile/user/editProfile";
    Http::set_extra_headers(wxGetApp().get_extra_header());
    Http        http = Http::post(base_url + preupload_profile_url);
    std::string file_type;
    json        j;
    j["id"]                 = fileInfo.settingId;
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    http.header("Content-Type", "application/json")
        .header("__CXY_REQUESTID_", to_string(uuid))
        .set_post_body(j.dump())
        .on_complete([&](std::string body, unsigned status) {
            CXCloudDataCenter::getInstance().setNetworkError(false);
            json j = json();
            try {
                j = json::parse(body);
            } catch (nlohmann::detail::parse_error& err) {
                BOOST_LOG_TRIVIAL(error) << "SyncUserPresets CommunicateWithCXCloud preUpdateProfile_update parse body fail";
                return;
            }
            if (j["code"] != 0) {
                if (j["code"].get<int>() == 4) {
                    CXCloudDataCenter::getInstance().setTokenInvalid();
                    BOOST_LOG_TRIVIAL(error) << "SyncUserPresets CommunicateWithCXCloud preUpdateProfile_update fail.code=" << j["code"];
                } else if (j["code"].get<int>() == 517) {   //  数据不存在
                    nRet = 517;
                }
                BOOST_LOG_TRIVIAL(error) << "SyncUserPresets CommunicateWithCXCloud preUpdateProfile_update fail.code=" << j["code"]
                                         << ";body=" << body;
                return;
            }
            std::string setting_id     = j["result"]["id"];
            auto        lastModifyTime = j["result"]["lastModifyTime"];
            auto        uploadPolicy   = j["result"]["uploadPolicy"];
            std::string OSSAccessKeyId = uploadPolicy["OSSAccessKeyId"];
            std::string Signature      = uploadPolicy["Signature"];
            std::string Policy         = uploadPolicy["Policy"];
            std::string Key            = uploadPolicy["Key"];
            std::string Callback       = uploadPolicy["Callback"];
            std::string Host           = uploadPolicy["Host"];
            Http        http           = Http::post(Host);
            http.form_add("OSSAccessKeyId", OSSAccessKeyId)
                .form_add("Signature", Signature)
                .form_add("Policy", Policy)
                .form_add("Key", Key)
                .form_add("Callback", Callback)
                .form_add_file("File", fileInfo.file, fileInfo.name)
                .on_complete([&](std::string body, unsigned status) {
                    nRet = 0;
                })
                .on_error([&](std::string body, std::string error, unsigned status) {
                    nRet       = 21;
                    retInfo.updatedInfo = "hold";
                    BOOST_LOG_TRIVIAL(error) << "CommunicateWithCXCloud preUpdateProfile_update post host fail.err=" << error << ",status=" << status;
                })
                .on_progress([&](Http::Progress progress, bool& cancel) {

                })
                .perform_sync();
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            nRet       = 1;
            retInfo.updatedInfo = "hold";
            BOOST_LOG_TRIVIAL(error) << "CommunicateWithCXCloud preUpdateProfile_update fail.err=" << error << ",status=" << status;
            CXCloudDataCenter::getInstance().setNetworkError(true);
            CXCloudDataCenter::getInstance().resetNetworkErrorRetryTime();
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {

        })
        .perform_sync();
    return nRet;
}

//  删除用户预设
int CommunicateWithCXCloud::deleteProfile(const std::string& ssDeleteSettingId)
{
    int nRet = -1;
    if (ssDeleteSettingId.empty())
        return nRet;
    std::string del_setting_id = ssDeleteSettingId;

    std::string base_url              = get_cloud_api_url();
    auto        preupload_profile_url = "/api/cxy/v2/slice/profile/user/deleteProfile";
    Http::set_extra_headers(wxGetApp().get_extra_header());
    Http http = Http::post(base_url + preupload_profile_url);
    json j;
    j["id"]                 = del_setting_id;
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    http.header("Content-Type", "application/json")
        .header("__CXY_REQUESTID_", to_string(uuid))
        .set_post_body(j.dump())
        .on_complete([&](std::string body, unsigned status) {
            CXCloudDataCenter::getInstance().setNetworkError(false);
            json j;
            try {
                j = json::parse(body);
            } catch (nlohmann::detail::parse_error& err) {
                BOOST_LOG_TRIVIAL(error) << "SyncUserPresets CommunicateWithCXCloud deleteProfile parse body fail";
                return;
            }
            if (j["code"] == 0) {
                nRet = 0;

                BOOST_LOG_TRIVIAL(trace) << "sync_preset: sync operation: delete success! setting id = " << del_setting_id;
            } else {
                if (j["code"].get<int>() == 4) {
                    CXCloudDataCenter::getInstance().setTokenInvalid();
                    BOOST_LOG_TRIVIAL(error) << "SyncUserPresets CommunicateWithCXCloud deleteProfile fail.code=" << j["code"];
                }
                BOOST_LOG_TRIVIAL(info) << "delete setting = " << del_setting_id << " failed";
            }
        })
        .on_error([&](std::string body, std::string error, unsigned status) {
            BOOST_LOG_TRIVIAL(error) << "CommunicateWithCXCloud deleteProfile fail.err=" << error << ",status=" << status;
            CXCloudDataCenter::getInstance().setNetworkError(true);
            CXCloudDataCenter::getInstance().resetNetworkErrorRetryTime();
        })
        .on_progress([&](Http::Progress progress, bool& cancel) {

        })
        .perform_sync();
    return nRet;
}

void CommunicateWithCXCloud::setLastError(const std::string& code, const std::string& msg) {
    m_lastError         = LastError();
    m_lastError.code    = code;
    m_lastError.message = msg;
}

int CommunicateWithCXCloud::saveSyncDataToLocal(const UserInfo&            userInfo,
                                                const UserProfileListItem& userProfileListItem,
                                                const std::string&         body,
                                                std::string&               saveJsonFile)
{ 
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets saveSyncDataToLocal start...";
    std::string outJsonFile;
    std::string outInfoFile;
    if (userInfo.bLogin) {
        fs::path folder_local = fs::path(data_dir()).append("user").append(userInfo.userId).append("sync_data");
        if (!fs::exists(folder_local)) {
            fs::create_directory(folder_local);
        }
        outJsonFile = fs::path(folder_local).append("sync_data.json").string();
        outInfoFile = fs::path(folder_local).append("sync_data.info").string();

        if (fs::exists(outJsonFile)) {
            auto file_time = fs::last_write_time(outJsonFile);
            if(file_time==userProfileListItem.lastModifyTime)
                return 0;
            fs::remove(outJsonFile);
        }
        json j = json();
        try {
            j = json::parse(body);
            boost::nowide::ofstream c;
            c.open(outJsonFile, std::ios::out | std::ios::trunc);
            c << std::setw(4) << j << std::endl;
            c.close();
            CXCloudDataCenter::getInstance().setSyncData(j);
        } catch (...) {
            boost::nowide::ofstream c;
            c.open(outJsonFile, std::ios::out | std::ios::trunc);
            c << std::setw(4) << body << std::endl;
            c.close();
        }
        fs::last_write_time(outJsonFile, userProfileListItem.lastModifyTime);
        saveJsonFile = outJsonFile;

        //  保存info文件
        {
            if (fs::exists(outInfoFile)) {
                fs::remove(outInfoFile);
            }
            boost::nowide::ofstream c2;
            c2.open(outInfoFile, std::ios::out | std::ios::trunc);
            c2 << "sync_info =" << std::endl;
            c2 << "user_id = " << userInfo.userId << std::endl;
            c2 << "setting_id =" << userProfileListItem.id << std::endl;
            c2 << "base_id = " << std::endl;
            c2 << "updated_time =" << userProfileListItem.lastModifyTime << std::endl;
            c2.close();
        }
        fs::last_write_time(outInfoFile, userProfileListItem.lastModifyTime);
        PreUpdateProfileRetInfo retInfo;
        retInfo.settingId = userProfileListItem.id;
        retInfo.updateTime = userProfileListItem.lastModifyTime;
        CXCloudDataCenter::getInstance().setConfigFileRetInfo(retInfo);
    }
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets saveSyncDataToLocal end";
    return 0;
}

int CommunicateWithCXCloud::saveLocalDeviceToLocal(const UserInfo&            userInfo,
                                                   const UserProfileListItem& userProfileListItem,
                                                   const std::string&         body,
                                                   std::string&               saveJsonFile)
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets  saveLocalDeviceToLocal start...";
    int         nRet = 0;
    std::string outJsonFile;
    if (userInfo.bLogin) {
        fs::path folder_local = fs::path(data_dir()).append("user").append(userInfo.userId).append("local_device");
        if (!fs::exists(folder_local)) {
            fs::create_directory(folder_local);
        }
        outJsonFile = fs::path(folder_local).append("account_device_info.json").string();
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets  saveLocalDeviceToLocal outJsonFile=" << outJsonFile;
        //if (fs::exists(outJsonFile)) {
        //    fs::remove(outJsonFile);
        //}
        json j = json();
        try {
            j = json::parse(body);
            json j     = json::parse(body);
            bool check = false;
            if (j["accounts"].type() == detail::value_t::array) {
                for (const auto& account_json : j["accounts"]) {
                    if (account_json.find("my_devices") != account_json.cend()) {
                        check = true;
                    }
                }
            }

            if (check) {
                {
                    std::lock_guard<std::mutex> lock(AccountDeviceMgr::getFileMutex());
                    
                    std::ofstream temp_file(outJsonFile);
                    if (!temp_file.is_open()) {
                        BOOST_LOG_TRIVIAL(error) << "SyncUserPresets  saveLocalDeviceToLocal open outJsonFile fail";
                        return -1;
                    }
                    temp_file << std::setw(4) << j;
                    temp_file.close();
                    saveJsonFile = outJsonFile;
                }
                AccountDeviceMgr::getInstance().load();
                AccountDeviceMgr::getInstance().reset_account_device_file_id(userProfileListItem.id);
            } else {
                BOOST_LOG_TRIVIAL(error) << "CommunicateWithCXCloud saveLocalDeviceToLocal fail. data no my_devices filed";
                nRet = -2;
            }
        } catch (...) {
            BOOST_LOG_TRIVIAL(error) << "CommunicateWithCXCloud saveLocalDeviceToLocal fail. crash";
            nRet = -3;
        }
    }
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets  saveLocalDeviceToLocal end";
    return nRet;
}

/////////////////////////////////////////////////////////////////////////////////////////////
CommunicateWithFrontPage::CommunicateWithFrontPage() {
    m_setIgnoreField = {"base_id",
                        "filament_id",
                        "filament_settings_id",
                        "from",
                        "inherits",
                        "name",
                        "is_custom_defined",
                        "type",
                        "update_time",
                        "updated_time",
                        "printer_settings_id",
                        "print_settings_id",
                        "machine_start_gcode",
                        "machine_end_gcode",
                        "filament_start_gcode",
                        "filament_end_gcode",
                        "change_filament_gcode",
                        "layer_change_gcode",
                        "user_id",
                        "version",
                        "setting_id",
                        "printer_select_mac"};
}
CommunicateWithFrontPage::~CommunicateWithFrontPage() {}

int CommunicateWithFrontPage::getUserPresetParams(const std::map<std::string, std::map<std::string, std::string>>& mapUserCloudPresets,
                                                  std::string&                                                     ssJsonData)
{
    int nRet = -1;
    m_mapPrinterData.clear();
    m_printerId = 0;
    m_filamentId = 0;
    m_processId  = 0;
    for (int i = 0; i < 2; ++i) {
        for (auto item : mapUserCloudPresets) {
            std::vector<std::string> printer_names;
            std::vector<std::string> printer_models;
            std::vector<std::string> nozzles;
            std::string              printer_model;
            std::string              nozzle;
            std::string              preset_type = item.second["type"];
            std::string              preset_type_path;
            if (i == 0) {
                if (preset_type == "printer") {
                    preset_type_path = "machine";
                    std::string ssJsonData;
                    getPrinterPresetParams(item.second, ssJsonData);
                }
            } else if (i == 1) {
                if (preset_type == "filament" || preset_type == "materia") {
                    preset_type_path = "filament";
                    std::string ssJsonData;
                    getFilamentPresetParams(item.second, ssJsonData);
                } else if (preset_type == "print" || preset_type == "process") {
                    preset_type_path = "process";
                    std::string ssJsonData;
                    getProcessPresetParams(item.second, ssJsonData);
                }
            }
        }
    }
    json data_array = json::array();
    for (auto item : m_mapPrinterData) {
        data_array.push_back(item.second);
    }
    std::string data_temp = data_array.dump();

    if (Slic3r::GUI::wxGetApp().app_config->get("showDebugLog") == "1") {
        BOOST_LOG_TRIVIAL(warning) << "getUserPresetParams json=" << data_temp;
    }

    json j;
    j["sequence_id"] = "";
    j["command"]     = "get_user_preset_params";
    j["data"]             = wxBase64Encode(data_temp.data(), data_temp.size()).ToStdString();
    ssJsonData       = j.dump();
    boost::replace_all(ssJsonData, "\\", "\\\\");
    boost::replace_all(ssJsonData, "'", "");
    return nRet;
}

int CommunicateWithFrontPage::getPrinterPresetParams(const std::map<std::string, std::string>& mapPrinterPreset, std::string& ssJsonData)
{
    int                      nRet = -1;

    std::string printer_name;
    std::string printer_model;
    std::string nozzle;
    printer_name = getMapValue(mapPrinterPreset, "inherits");

    if (printer_name.empty()) {
        printer_model = getMapValue(mapPrinterPreset, "printer_model");
        nozzle        = getMapValue(mapPrinterPreset, "nozzle_diameter");
        {
            int lastNoZero = nozzle.length();
            if (!nozzle.empty() && nozzle.find(".") != std::string::npos) {
                for (int i = lastNoZero - 1; i >= 0; --i) {
                    if (nozzle.c_str()[i] != '0') {
                        lastNoZero = i;
                        break;
                    }
                }
                nozzle = nozzle.substr(0, lastNoZero+1);
            }
        }
        
        printer_name  = printer_model + " " + nozzle + " nozzle";
    } else {
        auto pos     = printer_name.find_last_of("@");
        printer_name = printer_name.substr(pos + 1);
        std::regex  re(R"((\d+\.\d+ )nozzle)");
        std::smatch match;

        if (std::regex_search(printer_name, match, re)) {
            nozzle        = match[1];
            printer_model = printer_name.substr(0, match.position() - 1);
        } else {
            printer_model = printer_name;
        }
    }

    if (m_mapPrinterData.find(printer_name) == m_mapPrinterData.cend()) {
        json data_object;
        data_object["filament"]        = json::array();
        data_object["process"]         = json::array();
        data_object["printer"]         = json::array();
        data_object["id"]              = m_printerId++;
        data_object["printer_name"]    = printer_model;
        data_object["nozzle_diameter"] = nozzle;
        json preset_data_object;
        preset_data_object["name"] = getMapValue(mapPrinterPreset, "name");
        preset_data_object["id"]   = getMapValue(mapPrinterPreset, "setting_id");
        long long  timestamp       = std::atoll(getMapValue(mapPrinterPreset, "updated_time").c_str());
        wxDateTime dateTime;
        dateTime.Set((time_t) timestamp);
        preset_data_object["update_time"] = dateTime.FormatISOCombined(' ').ToStdString();
        json modifycations_array          = json::array();
        for (auto preset_data : mapPrinterPreset) {
            if (m_setIgnoreField.find(preset_data.first) != m_setIgnoreField.end()) {
                continue;
            }
            json                   modifycations_object;
            const ConfigOptionDef* config = print_config_def.get(preset_data.first);
            if (config) {
                modifycations_object["name"]  = Slic3r::I18N::translate(config->label);
                modifycations_object["value"] = preset_data.second;
                modifycations_array.push_back(modifycations_object);
            }
        }
        preset_data_object["modifycations"] = modifycations_array;
        data_object["printer"].push_back(preset_data_object);

        m_mapPrinterData.emplace(printer_name, data_object);
        m_mapPrinterName2PrinterModel.emplace(std::make_pair(preset_data_object["name"], nozzle), std::make_pair(printer_model, nozzle));
    } else {
        auto& data_object = m_mapPrinterData[printer_name];
        json  preset_data_object;
        preset_data_object["name"] = getMapValue(mapPrinterPreset, "name");
        preset_data_object["id"]   = getMapValue(mapPrinterPreset, "setting_id");

        long long  timestamp = std::atoll(getMapValue(mapPrinterPreset, "updated_time").c_str());
        wxDateTime dateTime;
        dateTime.Set((time_t) timestamp);
        preset_data_object["update_time"] = dateTime.FormatISOCombined(' ').ToStdString();
        json modifycations_array          = json::array();
        for (auto preset_data : mapPrinterPreset) {
            if (m_setIgnoreField.find(preset_data.first) != m_setIgnoreField.end()) {
                continue;
            }
            json                   modifycations_object;
            const ConfigOptionDef* config = print_config_def.get(preset_data.first);
            if (config) {
                modifycations_object["name"]  = Slic3r::I18N::translate(config->label);
                modifycations_object["value"] = preset_data.second;
                modifycations_array.push_back(modifycations_object);
            }
        }
        preset_data_object["modifycations"] = modifycations_array;
        data_object["printer"].push_back(preset_data_object);
        m_mapPrinterName2PrinterModel.emplace(std::make_pair(preset_data_object["name"], nozzle), std::make_pair(printer_model, nozzle));
    }

    return nRet;
}
int CommunicateWithFrontPage::getFilamentPresetParams(const std::map<std::string, std::string>& mapFilamentPreset, std::string& ssJsonData)
{
    int                      nRet = -1;
    std::vector<std::string> printer_names;
    std::vector<std::string> printer_models;
    std::vector<std::string> nozzles;
    std::string printer_name;
    std::string printer_model;
    std::string nozzle;
    std::string preset_type_path = "filament";

    if (getMapValue(mapFilamentPreset, "from") == "system") {
        return nRet;
    }

    printer_name = getMapValue(mapFilamentPreset, "compatible_printers");

    if (printer_name.empty()) {
        printer_name = getMapValue(mapFilamentPreset, "inherits");
        auto pos     = printer_name.find_last_of("@");
        printer_name = printer_name.substr(pos + 1);
        printer_names.emplace_back(printer_name);
    } else {
        if (printer_name.find(",") != std::string::npos) {
            boost::split(printer_names, printer_name, boost::is_any_of(","));
        } else {
            boost::split(printer_names, printer_name, boost::is_any_of(";"));
        }
    }

    std::regex  re(R"((\d+\.\d+ )nozzle)");
    std::smatch match;
    for (auto& printer_name : printer_names) {
        if (printer_name.empty())
            continue;

        if (std::regex_search(printer_name, match, re)) {
            nozzle        = match[1];
            printer_model = printer_name.substr(0, match.position() - 1);
        } else {
            std::string vendor_name;
            auto        pos = printer_name.find_first_of(" ");
            if (pos != std::string::npos) {
                vendor_name = printer_name.substr(0, pos);
            }
            auto inherit_preset      = getMapValue(mapFilamentPreset, "inherits");
            auto inherit_preset_file = (boost::filesystem::path(Slic3r::resources_dir()) / "profiles" / vendor_name / preset_type_path /
                                        (inherit_preset + ".json"));
            json inherit_preset_json;

            if (fs::exists(inherit_preset_file)) {
                boost::nowide::ifstream ifs(inherit_preset_file.string());
                ifs >> inherit_preset_json;
            }
            if (inherit_preset_json.contains("compatible_printers")) {
                printer_name = inherit_preset_json["compatible_printers"][0];
                std::regex  re(R"((\d+\.\d+ )nozzle)");
                std::smatch match;

                if (std::regex_search(printer_name, match, re)) {
                    nozzle        = match[1];
                    printer_model = printer_name.substr(0, match.position() - 1);
                }
            } else {
                printer_model = printer_name;
            }
        }
        printer_models.emplace_back(printer_model);
        nozzles.emplace_back(nozzle);
    }

    for (size_t i = 0; i < printer_names.size(); i++) {
        printer_name  = printer_names[i];
        if (printer_name.empty())
            continue;

        std::string inherits = getMapValue(mapFilamentPreset, "inherits");
        if (!inherits.empty()) {
            if (!CLocalDataCenter::getInstance().isFilamentPresetExisted(inherits)) {
                BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets CommunicateWithFrontPage getFilamentPresetParams." << inherits
                                           << " not existed";
                continue;
            }
        }

        printer_model = printer_models[i];
        nozzle        = nozzles[i];

        auto iter = m_mapPrinterName2PrinterModel.find(std::make_pair(printer_name, nozzle));
        if (iter == m_mapPrinterName2PrinterModel.end() && nozzle.empty()) {
            for (auto iter2 = m_mapPrinterName2PrinterModel.begin(); iter2 != m_mapPrinterName2PrinterModel.end(); iter2++) {
                if (iter2->first.first == printer_name) {
                    iter = iter2;
                    break;
                }
            }
        }
        if (printer_name.find("Creality") == std::string::npos && iter != m_mapPrinterName2PrinterModel.end()) {
            printer_model = iter->second.first;
            printer_name  = iter->second.first + " " + iter->second.second + " nozzle";
        } else {
            
        }

        if (m_mapPrinterData.find(printer_name) == m_mapPrinterData.cend()) {
            json data_object;
            data_object["filament"]        = json::array();
            data_object["process"]         = json::array();
            data_object["printer"]         = json::array();
            data_object["id"]              = i++;
            data_object["printer_name"]    = printer_model;
            data_object["nozzle_diameter"] = nozzle;
            json preset_data_object;
            preset_data_object["name"] = getMapValue(mapFilamentPreset, "name");
            preset_data_object["id"]   = getMapValue(mapFilamentPreset, "setting_id");
            long long  timestamp       = std::atoll(getMapValue(mapFilamentPreset, "updated_time").c_str());
            wxDateTime dateTime;
            dateTime.Set((time_t) timestamp);
            preset_data_object["update_time"] = dateTime.FormatISOCombined(' ').ToStdString();
            json modifycations_array          = json::array();
            for (auto preset_data : mapFilamentPreset) {
                if (m_setIgnoreField.find(preset_data.first) != m_setIgnoreField.end()) {
                    continue;
                }
                json                   modifycations_object;
                const ConfigOptionDef* config = print_config_def.get(preset_data.first);
                if (config) {
                    modifycations_object["name"]  = Slic3r::I18N::translate(config->label);
                    modifycations_object["value"] = preset_data.second;
                    modifycations_array.push_back(modifycations_object);
                }
            }
            preset_data_object["modifycations"] = modifycations_array;

            data_object["filament"].push_back(preset_data_object);

            m_mapPrinterData.emplace(printer_name, data_object);
        } else {
            auto& data_object = m_mapPrinterData[printer_name];
            json  preset_data_object;
            preset_data_object["name"] = getMapValue(mapFilamentPreset, "name");
            preset_data_object["id"]   = getMapValue(mapFilamentPreset, "setting_id");

            long long  timestamp = std::atoll(getMapValue(mapFilamentPreset, "updated_time").c_str());
            wxDateTime dateTime;
            dateTime.Set((time_t) timestamp);
            preset_data_object["update_time"] = dateTime.FormatISOCombined(' ').ToStdString();
            json modifycations_array          = json::array();
            for (auto preset_data : mapFilamentPreset) {
                if (m_setIgnoreField.find(preset_data.first) != m_setIgnoreField.end()) {
                    continue;
                }
                json                   modifycations_object;
                const ConfigOptionDef* config = print_config_def.get(preset_data.first);
                if (config) {
                    modifycations_object["name"]  = Slic3r::I18N::translate(config->label);
                    modifycations_object["value"] = preset_data.second;
                    modifycations_array.push_back(modifycations_object);
                }
            }
            preset_data_object["modifycations"] = modifycations_array;
            data_object["filament"].push_back(preset_data_object);
        }
    }

    return nRet;
}
int CommunicateWithFrontPage::getProcessPresetParams(const std::map<std::string, std::string>& mapProcessPreset, std::string& ssJsonData)
{
    int nRet = -1;
    std::vector<std::string> printer_names;
    std::vector<std::string> printer_models;
    std::vector<std::string> nozzles;
    std::string              printer_name;
    std::string              printer_model;
    std::string              nozzle;
    std::string              preset_type_path = "process";

    if (getMapValue(mapProcessPreset, "from") == "system") {
        return nRet;
    }

    printer_name = getMapValue(mapProcessPreset, "compatible_printers");

    if (printer_name.empty()) {
        printer_name = getMapValue(mapProcessPreset, "inherits");
        auto pos     = printer_name.find_last_of("@");
        printer_name = printer_name.substr(pos + 1);
        printer_names.emplace_back(printer_name);
    } else {
        if (printer_name.find(",") != std::string::npos) {
            boost::split(printer_names, printer_name, boost::is_any_of(","));
        } else {
            boost::split(printer_names, printer_name, boost::is_any_of(";"));
        }
    }

    std::regex  re(R"((\d+\.\d+ )nozzle)");
    std::smatch match;
    for (auto& printer_name : printer_names) {
        if (printer_name.empty())
            continue;

        if (std::regex_search(printer_name, match, re)) {
            nozzle        = match[1];
            printer_model = printer_name.substr(0, match.position() - 1);
        } else {
            std::string vendor_name;
            auto        pos = printer_name.find_first_of(" ");
            if (pos != std::string::npos) {
                vendor_name = printer_name.substr(0, pos);
            }
            auto inherit_preset      = getMapValue(mapProcessPreset, "inherits");
            auto inherit_preset_file = (boost::filesystem::path(Slic3r::resources_dir()) / "profiles" / vendor_name / preset_type_path /
                                        (inherit_preset + ".json"));
            json inherit_preset_json;

            if (fs::exists(inherit_preset_file)) {
                boost::nowide::ifstream ifs(inherit_preset_file.string());
                ifs >> inherit_preset_json;
            }
            if (inherit_preset_json.contains("compatible_printers")) {
                printer_name = inherit_preset_json["compatible_printers"][0];
                std::regex  re(R"((\d+\.\d+ )nozzle)");
                std::smatch match;

                if (std::regex_search(printer_name, match, re)) {
                    nozzle        = match[1];
                    printer_model = printer_name.substr(0, match.position() - 1);
                }
            } else {
                printer_model = printer_name;
            }
        }
        printer_models.emplace_back(printer_model);
        nozzles.emplace_back(nozzle);
    }

    for (size_t i = 0; i < printer_names.size(); i++) {
        printer_name  = printer_names[i];
        if (printer_name.empty())
            continue;

        printer_model = printer_models[i];
        nozzle        = nozzles[i];

        auto iter = m_mapPrinterName2PrinterModel.find(std::make_pair(printer_name, nozzle));
        if (iter == m_mapPrinterName2PrinterModel.end() && nozzle.empty()) {
            for (auto iter2 = m_mapPrinterName2PrinterModel.begin(); iter2 != m_mapPrinterName2PrinterModel.end(); iter2++) {
                if (iter2->first.first == printer_name) {
                    iter = iter2;
                    break;
                }
            }
        }
        if (printer_name.find("Creality") == std::string::npos && iter != m_mapPrinterName2PrinterModel.end()) {
            printer_model = iter->second.first;
            printer_name  = iter->second.first + " " + iter->second.second + " nozzle";
        } else {
        }

        if (m_mapPrinterData.find(printer_name) == m_mapPrinterData.cend()) {
            json data_object;
            data_object["filament"]        = json::array();
            data_object["process"]         = json::array();
            data_object["printer"]         = json::array();
            data_object["id"]              = i++;
            data_object["printer_name"]    = printer_model;
            data_object["nozzle_diameter"] = nozzle;
            json preset_data_object;
            preset_data_object["name"] = getMapValue(mapProcessPreset, "name");
            preset_data_object["id"]   = getMapValue(mapProcessPreset, "setting_id");
            long long  timestamp       = std::atoll(getMapValue(mapProcessPreset, "updated_time").c_str());
            wxDateTime dateTime;
            dateTime.Set((time_t) timestamp);
            preset_data_object["update_time"] = dateTime.FormatISOCombined(' ').ToStdString();
            json modifycations_array          = json::array();
            for (auto preset_data : mapProcessPreset) {
                if (m_setIgnoreField.find(preset_data.first) != m_setIgnoreField.end()) {
                    continue;
                }
                json                   modifycations_object;
                const ConfigOptionDef* config = print_config_def.get(preset_data.first);
                if (config) {
                    modifycations_object["name"]  = Slic3r::I18N::translate(config->label);
                    modifycations_object["value"] = preset_data.second;
                    modifycations_array.push_back(modifycations_object);
                }
            }
            preset_data_object["modifycations"] = modifycations_array;

            data_object["process"].push_back(preset_data_object);

            m_mapPrinterData.emplace(printer_name, data_object);
        } else {
            auto& data_object = m_mapPrinterData[printer_name];
            json  preset_data_object;
            preset_data_object["name"] = getMapValue(mapProcessPreset, "name");
            preset_data_object["id"]   = getMapValue(mapProcessPreset, "setting_id");

            long long  timestamp = std::atoll(getMapValue(mapProcessPreset, "updated_time").c_str());
            wxDateTime dateTime;
            dateTime.Set((time_t) timestamp);
            preset_data_object["update_time"] = dateTime.FormatISOCombined(' ').ToStdString();
            json modifycations_array          = json::array();
            for (auto preset_data : mapProcessPreset) {
                if (m_setIgnoreField.find(preset_data.first) != m_setIgnoreField.end()) {
                    continue;
                }
                json                   modifycations_object;
                const ConfigOptionDef* config = print_config_def.get(preset_data.first);
                if (config) {
                    modifycations_object["name"]  = Slic3r::I18N::translate(config->label);
                    modifycations_object["value"] = preset_data.second;
                    modifycations_array.push_back(modifycations_object);
                }
            }
            preset_data_object["modifycations"] = modifycations_array;
            data_object["process"].push_back(preset_data_object);
        }
    }

    return nRet;
}

std::string CommunicateWithFrontPage::getMapValue(const std::map<std::string, std::string>& mapObj, const std::string& key) {
    std::string value;
    auto        iter = mapObj.find(key);
    if (iter != mapObj.end()) {
        value = iter->second;
    }
    return value;
}

}} // namespace Slic3r::GUI
