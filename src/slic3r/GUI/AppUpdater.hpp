#ifndef slic3r_AppUpdater_hpp_
#define slic3r_AppUpdater_hpp_

#include <string>
#include <memory>
#include <atomic>
#include <wx/event.h>
#include <wx/string.h>
#include "libslic3r/Semver.hpp"
#include "slic3r/Utils/Http.hpp"

#include <vector>

namespace Slic3r {
namespace GUI {

// Define events
wxDECLARE_EVENT(EVT_APP_UPDATE_PROGRESS, wxCommandEvent);
wxDECLARE_EVENT(EVT_APP_UPDATE_COMPLETE, wxCommandEvent);
wxDECLARE_EVENT(EVT_APP_UPDATE_ERROR, wxCommandEvent);
wxDECLARE_EVENT(EVT_APP_UPDATE_CANCELED, wxCommandEvent);

struct PackageInfo {
    std::string name;
    std::string url;
    long long size;
    std::string sha1;
};

class AppUpdater {
public:
    static AppUpdater& getInstance();
    
    // Start download of the new version (hot update packages)
    void start_download(const std::vector<PackageInfo>& packages, const std::string& version, const std::string& base_package_name, const std::string& manual_url = std::string());
    
    // Start download for single installer (legacy/fallback)
    void start_download(const std::string& url, const std::string& version);

    bool prepare_cached_hot_update(const std::vector<PackageInfo>& packages, const std::string& version, const std::string& base_package_name, const std::string& manual_url = "");

    bool prepare_cached_installer(const std::string& url, const std::string& version);

    bool prepare_cached_hot_update_from_version(const std::string& version, const std::string& manual_url = "");

    // Hotfix update flag: when the target version shares the same major.minor.patch as the
    // installed version (only the 4th build field differs), the updater must force-install
    // the full package because the nupkg name only carries the 3-part version.
    void set_force_full(bool force_full) { m_force_full = force_full; }
    bool get_force_full() const { return m_force_full; }

    // Cancel current download
    void cancel_download();
    
    // Install the downloaded update
    void install_update();
    
    // Check if download is in progress
    bool is_downloading() const { return m_downloading.load(); }
    
    // Get downloaded file path (if single file) or dir
    std::string get_downloaded_file_path() const { return m_downloaded_file_path; }

    // Get version being downloaded
    std::string get_version() const { return m_version; }

    // Retry download
    void retry();

    // Cleans up any partial download files (*.part) under the update cache root directory.
    // The input version is only used to locate the cache root via get_cache_dir(version).
    // Completed packages (e.g. *.nupkg) are not removed.
    void cleanup_partial_downloads(const std::string& version_for_path);

    // Helper to get cache directory: %LOCALAPPDATA%/crealityprint_squirrel/<version>/
    // Made public for version-specific storage access
    static std::string get_cache_dir(const std::string& version);

private:
    AppUpdater();
    ~AppUpdater() = default;
    AppUpdater(const AppUpdater&) = delete;
    AppUpdater& operator=(const AppUpdater&) = delete;

    // Worker function for downloading packages
    void download_packages();

    std::atomic_bool m_downloading{ false };
    std::string m_version;
    std::string m_downloaded_file_path;
    std::string m_base_package_name; // For RELEASES file
    std::string m_manual_url; // Manual download URL from API
    
    // Single file download (legacy)
    std::string m_url;
    
    // Multiple files download
    std::vector<PackageInfo> m_packages;
    
    // Keep reference to http request to allow cancellation
    Http::Ptr m_http;
    std::atomic_bool m_cancel{ false };
    bool m_is_hot_update = false;
    bool m_force_full = false;
};

}
}

#endif
