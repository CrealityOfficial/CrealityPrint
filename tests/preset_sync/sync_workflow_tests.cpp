// Regression harness: real production function bodies, fake network/UI/clock dependencies.

#define NOMINMAX
#include <atomic>
#include <boost/algorithm/string.hpp>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <curl/curl.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs = std::filesystem;
using std::string;
using json = nlohmann::json;
using Values = std::map<std::string, std::string>;
long long now_ms = 1800000000000LL;
std::ostringstream logs;
#define BOOST_LOG_TRIVIAL(level) logs
const std::string BBL_JSON_KEY_UPDATE_TIME = "updated_time";
namespace Slic3r {
namespace Utils {
long long get_current_time_utc() { return now_ms / 1000; }
} // namespace Utils
} // namespace Slic3r
namespace boost {
namespace chrono {
using milliseconds = std::chrono::milliseconds;
}
namespace this_thread {
void sleep_for(chrono::milliseconds v) { now_ms += v.count(); }
} // namespace this_thread
namespace uuids {
struct uuid {};
struct random_generator {
    uuid operator()() { return {}; }
};
std::string to_string(uuid) { return "fake-id"; }
} // namespace uuids
} // namespace boost
struct Preset {
    std::string name, setting_id, sync_info, base_id, parent, file, user_id;
    long long updated_time = 100;
    int type = 0;
    bool user = true;
    bool is_user() const { return user; }
    std::string inherits() const { return parent; }
    std::string get_cloud_type_string(int) const { return "printer"; }
    void save_info() {}
    void save(void *) {}
};
struct PresetBundle;
struct PresetCollection {
    std::vector<Preset> m_presets;
    void lock() {}
    void unlock() {}
    Preset *find_preset(const std::string &name, bool, bool) {
        for (auto &p : m_presets)
            if (p.name == name)
                return &p;
        return nullptr;
    }
    Preset *get_preset_base(Preset &) { return nullptr; }
    int get_user_presets(PresetBundle *, std::vector<Preset> &);
    void set_sync_info_and_save(std::string, std::string, std::string, long long, std::string = "");
};
std::function<void()> ui_hook;
struct PresetBundle {
    PresetCollection printers, filaments, prints;
    int get_differed_values_to_update(Preset &p, Values &out) {
        if (ui_hook) {
            auto cb = std::move(ui_hook);
            ui_hook = nullptr;
            cb();
        }
        out["updated_time"] = std::to_string(p.updated_time);
        return 0;
    }
};
struct User {
    std::string userId = "test-user";
};
struct App {
    PresetBundle *preset_bundle;
    User get_user() { return {}; }
    Values get_extra_header() { return {}; }
    std::vector<std::string> deletions;
    std::vector<std::string> get_delete_cache_presets_lock() { return deletions; }
    void preset_deleted_from_cloud(const std::string &id) {
        deletions.erase(std::remove(deletions.begin(), deletions.end(), id), deletions.end());
    }
};
App app;
App &wxGetApp() { return app; }
namespace GUI {
App &wxGetApp() { return app; }
} // namespace GUI
enum class ENDownloadPresetState {
    ENDPS_NOT_DOWNLOAD,
    ENDPS_DOWNLOADING,
    ENDPS_DOWNLOAD_SUCCESS,
    ENDPS_DOWNLOAD_FAILED
};
enum class ENDownloadConfigState {
    ENDCS_NOT_DOWNLOAD,
    ENDCS_DOWNLOAD_SUCCESS,
    ENDCS_DOWNLOAD_FAIL,
    ENDCS_CXCLOUD_NO_CONFIG
};
struct PreUpdateProfileRetInfo {
    std::string name, settingId, updatedInfo;
    long long updateTime = 0;
};
struct CXCloudDataCenter {
    bool m_bNetworkError = false;
    long long m_networkErrorRetryDelaySeconds = 0, m_networkErrorRetryBackoffSeconds = 0;
    ENDownloadPresetState state = ENDownloadPresetState::ENDPS_DOWNLOAD_SUCCESS;
    ENDownloadConfigState config_state = ENDownloadConfigState::ENDCS_CXCLOUD_NO_CONFIG;
    PreUpdateProfileRetInfo config_info;
    bool token_valid = true;
    bool isTokenValid() { return token_valid; }
    ENDownloadPresetState getDownloadPresetState() { return state; }
    ENDownloadConfigState getDownloadConfigToLocalState() { return config_state; }
    void setDownloadConfigToLocalState(ENDownloadConfigState v) { config_state = v; }
    PreUpdateProfileRetInfo getConfigFileRetInfo() { return config_info; }
    void setConfigFileRetInfo(const PreUpdateProfileRetInfo &v) { config_info = v; }
    void deleteUserPresetBySettingID(const std::string &) {}
    long long m_llNetworkErrorRetryTimestamp = 0;
    static CXCloudDataCenter &getInstance() {
        static CXCloudDataCenter s;
        return s;
    }
    bool isNetworkError() { return m_bNetworkError; }
    void setNetworkError(bool);
    void setTokenInvalid() { token_valid = false; }
    void resetNetworkErrorRetryTime(long long = 0);
    bool isNetworkErrorRetryTimeout();
    void setUserCloudPresets(std::string, std::string, Values) {}
    void updateUserCloudPresets(std::string, std::string, Values) {}
};
struct UploadFileInfo {
    std::string file, name, type, settingId;
};
struct CommunicateWithCXCloud {
    int preUpdateProfile_create(const UploadFileInfo &, PreUpdateProfileRetInfo &);
    int preUpdateProfile_update(const UploadFileInfo &, PreUpdateProfileRetInfo &);
    int deleteProfile(const std::string &);
};
struct SyncUserPresets {
    std::atomic_bool m_bRunning{true}, m_bSync{true};
    CommunicateWithCXCloud m_commWithCXCloud;
    bool canSyncToCXCloud() const;
    int doCheckNeedSyncConfigToCXCloud();
    int getSyncDataToFile(std::string &out) {
        out = "sync_data.json";
        return 0;
    }
    int doCheckNeedSyncPrinterToCXCloud();
    int doCheckNeedSyncToCXCloud();
    int doCheckNeedSyncFilamentToCXCloud();
    int doCheckNeedSyncProcessToCXCloud();
    int doCheckNeedDeleteFromCXCloud();
    void syncUserPresetsToFrontPage() {}
    int preUpdateProfile_create(const UploadFileInfo &f, PresetBundle *, Preset &, PreUpdateProfileRetInfo &r,
                                Values &) {
        return m_commWithCXCloud.preUpdateProfile_create(f, r);
    }
};
std::string get_vertion_type() { return "test"; }
std::string get_cloud_api_url() { return "mock://cloud"; }
std::string get_file_md5(const std::string &f) { return "fake-md5-" + f; }
std::vector<int> replies;
std::vector<int> observed;
std::vector<std::string> urls;
std::vector<int> oss_replies;
size_t oss_count = 0;
std::string response_headers;
struct Http {
    struct Progress {};
    std::string url;
    std::function<void(std::string, unsigned)> complete;
    std::function<void(std::string)> headers;
    std::function<void(std::string, std::string, unsigned)> error;
    static void set_extra_headers(Values) {}
    static Http post(std::string u) {
        Http h;
        h.url = u;
        return h;
    }
    Http &header(std::string, std::string) { return *this; }
    Http &on_header_callback(std::function<void(std::string)> f) {
        headers = f;
        return *this;
    }
    Http &timeout_connect(int) { return *this; }
    Http &timeout_max(int) { return *this; }
    Http &set_post_body(std::string) { return *this; }
    Http &on_complete(std::function<void(std::string, unsigned)> f) {
        complete = f;
        return *this;
    }
    Http &on_error(std::function<void(std::string, std::string, unsigned)> f) {
        error = f;
        return *this;
    }
    template <class F> Http &on_progress(F) { return *this; }
    Http &form_add(std::string, std::string) { return *this; }
    Http &form_add_file(std::string, std::string, std::string) { return *this; }
    void perform_sync() {
        if (headers)
            headers(response_headers);
        if (url == "mock://oss") {
            int code = oss_count < oss_replies.size() ? oss_replies[oss_count] : 0;
            ++oss_count;
            if (code == 0)
                complete("{}", 200);
            else
                error("", "", code);
            return;
        }
        urls.push_back(url);
        const auto i = observed.size();
        const int code = i < replies.size() ? replies[i] : 0;
        observed.push_back(code);
        if (code == 429) {
            error("", "", 429);
            return;
        }
        if (code != 0) {
            complete(json({{"code", code}, {"msg", "mock rate limit"}}).dump(), 200);
            return;
        }
        if (url.find("deleteProfile") != std::string::npos) {
            complete("{\"code\":0}", 200);
            return;
        }
        json policy = {{"OSSAccessKeyId", "fake"}, {"Signature", "fake"}, {"Policy", "fake"}, {"Key", "fake"},
                       {"Callback", "fake"},       {"Host", "mock://oss"}};
        complete(
            json({{"code", 0},
                  {"result", {{"id", "new-" + std::to_string(i)}, {"lastModifyTime", 100}, {"uploadPolicy", policy}}}})
                .dump(),
            200);
    }
};

