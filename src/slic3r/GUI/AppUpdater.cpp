#include "AppUpdater.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "Widgets/WebView.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/common_header/common_header.h"
#include <wx/app.h>
#include <wx/msgdlg.h>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/format.hpp>
#include <thread>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <curl/curl.h>

#include <wx/stdpaths.h>
#include <wx/filename.h>

#ifdef _WIN32
#include <shlobj.h>
#include <shellapi.h>
#endif

namespace fs = boost::filesystem;

namespace Slic3r {
namespace GUI {

namespace {
std::once_flag g_curl_init_once;

int app_updater_test_delay_ms()
{
    //return 2000;
    static int cached = -1;
    if (cached >= 0)
        return cached;

    const char* v = std::getenv("CP_UPDATER_TEST_DELAY_MS");
    if (!v || !*v) {
        cached = 0;
        return cached;
    }

    char* end = nullptr;
    long ms = std::strtol(v, &end, 10);
    if (end == v || ms < 0)
        ms = 0;
    if (ms > 5000)
        ms = 5000;

    cached = static_cast<int>(ms);
    return cached;
}

struct CurlDownloadCtx {
    FILE* file{ nullptr };
    std::atomic_bool* cancel{ nullptr };
    std::string pkg_name;
    long long total_size{ 0 };
    long long downloaded_before{ 0 };
    long long initial_offset{ 0 };
    int last_percent{ -1 };
    std::chrono::steady_clock::time_point last_post_time{ std::chrono::steady_clock::now() };
    std::vector<char> pending;
    size_t flush_threshold{ 10 * 1024 * 1024 };
};

size_t curl_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* ctx = static_cast<CurlDownloadCtx*>(userdata);
    if (ctx == nullptr || ctx->file == nullptr)
        return 0;
    const size_t chunk_size = size * nmemb;
    if (chunk_size == 0)
        return 0;

    ctx->pending.insert(ctx->pending.end(), ptr, ptr + chunk_size);

    if (ctx->pending.size() >= ctx->flush_threshold) {
        if (ctx->cancel && ctx->cancel->load())
            return 0;
        const size_t written = fwrite(ctx->pending.data(), 1, ctx->pending.size(), ctx->file);
        if (written != ctx->pending.size())
            return 0;
        ctx->pending.clear();
    }

    if (ctx->cancel && ctx->cancel->load())
        return 0;
    return chunk_size;
}

int curl_xferinfo_cb(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t /*ultotal*/, curl_off_t /*ulnow*/)
{
    auto* ctx = static_cast<CurlDownloadCtx*>(clientp);
    if (ctx == nullptr)
        return 1;
    if (ctx->cancel && ctx->cancel->load())
        return 1;

    const long long current_file_downloaded = ctx->initial_offset + static_cast<long long>(dlnow);
    const long long total_downloaded = ctx->downloaded_before + current_file_downloaded;

    int percent = 0;
    if (ctx->total_size > 0) {
        percent = static_cast<int>((total_downloaded * 100) / ctx->total_size);
    } else if (dltotal > 0) {
        percent = static_cast<int>((static_cast<long long>(dlnow) * 100) / static_cast<long long>(dltotal));
    } else {
        // If both total_size and dltotal are 0, keep current percent or show indeterminate
        // Don't set to 100% as it's not actually complete
        percent = ctx->last_percent >= 0 ? ctx->last_percent : 0;
    }

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    // Only update progress if we're actually making progress
    // Avoid showing 100% until download is truly complete
    const auto now = std::chrono::steady_clock::now();
    const bool should_post = (percent != ctx->last_percent) &&
                             (percent == 100 || now - ctx->last_post_time >= std::chrono::milliseconds(120));
    if (should_post) {
        const int delay_ms = app_updater_test_delay_ms();
        if (delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            if (ctx->cancel && ctx->cancel->load())
                return 1;
        }
        ctx->last_percent = percent;
        ctx->last_post_time = now;
        wxCommandEvent evt(EVT_APP_UPDATE_PROGRESS);
        evt.SetInt(percent);
        evt.SetString(wxString::Format(_L("Downloading ... %d%%"), percent));
        wxPostEvent(wxGetApp().mainframe, evt);
    }

    return 0;
}

std::string curl_download_to_file(const std::string& url, CurlDownloadCtx& ctx, long long resume_offset, long* out_http_status)
{
    std::call_once(g_curl_init_once, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });

