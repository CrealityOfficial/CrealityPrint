#include "SyncUserPresets.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"
#include "MainFrame.hpp"
#include "libslic3r/Time.hpp"
#include "MsgDialog.hpp"
#include "LoginTip.hpp"
#include "ParamsDialog.hpp"
#include "slic3r/GUI/FileDownloader.hpp"
namespace Slic3r { 
namespace GUI {
SyncUserPresets::SyncUserPresets(){

}
SyncUserPresets::~SyncUserPresets() { shutdown(); }

SyncUserPresets& SyncUserPresets::getInstance()
{
    static SyncUserPresets instance;
    return instance;
}

int SyncUserPresets::startup()
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets startup";
    if (m_bRunning) {
        return 1;
    }
    m_thread = std::thread(&SyncUserPresets::onRun, this);
    m_thread.detach();
    return 0; 
}

void SyncUserPresets::shutdown()
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets shutdown";
    if (!m_bRunning.load())
        return;
    m_bRunning.store(false);
    std::unique_lock<std::mutex> lock(m_mutexQuit);
    m_cvQuit.wait(lock, [this](){ return m_bStoped.load(); });
    lock.unlock();
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets shutdown end";
}
void SyncUserPresets::startSync() // 同步工作的启动
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets startSync";
    CXCloudDataCenter::getInstance().updateCXCloutLoginInfo(GUI::wxGetApp().app_config->get("cloud", "user_id"),
                                                            GUI::wxGetApp().app_config->get("cloud", "token"));
    m_bSync.store(true);
}
void SyncUserPresets::stopSync() // 同步工作的不启动
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets stopSync";
    m_bSync.store(false);
}

void SyncUserPresets::syncUserPresetsToLocal()
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets syncUserPresetsToLocal";
    m_mutexLstSyncCmd.lock();
    if (std::find(m_lstSyncCmd.begin(), m_lstSyncCmd.end(), ENSyncCmd::ENSC_SYNC_TO_LOCAL) == m_lstSyncCmd.end()
        && !m_bHasSyncToLocal) {
        m_lstSyncCmd.push_back(ENSyncCmd::ENSC_SYNC_TO_LOCAL);
        m_bHasSyncToLocal = true;
    }
    m_mutexLstSyncCmd.unlock();
}

void SyncUserPresets::syncUserPresetsToCXCloud()
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets syncUserPresetsToCXCloud";
    m_mutexLstSyncCmd.lock();
    m_lstSyncCmd.push_back(ENSyncCmd::ENSC_SYNC_TO_CXCLOUD_CREATE);
    m_mutexLstSyncCmd.unlock();
}

void SyncUserPresets::syncUserPresetsToFrontPage()
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets syncUserPresetsToFrontPage" ;
    m_mutexLstSyncCmd.lock();
    if (m_syncThreadState != ENSyncThreadState::ENTS_SYNC_TO_FRONT_PAGE) {
        if (std::find(m_lstSyncCmd.begin(), m_lstSyncCmd.end(), ENSyncCmd::ENSC_SYNC_TO_FRONT_PAGE) == m_lstSyncCmd.end()) {
            m_lstSyncCmd.push_back(ENSyncCmd::ENSC_SYNC_TO_FRONT_PAGE);
        } else {
            BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets has found syncUserPresetsToFrontPage";
        }
    } else {
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets threadState=" << (int)m_syncThreadState;
    }
    m_mutexLstSyncCmd.unlock();
}

void SyncUserPresets::syncConfigToCXCloud()
{
    m_mutexLstSyncCmd.lock();
    if (std::find(m_lstSyncCmd.begin(), m_lstSyncCmd.end(), ENSyncCmd::ENSC_SYNC_CONFIG_TO_CXCLOUD) == m_lstSyncCmd.end()) {
        m_lstSyncCmd.push_back(ENSyncCmd::ENSC_SYNC_CONFIG_TO_CXCLOUD);
    }
    m_mutexLstSyncCmd.unlock();
}

void SyncUserPresets::setAppHasStartuped() { 
    m_bAppHasStartuped.store(true);
}
void SyncUserPresets::logout() {
    m_bHasSyncToLocal = false;
}

