#ifndef slic3r_CommunicateWithCXCloud_hpp_
#define slic3r_CommunicateWithCXCloud_hpp_

#include <string>
#include <vector>
#include <map>
#include <set>

#include "libslic3r/PresetBundle.hpp"

namespace Slic3r {
namespace GUI {

    struct UserProfileListItem
    {
        std::string id = "";
        long long                lastModifyTime = 0;
        std::vector<std::string> nozzleDiameter;
        std::string printerIntName = "";
        std::string type           = "";
        std::string version        = "";
        std::string zipUrl         = "";
    };

    struct PreUpdateProfileRetInfo
    {
        std::string name = "";
        std::string settingId = "";
        std::string updatedInfo = "";
        long long   updateTime = 0;
    };

    struct UploadFileInfo
    {
        std::string file = "";
        std::string name = "";
        std::string type     = "";
        std::string settingId = "";
    };

    struct CXCloudLoginInfo
    {
        std::string token = "";
        std::string userId = "";
        bool tokenValid = false;
    };

    struct LocalUserPreset
    {
        std::string file     = "";
        std::string infoFile = "";
        std::string name     = "";
        std::string type     = "";
        bool        isJson   = false;
        bool        needDel  = true;
    };
    struct SyncToLocalRetInfo
    {
        std::vector<LocalUserPreset> vtLocalUserPreset;
        bool                         bPrinterAllOk  = true;
        bool                         bFilamentAllOk = true;
        bool                         bProcessAllOk  = true;
    };


    enum class ENDownloadConfigState {
        ENDCS_NOT_DOWNLOAD,         // Not started
        ENDCS_DOWNLOAD_SUCCESS,     // Download succeeded
        ENDCS_DOWNLOAD_FAIL,        // Download failed
        ENDCS_CXCLOUD_NO_CONFIG     // No config file in CXCloud
    };

    enum class ENDownloadPresetState {
        ENDPS_NOT_DOWNLOAD,         // Not started
        ENDPS_DOWNLOADING,          // Downloading
        ENDPS_DOWNLOAD_SUCCESS,     // Download succeeded
        ENDPS_DOWNLOAD_FAILED,      // Download failed
    };

    class CXCloudDataCenter
    {
    public:
        static CXCloudDataCenter& getInstance();

        std::map<std::string, std::map<std::string, std::string>> getUserCloudPresets();
        bool setUserCloudPresets(const std::string& presetName, const std::string& settingID, const std::map<std::string, std::string>& mapPresetValue);
        void cleanUserCloudPresets();
        void updateUserCloudPresets(const std::string& presetName, const std::string& settingID, const std::map<std::string, std::string>& mapPresetValue);
        int  deleteUserPresetBySettingID(const std::string& settingID);

        void setDownloadConfigToLocalState(ENDownloadConfigState state);
        // -1: not started, 0: download succeeded, 1: download failed, 2: no config file
        ENDownloadConfigState getDownloadConfigToLocalState();
        // Whether updating the config file has timed out
        bool isUpdateConfigFileTimeout();
        void resetUpdateConfigFileTime();
        // Whether retrying after a network error has timed out
        bool isNetworkErrorRetryTimeout();
        void resetNetworkErrorRetryTime();
        bool isNetworkError();
        void setNetworkError(bool bError);
        void setConfigFileRetInfo(const PreUpdateProfileRetInfo& retInfo);
        const PreUpdateProfileRetInfo& getConfigFileRetInfo();
        void                           setSyncData(const json& j) { m_jsonCXCloudSyncData = j; }
        const json&                    getSyncData() { return m_jsonCXCloudSyncData; };
        void                           updateCXCloutLoginInfo(const std::string& userId, const std::string& token);
        void                           setTokenInvalid(bool bTokenValid = false);
        bool                           isTokenValid();
        bool getProcessPresetDirty(){ return m_bProcessPresetDirty; }
        void setProcessPresetDirty(bool bDirty) { m_bProcessPresetDirty = bDirty; }
        bool getFilamentPresetDirty() { return m_bFilamentPresetDirty; }
        void setFilamentPresetDirty(bool bDirty) { m_bFilamentPresetDirty = bDirty; }
        bool getFirstSelectProcessPreset() { return m_bFirstSelectProcessPreset; }
        void setFirstSelectProcessPreset(bool bFirst) { m_bFirstSelectProcessPreset = bFirst; }
        void setDownloadPresetState(ENDownloadPresetState state) { m_enDownloadPresetState.store(state); } 
        ENDownloadPresetState getDownloadPresetState() { return m_enDownloadPresetState.load(); }
        bool isDownloadPresetFinished();

