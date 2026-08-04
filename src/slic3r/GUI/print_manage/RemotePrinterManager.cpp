#include "RemotePrinterManager.hpp"
#include <algorithm>
#include <chrono>
#include <future>
#include <string>

namespace RemotePrint {

size_t read_callback(void* ptr, size_t size, size_t nmemb, void* stream)
{
    std::ifstream* file = static_cast<std::ifstream*>(stream);
    file->read(static_cast<char*>(ptr), size * nmemb);
    return file->gcount();
}

int progress_callback(void* ptr, curl_off_t totalToDownload, curl_off_t nowDownloaded, curl_off_t totalToUpload, curl_off_t nowUploaded)
{
    auto* progressCallback = static_cast<std::function<void(float)>*>(ptr);
    if (totalToUpload > 0 && progressCallback) {
        (*progressCallback)(static_cast<float>(nowUploaded) / totalToUpload * 100.0f);
    }
    return 0;
}

RemotePrinterManager::RemotePrinterManager():stop_flag(false) 
{
//     m_pLanPrinterInterface = new LanPrinterInterface();
//     m_pOctoPrinterInterface = new OctoPrintInterface();
    m_pKlipperInterface = new KlipperInterface();
    m_pKlipper4408Interface = new Klipper4408Interface();
    m_pKlipperCXInterface = new KlipperCXInterface();

    m_uploadThread = std::thread(&RemotePrinterManager::uploadThread, this);
    for (int i = 0; i < 3; ++i) {
            m_multUploadThreads.emplace_back([this] { workerThread(); });
        }
}
void RemotePrinterManager::workerThread()
{
    while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                condition.wait(lock, [this] {
                    return stop_flag || !tasks.empty();
                });
                
                if (stop_flag) {
                    return;
                }
                
                task = std::move(tasks.front());
                tasks.pop();
            }
            
            task(); // 执行下载任务
        }
}
RemotePrinterManager::~RemotePrinterManager()
{
    m_bExit.store(true, std::memory_order_relaxed);
    stop_flag.store(true, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(m_mtxUpload);
        for (auto& item : m_tasksById)
            request_upload_cancel(item.second->cancelToken);
        m_uploadTasks.clear();
    }
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        std::queue<std::function<void()>> empty;
        tasks.swap(empty);
    }

    m_cvUpload.notify_all();
    condition.notify_all();

    if (m_uploadThread.joinable()) {
        m_uploadThread.join();
    }
    for (auto& t : m_multUploadThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

//     delete m_pLanPrinterInterface;
//     delete m_pOctoPrinterInterface;
    delete m_pKlipperInterface;
    delete m_pKlipper4408Interface;
    delete m_pKlipperCXInterface;
}

void RemotePrinterManager::uploadThread() 
{
    while (!m_bExit.load(std::memory_order_relaxed)) {
        std::unique_lock<std::mutex> lock(m_mtxUpload);
        m_cvUpload.wait(lock, [this] { return !m_uploadTasks.empty() || m_bExit.load(std::memory_order_relaxed); });

        if (m_bExit.load(std::memory_order_relaxed))
            break;

        auto task = m_uploadTasks.front();
        m_uploadTasks.pop_front();
        lock.unlock();

        runUploadTask(task);
    }
}

std::shared_ptr<RemotePrinterManager::ManagedUploadTask> RemotePrinterManager::createUploadTask(
    const std::string& ipAddress,
    const std::string& fileName,
    const std::string& filePath,
    ProgressCallback progressCallback,
    StatusCallback statusCallback,
    CompleteCallback completeCallback)
{
    auto task = std::make_shared<ManagedUploadTask>();
    task->taskId = "upload-" + std::to_string(m_nextTaskId.fetch_add(1, std::memory_order_relaxed));
    task->ipAddress = ipAddress;
    task->fileName = fileName;
    task->filePath = filePath;
    task->progressCallback = std::move(progressCallback);
    task->statusCallback = std::move(statusCallback);
    task->completeCallback = std::move(completeCallback);

    std::lock_guard<std::mutex> lock(m_mtxUpload);
    m_tasksById[task->taskId] = task;
    m_latestTaskByAddress[ipAddress] = task->taskId;
    m_lastUploadMap[ipAddress] = {
        fileName, filePath, task->progressCallback, task->statusCallback, task->completeCallback
    };
    return task;
}

