#ifndef slic3r_SyncUserPresets_hpp_
#define slic3r_SyncUserPresets_hpp_

#include <thread>
#include <mutex>
#include <list>
#include <atomic>
#include <condition_variable>

#include "CommunicateWithCXCloud.hpp"

namespace Slic3r {
namespace GUI {

class SyncUserPresets
{
public:
    enum class ENSyncCmd {ENSC_NULL, 
        ENSC_SYNC_TO_LOCAL, 
        ENSC_SYNC_TO_CXCLOUD_CREATE, 
        ENSC_SYNC_TO_FRONT_PAGE,         // Sync data to front page
        ENSC_SYNC_CONFIG_TO_CXCLOUD    // Sync configuration to CXCloud
    };

    enum class ENSyncThreadState {
        ENTS_IDEL_CHECK,        // Idle state, check if sync is needed
        ENTS_SYNC_TO_LOCAL,     // Sync to local
        ENTS_SYNC_TO_FRONT_PAGE // Sync to front page
    };

    static SyncUserPresets& getInstance();

    int startup();      // Start worker thread
    void shutdown();    // Stop worker thread
    void startSync();   // Start synchronization
    void stopSync();    // Stop synchronization
    void syncUserPresetsToLocal();
    void syncUserPresetsToCXCloud();
    void syncUserPresetsToFrontPage();
    void syncConfigToCXCloud();
    void setAppHasStartuped();
    void logout();

private:
    SyncUserPresets();
    ~SyncUserPresets();

protected:
    void onRun();
    void reloadPresetsInUiThread();
    int  doSyncToLocal(SyncToLocalRetInfo& syncToLocalRetInfo);
    int  doCheckNeedSyncToCXCloud();
    int  doCheckNeedSyncPrinterToCXCloud();
    int  doCheckNeedSyncFilamentToCXCloud();
    int  doCheckNeedSyncProcessToCXCloud();
    int  preUpdateProfile_create(const UploadFileInfo&       fileInfo,
                                 PresetBundle* preset_bundle,
                                 Preset& preset,
                                 PreUpdateProfileRetInfo& retInfo,
                                 std::map<std::string, std::string>& values_map);
    int  doCheckNeedDeleteFromCXCloud();
    // Check whether Creality.conf needs to be synced to CXCloud
    int  doCheckNeedSyncConfigToCXCloud();
    int  getSyncDataToFile(std::string& outJsonFile);
    int  copyOldPresetToBackup();
    int  getLocalUserPresets(std::vector<LocalUserPreset>& vtLocalUserPreset);
    int  delLocalUserPresetsInUiThread(const SyncToLocalRetInfo& syncToLocalRetInfo);

protected:
    // Invalidates UI callbacks queued by a previous login session.
    std::atomic<unsigned long long> m_accountGeneration{0};
    std::thread m_thread;
    std::atomic_bool     m_bRunning = false;
    std::atomic_bool     m_bSync    = false;
    std::atomic_bool         m_bStoped  = false;
    std::mutex               m_mutexQuit;
    std::condition_variable  m_cvQuit;
    std::list<ENSyncCmd> m_lstSyncCmd;
    std::mutex           m_mutexLstSyncCmd;
    CommunicateWithCXCloud m_commWithCXCloud;
    CommunicateWithFrontPage m_commWithFrontPage;

    ENSyncThreadState m_syncThreadState = ENSyncThreadState::ENTS_IDEL_CHECK;
    std::atomic_bool  m_bAppHasStartuped = false;
    bool m_bHasSyncToLocal  = false;
    bool m_bTokenInvalidHasTip = false;
};

}
}

#endif
