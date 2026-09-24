#include "slic3r/GUI/FileDownloader.hpp"

#include <boost/nowide/cstdio.hpp>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
void checkMulti(CURLMcode result)
{
    if (result != CURLM_OK)
        throw std::runtime_error(std::string("Preset download: ") + curl_multi_strerror(result));
}

void checkEasy(CURLcode result)
{
    if (result != CURLE_OK)
        throw std::runtime_error(std::string("Preset download: ") + curl_easy_strerror(result));
}
}

CurlConnectionPool::DownloadItem::DownloadItem(std::string u, std::string f)
    : url(std::move(u)), filename(std::move(f))
{}

CurlConnectionPool::DownloadItem::~DownloadItem()
{
    if (handle)
        curl_easy_cleanup(handle);
    if (stream.is_open())
        stream.close();
    if (opened && !completed)
        boost::nowide::remove(filename.c_str());
}

CurlConnectionPool::CurlConnectionPool(int max_connections)
    : multi_handle_(nullptr), max_connections_(max_connections > 0 ? max_connections : 1)
{
    multi_handle_ = curl_multi_init();
    if (!multi_handle_)
        throw std::runtime_error("Failed to initialize curl multi handle");
}

CurlConnectionPool::~CurlConnectionPool()
{
    clearActiveDownloads();
    curl_multi_cleanup(multi_handle_);
}

void CurlConnectionPool::clearActiveDownloads() noexcept
{
    for (const auto& entry : active_)
        curl_multi_remove_handle(multi_handle_, entry.first);
    active_.clear();
}

bool CurlConnectionPool::addDownload(const std::string& url, const std::string& filename)
{
    try {
        std::string escaped_url = url;
        for (size_t pos = 0; (pos = escaped_url.find(' ', pos)) != std::string::npos; pos += 3)
            escaped_url.replace(pos, 1, "%20");
        pending_.emplace_back(std::move(escaped_url), filename);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error queuing download: " << e.what() << std::endl;
        return false;
    }
}

bool CurlConnectionPool::startNextDownload()
{
    auto item = std::make_unique<DownloadItem>(pending_.front().first, pending_.front().second);
    pending_.pop_front();
    item->stream.open(item->filename, std::ios::binary | std::ios::trunc);
    item->opened = item->stream.is_open();
    if (!item->opened) {
        std::cerr << "Failed to open download file: " << item->filename << std::endl;
        return false;
    }

    item->handle = curl_easy_init();
    if (!item->handle)
        throw std::runtime_error("Failed to initialize curl download handle");
    CURL* handle = item->handle;
    checkEasy(curl_easy_setopt(handle, CURLOPT_URL, item->url.c_str()));
    checkEasy(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeDataCallback));
    checkEasy(curl_easy_setopt(handle, CURLOPT_WRITEDATA, &item->stream));
    checkEasy(curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L));
    checkEasy(curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L));
    checkEasy(curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L));
    checkEasy(curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L));
    checkEasy(curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 10L));
    checkEasy(curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, 1L));
    checkEasy(curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, 60L));

    // Own the stream before registering callbacks: allocation failure must not
    // leave curl pointing at a destroyed download item.
    active_.emplace(handle, std::move(item));
    const CURLMcode result = curl_multi_add_handle(multi_handle_, handle);
    if (result != CURLM_OK) {
        active_.erase(handle);
        checkMulti(result);
    }
    return true;
}

bool CurlConnectionPool::performDownloads(const std::function<bool()>& cancelled)
{
    bool success = true;
    try {
        while (!pending_.empty() || !active_.empty()) {
            if (cancelled && cancelled()) {
                clearActiveDownloads();
                pending_.clear();
                return false;
            }
            while (!pending_.empty() && active_.size() < max_connections_)
                if (!startNextDownload())
                    success = false;

            int running = 0;
            checkMulti(curl_multi_perform(multi_handle_, &running));
            if (!cleanupCompletedDownloads())
                success = false;

            // Completed streams are closed before refilling their slots.
            if (!pending_.empty() && active_.size() < max_connections_)
                continue;
            if (!active_.empty()) {
                int numfds = 0;
                checkMulti(curl_multi_wait(multi_handle_, nullptr, 0, 100, &numfds));
                if (numfds == 0)
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    } catch (...) {
        clearActiveDownloads();
        pending_.clear();
        throw;
    }
    return success;
}

size_t CurlConnectionPool::writeDataCallback(void* ptr, size_t size, size_t nmemb, void* userdata) noexcept
{
    if (!userdata || (size != 0 && nmemb > (std::numeric_limits<size_t>::max)() / size))
        return 0;
    const size_t bytes = size * nmemb;
    if (bytes > static_cast<size_t>((std::numeric_limits<std::streamsize>::max)()))
        return 0;
    try {
        auto* stream = static_cast<boost::nowide::ofstream*>(userdata);
        stream->write(static_cast<const char*>(ptr), static_cast<std::streamsize>(bytes));
        return *stream ? bytes : 0;
    } catch (...) {
        // Do not let C++ exceptions escape through libcurl's C callback boundary.
        return 0;
    }
}

bool CurlConnectionPool::cleanupCompletedDownloads()
{
    bool success = true;
    int msgs_left = 0;
    while (CURLMsg* msg = curl_multi_info_read(multi_handle_, &msgs_left)) {
        if (msg->msg != CURLMSG_DONE)
            continue;
        const CURLcode result = msg->data.result;
        auto it = active_.find(msg->easy_handle);
        if (it == active_.end())
            continue;
        DownloadItem& item = *it->second;
        item.stream.close();
        item.completed = result == CURLE_OK && !item.stream.fail();
        if (!item.completed) {
            success = false;
            std::cerr << "Download failed: " << item.filename << " - "
                      << (result == CURLE_OK ? "file write/close failed" : curl_easy_strerror(result)) << std::endl;
        }
        curl_multi_remove_handle(multi_handle_, it->first);
        active_.erase(it);
    }
    return success;
}