    private:
        CXCloudDataCenter() = default;
        ~CXCloudDataCenter() = default;

    private:
        std::map<std::string, std::map<std::string, std::string>> m_mapUserCloudPresets;
        std::map<std::string, std::string>                        m_mapSettingID2PresetName;
        std::mutex                                                 m_mutexUserCloudPresets;
        ENDownloadConfigState                                     m_enDownloadConfigToLocalState = ENDownloadConfigState::ENDCS_NOT_DOWNLOAD;
        long long m_llUpdateConfigFileTimestamp = 0;
        long long m_llNetworkErrorRetryTimestamp = 0;
        PreUpdateProfileRetInfo                                   m_configFileRetInfo;
        json                                                      m_jsonCXCloudSyncData = json();
        CXCloudLoginInfo                                          m_cxCloudLoginInfo;
        std::mutex                                                m_cxCloudLoginInfoMutex;
        bool m_bProcessPresetDirty = false;
        bool m_bFilamentPresetDirty = false;
        bool m_bFirstSelectProcessPreset = true;
        bool m_bNetworkError = false;
        std::atomic<ENDownloadPresetState> m_enDownloadPresetState = ENDownloadPresetState::ENDPS_NOT_DOWNLOAD;
    };

    class CLocalDataCenter
    {
    public:
        static CLocalDataCenter& getInstance();

        bool isPrinterPresetExisted(const std::string& presetName);
        bool isFilamentPresetExisted(const std::string& presetName);
        bool isProcessPresetExisted(const std::string& presetName);

    private:
        CLocalDataCenter()   = default;
        ~CLocalDataCenter() = default;
    };

    struct UserInfo;
    class CommunicateWithCXCloud
    {
    public:
        struct LastError
        {
            std::string code      = "";
            std::string message   = "";
            std::string requestId = "";
        };
        CommunicateWithCXCloud();
        ~CommunicateWithCXCloud();
        // User profile list
        int getUserProfileList(std::vector<UserProfileListItem>& vtUserProfileListItem);
        // Download user preset
        int downloadUserPreset(const UserProfileListItem& userProfileListItem, std::string& saveJsonFile);
        // Pre-upload user profile configuration
        int preUpdateProfile_create(const UploadFileInfo& fileInfo, PreUpdateProfileRetInfo& retInfo);
        int preUpdateProfile_update(const UploadFileInfo& fileInfo, PreUpdateProfileRetInfo& retInfo);
        // Delete user preset
        int deleteProfile(const std::string& ssDeleteSettingId);
        const LastError& getLastError() { return m_lastError; }

    private:
        void setLastError(const std::string& code, const std::string& msg);
        int  saveSyncDataToLocal(const UserInfo&            userInfo,
                                 const UserProfileListItem& userProfileListItem,
                                 const std::string&         body,
                                 std::string&               saveJsonFile);
        int  saveLocalDeviceToLocal(const UserInfo&            userInfo,
                                    const UserProfileListItem& userProfileListItem,
                                    const std::string&         body,
                                    std::string&               saveJsonFile);

    private:
        LastError m_lastError;
    };


    class CommunicateWithFrontPage
    {
    public:
        CommunicateWithFrontPage();
        ~CommunicateWithFrontPage();

        int getUserPresetParams(const std::map<std::string, std::map<std::string, std::string>>& mapUserCloudPresets, std::string& ssJsonData);

    private:
        int getPrinterPresetParams(const std::map<std::string, std::string>& mapPrinterPreset, std::string& ssJsonData);
        int getFilamentPresetParams(const std::map<std::string, std::string>& mapFilamentPreset, std::string& ssJsonData);
        int getProcessPresetParams(const std::map<std::string, std::string>& mapProcessPreset, std::string& ssJsonData);
        std::string getMapValue(const std::map<std::string, std::string>& mapObj, const std::string& key);

    private:
        std::set<std::string> m_setIgnoreField;
        size_t                                m_printerId = 0;
        size_t                                m_filamentId = 0;
        size_t                                m_processId = 0;
        std::unordered_map<std::string, json> m_mapPrinterData;
        std::map<std::pair<std::string, std::string>, std::pair<std::string, std::string>> m_mapPrinterName2PrinterModel;
    };
}
}

#endif