std::string RemotePrinterManager::pushUploadTasks(const std::string& ipAddress, const std::string& fileName, const std::string& filePath, std::function<void(std::string, float,double)> progressCallback, std::function<void(std::string, int)> uploadStatusCallback, std::function<void(std::string, std::string)> onCompleteCallback)
{
    if (m_bExit.load(std::memory_order_relaxed))
        return {};

    auto task = createUploadTask(ipAddress, fileName, filePath, std::move(progressCallback),
                                 std::move(uploadStatusCallback), std::move(onCompleteCallback));
    bool rejected = m_bExit.load(std::memory_order_relaxed);
    if (rejected) {
        request_upload_cancel(task->cancelToken);
        finishTask(task, UploadTaskState::Cancelled, 601);
        unregisterTask(task);
        return {};
    }

    // A cancelled preparation request may still be timing out in its own
    // network stack. Keep the primary upload lane available by using the
    // existing bounded worker pool until that background cleanup completes.
    if (m_preparingCleanupTasks.load(std::memory_order_acquire) > 0) {
        addDownloadTask([this, task]() {
            runUploadTask(task);
        });
    } else {
        {
            std::lock_guard<std::mutex> lock(m_mtxUpload);
            if (m_bExit.load(std::memory_order_relaxed)) {
                request_upload_cancel(task->cancelToken);
                rejected = true;
            } else {
                m_uploadTasks.emplace_back(task);
            }
        }
        if (rejected) {
            finishTask(task, UploadTaskState::Cancelled, 601);
            unregisterTask(task);
            return {};
        }
        m_cvUpload.notify_one();
    }
    return task->taskId;
}
void RemotePrinterManager::addDownloadTask(const std::function<void()>& task) {
        if (stop_flag.load(std::memory_order_relaxed)) {
            return;
        }
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop_flag.load(std::memory_order_relaxed)) {
                return;
            }
            tasks.push(task);
        }
        condition.notify_one();
    }
std::string RemotePrinterManager::pushUploadMultTasks(const std::string& ipAddress, const std::string& fileName, const std::string& filePath, std::function<void(std::string, float,double)> progressCallback, std::function<void(std::string, int)> uploadStatusCallback, std::function<void(std::string, std::string)> onCompleteCallback)
{
    if (m_bExit.load(std::memory_order_relaxed))
        return {};

    auto task = createUploadTask(ipAddress, fileName, filePath, std::move(progressCallback),
                                 std::move(uploadStatusCallback), std::move(onCompleteCallback));
    addDownloadTask([this, task]() {
        runUploadTask(task);
    });
    return task->taskId;
}
void RemotePrinterManager::uploadFileByLan(const std::string& ipAddress, const std::string& fileName, const std::string& filePath, std::function<void(float,double)> progressCallback, std::function<void(std::string, int)> uploadStatusCallback, std::function<void(std::string, std::string)> onCompleteCallback) 
{
    m_pLanPrinterInterface->sendFileToDevice(
        ipAddress, fileName, filePath, progressCallback, nullptr, nullptr, make_upload_cancel_token());
}