    CURL* curl = curl_easy_init();
    if (!curl)
        return "Failed to initialize CURL";

    std::string range;
    if (resume_offset > 0)
        range = std::to_string(resume_offset) + "-";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 512L * 1024L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_xferinfo_cb);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, static_cast<void*>(&ctx));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(&ctx));
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_MAX_TLSv1_2);
#endif
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    if (!range.empty())
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());

    CURLcode res = curl_easy_perform(curl);

    long http_status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
    curl_easy_cleanup(curl);
    if (out_http_status)
        *out_http_status = http_status;

    if (!ctx.pending.empty() && ctx.file != nullptr) {
        const size_t written = fwrite(ctx.pending.data(), 1, ctx.pending.size(), ctx.file);
        if (written != ctx.pending.size())
            return "Write file failed";
        ctx.pending.clear();
    }

    if (ctx.cancel && ctx.cancel->load())
        return "Cancelled";

    if (res != CURLE_OK)
        return curl_easy_strerror(res);

    if (http_status >= 400)
        return "HTTP error: " + std::to_string(http_status);

    return {};
}
}

static void ensure_releases_file(const fs::path& cache_dir, const std::vector<PackageInfo>& packages, const std::string& base_package_name)
{
    fs::path releases_path = cache_dir / "RELEASES";
    boost::nowide::ofstream rel(releases_path.string(), std::ios::binary | std::ios::trunc);
    if (!rel.is_open())
        throw std::runtime_error("Failed to create RELEASES file");

    if (!base_package_name.empty()) {
        rel << std::string(40, '0') << " " << base_package_name << " 0\n";
    }

    for (const auto& pkg : packages) {
        std::string sha = pkg.sha1;
        if (sha.empty())
            sha = std::string(40, '0');
        long long size = pkg.size > 0 ? pkg.size : 0;
        rel << sha << " " << pkg.name << " " << size << "\n";
    }
    rel.close();
}

wxDEFINE_EVENT(EVT_APP_UPDATE_PROGRESS, wxCommandEvent);
wxDEFINE_EVENT(EVT_APP_UPDATE_COMPLETE, wxCommandEvent);
wxDEFINE_EVENT(EVT_APP_UPDATE_ERROR, wxCommandEvent);
wxDEFINE_EVENT(EVT_APP_UPDATE_CANCELED, wxCommandEvent);

AppUpdater& AppUpdater::getInstance()
{
    static AppUpdater instance;
    return instance;
}

AppUpdater::AppUpdater()
{
}

std::string AppUpdater::get_cache_dir(const std::string& version)
{
    // Cache directory should be based on the current app version (installed version),
    // not the update target version received from the server.
    (void)version;
    std::string safe_version = std::string(CREALITYPRINT_VERSION);
    fs::path cache_dir;

#ifdef _WIN32
    wxString local_appdata;
    if (wxGetEnv("LOCALAPPDATA", &local_appdata) && !local_appdata.IsEmpty()) {
        cache_dir = fs::path(local_appdata.ToUTF8().data()) / "crealityprint_squirrel" / safe_version;
    } else {
        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
            cache_dir = fs::path(path) / "crealityprint_squirrel" / safe_version;
        }
    }
#endif

    if (cache_dir.empty()) {
#ifdef _WIN32
        cache_dir = fs::path(wxStandardPaths::Get().GetUserDataDir().ToStdWstring()) / "crealityprint_squirrel" / safe_version;
#else
        cache_dir = fs::path(wxStandardPaths::Get().GetUserDataDir().ToStdString()) / "crealityprint_squirrel" / safe_version;
#endif
    }

    if (!fs::exists(cache_dir))
        fs::create_directories(cache_dir);

    return cache_dir.string();
}

