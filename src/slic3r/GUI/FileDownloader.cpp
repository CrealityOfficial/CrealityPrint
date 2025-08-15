#include "slic3r/GUI/FileDownloader.hpp"
#include <exception>
// 实现部分
CurlConnectionPool::CurlConnectionPool(int max_connections)
    : multi_handle_(curl_multi_init()),
      still_running_(0),
      max_connections_(max_connections) {
    if (!multi_handle_) {
        throw std::runtime_error("Failed to initialize curl multi handle");
    }
}

CurlConnectionPool::~CurlConnectionPool() {
    // 清理所有easy handles
    for (auto handle : easy_handles_) {
        curl_multi_remove_handle(multi_handle_, handle);
        curl_easy_cleanup(handle);
    }
    
    // 清理multi handle
    if (multi_handle_) {
        curl_multi_cleanup(multi_handle_);
    }
}

bool CurlConnectionPool::addDownload(const std::string url, const std::string filename) {
    try {
        // 创建下载项
        std::shared_ptr<DownloadItem> item(new DownloadItem(url, filename));
        download_items_.emplace_back(item);
        
        if (!item->stream.is_open()) {
            download_items_.pop_back();
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }
        
        // 创建easy handle
        CURL* handle = curl_easy_init();
        if (!handle) {
            download_items_.pop_back();
            return false;
        }
        
        // 设置easy handle选项
        curl_easy_setopt(handle, CURLOPT_URL, item->url.c_str());
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeDataCallback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &item->stream);
        curl_easy_setopt(handle, CURLOPT_PRIVATE, item->filename.c_str());
        curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
        
        // 启用连接复用
        curl_easy_setopt(handle, CURLOPT_FRESH_CONNECT, 0L);
        curl_easy_setopt(handle, CURLOPT_FORBID_REUSE, 0L);
        
        // 添加到multi handle
        curl_multi_add_handle(multi_handle_, handle);
        easy_handles_.push_back(handle);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error adding download: " << e.what() << std::endl;
        return false;
    }
}

void CurlConnectionPool::performDownloads() {
    // 初始执行
    curl_multi_perform(multi_handle_, &still_running_);
    
    // 主下载循环
    while (still_running_) {
        fd_set fdread, fdwrite, fdexcep;
        int maxfd = -1;
        long curl_timeo = -1;
        
        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);
        
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        
        // 获取超时设置
        curl_multi_timeout(multi_handle_, &curl_timeo);
        if (curl_timeo >= 0) {
            timeout.tv_sec = curl_timeo / 1000;
            if (timeout.tv_sec > 1) {
                timeout.tv_sec = 1;
            } else {
                timeout.tv_usec = (curl_timeo % 1000) * 1000;
            }
        }
        
        // 获取文件描述符集
        curl_multi_fdset(multi_handle_, &fdread, &fdwrite, &fdexcep, &maxfd);
        
        // 等待活动或超时
        if (maxfd == -1) {
            // 没有文件描述符，等待一段时间
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            // 使用select等待I/O活动
            select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
        }
        
        // 执行传输
        curl_multi_perform(multi_handle_, &still_running_);
        
        // 清理完成的下载
        cleanupCompletedDownloads();
    }
    download_items_.clear();
}

size_t CurlConnectionPool::writeDataCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* stream = static_cast<std::ofstream*>(userdata);
    size_t written = 0;
    
    try{
        stream->write(static_cast<char*>(ptr), size * nmemb);
        written = size * nmemb;
    }catch(std::exception e)
    {
        
    }

    return written;
}

void CurlConnectionPool::cleanupCompletedDownloads() {
    CURLMsg* msg = nullptr;
    int msgs_left = 0;
    
    while ((msg = curl_multi_info_read(multi_handle_, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL* handle = msg->easy_handle;
            char* filename = nullptr;
            
            // 获取文件名
            curl_easy_getinfo(handle, CURLINFO_PRIVATE, &filename);
            
            // 输出下载结果
            if (msg->data.result == CURLE_OK) {
                std::cout << "Download completed: " << filename << std::endl;
            } else {
                std::cerr << "Download failed: " << filename 
                          << " - " << curl_easy_strerror(msg->data.result) << std::endl;
            }
            
            // 从multi handle中移除并清理
            curl_multi_remove_handle(multi_handle_, handle);
            
            // 从easy_handles_中移除
            auto it = std::find(easy_handles_.begin(), easy_handles_.end(), handle);
            if (it != easy_handles_.end()) {
                easy_handles_.erase(it);
            }
            
            curl_easy_cleanup(handle);
        }
    }
}