#include "sync_workflow_functions.inc"

void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
PresetBundle bundle;
SyncUserPresets sync_worker;
CXCloudDataCenter &cloud = CXCloudDataCenter::getInstance();

PresetCollection &collection(int type) {
    return type == 0 ? bundle.printers : type == 1 ? bundle.filaments : bundle.prints;
}
void reset(int count = 6, int type = 0, bool synced = false) {
    bundle.printers.m_presets.clear();
    bundle.filaments.m_presets.clear();
    bundle.prints.m_presets.clear();
    for (int i = 0; i < count; ++i) {
        Preset p;
        p.name = "preset-" + std::to_string(i);
        p.file = p.name + ".json";
        p.type = type;
        if (synced)
            p.setting_id = "cloud-" + std::to_string(i);
        collection(type).m_presets.push_back(p);
    }
    app.preset_bundle = &bundle;
    app.deletions.clear();
    cloud.setNetworkError(false);
    cloud.state = ENDownloadPresetState::ENDPS_DOWNLOAD_SUCCESS;
    cloud.token_valid = true;
    cloud.config_state = ENDownloadConfigState::ENDCS_CXCLOUD_NO_CONFIG;
    cloud.config_info = {};
    sync_worker.m_bRunning = true;
    sync_worker.m_bSync = true;
    ui_hook = nullptr;
    observed.clear();
    replies.clear();
    urls.clear();
    oss_replies.clear();
    oss_count = 0;
    response_headers.clear();
}
void scan() { sync_worker.doCheckNeedSyncToCXCloud(); }