bool RemotePrinterManager::cancelUploadTask(const std::string& taskId)
{
    std::shared_ptr<ManagedUploadTask> task;
    {
        std::lock_guard<std::mutex> lock(m_mtxUpload);
        auto it = m_tasksById.find(taskId);
        if (it == m_tasksById.end())
            return false;
        task = it->second;
    }

    request_upload_cancel(task->cancelToken);
    auto state = task->state.load(std::memory_order_acquire);
    while (true) {
        if (task->terminal.load(std::memory_order_acquire) ||
            state == UploadTaskState::Cancelled || state == UploadTaskState::Succeeded ||
            state == UploadTaskState::Failed)
            return false;

        if (state == UploadTaskState::Queued) {
            if (!task->state.compare_exchange_weak(
                    state, UploadTaskState::CancelRequested, std::memory_order_acq_rel)) {
                continue;
            }
            BOOST_LOG_TRIVIAL(warning) << "[UploadCancel] logical cancel queued task=" << task->taskId;
            finishTask(task, UploadTaskState::Cancelled, 601);
            unregisterTask(task);
            return true;
        }

        if (state == UploadTaskState::CancelRequested) {
            if (!is_upload_file_in_use(task->cancelToken) &&
                !task->terminal.load(std::memory_order_acquire)) {
                bool expected = false;
                if (task->backgroundCleanup.compare_exchange_strong(
                        expected, true, std::memory_order_acq_rel)) {
                    m_preparingCleanupTasks.fetch_add(1, std::memory_order_acq_rel);
                }
                BOOST_LOG_TRIVIAL(warning)
                    << "[UploadCancel] file released; logical cancel task=" << task->taskId;
                finishTask(task, UploadTaskState::Cancelled, 601);
                unregisterTask(task);
            }
            return true;
        }

        if (task->state.compare_exchange_weak(
                state, UploadTaskState::CancelRequested, std::memory_order_acq_rel)) {
            if (is_upload_file_in_use(task->cancelToken)) {
                BOOST_LOG_TRIVIAL(warning)
                    << "[UploadCancel] waiting for file release, task=" << task->taskId;
            } else {
                bool expected = false;
                if (task->backgroundCleanup.compare_exchange_strong(
                        expected, true, std::memory_order_acq_rel)) {
                    m_preparingCleanupTasks.fetch_add(1, std::memory_order_acq_rel);
                }
                BOOST_LOG_TRIVIAL(warning)
                    << "[UploadCancel] logical cancel during preparation, task=" << task->taskId;
                finishTask(task, UploadTaskState::Cancelled, 601);
                unregisterTask(task);
            }
            return true;
        }
    }
}

void RemotePrinterManager::cancelUpload(const std::string& ipAddress)
{
    std::string taskId;
    {
        std::lock_guard<std::mutex> lock(m_mtxUpload);
        auto it = m_latestTaskByAddress.find(ipAddress);
        if (it != m_latestTaskByAddress.end())
            taskId = it->second;
    }
    if (!taskId.empty())
        cancelUploadTask(taskId);
}

void RemotePrinterManager::setOldPrinterMap(std::string& ipAddress)
{
    std::lock_guard<std::mutex> lock(m_mtxPrinterMeta);
    if (std::find(oldPrinters.begin(), oldPrinters.end(), ipAddress) == oldPrinters.end())
        oldPrinters.push_back(ipAddress);
}

void RemotePrinterManager::setKlipperPrinterMap(const std::string& ipAddress,int port)
{
    std::lock_guard<std::mutex> lock(m_mtxPrinterMeta);
    mapKlipperPort[ipAddress] = port;
}

int RemotePrinterManager::getKlipperPrinterMap(const std::string& ipAddress)
{
    std::lock_guard<std::mutex> lock(m_mtxPrinterMeta);
    for (const auto& pair : mapKlipperPort)
    {
        if (pair.first == ipAddress)
        {
            return pair.second;
        }
    }
    return 80;
}