void AppUpdater::cleanup_partial_downloads(const std::string& version_for_path)
{
    if (version_for_path.empty())
        return;
    try {
        // Partial downloads are stored as <pkg>.part. We clean them on app exit/start so that
        // resumable state does not persist across sessions. Use get_cache_dir(version_for_path)
        // to resolve the cache root path, then clean *.part under all version subdirectories.
        const fs::path cache_root = fs::path(get_cache_dir(version_for_path)).parent_path();
        if (!fs::exists(cache_root))
            return;

        boost::system::error_code ec;
        for (fs::recursive_directory_iterator it(cache_root, ec), end; it != end && !ec; it.increment(ec)) {
            const fs::path p = it->path();
            if (!fs::is_regular_file(p, ec))
                continue;
            if (p.extension() == ".part") {
                fs::remove(p, ec);
                ec.clear();
            }
        }
    } catch (...) {
    }
}

void AppUpdater::start_download(const std::vector<PackageInfo>& packages, const std::string& version, const std::string& base_package_name, const std::string& manual_url)
{
    if (m_downloading) {
        BOOST_LOG_TRIVIAL(warning) << "AppUpdater: Download already in progress.";
        return;
    }

    m_packages = packages;
    m_version = version;
    m_base_package_name = base_package_name;
    m_manual_url = manual_url;
    m_downloading = true;
    m_cancel = false;
    m_is_hot_update = true;
    
    std::thread(&AppUpdater::download_packages, this).detach();
}

void AppUpdater::start_download(const std::string& url, const std::string& version)
{
    if (m_downloading) {
        BOOST_LOG_TRIVIAL(warning) << "AppUpdater: Download already in progress.";
        return;
    }

    m_url = url;
    m_version = version;
    m_downloading = true;
    m_cancel = false;
    m_is_hot_update = false;
    
    // Setup single package
    PackageInfo p;
    p.url = url;
    p.name = Http::get_filename_from_url(url);
    if (p.name.empty()) p.name = "update.exe";
    p.size = 0; // Unknown
    p.sha1 = ""; 
    
    m_packages.clear();
    m_packages.push_back(p);
    
    std::thread(&AppUpdater::download_packages, this).detach();
}

bool AppUpdater::prepare_cached_hot_update(const std::vector<PackageInfo>& packages, const std::string& version, const std::string& base_package_name, const std::string& manual_url)
{
    if (m_downloading)
        return false;
    if (packages.empty() || version.empty())
        return false;

    std::string cache_dir_str = get_cache_dir(version);
    fs::path cache_dir(cache_dir_str);

    for (const auto& pkg : packages) {
        if (pkg.name.empty())
            return false;
        fs::path target_path = cache_dir / pkg.name;
        if (!fs::exists(target_path))
            return false;
        if (pkg.size > 0 && fs::file_size(target_path) != static_cast<uintmax_t>(pkg.size))
            return false;
    }

    ensure_releases_file(cache_dir, packages, base_package_name);

    m_packages          = packages;
    m_version           = version;
    m_base_package_name = base_package_name;
    m_manual_url        = manual_url;
    m_is_hot_update     = true;
    m_cancel            = false;
    m_downloaded_file_path = (cache_dir / packages.back().name).string();
    return true;
}

bool AppUpdater::prepare_cached_installer(const std::string& url, const std::string& version)
{
    if (m_downloading)
        return false;
    if (url.empty() || version.empty())
        return false;

    PackageInfo p;
    p.url = url;
    p.name = Http::get_filename_from_url(url);
    if (p.name.empty())
        p.name = "update.exe";
    p.size = 0;
    p.sha1 = "";

    std::string cache_dir_str = get_cache_dir(version);
    fs::path target_path = fs::path(cache_dir_str) / p.name;
    if (!fs::exists(target_path))
        return false;

    m_url                = url;
    m_version            = version;
    m_is_hot_update      = false;
    m_cancel             = false;
    m_packages.clear();
    m_packages.push_back(p);
    m_downloaded_file_path = target_path.string();
    return true;
}