void merge_and_snapshot() {
    for (int type = 0; type < 3; ++type) {
        reset(6, type, true);
        scan();
        require(observed.empty(), "unchanged synced presets uploaded");
        reset(6, type);
        for (auto state : {ENDownloadPresetState::ENDPS_NOT_DOWNLOAD, ENDownloadPresetState::ENDPS_DOWNLOADING,
                           ENDownloadPresetState::ENDPS_DOWNLOAD_FAILED}) {
            cloud.state = state;
            scan();
            require(observed.empty(), "upload ran before successful reconciliation");
        }
        cloud.state = ENDownloadPresetState::ENDPS_DOWNLOAD_SUCCESS;
        ui_hook = [type] {
            for (auto &p : collection(type).m_presets) {
                p.setting_id = "cloud-reconciled";
                p.sync_info.clear();
            }
        };
        scan();
        require(observed.empty(), "stale snapshot created cloud duplicates");
        reset(6, type);
        scan();
        require(observed.size() == 6, "valid new presets failed to sync after reconciliation");
        scan();
        require(observed.size() == 6, "successfully synced presets uploaded again");
    }
    std::cout << "PASS reconciliation gate and stale snapshots for all three preset types\n";
}

void throttled_uploads() {
    for (int type = 0; type < 3; ++type) {
        for (bool update : {false, true}) {
            for (int code : {1088, 429}) {
                reset(6, type, update);
                if (update)
                    for (auto &p : collection(type).m_presets)
                        p.sync_info = "update";
                replies = {0, code};
                scan();
                require(observed.size() == 2, "batch continued after rate limiting");
                const auto &pending = collection(type).m_presets[1];
                require(update ? pending.sync_info == "update" : pending.setting_id.empty(),
                        "pending operation lost on throttle");
                scan();
                require(observed.size() == 2, "retried during cooldown");
                now_ms = cloud.m_llNetworkErrorRetryTimestamp * 1000 + 59000;
                scan();
                require(observed.size() == 2, "retried before 60 seconds");
                now_ms += 1000;
                scan();
                require(observed.size() == 7, "pending uploads did not recover after cooldown");
                require(!cloud.isNetworkError(), "successful recovery did not clear cooldown");
            }
        }
    }
    std::cout << "PASS 1088/429 stop create/update batches, retain pending changes and recover\n";
}