void RemotePrinterManager::deliverStatus(const std::shared_ptr<ManagedUploadTask>& task, int statusCode)
{
    bool expected = false;
    if (!task->statusDelivered.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return;
    if (task->statusCallback)
        task->statusCallback(task->ipAddress, statusCode);
}

void RemotePrinterManager::finishTask(const std::shared_ptr<ManagedUploadTask>& task,
                                      UploadTaskState state,
                                      int statusCode)
{
    bool expected = false;
    if (!task->terminal.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return;

    task->state.store(state, std::memory_order_release);
    deliverStatus(task, statusCode);
}

void RemotePrinterManager::unregisterTask(const std::shared_ptr<ManagedUploadTask>& task)
{
    std::lock_guard<std::mutex> lock(m_mtxUpload);
    m_tasksById.erase(task->taskId);
    auto latest = m_latestTaskByAddress.find(task->ipAddress);
    if (latest != m_latestTaskByAddress.end() && latest->second == task->taskId)
        m_latestTaskByAddress.erase(latest);
}

void RemotePrinterManager::runUploadTask(const std::shared_ptr<ManagedUploadTask>& task)
{
    auto expected_state = UploadTaskState::Queued;
    if (task->terminal.load(std::memory_order_acquire) ||
        !task->state.compare_exchange_strong(
            expected_state, UploadTaskState::Running, std::memory_order_acq_rel)) {
        unregisterTask(task);
        return;
    }

    try {
        pushFile(task);
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Upload task dispatch failed, task=" << task->taskId
                                 << ", error=" << e.what();
        finishTask(task,
                   is_upload_cancelled(task->cancelToken) ? UploadTaskState::Cancelled
                                                          : UploadTaskState::Failed,
                   is_upload_cancelled(task->cancelToken) ? 601 : 1000);
    } catch (...) {
        finishTask(task,
                   is_upload_cancelled(task->cancelToken) ? UploadTaskState::Cancelled
                                                          : UploadTaskState::Failed,
                   is_upload_cancelled(task->cancelToken) ? 601 : 1000);
    }
    unregisterTask(task);
    if (task->backgroundCleanup.exchange(false, std::memory_order_acq_rel)) {
        m_preparingCleanupTasks.fetch_sub(1, std::memory_order_acq_rel);
        BOOST_LOG_TRIVIAL(warning)
            << "[UploadCancel] background preparation cleanup completed, task=" << task->taskId;
    }
}

void RemotePrinterManager::pushFile(const std::shared_ptr<ManagedUploadTask>& task)
{
    const RemotePrinerType printerType = determinePrinterType(task->ipAddress);

    auto progressCallback = [task](float progress, double speed) {
        if (!task->terminal.load(std::memory_order_acquire) &&
            !is_upload_cancelled(task->cancelToken) && task->progressCallback) {
            task->progressCallback(task->ipAddress, progress, speed);
        }
    };
    auto statusCallback = [task](int statusCode) {
        // Protocol callbacks may run before their request objects and file handles
        // are destroyed. Record the result here and publish it only after the
        // protocol future has completed.
        task->reportedStatus.store(statusCode, std::memory_order_relaxed);
        task->statusReported.store(true, std::memory_order_release);
    };
    auto completeCallback = [task](std::string body) {
        const bool protocolFailed =
            task->statusReported.load(std::memory_order_acquire) &&
            task->reportedStatus.load(std::memory_order_relaxed) != 0;
        if (!task->terminal.load(std::memory_order_acquire) &&
            !is_upload_cancelled(task->cancelToken) && !protocolFailed && task->completeCallback) {
            task->completeCallback(task->ipAddress, std::move(body));
        }
    };

    std::future<void> future;
    switch (printerType) {
    case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER:
        future = m_pKlipperInterface->sendFileToDevice(
            task->ipAddress, getKlipperPrinterMap(task->ipAddress), task->fileName, task->filePath,
            progressCallback, statusCallback, completeCallback, task->cancelToken);
        break;
    case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
        future = m_pKlipper4408Interface->sendFileToDevice(
            task->ipAddress, 80, task->fileName, task->filePath,
            progressCallback, statusCallback, completeCallback, task->cancelToken);
        break;
    case RemotePrinerType::REMOTE_PRINTER_TYPE_CX:
        future = m_pKlipperCXInterface->sendFileToDevice(
            task->ipAddress, 80, task->fileName, task->filePath,
            progressCallback, statusCallback, completeCallback, task->cancelToken);
        break;
    case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
        if (m_pLanPrinterInterface) {
            future = m_pLanPrinterInterface->sendFileToDevice(
                task->ipAddress, task->fileName, task->filePath,
                progressCallback, statusCallback, completeCallback, task->cancelToken);
        } else {
            finishTask(task, UploadTaskState::Failed, 1000);
            return;
        }
        break;
    case RemotePrinerType::REMOTE_PRINTER_TYPE_OCTOPRINT:
        if (m_pOctoPrinterInterface)
            m_pOctoPrinterInterface->sendFileToDevice(task->ipAddress, "", task->filePath);
        finishTask(task, m_pOctoPrinterInterface ? UploadTaskState::Succeeded : UploadTaskState::Failed,
                   m_pOctoPrinterInterface ? 0 : 1000);
        return;
    default:
        finishTask(task, UploadTaskState::Failed, 1000);
        return;
    }

    while (future.valid() && future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
        if (m_bExit.load(std::memory_order_relaxed))
            request_upload_cancel(task->cancelToken);
    }

    if (future.valid()) {
        try {
            future.get();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Upload task failed with exception, task: " << task->taskId
                                     << ", error: " << e.what();
            if (is_upload_cancelled(task->cancelToken))
                finishTask(task, UploadTaskState::Cancelled, 601);
            else
                finishTask(task, UploadTaskState::Failed, 1000);
            return;
        } catch (...) {
            finishTask(task,
                       is_upload_cancelled(task->cancelToken) ? UploadTaskState::Cancelled : UploadTaskState::Failed,
                       is_upload_cancelled(task->cancelToken) ? 601 : 1000);
            return;
        }
    }

    if (is_upload_cancelled(task->cancelToken)) {
        if (is_upload_file_in_use(task->cancelToken)) {
            BOOST_LOG_TRIVIAL(error)
                << "[UploadCancel] protocol returned while file is still marked in use, task="
                << task->taskId;
        } else {
            BOOST_LOG_TRIVIAL(warning)
                << "[UploadCancel] worker stopped and file released, task=" << task->taskId;
        }
        finishTask(task, UploadTaskState::Cancelled, 601);
    } else if (!task->terminal.load(std::memory_order_acquire)) {
        const bool statusReported = task->statusReported.load(std::memory_order_acquire);
        const int statusCode = task->reportedStatus.load(std::memory_order_relaxed);
        if (statusReported && statusCode != 0)
            finishTask(task, UploadTaskState::Failed, statusCode);
        else
            finishTask(task, UploadTaskState::Succeeded, 0);
    }
}

RemotePrinerType RemotePrinterManager::determinePrinterType(const std::string& ipAddress)
{
    std::lock_guard<std::mutex> lock(m_mtxPrinterMeta);
    bool isExists = (std::find(oldPrinters.begin(),oldPrinters.end(), ipAddress) != oldPrinters.end());
    if(isExists)
        return RemotePrinerType::REMOTE_PRINTER_TYPE_LAN;

    for (const auto& pair : mapKlipperPort)
    {
        if (pair.first == ipAddress)
        {
            return RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER;
        }
    }

    if(ipAddress.find('.') !=-1)
        return RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408;

    return RemotePrinerType::REMOTE_PRINTER_TYPE_CX;
}
void RemotePrinterManager::retryUpload(const std::string& ipAddress)
{
    UploadTask task;
    {
        std::lock_guard<std::mutex> lock(m_mtxUpload);
        auto it = m_lastUploadMap.find(ipAddress);
        if (it == m_lastUploadMap.end()) {
            std::cerr << "[RetryUpload] No previous upload task found for IP: " << ipAddress << std::endl;
            return;
        }
        task = it->second;
    }

    pushUploadTasks(ipAddress, task.fileName, task.filePath, task.progressCallback,
                    task.uploadStatusCallback, task.onCompleteCallback);
}

}