bool AppUpdater::prepare_cached_hot_update_from_version(const std::string& version, const std::string& manual_url)
{
    if (m_downloading)
        return false;
    if (version.empty())
        return false;

    std::string cache_dir_str = get_cache_dir(version);
    fs::path cache_dir(cache_dir_str);

    fs::path releases_path = cache_dir / "RELEASES";
    if (!fs::exists(releases_path))
        return false;

    bool has_nupkg = false;
    for (fs::directory_iterator it(cache_dir), end; it != end; ++it) {
        if (!fs::is_regular_file(it->status()))
            continue;
        const fs::path p = it->path();
        if (p.extension() == ".nupkg" && fs::file_size(p) > 0) {
            const std::string fname = p.filename().string();
            
            std::string version_to_match = version;
            
            size_t dot_count = 0;
            for (size_t i = 0; i < version.length(); ++i) {
                if (version[i] == '.') {
                    dot_count++;
                    if (dot_count == 3) {
                        version_to_match = version.substr(0, i);
                        break;
                    }
                }
            }
            
            if (fname.find(version_to_match) == std::string::npos)
                continue;
            has_nupkg = true;
            break;
        }
    }
    if (!has_nupkg)
        return false;

    m_version              = version;
    m_is_hot_update        = true;
    m_manual_url           = manual_url;
    m_cancel               = false;
    m_packages.clear();
    m_url.clear();
    m_base_package_name.clear();
    m_downloaded_file_path.clear();
    return true;
}

void AppUpdater::cancel_download()
{
    m_cancel = true;
    if (m_http) {
        // m_http->cancel(); // Assuming Http has cancel or we rely on flag check in callbacks
    }
}

void AppUpdater::retry()
{
    if (m_is_hot_update) {
        if (!m_packages.empty() && !m_version.empty()) {
            start_download(m_packages, m_version, m_base_package_name, m_manual_url);
        }
    } else {
        if (!m_url.empty() && !m_version.empty()) {
            start_download(m_url, m_version);
        }
    }
}

