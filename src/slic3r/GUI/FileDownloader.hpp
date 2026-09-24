#ifndef slic3r_GUI_CurlConnectionPool_hpp_
#define slic3r_GUI_CurlConnectionPool_hpp_

#include <boost/nowide/fstream.hpp>
#include <curl/curl.h>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

class CurlConnectionPool {
public:
    explicit CurlConnectionPool(int max_connections = 5);
    ~CurlConnectionPool();

    // Queue metadata only; open files and curl handles when a slot is free.
    bool addDownload(const std::string& url, const std::string& filename);
    // Failed/cancelled downloads leave no partial file. Successful files remain.
    bool performDownloads(const std::function<bool()>& cancelled = {});

    CurlConnectionPool(const CurlConnectionPool&) = delete;
    CurlConnectionPool& operator=(const CurlConnectionPool&) = delete;

private:
    struct DownloadItem {
        std::string url;
        std::string filename;
        boost::nowide::ofstream stream;
        CURL* handle = nullptr;
        bool opened = false;
        bool completed = false;

        DownloadItem(std::string u, std::string f);
        ~DownloadItem();
        DownloadItem(const DownloadItem&) = delete;
        DownloadItem& operator=(const DownloadItem&) = delete;
    };

    static size_t writeDataCallback(void* ptr, size_t size, size_t nmemb, void* userdata) noexcept;
    bool startNextDownload();
    bool cleanupCompletedDownloads();
    void clearActiveDownloads() noexcept;

    CURLM* multi_handle_;
    size_t max_connections_;
    std::deque<std::pair<std::string, std::string>> pending_;
    std::map<CURL*, std::unique_ptr<DownloadItem>> active_;
};
#endif