void retry_policy() {
    reset(1);
    replies.assign(10, 1088);
    for (long long expected : {60, 120, 240, 300, 300}) {
        scan();
        require(cloud.m_networkErrorRetryDelaySeconds == expected, "backoff did not increase/cap");
        now_ms = (cloud.m_llNetworkErrorRetryTimestamp + expected) * 1000;
    }
    replies.clear();
    scan();
    require(cloud.m_networkErrorRetryBackoffSeconds == 0, "success did not reset backoff");
    reset(1);
    replies = {429};
    response_headers = "HTTP/1.1 429 Too Many Requests\r\nContent-Type: application/json\r\nretry-after: 900\r\n\r\n";
    scan();
    require(cloud.m_networkErrorRetryDelaySeconds == 900, "accumulated Retry-After header ignored");
    now_ms = (cloud.m_llNetworkErrorRetryTimestamp + 300) * 1000;
    require(!sync_worker.canSyncToCXCloud(), "server retry delay shortened to fallback cap");
    require(preset_sync_retry_after("Retry-After: invalid\r\n") == 0, "invalid retry value accepted");
    require(preset_sync_retry_after("Retry-After: -5\r\n") == 0, "negative retry value accepted");
    now_ms = 1445412420000LL; // 2015-10-21 07:27 UTC
    require(preset_sync_retry_after("HTTP/1.1 429\r\nRetry-After: Wed, 21 Oct 2015 07:28:00 GMT\r\n") == 60,
            "HTTP-date Retry-After ignored");
    std::cout << "PASS exponential backoff, success reset, numeric/date Retry-After\n";
}

void deletes_and_config() {
    for (int code : {1088, 429}) {
        reset(0);
        app.deletions = {"a", "b", "c"};
        replies = {code};
        scan();
        require(observed.size() == 1 && app.deletions.size() == 3, "delete batch continued/lost pending IDs");
        scan();
        require(observed.size() == 1, "delete retried during cooldown");
        now_ms = (cloud.m_llNetworkErrorRetryTimestamp + 60) * 1000;
        scan();
        require(app.deletions.empty(), "delete did not recover");
        reset(0);
        replies = {code};
        sync_worker.doCheckNeedSyncConfigToCXCloud();
        require(cloud.config_state == ENDownloadConfigState::ENDCS_CXCLOUD_NO_CONFIG, "config create not retryable");
        sync_worker.doCheckNeedSyncConfigToCXCloud();
        require(observed.size() == 1, "config ignored cooldown");
        now_ms = (cloud.m_llNetworkErrorRetryTimestamp + 60) * 1000;
        sync_worker.doCheckNeedSyncConfigToCXCloud();
        require(cloud.config_state == ENDownloadConfigState::ENDCS_DOWNLOAD_SUCCESS, "config create did not recover");
    }
    std::cout << "PASS deletion and configuration share retry gate and retain pending work\n";
}

void oss_retry_identity() {
    reset(1);
    oss_replies = {429};
    scan();
    require(!bundle.printers.m_presets[0].setting_id.empty() && bundle.printers.m_presets[0].sync_info == "update",
            "allocated ID lost on OSS throttling");
    now_ms = (cloud.m_llNetworkErrorRetryTimestamp + 60) * 1000;
    scan();
    require(urls.size() == 2 && urls[1].find("editProfile") != std::string::npos, "OSS retry created a second record");
    reset(0);
    oss_replies = {429};
    sync_worker.doCheckNeedSyncConfigToCXCloud();
    now_ms = (cloud.m_llNetworkErrorRetryTimestamp + 60) * 1000;
    sync_worker.doCheckNeedSyncConfigToCXCloud();
    require(urls.size() == 2 && urls[1].find("editProfile") != std::string::npos,
            "config OSS retry created a second record");
    std::cout << "PASS OSS throttling retains allocated IDs and retries with editProfile\n";
}

void cancellation_and_cross_type_limit() {
    reset(1);
    sync_worker.m_bSync = false;
    scan();
    require(observed.empty(), "disabled sync sent a request");
    sync_worker.m_bSync = true;
    sync_worker.m_bRunning = false;
    scan();
    require(observed.empty(), "shutdown sent a request");
    sync_worker.m_bRunning = true;
    cloud.token_valid = false;
    scan();
    require(observed.empty(), "invalid login sent a request");

    reset(1);
    bundle.filaments.m_presets = bundle.printers.m_presets;
    bundle.prints.m_presets = bundle.printers.m_presets;
    replies = {1088};
    scan();
    require(observed.size() == 1, "rate limit did not stop other preset types");
    now_ms = (cloud.m_llNetworkErrorRetryTimestamp + 60) * 1000;
    scan();
    require(observed.size() == 4, "remaining preset types did not recover");
    std::cout << "PASS disabled/shutdown/login gates and cross-type rate limiting\n";
}

int main() {
    try {
        merge_and_snapshot();
        throttled_uploads();
        retry_policy();
        deletes_and_config();
        oss_retry_identity();
        cancellation_and_cross_type_limit();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