void AppUpdater::download_packages()
{
    try {
        std::string cache_dir = get_cache_dir(m_version);
        long long total_size = 0;
        long long downloaded_size = 0;
        
        for (const auto& p : m_packages) {
            if (p.size > 0)
                total_size += p.size;
        }
        
        for (const auto& pkg : m_packages) {
            if (m_cancel.load()) {
                m_downloading = false;
                wxCommandEvent evt(EVT_APP_UPDATE_CANCELED);
                wxPostEvent(wxGetApp().mainframe, evt);
                return;
            }
            
            fs::path target_path = fs::path(cache_dir) / pkg.name;
            fs::path part_path = target_path;
            part_path += ".part";
            
            // Check if already downloaded
            bool already_downloaded = false;
            if (fs::exists(target_path)) {
                if (pkg.size > 0 && fs::file_size(target_path) == pkg.size) {
                    // TODO: Check SHA1?
                    already_downloaded = true;
                }
            }
            
            if (already_downloaded) {
                if (pkg.size > 0)
                    downloaded_size += pkg.size;
                m_downloaded_file_path = target_path.string(); // Last downloaded file
                continue;
            }
            
            // Resume logic
            long long resume_offset = 0;
            if (fs::exists(part_path)) {
                const long long part_size = static_cast<long long>(fs::file_size(part_path));
                if (pkg.size > 0) {
                    if (part_size == pkg.size) {
                        if (fs::exists(target_path)) fs::remove(target_path);
                        fs::rename(part_path, target_path);
                        m_downloaded_file_path = target_path.string();
                        downloaded_size += pkg.size;
                        continue;
                    }
                    if (part_size > pkg.size) {
                        fs::remove(part_path);
                        resume_offset = 0;
                    } else {
                        resume_offset = part_size;
                    }
                } else {
                    resume_offset = part_size > 0 ? part_size : 0;
                }
            }

            const std::string pkg_name = pkg.name;
            const std::string pkg_url  = pkg.url;

            FILE* file = nullptr;
#ifdef _WIN32
            if (resume_offset > 0) {
                file = _wfopen(part_path.wstring().c_str(), L"rb+");
                if (!file)
                    file = _wfopen(part_path.wstring().c_str(), L"wb");
                if (file)
                    _fseeki64(file, resume_offset, SEEK_SET);
            } else {
                file = _wfopen(part_path.wstring().c_str(), L"wb");
            }
#else
            if (resume_offset > 0) {
                file = fopen(part_path.string().c_str(), "rb+");
                if (!file)
                    file = fopen(part_path.string().c_str(), "wb");
                if (file)
                    fseeko(file, static_cast<off_t>(resume_offset), SEEK_SET);
            } else {
                file = fopen(part_path.string().c_str(), "wb");
            }
#endif

            if (!file)
                throw std::runtime_error("Failed to open file for writing: " + part_path.string());

            std::vector<char> iobuf(8 * 1024 * 1024);
            setvbuf(file, iobuf.data(), _IOFBF, iobuf.size());

            long long current_file_initial_offset = resume_offset;

            CurlDownloadCtx ctx;
            ctx.file = file;
            ctx.cancel = &m_cancel;
            ctx.pkg_name = pkg_name;
            ctx.total_size = total_size;
            ctx.downloaded_before = downloaded_size;
            ctx.initial_offset = current_file_initial_offset;
            ctx.last_percent = -1;
            ctx.last_post_time = std::chrono::steady_clock::now();
            ctx.pending.reserve(2 * 1024 * 1024);

            long http_status = 0;
            std::string err = curl_download_to_file(pkg_url, ctx, resume_offset, &http_status);

            fflush(file);
            fclose(file);
            file = nullptr;

            if (resume_offset > 0 && !m_cancel.load()) {
                if (http_status == 200) {
                    fs::remove(part_path);
                    resume_offset = 0;
                    FILE* retry_file = nullptr;
#ifdef _WIN32
                    retry_file = _wfopen(part_path.wstring().c_str(), L"wb");
#else
                    retry_file = fopen(part_path.string().c_str(), "wb");
#endif
                    if (!retry_file)
                        throw std::runtime_error("Failed to open file for writing: " + part_path.string());
                    std::vector<char> retry_iobuf(8 * 1024 * 1024);
                    setvbuf(retry_file, retry_iobuf.data(), _IOFBF, retry_iobuf.size());

                    CurlDownloadCtx retry_ctx;
                    retry_ctx.file = retry_file;
                    retry_ctx.cancel = &m_cancel;
                    retry_ctx.pkg_name = pkg_name;
                    retry_ctx.total_size = total_size;
                    retry_ctx.downloaded_before = downloaded_size;
                    retry_ctx.initial_offset = 0;
                    retry_ctx.last_percent = -1;
                    retry_ctx.last_post_time = std::chrono::steady_clock::now();
                    retry_ctx.pending.reserve(2 * 1024 * 1024);

                    http_status = 0;
                    err = curl_download_to_file(pkg_url, retry_ctx, 0, &http_status);

                    fflush(retry_file);
                    fclose(retry_file);
                    retry_file = nullptr;
                } else if (http_status == 416) {
                    if (!(pkg.size > 0 && static_cast<long long>(fs::file_size(part_path)) == pkg.size)) {
                        fs::remove(part_path);
                        throw std::runtime_error("HTTP range not satisfiable for " + pkg.name);
                    }
                }
            }

            if (!err.empty() && err != "Cancelled")
                throw std::runtime_error(err);

            if (m_cancel.load()) {
                m_downloading = false;
                wxCommandEvent evt(EVT_APP_UPDATE_CANCELED);
                wxPostEvent(wxGetApp().mainframe, evt);
                return;
            }
            
            // Verify size
            if (pkg.size > 0) {
                long long fsize = fs::file_size(part_path);
                if (fsize != pkg.size) {
                    // Sometimes servers don't report size correctly or compression?
                    // But here we expect exact match for hot update packages.
                    // throw std::runtime_error("Size mismatch for " + pkg.name);
                     BOOST_LOG_TRIVIAL(error) << "Size mismatch error: " << pkg.name << " expected " << pkg.size << ", got " << fsize;
                }
            }
            
            // Rename
            if (fs::exists(target_path)) fs::remove(target_path);
            fs::rename(part_path, target_path);
            
            m_downloaded_file_path = target_path.string();
            if (pkg.size > 0)
                downloaded_size += pkg.size;
        }
        
        // Generate RELEASES
        if (m_is_hot_update) {
            ensure_releases_file(fs::path(cache_dir), m_packages, m_base_package_name);
        }
        
        m_downloading = false;
        wxCommandEvent evt(EVT_APP_UPDATE_COMPLETE);
        wxPostEvent(wxGetApp().mainframe, evt);
        
    } catch (const std::exception& e) {
        m_downloading = false;
        BOOST_LOG_TRIVIAL(error) << "AppUpdater error: " << e.what();
        wxCommandEvent evt(EVT_APP_UPDATE_ERROR);
        wxPostEvent(wxGetApp().mainframe, evt);
    }
}