void SyncUserPresets::onRun()
{
    m_bRunning.store(true);
    std::list<ENSyncCmd> lstSyncCmd;
    while (m_bRunning.load()) {
        if (!m_bSync.load()) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        std::string res = GUI::wxGetApp().app_config->get("is_first_install");
        if (!CXCloudDataCenter::getInstance().isTokenValid()&&res=="1") 
        {
            if (m_bAppHasStartuped.load() && !m_bTokenInvalidHasTip) {
                wxGetApp().CallAfter([=] {
                    wxGetApp().mainframe->m_param_dialog->Close();
                    wxGetApp().mainframe->select_tab(MainFrame::tpHome);
                    wxGetApp().swith_community_sub_page("token_expired");
                });
                m_bTokenInvalidHasTip = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        m_bTokenInvalidHasTip = false;

        if (CXCloudDataCenter::getInstance().isNetworkError() && !CXCloudDataCenter::getInstance().isNetworkErrorRetryTimeout()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        lstSyncCmd.clear();
        m_mutexLstSyncCmd.lock();
        lstSyncCmd = m_lstSyncCmd;
        m_lstSyncCmd.clear();
        m_mutexLstSyncCmd.unlock();

        for (auto cmd : lstSyncCmd) {
            m_syncThreadState = ENSyncThreadState::ENTS_IDEL_CHECK;
            if (cmd == ENSyncCmd::ENSC_SYNC_TO_LOCAL) {
                m_syncThreadState = ENSyncThreadState::ENTS_SYNC_TO_LOCAL;
                CXCloudDataCenter::getInstance().setDownloadPresetState(ENDownloadPresetState::ENDPS_DOWNLOADING);
                SyncToLocalRetInfo syncToLocalRetInfo;
                if (doSyncToLocal(syncToLocalRetInfo) != 0) {
                    CXCloudDataCenter::getInstance().setDownloadPresetState(ENDownloadPresetState::ENDPS_DOWNLOAD_FAILED);
                    continue;
                }
                while (wxGetApp().plater()->isLoadingGCode()) {
                    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets isLoadingGCode...";
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
                wxGetApp().CallAfter([=] { 
                    //delLocalUserPresetsInUiThread(syncToLocalRetInfo);
                    reloadPresetsInUiThread();
                    CXCloudDataCenter::getInstance().setDownloadPresetState(ENDownloadPresetState::ENDPS_DOWNLOAD_SUCCESS);
                });
                syncConfigToCXCloud();
                syncUserPresetsToFrontPage();
            } else if (cmd == ENSyncCmd::ENSC_SYNC_TO_FRONT_PAGE) {
                m_syncThreadState = ENSyncThreadState::ENTS_SYNC_TO_FRONT_PAGE;
                std::string jsonData;
                m_commWithFrontPage.getUserPresetParams(CXCloudDataCenter::getInstance().getUserCloudPresets(), jsonData);

                auto response_js = wxString::Format("window.handleStudioCmd('%s')", jsonData);
                wxGetApp().CallAfter([=]() { 
                    wxGetApp().run_script(response_js); 
                });
            } else if (cmd == ENSyncCmd::ENSC_SYNC_CONFIG_TO_CXCLOUD) {
                if (CXCloudDataCenter::getInstance().isUpdateConfigFileTimeout() &&
                    CXCloudDataCenter::getInstance().getDownloadConfigToLocalState() != ENDownloadConfigState::ENDCS_NOT_DOWNLOAD) {
                    CXCloudDataCenter::getInstance().resetUpdateConfigFileTime();
                    //  检测是否配置文件需要同步到创想云
                    doCheckNeedSyncConfigToCXCloud();
                } else {
                    //syncConfigToCXCloud();
                }
            }
        }

        //  检测是否有数据需要同步到创想云
        if (CXCloudDataCenter::getInstance().isTokenValid()) {
            doCheckNeedSyncToCXCloud();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    //  退出时，同步一次配置文件到创想云
    if (m_bSync.load() && CXCloudDataCenter::getInstance().getDownloadConfigToLocalState() != ENDownloadConfigState::ENDCS_NOT_DOWNLOAD) {
        //  检测是否配置文件需要同步到创想云
        doCheckNeedSyncConfigToCXCloud();
    }
    m_bStoped.store(true);
    m_cvQuit.notify_one();
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets thread quited";
}

void SyncUserPresets::reloadPresetsInUiThread()
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets reloadPresetsInUiThread start...";
    auto* app_config = GUI::wxGetApp().app_config;

    //app_config->clear_section("presets");
    //auto printer_name = GUI::wxGetApp().preset_bundle->m_curPrinterPresetName;
    //app_config->set("presets", PRESET_PRINTER_NAME, printer_name);
    //std::string filamentName = GUI::wxGetApp().preset_bundle->m_curFilamentPresetName;
    //app_config->set("presets", PRESET_FILAMENT_NAME, filamentName);
    //std::string processName = GUI::wxGetApp().preset_bundle->m_curProcessPresetName;
    //app_config->set("presets", PRESET_PRINT_NAME, processName);
    //  重新加载预设文件
    //GUI::wxGetApp().preset_bundle->load_presets(*app_config, ForwardCompatibilitySubstitutionRule::EnableSilentDisableSystem);
    //GUI::wxGetApp().load_current_presets();
    //GUI::wxGetApp().plater()->set_bed_shape();
    std::map<std::string, std::map<std::string, std::string>> userCloudPresets = CXCloudDataCenter::getInstance().getUserCloudPresets();
    GUI::wxGetApp().preset_bundle->load_user_presets(*app_config, userCloudPresets, ForwardCompatibilitySubstitutionRule::Enable);
    GUI::wxGetApp().preset_bundle->save_user_presets(*app_config, GUI::wxGetApp().get_delete_cache_presets());
    GUI::wxGetApp().mainframe->update_side_preset_ui();
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets reloadPresetsInUiThread end";
}

int SyncUserPresets::doSyncToLocal(SyncToLocalRetInfo& syncToLocalRetInfo)
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doSyncToLocal start...";
    CXCloudDataCenter::getInstance().cleanUserCloudPresets();
    int                              nRet = 0;
    std::vector<UserProfileListItem> vtUserProfileListItem;
    nRet = m_commWithCXCloud.getUserProfileList(vtUserProfileListItem);
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets getUserProfileList count=" << vtUserProfileListItem.size() << " ret=" << nRet;
    if (nRet != 0) {
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doSyncToLocal end";
        return nRet;
    }

//    copyOldPresetToBackup();

    getLocalUserPresets(syncToLocalRetInfo.vtLocalUserPreset);

    CXCloudDataCenter::getInstance().setDownloadConfigToLocalState(ENDownloadConfigState::ENDCS_CXCLOUD_NO_CONFIG);
    int i = 0;
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets downloadUserPreset ...";
    fs::path tmpPath = fs::path(fs::temp_directory_path()).append("creality_presets");
    if (!fs::exists(tmpPath)) {
            fs::create_directory(tmpPath);
        }else{
            fs::remove_all(tmpPath);
            fs::create_directory(tmpPath);
        }
    CurlConnectionPool pool(3);
    for (auto iter = vtUserProfileListItem.begin(); iter != vtUserProfileListItem.end() && m_bRunning.load(); iter++) {
        int         start_pos = iter->zipUrl.find_last_of("/");
        fs::path tmp_path = fs::path(tmpPath).append(iter->zipUrl.substr(start_pos + 1));
        pool.addDownload(iter->zipUrl, tmp_path.string());
    }
    pool.performDownloads();
    for (auto iter = vtUserProfileListItem.begin(); iter != vtUserProfileListItem.end() && m_bRunning.load(); iter++) {
        int         start_pos = iter->zipUrl.find_last_of("/");
        fs::path tmp_path = fs::path(tmpPath).append(iter->zipUrl.substr(start_pos + 1));
        if(fs::exists(tmp_path)&&fs::file_size(tmp_path)>0){
            try{
                //确保下载的文件内容完整
                json valid_json;
                boost::nowide::ifstream ifs(tmp_path.string());
                ifs >> valid_json;
                iter->zipUrl = wxString::Format("http://127.0.0.1:%d/creality_presets/%s",GUI::wxGetApp().get_server_port(),iter->zipUrl.substr(start_pos + 1)).ToStdString();
            }catch (std::exception& e){
                BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets downloadUserPreset download" << i++ << " error=" << e.what();
                continue;
            }
           
        }
    }
    for (auto iter = vtUserProfileListItem.begin(); iter != vtUserProfileListItem.end() && m_bRunning.load(); iter++) {
        const UserProfileListItem& item = *iter;
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets downloadUserPreset " << i++;
        do {
            std::string saveJsonFile = "";
            nRet = m_commWithCXCloud.downloadUserPreset(item, saveJsonFile);
            if (item.type == "sync_data") { //  配置文件
                if (nRet == 0) {
                    CXCloudDataCenter::getInstance().setDownloadConfigToLocalState(ENDownloadConfigState::ENDCS_DOWNLOAD_SUCCESS);
                } else {
                    CXCloudDataCenter::getInstance().setDownloadConfigToLocalState(ENDownloadConfigState::ENDCS_DOWNLOAD_FAIL);
                }
            }
            if (nRet != 0) {
                if (item.type == "printer") {
                    syncToLocalRetInfo.bPrinterAllOk = false;
                } else if (item.type == "filament" || item.type == "materia") {
                    syncToLocalRetInfo.bFilamentAllOk = false;
                } else if (item.type == "process") {
                    syncToLocalRetInfo.bProcessAllOk = false;
                }
            } else {
                auto iter = std::find_if(syncToLocalRetInfo.vtLocalUserPreset.begin(), syncToLocalRetInfo.vtLocalUserPreset.end(),
                                         [saveJsonFile](const LocalUserPreset& preset) { return preset.file == saveJsonFile; });
                if (iter != syncToLocalRetInfo.vtLocalUserPreset.end()) {
                    iter->needDel = false;
                }
            }
            if (nRet != 0) {
                BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets downloadUserPreset error=" << nRet << ". Retry.";
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
        } while (nRet != 0 && m_bRunning.load());
    }

    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doSyncToLocal end";
    return nRet; 
}

int SyncUserPresets::doCheckNeedSyncToCXCloud() { 
    int nRet = 0;
    if (doCheckNeedSyncPrinterToCXCloud() != 0) {
        nRet = 1;
        if (CXCloudDataCenter::getInstance().isNetworkError()) {
            return nRet;
        }
    }
    if (doCheckNeedSyncFilamentToCXCloud() != 0) {
        nRet = 1;
        if (CXCloudDataCenter::getInstance().isNetworkError()) {
            return nRet;
        }
    }
    if (doCheckNeedSyncProcessToCXCloud() != 0) {
        nRet = 1;
        if (CXCloudDataCenter::getInstance().isNetworkError()) {
            return nRet;
        }
    }
    if (doCheckNeedDeleteFromCXCloud() != 0) {
        nRet = 1;
        if (CXCloudDataCenter::getInstance().isNetworkError()) {
            return nRet;
        }
    }
    if (nRet != 0) {
        syncUserPresetsToFrontPage();
    }
    return 0; 
}

int SyncUserPresets::doCheckNeedSyncPrinterToCXCloud() { 
    int                 nRet = 0;
    std::vector<Preset> presets_to_sync;
    PresetBundle*       preset_bundle = GUI::wxGetApp().preset_bundle;

    //  获取需要添加和更新的preset
    int                 sync_count = 0;
    sync_count = preset_bundle->printers.get_user_presets(preset_bundle, presets_to_sync);
    if (sync_count > 0) {
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncPrinterToCXCloud start...";
        for (Preset& preset : presets_to_sync) {
            auto setting_id = preset.setting_id;

            if ((preset.setting_id.empty() && preset.sync_info.empty()) || 
                (preset.setting_id.empty() && preset.sync_info.compare("update") == 0) ||
                preset.sync_info.compare("create") == 0) 
            {
                std::map<std::string, std::string> values_map;
                int ret = preset_bundle->get_differed_values_to_update(preset, values_map);
                if (ret == 0) {
                    PreUpdateProfileRetInfo retInfo;
                    UploadFileInfo          fileInfo;
                    fileInfo.file = preset.file;
                    fileInfo.name = preset.name;
                    fileInfo.type = preset.get_cloud_type_string(preset.type);

                    ret = m_commWithCXCloud.preUpdateProfile_create(fileInfo, retInfo);
                    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncPrinterToCXCloud preUpdateProfile_create ret=" << ret;
                    if (ret == 0) {
                        // preset_bundle->printers.set_sync_info_and_save(preset.name, setting_id, updated_info, update_time);
                    }
                    if (!retInfo.settingId.empty()) {
                        auto update_time_str = values_map[BBL_JSON_KEY_UPDATE_TIME];
                        if (!update_time_str.empty())
                            retInfo.updateTime = std::atoll(update_time_str.c_str());
                        if (retInfo.updateTime == 0) {
                            retInfo.updateTime    = Slic3r::Utils::get_current_time_utc();
                        }
                        preset_bundle->printers.set_sync_info_and_save(preset.name, retInfo.settingId, retInfo.updatedInfo,
                                                                       retInfo.updateTime, wxGetApp().get_user().userId);
                        values_map["updated_time"] = std::to_string(retInfo.updateTime);
                        values_map.emplace("name", preset.name);
                        values_map.emplace("setting_id", retInfo.settingId);
                        CXCloudDataCenter::getInstance().setUserCloudPresets(preset.name, retInfo.settingId, values_map);
                    }
                }
                nRet = 1;
            } else if (preset.sync_info.compare("update") == 0) {
                if (preset.setting_id.empty()) {
                    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncPrinterToCXCloud sync_info=update, setting_id is empty,name=" << preset.name;
                    continue;
                }

                std::map<std::string, std::string> values_map;
                int                                ret = preset_bundle->get_differed_values_to_update(preset, values_map);
                if (ret == 0) {
                    PreUpdateProfileRetInfo retInfo;
                    UploadFileInfo          fileInfo;
                    fileInfo.file = preset.file;
                    fileInfo.name = preset.name;
                    fileInfo.settingId = preset.setting_id;

                    ret = m_commWithCXCloud.preUpdateProfile_update(fileInfo, retInfo);
                    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncPrinterToCXCloud preUpdateProfile_update ret=" << ret;
                    if (ret == 0 || ret == 1 || ret == 21) {
                        if (ret == 0) {
                            retInfo.updateTime = Slic3r::Utils::get_current_time_utc();
                        } else {
                            auto update_time_str = values_map[BBL_JSON_KEY_UPDATE_TIME];
                            if (!update_time_str.empty())
                                retInfo.updateTime = std::atoll(update_time_str.c_str());
                        }
                        preset_bundle->printers.set_sync_info_and_save(preset.name, preset.setting_id, retInfo.updatedInfo,
                                                                       retInfo.updateTime, wxGetApp().get_user().userId);
                        values_map["updated_time"] = std::to_string(retInfo.updateTime);
                        values_map.emplace("name", preset.name);
                        CXCloudDataCenter::getInstance().updateUserCloudPresets(preset.name, preset.setting_id, values_map);
                        nRet = 1;
                    } else if (ret == 517) {    //  数据不存在，说明可能被删除了，则进行创建
                        fileInfo.type = preset.get_cloud_type_string(preset.type);
                        fileInfo.settingId = "";

                        ret = preUpdateProfile_create(fileInfo, preset_bundle, preset, retInfo, values_map);
                        nRet = 1;
                    }
                }
            } else {
                BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncPrinterToCXCloud err sync_info=" << preset.sync_info
                                           << ",setting_id=" << preset.setting_id << ",name=" << preset.name;
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncPrinterToCXCloud end";
    }
    return nRet; 
}

int SyncUserPresets::doCheckNeedSyncFilamentToCXCloud()
{
    int                 nRet = 0;
    std::vector<Preset> presets_to_sync;
    PresetBundle*       preset_bundle = GUI::wxGetApp().preset_bundle;

    //  获取需要添加和更新的preset
    int sync_count = 0;
    sync_count     = preset_bundle->filaments.get_user_presets(preset_bundle, presets_to_sync);
    if (sync_count > 0) {
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncFilamentToCXCloud start...";
        for (Preset& preset : presets_to_sync) {
            auto setting_id = preset.setting_id;

            if ((preset.setting_id.empty() && preset.sync_info.empty()) || 
                (preset.setting_id.empty() && preset.sync_info.compare("update") == 0) ||
                preset.sync_info.compare("create") == 0) {
                std::map<std::string, std::string> values_map;
                int                                ret = preset_bundle->get_differed_values_to_update(preset, values_map);
                auto                               iter = values_map.find("compatible_printers");
                if (iter != values_map.end()) {
                    std::string printer = iter->second;
                    boost::replace_all(printer, "\"", "");
                    values_map["compatible_printers"] = printer;
                }
                if (ret == 0) {
                    PreUpdateProfileRetInfo retInfo;
                    UploadFileInfo          fileInfo;
                    fileInfo.file = preset.file;
                    fileInfo.name = preset.name;
                    fileInfo.type = preset.get_cloud_type_string(preset.type);

                    ret = m_commWithCXCloud.preUpdateProfile_create(fileInfo, retInfo);
                    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncFilamentToCXCloud preUpdateProfile_create ret=" << ret;
                    if (ret == 0) {
                        // preset_bundle->printers.set_sync_info_and_save(preset.name, setting_id, updated_info, update_time);
                    }
                    if (!retInfo.settingId.empty()) {
                        auto update_time_str = values_map[BBL_JSON_KEY_UPDATE_TIME];
                        if (!update_time_str.empty())
                            retInfo.updateTime = std::atoll(update_time_str.c_str());
                        if (retInfo.updateTime == 0) {
                            retInfo.updateTime = Slic3r::Utils::get_current_time_utc();
                        }
                        preset_bundle->filaments.set_sync_info_and_save(preset.name, retInfo.settingId, retInfo.updatedInfo,
                                                                        retInfo.updateTime, wxGetApp().get_user().userId);
                        values_map["updated_time"] = std::to_string(retInfo.updateTime);
                        values_map.emplace("name", preset.name);
                        values_map.emplace("setting_id", retInfo.settingId);
                        CXCloudDataCenter::getInstance().setUserCloudPresets(preset.name, retInfo.settingId, values_map);
                    }
                }
                nRet = 1;
            } else if (preset.sync_info.compare("update") == 0) {
                if (preset.setting_id.empty()) {
                    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncFilamentToCXCloud sync_info=update, setting_id is empty,name=" << preset.name;
                    continue;
                }

                std::map<std::string, std::string> values_map;
                int                                ret  = preset_bundle->get_differed_values_to_update(preset, values_map);
                auto                               iter = values_map.find("compatible_printers");
                if (iter != values_map.end()) {
                    std::string printer = iter->second;
                    boost::replace_all(printer, "\"", "");
                    values_map["compatible_printers"] = printer;
                }
                if (ret == 0) {
                    PreUpdateProfileRetInfo retInfo;
                    UploadFileInfo          fileInfo;
                    fileInfo.file      = preset.file;
                    fileInfo.name      = preset.name;
                    fileInfo.settingId = preset.setting_id;

                    ret = m_commWithCXCloud.preUpdateProfile_update(fileInfo, retInfo);
                    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncFilamentToCXCloud preUpdateProfile_update ret=" << ret;
                    if (ret == 0 || ret == 1 || ret == 21) {
                        if (ret == 0) {
                            retInfo.updateTime = Slic3r::Utils::get_current_time_utc();
                        } else {
                            auto update_time_str = values_map[BBL_JSON_KEY_UPDATE_TIME];
                            if (!update_time_str.empty())
                                retInfo.updateTime = std::atoll(update_time_str.c_str());
                        }
                        preset_bundle->filaments.set_sync_info_and_save(preset.name, preset.setting_id, retInfo.updatedInfo,
                                                                        retInfo.updateTime, wxGetApp().get_user().userId);
                        values_map["updated_time"] = std::to_string(retInfo.updateTime);
                        values_map.emplace("name", preset.name);
                        CXCloudDataCenter::getInstance().updateUserCloudPresets(preset.name, preset.setting_id, values_map);
                        nRet = 1;
                    } else if (ret == 517) { //  数据不存在，说明可能被删除了，则进行创建
                        fileInfo.type = preset.get_cloud_type_string(preset.type);
                        fileInfo.settingId = "";

                        ret = preUpdateProfile_create(fileInfo, preset_bundle, preset, retInfo, values_map);
                        nRet = 1;
                    }
                }
            } else {
                BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncFilamentToCXCloud err sync_info=" << preset.sync_info 
                    << ",setting_id=" << preset.setting_id << ",name=" << preset.name;
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncFilamentToCXCloud end";
    }

    return nRet;
}

int SyncUserPresets::doCheckNeedSyncProcessToCXCloud()
{
    int                 nRet = 0;
    std::vector<Preset> presets_to_sync;
    PresetBundle*       preset_bundle = GUI::wxGetApp().preset_bundle;

    //  获取需要添加和更新的preset
    int sync_count = 0;
    sync_count     = preset_bundle->prints.get_user_presets(preset_bundle, presets_to_sync);
    if (sync_count > 0) {
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncProcessToCXCloud start...";
        for (Preset& preset : presets_to_sync) {
            auto setting_id = preset.setting_id;

            if ((preset.setting_id.empty() && preset.sync_info.empty()) || 
                (preset.setting_id.empty() && preset.sync_info.compare("update") == 0) ||
                preset.sync_info.compare("create") == 0) {
                std::map<std::string, std::string> values_map;
                int                                ret = preset_bundle->get_differed_values_to_update(preset, values_map);
                auto                               iter = values_map.find("compatible_printers");
                if (iter != values_map.end()) {
                    std::string printer = iter->second;
                    boost::replace_all(printer, "\"", "");
                    values_map["compatible_printers"] = printer;
                }
                if (ret == 0) {
                    PreUpdateProfileRetInfo retInfo;
                    UploadFileInfo          fileInfo;
                    fileInfo.file = preset.file;
                    fileInfo.name = preset.name;
                    fileInfo.type = preset.get_cloud_type_string(preset.type);

                    ret = m_commWithCXCloud.preUpdateProfile_create(fileInfo, retInfo);
                    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncProcessToCXCloud preUpdateProfile_create ret=" << ret;
                    if (ret == 0) {
                        // preset_bundle->printers.set_sync_info_and_save(preset.name, setting_id, updated_info, update_time);
                    }
                    if (!retInfo.settingId.empty()) {
                        auto update_time_str = values_map[BBL_JSON_KEY_UPDATE_TIME];
                        if (!update_time_str.empty())
                            retInfo.updateTime = std::atoll(update_time_str.c_str());
                        if (retInfo.updateTime == 0) {
                            retInfo.updateTime = Slic3r::Utils::get_current_time_utc();
                        }
                        preset_bundle->prints.set_sync_info_and_save(preset.name, retInfo.settingId, retInfo.updatedInfo,
                                                                     retInfo.updateTime, wxGetApp().get_user().userId);
                        values_map["updated_time"] = std::to_string(retInfo.updateTime);
                        values_map.emplace("name", preset.name);
                        values_map.emplace("setting_id", retInfo.settingId);
                        CXCloudDataCenter::getInstance().setUserCloudPresets(preset.name, retInfo.settingId, values_map);
                    }
                }
                nRet = 1;
            } else if (preset.sync_info.compare("update") == 0) {
                if (preset.setting_id.empty()) {
                    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncProcessToCXCloud sync_info=update, setting_id is empty,name=" << preset.name;
                    continue;
                }

                std::map<std::string, std::string> values_map;
                int                                ret  = preset_bundle->get_differed_values_to_update(preset, values_map);
                auto                               iter = values_map.find("compatible_printers");
                if (iter != values_map.end()) {
                    std::string printer = iter->second;
                    boost::replace_all(printer, "\"", "");
                    values_map["compatible_printers"] = printer;
                }
                if (ret == 0) {
                    PreUpdateProfileRetInfo retInfo;
                    UploadFileInfo          fileInfo;
                    fileInfo.file      = preset.file;
                    fileInfo.name      = preset.name;
                    fileInfo.settingId = preset.setting_id;

                    ret = m_commWithCXCloud.preUpdateProfile_update(fileInfo, retInfo);
                    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncProcessToCXCloud preUpdateProfile_update ret=" << ret;
                    if (ret == 0 || ret == 1 || ret == 21) {
                        if (ret == 0) {
                            retInfo.updateTime = Slic3r::Utils::get_current_time_utc();
                        } else {
                            auto update_time_str = values_map[BBL_JSON_KEY_UPDATE_TIME];
                            if (!update_time_str.empty())
                                retInfo.updateTime = std::atoll(update_time_str.c_str());
                        }
                        preset_bundle->prints.set_sync_info_and_save(preset.name, preset.setting_id, retInfo.updatedInfo,
                                                                     retInfo.updateTime, wxGetApp().get_user().userId);
                        values_map["updated_time"] = std::to_string(retInfo.updateTime);
                        values_map.emplace("name", preset.name);
                        CXCloudDataCenter::getInstance().updateUserCloudPresets(preset.name, preset.setting_id, values_map);
                        nRet = 1;
                    } else if (ret == 517) { //  数据不存在，说明可能被删除了，则进行创建
                        fileInfo.type = preset.get_cloud_type_string(preset.type);
                        fileInfo.settingId = "";

                        ret = preUpdateProfile_create(fileInfo, preset_bundle, preset, retInfo, values_map);
                        nRet = 1;
                    }
                }
            } else {
                BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncProcessToCXCloud err sync_info=" << preset.sync_info
                                           << ",setting_id=" << preset.setting_id << ",name=" << preset.name;
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncProcessToCXCloud end";
    }

    return nRet;
}

int SyncUserPresets::preUpdateProfile_create(const UploadFileInfo& fileInfo,
                                             PresetBundle* preset_bundle,
                                             Preset& preset,
                                             PreUpdateProfileRetInfo& retInfo,
                                             std::map<std::string, std::string>& values_map)
{
    int nRet = 0;
    nRet     = m_commWithCXCloud.preUpdateProfile_create(fileInfo, retInfo);
    if (fileInfo.type == "printer")
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncPrinterToCXCloud preUpdateProfile_create ret=" << nRet;
    else if (fileInfo.type == "materia")
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncFilamentToCXCloud preUpdateProfile_create ret=" << nRet;
    else if (fileInfo.type == "process")
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncProcessToCXCloud preUpdateProfile_create ret=" << nRet;
    else
        BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets type=" << fileInfo.type.c_str() << " preUpdateProfile_create ret=" << nRet;
    if (nRet == 0) {
        // preset_bundle->printers.set_sync_info_and_save(preset.name, setting_id, updated_info, update_time);
    }
    if (!retInfo.settingId.empty()) {
        auto update_time_str = values_map[BBL_JSON_KEY_UPDATE_TIME];
        if (!update_time_str.empty())
            retInfo.updateTime = std::atoll(update_time_str.c_str());
        if (retInfo.updateTime == 0) {
            retInfo.updateTime = Slic3r::Utils::get_current_time_utc();
        }
        if (fileInfo.type == "printer") {
            preset_bundle->printers.set_sync_info_and_save(preset.name, retInfo.settingId, retInfo.updatedInfo, retInfo.updateTime,
                                                         wxGetApp().get_user().userId);
        } else if (fileInfo.type == "materia") {
            preset_bundle->filaments.set_sync_info_and_save(preset.name, retInfo.settingId, retInfo.updatedInfo, retInfo.updateTime,
                                                         wxGetApp().get_user().userId);
        } else if (fileInfo.type == "process") {
            preset_bundle->prints.set_sync_info_and_save(preset.name, retInfo.settingId, retInfo.updatedInfo, retInfo.updateTime,
                                                         wxGetApp().get_user().userId);
        }
        values_map["updated_time"] = std::to_string(retInfo.updateTime);
        values_map.emplace("name", preset.name);
        values_map.emplace("setting_id", retInfo.settingId);
        CXCloudDataCenter::getInstance().setUserCloudPresets(preset.name, retInfo.settingId, values_map);
    }
    return nRet;
}

int SyncUserPresets::doCheckNeedDeleteFromCXCloud() 
{ 
    int nRet = 0;
    //  获取需要删除的preset
    std::vector<string> delete_cache_presets = GUI::wxGetApp().get_delete_cache_presets_lock();
    for (auto it = delete_cache_presets.begin(); it != delete_cache_presets.end(); it++) {
        if ((*it).empty())
            continue;
        if (m_commWithCXCloud.deleteProfile(*it) == 0) {
            GUI::wxGetApp().preset_deleted_from_cloud(*it);
            CXCloudDataCenter::getInstance().deleteUserPresetBySettingID(*it);
            nRet = 1;
        } else {
            BOOST_LOG_TRIVIAL(error) << "SyncUserPresets doCheckNeedDeleteFromCXCloud delete fail. settingID=[" << *it << "]";
        }
    }

    return nRet; // 0:没有删除创想云数据, 1:删除了创想云数据
}

int SyncUserPresets::doCheckNeedSyncConfigToCXCloud()
{
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncConfigToCXCloud start...";
    int nRet = 0;

    if (CXCloudDataCenter::getInstance().getDownloadConfigToLocalState() == ENDownloadConfigState::ENDCS_CXCLOUD_NO_CONFIG) { //  ������û�������ļ�
        PreUpdateProfileRetInfo retInfo;
        UploadFileInfo          fileInfo;
        std::string             outFileName = "";
        if (getSyncDataToFile(outFileName) != 0) {
            BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncConfigToCXCloud end";
            return -1;
        }
        fileInfo.file = outFileName;
        fileInfo.name = fs::path(fileInfo.file).filename().string();
        fileInfo.type = "sync_data";

        int ret = m_commWithCXCloud.preUpdateProfile_create(fileInfo, retInfo);
        if (ret == 0) {
            CXCloudDataCenter::getInstance().setConfigFileRetInfo(retInfo);
            CXCloudDataCenter::getInstance().setDownloadConfigToLocalState(ENDownloadConfigState::ENDCS_DOWNLOAD_SUCCESS);
        } else {
            CXCloudDataCenter::getInstance().setDownloadConfigToLocalState(ENDownloadConfigState::ENDCS_DOWNLOAD_FAIL);
        }
    } else if (CXCloudDataCenter::getInstance().getDownloadConfigToLocalState() == ENDownloadConfigState::ENDCS_DOWNLOAD_SUCCESS) {
        PreUpdateProfileRetInfo retInfo;
        UploadFileInfo          fileInfo;
        std::string             outFileName = "";
        if (getSyncDataToFile(outFileName) != 0) {
            BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncConfigToCXCloud end";
            return -1;
        }
        fileInfo.file      = outFileName;
        fileInfo.name      = fs::path(fileInfo.file).filename().string();
        fileInfo.settingId = CXCloudDataCenter::getInstance().getConfigFileRetInfo().settingId;

        int ret = m_commWithCXCloud.preUpdateProfile_update(fileInfo, retInfo);
        if (ret == 0) {
            BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncConfigToCXCloud updateConfigFile success";
        } else {
            BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncConfigToCXCloud updateConfigFile fail";
        }
    } else if (CXCloudDataCenter::getInstance().getDownloadConfigToLocalState() == ENDownloadConfigState::ENDCS_DOWNLOAD_FAIL) {
        BOOST_LOG_TRIVIAL(error) << "SyncUserPresets doCheckNeedSyncConfigToCXCloud getDownloadConfigToLocalState = ENDCS_DOWNLOAD_FAIL";
    }
    BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets doCheckNeedSyncConfigToCXCloud end";
    return nRet;
}

int SyncUserPresets::getSyncDataToFile(std::string& outJsonFile)
{
    int         nRet = 0;
    std::string configFileData;
    try {
        fs::path path = fs::path(Slic3r::data_dir()).append("Creality.conf");
        if (fs::exists(path)) {
            boost::nowide::ifstream ifs(path.string());
            if (!ifs.is_open()) {
                // 获取错误码
                std::error_code error_code = std::make_error_code(std::errc::no_such_file_or_directory);
                // setLastError(std::to_string(error_code.value()), error_code.message());
            } else {
                // file >> json;
                std::stringstream input_stream;
                input_stream << ifs.rdbuf();
                ifs.close();
                configFileData = input_stream.str();
            }
        }

        size_t      last_pos    = configFileData.find_last_of('}');
        std::string left_string = configFileData.substr(0, last_pos + 1);
        json        j           = json::parse(left_string);
        json        jsonOut           = json();
        jsonOut["type"]               = "";
        jsonOut["name"]               = "";
        jsonOut["version"]            = "";
        jsonOut["setting_id"]         = "";
        jsonOut["update_time"]        = "";
        jsonOut["current_machine"]    = "";
        jsonOut["filaments"]          = json::array();
        jsonOut["machines"]           = json::array();

        if (j.contains("filaments")) {
            jsonOut["filaments"] = j["filaments"]; 
        }
        if (j.contains("orca_presets")) {
            jsonOut["machines"] = j["orca_presets"];
        }
        if (j.contains("presets") && j["presets"].contains("machine")) {
            jsonOut["current_machine"] = j["presets"]["machine"];
        }

        json jsonSyncData = CXCloudDataCenter::getInstance().getSyncData();
        jsonSyncData["update_time"] = "";

        if (jsonSyncData == jsonOut) {
            BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets getSyncDataToFile localSyncData equal cxcloudSyncData";
            return 1;
        }
        jsonOut["update_time"] = std::to_string(Slic3r::Utils::get_current_time_utc());

        fs::path tmpPath = fs::path(Slic3r::data_dir()).append(PRESET_USER_DIR).append("Temp");
        if (!fs::exists(tmpPath)) {
            fs::create_directory(tmpPath);
        }
        outJsonFile = tmpPath.append("sync_data.json").string();
        boost::nowide::ofstream c;
        c.open(outJsonFile, std::ios::out | std::ios::trunc);
        c << std::setw(4) << jsonOut << std::endl;
        c.close();

    } catch (...) {
        nRet = -1;

        BOOST_LOG_TRIVIAL(error) << "SyncUserPresets getSyncDataToFile catch";
    }
    return nRet;
}

int SyncUserPresets::copyOldPresetToBackup()
{
    int nRet = 0;
    try {
        const UserInfo& userInfo = wxGetApp().get_user();
        std::string     user     = userInfo.userId;
        fs::path        user_folder = fs::path(data_dir()).append(PRESET_USER_DIR).append(userInfo.userId);
        fs::path        backupFolder = fs::path(user_folder).append("backup");
        if (!fs::exists(backupFolder))
            fs::create_directory(backupFolder);
        std::vector vtFolder = {"filament", "local_device", "machine", "process", "sync_data"};
        for (auto folder : vtFolder) {
            if (!fs::exists(fs::path(user_folder).append(folder))) {
                continue;
            }

            fs::create_directories(fs::path(backupFolder).append(folder));
            for (const auto& entry : fs::directory_iterator(fs::path(user_folder).append(folder))) {
                if (!entry.is_directory()) {
                    fs::copy_file(entry.path(), fs::path(backupFolder).append(folder).append(entry.path().filename()),
                                  fs::copy_option::overwrite_if_exists);
                }
            }
            fs::remove_all(fs::path(user_folder).append(folder));
            fs::create_directory(fs::path(user_folder).append(folder));
        }
        /*
        if (fs::exists(fs::path(user_folder).append("filament"))) {
            fs::create_directories(fs::path(backupFolder).append("filament"));
            fs::copy_directory(fs::path(user_folder).append("filament"), fs::path(backupFolder).append("filament"));
            fs::remove(fs::path(user_folder).append("filament"));
        }
        if (fs::exists(fs::path(user_folder).append("local_device"))) {
            fs::create_directories(fs::path(backupFolder).append("local_device"));
            fs::copy_directory(fs::path(user_folder).append("local_device"), fs::path(backupFolder).append("local_device"));
            fs::remove(fs::path(user_folder).append("local_device"));
        }
        if (fs::exists(fs::path(user_folder).append("machine"))) {
            fs::create_directories(fs::path(backupFolder).append("machine"));
            fs::copy_directory(fs::path(user_folder).append("machine"), fs::path(backupFolder).append("machine"));
            fs::remove(fs::path(user_folder).append("machine"));
        }
        if (fs::exists(fs::path(user_folder).append("process"))) {
            fs::create_directories(fs::path(backupFolder).append("process"));
            fs::copy_directory(fs::path(user_folder).append("process"), fs::path(backupFolder).append("process"));
            fs::remove(fs::path(user_folder).append("process"));
        }
        if (fs::exists(fs::path(user_folder).append("sync_data"))) {
            fs::create_directories(fs::path(backupFolder).append("sync_data"));
            fs::copy_directory(fs::path(user_folder).append("sync_data"), fs::path(backupFolder).append("sync_data"));
            fs::remove(fs::path(user_folder).append("sync_data"));
        }*/

    } catch (const fs::filesystem_error& e) {
        BOOST_LOG_TRIVIAL(error) << "SyncUserPresets copyOldPresetToBackup catch." << e.what();
    }
    return nRet;
}

int SyncUserPresets::getLocalUserPresets(std::vector<LocalUserPreset>& vtLocalUserPreset)
{
    const UserInfo& userInfo    = wxGetApp().get_user();
    std::string     user        = userInfo.userId;
    fs::path        user_folder = fs::path(data_dir()).append(PRESET_USER_DIR).append(userInfo.userId);
    std::vector     vtFolder    = {"machine", "filament", "process"};
    for (auto folder : vtFolder) {
        for (const auto& entry : fs::directory_iterator(fs::path(user_folder).append(folder))) {
            if (entry.is_directory()) {
                for (const auto& baseEntry : fs::directory_iterator(fs::path(user_folder).append(folder).append(entry.path().filename()))) {
                    if (!baseEntry.is_directory()) {
                        LocalUserPreset localUserPreset;
                        localUserPreset.file = fs::path(baseEntry.path()).make_preferred().string();
                        localUserPreset.name = baseEntry.path().filename().string();
                        localUserPreset.type = folder;
                        if (entry.path().extension().string() == ".json") {
                            localUserPreset.isJson   = true;
                            localUserPreset.infoFile = fs::path(entry.path().parent_path()).append(localUserPreset.name + ".info").string();
                            if (!fs::exists(localUserPreset.infoFile)) {
                                localUserPreset.infoFile = "";
                            }
                            vtLocalUserPreset.push_back(localUserPreset);
                        } else {
                            localUserPreset.isJson = false;
                        }
                    }
                }
            } else {
                LocalUserPreset localUserPreset;
                localUserPreset.file = fs::path(entry.path()).make_preferred().string();
                localUserPreset.name = entry.path().filename().stem().string();
                localUserPreset.type = folder;
                if (entry.path().extension().string() == ".json") {
                    localUserPreset.isJson   = true;
                    localUserPreset.infoFile = fs::path(entry.path().parent_path()).append(localUserPreset.name + ".info").string();
                    if (!fs::exists(localUserPreset.infoFile)) {
                        localUserPreset.infoFile = "";
                    }
                    vtLocalUserPreset.push_back(localUserPreset);
                } else {
                    localUserPreset.isJson = false;
                }
            }
        }
        if (!fs::exists(fs::path(user_folder).append(folder).append("base"))) {
            continue;
        }
        for (const auto& entry : fs::directory_iterator(fs::path(user_folder).append(folder).append("base"))) {
            if (entry.is_directory()) {
                for (const auto& baseEntry :
                        fs::directory_iterator(fs::path(user_folder).append(folder).append(entry.path().filename()))) {
                    if (!baseEntry.is_directory()) {
                        LocalUserPreset localUserPreset;
                        localUserPreset.file = fs::path(baseEntry.path()).make_preferred().string();
                        localUserPreset.name = baseEntry.path().filename().string();
                        localUserPreset.type = folder;
                        if (entry.path().extension().string() == ".json") {
                            localUserPreset.isJson = true;
                            localUserPreset.infoFile =
                                fs::path(entry.path().parent_path()).append(localUserPreset.name + ".info").string();
                            if (!fs::exists(localUserPreset.infoFile)) {
                                localUserPreset.infoFile = "";
                            }
                            vtLocalUserPreset.push_back(localUserPreset);
                        } else {
                            localUserPreset.isJson = false;
                        }
                    }
                }
            } else {
                LocalUserPreset localUserPreset;
                localUserPreset.file = fs::path(entry.path()).make_preferred().string();
                localUserPreset.name = entry.path().filename().stem().string();
                localUserPreset.type = folder;
                if (entry.path().extension().string() == ".json") {
                    localUserPreset.isJson   = true;
                    localUserPreset.infoFile = fs::path(entry.path().parent_path()).append(localUserPreset.name + ".info").string();
                    if (!fs::exists(localUserPreset.infoFile)) {
                        localUserPreset.infoFile = "";
                    }
                    vtLocalUserPreset.push_back(localUserPreset);
                } else {
                    localUserPreset.isJson = false;
                }
            }
        }
    }
    return 0;
}

int SyncUserPresets::delLocalUserPresetsInUiThread(const SyncToLocalRetInfo& syncToLocalRetInfo)
{
    std::vector<LocalUserPreset> vtNeedDel;
    for (auto item : syncToLocalRetInfo.vtLocalUserPreset) {
        if (item.isJson && item.needDel) {
            if (item.type == "machine" && syncToLocalRetInfo.bPrinterAllOk) {
                vtNeedDel.push_back(item);
            } else if (item.type == "filament" && syncToLocalRetInfo.bFilamentAllOk) {
                vtNeedDel.push_back(item);
            } else if (item.type == "process" && syncToLocalRetInfo.bProcessAllOk) {
                vtNeedDel.push_back(item);
            }
        }
    }

    for (auto item : vtNeedDel) {
        // continue;
        if (item.isJson && item.needDel) {
            PresetCollection* collection = nullptr;
            wxString          strTip     = "";
            if (item.type == "machine" && syncToLocalRetInfo.bPrinterAllOk) {
                collection = &wxGetApp().preset_bundle->printers;
                strTip     = wxString::Format(
                    _L("The cloud printer preset [%s] has been deleted. \nDo you want to delete the local printer preset?"),
                    from_u8(item.name));
            } else if (item.type == "filament" && syncToLocalRetInfo.bFilamentAllOk) {
                collection = &wxGetApp().preset_bundle->filaments;
                strTip     = wxString::Format(
                    _L("The cloud filament preset [%s] has been deleted. \nDo you want to delete the local filament preset?"),
                    from_u8(item.name));
            } else if (item.type == "process" && syncToLocalRetInfo.bProcessAllOk) {
                collection = &wxGetApp().preset_bundle->prints;
                strTip     = wxString::Format(
                    _L("The cloud process preset [%s] has been deleted. \nDo you want to delete the local process preset?"),
                    from_u8(item.name));
            }
            if (collection != nullptr) {
                Preset* preset = collection->find_preset(item.name);
                if (preset != nullptr) {
                    if ((preset->setting_id.empty() && preset->sync_info.empty()) || preset->sync_info.compare("create") == 0) {
                        BOOST_LOG_TRIVIAL(warning)
                            << "SyncUserPresets delLocalUserPresetsInUiThread preset[" << item.name << "] need update to cxcloud";
                        continue;
                    }
                }

            }
            {
                fs::path    path(item.file);
                std::string parentPathName = path.parent_path().filename().string();
                if (path.parent_path().filename() != "base") {
                    auto iter = std::find_if(syncToLocalRetInfo.vtLocalUserPreset.begin(), syncToLocalRetInfo.vtLocalUserPreset.end(),
                                        [path](const LocalUserPreset& preset) {
                                            fs::path pathbase = fs::path(path.parent_path()).append("base").append(path.filename());
                                            return preset.file == pathbase;
                                        });
                    if (iter != syncToLocalRetInfo.vtLocalUserPreset.end()) {
                        if (fs::exists(item.file)) {
                            fs::remove(item.file);
                            fs::remove(item.infoFile);

                            BOOST_LOG_TRIVIAL(warning) << "SyncUserPresets delLocalUserPresetsInUiThread preset[" << item.name << "] has move to base";
                        }
                        continue;
                    }
                }
            }

            if (collection != nullptr) {
                Preset*  preset = collection->find_preset(fs::path(item.file).filename().string());
                MessageDialog msgDlg(nullptr, strTip, wxEmptyString, wxICON_QUESTION | wxYES_NO);
                int           res = msgDlg.ShowModal();
                if (res == wxID_YES) {
                    if (preset != nullptr)
                        collection->delete_preset(item.name);
                    else {
                        if (fs::exists(item.file)) {
                            fs::remove(item.file);
                            fs::remove(item.infoFile);
                        }
                    }
                }
            }
        }
    }

    return 0;
}

}} // namespace Slic3r::GUI
