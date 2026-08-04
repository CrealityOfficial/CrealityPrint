#ifndef REMOTE_PRINTER_MANAGER_H
#define REMOTE_PRINTER_MANAGER_H

#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <deque>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include "Device/LanPrinterInterface.hpp"
#include "Device/OctoPrintInterface.hpp"
#include "Device/KlipperInterface.hpp"
#include "Device/Klipper4408Interface.hpp"
#include <curl/curl.h>
#include "Device/KlipperCXInterface.hpp"
#include "UploadCancellation.hpp"
namespace RemotePrint {

// Upload file timeout (seconds)
#define REMOTE_PRINTER_UPLOAD_TIMEOUT_SECONDS 6
// Maximum retry count
const int MAX_RETRY = 100;

enum class RemotePrinerType {
    REMOTE_PRINTER_TYPE_NONE = -1,
    REMOTE_PRINTER_TYPE_LAN,        // lan
    REMOTE_PRINTER_TYPE_OCTOPRINT,  // octoprint
    REMOTE_PRINTER_TYPE_KLIPPER,    // klipper
    REMOTE_PRINTER_TYPE_KLIPPER4408, // klipper4408
    REMOTE_PRINTER_TYPE_CX,
};

enum class UploadTaskState {
    Queued,
    Running,
    CancelRequested,
    Cancelled,
    Succeeded,
    Failed,
};

size_t read_callback(void* ptr, size_t size, size_t nmemb, void* stream);
int    progress_callback(void* ptr, curl_off_t totalToDownload, curl_off_t nowDownloaded, curl_off_t totalToUpload, curl_off_t nowUploaded);
std::string url_encode(const std::string &value);

class RemotePrinterManager
{
public:
    static RemotePrinterManager& getInstance()
    {
        static std::unique_ptr<RemotePrinterManager> instance;
        static std::once_flag flag;
        std::call_once(flag, []() {
            instance.reset(new RemotePrinterManager());
        });
        return *instance;
    }

    static void destroyInstance() {
        instance.reset();
    }
    struct UploadTask
    {
        std::string                                     fileName;
        std::string                                     filePath;
        std::function<void(std::string, float, double)> progressCallback;
        std::function<void(std::string, int)>           uploadStatusCallback;
        std::function<void(std::string, std::string)>   onCompleteCallback;
    };


    ~RemotePrinterManager();

    void uploadThread();
    std::string pushUploadTasks(const std::string&               ipAddress,
                        const std::string&                      fileName,
                        const std::string&                      filePath,
                        std::function<void(std::string, float,double)> progressCallback,
                        std::function<void(std::string, int)>   uploadStatusCallback = nullptr, 
                        std::function<void(std::string, std::string)>   onCompleteCallback = nullptr);
    std::string pushUploadMultTasks(const std::string&               ipAddress,
                        const std::string&                      fileName,
                        const std::string&                      filePath,
                        std::function<void(std::string, float,double)> progressCallback,
                        std::function<void(std::string, int)>   uploadStatusCallback = nullptr, 
                        std::function<void(std::string, std::string)>   onCompleteCallback = nullptr);
    bool cancelUploadTask(const std::string& taskId);
    void cancelUpload(const std::string& ipAddress);
    void uploadFileByLan(const std::string& ipAddress,
                         const std::string& fileName,
                         const std::string& filePath,
                         std::function<void(float,double)> progressCallback = nullptr,
                         std::function<void(std::string, int)>   uploadStatusCallback = nullptr,
                         std::function<void(std::string, std::string)>   onCompleteCallback = nullptr);

    void setOldPrinterMap(std::string& ipAddress);
    void setKlipperPrinterMap(const std::string& ipAddress,int port);
    int getKlipperPrinterMap(const std::string& ipAddress);
    void addDownloadTask(const std::function<void()>& task);
    void retryUpload(const std::string& ipAddress);

    void setUploadTimeout(int seconds) { m_uploadTimeoutSeconds = seconds; }
    int getUploadTimeout() const { return m_uploadTimeoutSeconds; }

private:
    using ProgressCallback = std::function<void(std::string, float, double)>;
    using StatusCallback = std::function<void(std::string, int)>;
    using CompleteCallback = std::function<void(std::string, std::string)>;

    struct ManagedUploadTask {
        std::string taskId;
        std::string ipAddress;
        std::string fileName;
        std::string filePath;
        ProgressCallback progressCallback;
        StatusCallback statusCallback;
        CompleteCallback completeCallback;
        UploadCancelToken cancelToken = make_upload_cancel_token();
        std::atomic<UploadTaskState> state {UploadTaskState::Queued};
        std::atomic_bool terminal {false};
        std::atomic_bool statusDelivered {false};
        std::atomic_bool statusReported {false};
        std::atomic_bool backgroundCleanup {false};
        std::atomic_int reportedStatus {0};
    };

    RemotePrinterManager();

    RemotePrinterManager(const RemotePrinterManager&)            = delete;
    RemotePrinterManager& operator=(const RemotePrinterManager&) = delete;

    std::shared_ptr<ManagedUploadTask> createUploadTask(
        const std::string& ipAddress,
        const std::string& fileName,
        const std::string& filePath,
        ProgressCallback progressCallback,
        StatusCallback statusCallback,
        CompleteCallback completeCallback);
    void runUploadTask(const std::shared_ptr<ManagedUploadTask>& task);
    void pushFile(const std::shared_ptr<ManagedUploadTask>& task);
    void deliverStatus(const std::shared_ptr<ManagedUploadTask>& task, int statusCode);
    void finishTask(const std::shared_ptr<ManagedUploadTask>& task, UploadTaskState state, int statusCode);
    void unregisterTask(const std::shared_ptr<ManagedUploadTask>& task);
    void workerThread();
    LanPrinterInterface*  m_pLanPrinterInterface {nullptr};
    OctoPrintInterface*   m_pOctoPrinterInterface {nullptr};
    KlipperInterface*     m_pKlipperInterface {nullptr};
    Klipper4408Interface* m_pKlipper4408Interface {nullptr};
    KlipperCXInterface*   m_pKlipperCXInterface {nullptr};

    std::mutex m_mtxUpload;
    std::condition_variable m_cvUpload;
    std::deque<std::shared_ptr<ManagedUploadTask>> m_uploadTasks;
    std::unordered_map<std::string, std::shared_ptr<ManagedUploadTask>> m_tasksById;
    std::unordered_map<std::string, std::string> m_latestTaskByAddress;
    std::queue<std::function<void()>> tasks;
    std::vector<std::thread> m_multUploadThreads;
    std::thread m_uploadThread;
    std::atomic<bool> m_bExit { false };

    static std::unique_ptr<RemotePrinterManager> instance;
    static std::once_flag flag;

    RemotePrinerType determinePrinterType(const std::string& ipAddress);

    std::mutex m_mtxPrinterMeta;
    std::vector<std::string> oldPrinters;
    std::map<std::string,int> mapKlipperPort;
    std::condition_variable condition;
    std::atomic<bool> stop_flag;
    std::mutex queue_mutex;
    std::map<std::string, UploadTask> m_lastUploadMap;
    std::atomic_uint64_t m_nextTaskId {1};
    std::atomic_int m_preparingCleanupTasks {0};

    int m_uploadTimeoutSeconds = REMOTE_PRINTER_UPLOAD_TIMEOUT_SECONDS;
};
} // namespace RemotePrint

#endif // REMOTE_PRINTER_MANAGER_H