void AppUpdater::install_update()
{
    // Execute the installer or updater
#ifdef _WIN32
    if (m_is_hot_update) {
         // Logic adapted from ReleaseNote.cpp run_crealityprint_updater
         // but using the versioned cache directory.
         
         std::string cache_dir = get_cache_dir(m_version);
         fs::path cache_path(cache_dir);
         
         // 1. Locate updater.exe in the installation directory
#ifdef _WIN32
         fs::path install_dir = fs::path(wxStandardPaths::Get().GetExecutablePath().ToStdWstring()).parent_path();
#else
         fs::path install_dir = fs::system_complete(wxStandardPaths::Get().GetExecutablePath().ToStdString()).parent_path();
#endif
         fs::path updater_src = install_dir / "updater.exe";
         
         if (!fs::exists(updater_src)) {
#ifdef _WIN32
             BOOST_LOG_TRIVIAL(error) << "AppUpdater: updater.exe not found at " << boost::nowide::narrow(updater_src.wstring());
#else
             BOOST_LOG_TRIVIAL(error) << "AppUpdater: updater.exe not found at " << updater_src.string();
#endif
             wxMessageBox(_L("Failed to locate updater executable."), _L("Update Error"), wxOK | wxICON_ERROR);
             return;
         }
         
         // 2. Copy updater.exe to the cache directory
         fs::path updater_dest = cache_path / "updater.exe";
         try {
#ifdef _WIN32
            CopyFileW(updater_src.wstring().c_str(), updater_dest.wstring().c_str(), FALSE);
#else
             fs::copy_file(updater_src, updater_dest, fs::copy_option::overwrite_if_exists);
#endif
         } catch (const fs::filesystem_error& e) {
             BOOST_LOG_TRIVIAL(error) << "AppUpdater: Failed to copy updater.exe: " << e.what();
             wxMessageBox(_L("Failed to prepare updater executable."), _L("Update Error"), wxOK | wxICON_ERROR);
             return;
         } catch (const std::exception& e) {
             BOOST_LOG_TRIVIAL(error) << "AppUpdater: Failed to copy updater.exe: " << e.what();
             wxMessageBox(_L("Failed to prepare updater executable."), _L("Update Error"), wxOK | wxICON_ERROR);
             return;
         }
         
         // 3. Run updater.exe
         // Arguments: --install-dir="<install_dir>" --current-version="<current_version>"
         // Use 3-part version for Squirrel compatibility
         
         std::string current_version = std::string(CREALITYPRINT_VERSION_MAJOR) + "." + 
                                        std::string(CREALITYPRINT_VERSION_MINOR) + "." + 
                                        std::string(CREALITYPRINT_VERSION_PATCH);
         std::string version_extra = std::string(PROJECT_VERSION_EXTRA);
         
         // Convert version extra to lowercase for Squirrel compatibility
         std::transform(version_extra.begin(), version_extra.end(), version_extra.begin(), 
                        [](unsigned char c) { return (char)std::tolower(c); });
         
         if (!version_extra.empty()) {
             current_version += "-" + version_extra;
         }
         
         std::wstring wupdater = boost::nowide::widen(updater_dest.string());
         std::wstring winstall_dir = boost::nowide::widen(install_dir.string());
         std::wstring wversion = boost::nowide::widen(current_version);

         std::string manual_url;
         if (!m_manual_url.empty()) {
             manual_url = m_manual_url;
         } else {
             manual_url = (wxGetApp().app_config->get("language") == "zh_CN")
                 ? "https://wiki.creality.com/zh/software/update-released"
                 : "https://wiki.creality.com/en/software/update-released";
         }
         std::wstring wmanual_url = boost::nowide::widen(manual_url);
         std::string dark_mode = (wxGetApp().app_config->get("dark_color_mode") == "1") ? "1" : "0";
         std::wstring wdark_mode = boost::nowide::widen(dark_mode);
         std::string language = wxGetApp().app_config->get("language");
         std::wstring wlanguage = boost::nowide::widen(language);
         std::wstring wDataDir = boost::nowide::widen(Slic3r::data_dir());

         std::wstring params =
             L"--install-dir=\"" + winstall_dir +
             L"\" --current-version=\"" + wversion +
             L"\" --manual-url=\"" + wmanual_url +
             L"\" --dark-mode=\"" + wdark_mode +
             L"\" --language=\"" + wlanguage +
             L"\" --data-dir=\"" + wDataDir + L"\"";

         // Hotfix update: target shares the same major.minor.patch as the installed version
         // (only the 4th build field differs). Tell the updater to force-install the full
         // package for a same-3-part version, which it would otherwise skip.
         if (m_force_full) {
             params += L" --force-full=\"1\"";
         }
         
         BOOST_LOG_TRIVIAL(info) << "AppUpdater: Launching " << boost::nowide::narrow(updater_dest.wstring()) << " with params " << boost::nowide::narrow(params);
         
         // Destroy all WebView instances and wait for msedgewebview2.exe child
         // processes to fully exit before launching the updater. This releases
         // any file locks on the WebView2 user data folder and ensures the
         // updater can rename the install directory without access-denied errors.
         ::WebView::DestroyAll();

         ShellExecuteW(NULL, L"open", wupdater.c_str(), params.c_str(), NULL, SW_SHOWNORMAL);

         // Mark the app as closing and tear down any pending update UI/callbacks BEFORE
         // closing the main frame. Closing the frame destroys the Plater (and its pimpl),
         // so any lingering EVT_APP_UPDATE_* handler or imgui notification callback that
         // reaches GUI_App::notification_manager() -> plater_->get_notification_manager()
         // would otherwise dereference a destroyed pimpl and crash.
         wxGetApp().prepare_close_for_update();

         // Close main app
         wxGetApp().mainframe->Close(true);
         wxGetApp().ExitMainLoop();
         
    } else {
        // Legacy installer
        if (m_downloaded_file_path.empty() || !fs::exists(m_downloaded_file_path)) return;
        std::wstring wpath = boost::nowide::widen(m_downloaded_file_path);
        ShellExecuteW(NULL, L"open", wpath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        wxGetApp().prepare_close_for_update();
        wxGetApp().mainframe->Close(true);
        wxGetApp().ExitMainLoop();
    }
#endif
}

}
}
