#include "TimeLapseShareManager.hpp"

#include "DeviceVideoDownloader.hpp"
#include "slic3r/GUI/UploadFile.hpp"
#include "slic3r/GUI/VideoOssUploader.hpp"

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cctype>
#include <cstddef>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Slic3r { namespace GUI { namespace TimeLapseShare {
namespace {

namespace fs = boost::filesystem;
constexpr std::size_t MAX_QUEUED_JOBS = 16;

bool IsSafeObjectKeyComponent(const std::string& value)
{
    if (value.empty() || value.size() > 256)
        return false;

    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return ch >= 0x20 && ch != 0x7f && ch != '/' && ch != '\\';
    });
}

std::string ShareTitle(const ShareRequest& request)
{
    std::string title = !request.gcode_name.empty() ? request.gcode_name : request.video_name;
    if (title.empty())
        title = request.video_id;

    if (title.size() >= 6) {
        std::string suffix = title.substr(title.size() - 6);
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (suffix == ".gcode")
            title.resize(title.size() - 6);
    }
    return title;
}

class TemporaryVideo final
{
public:
    TemporaryVideo()
    {
        const auto id = boost::uuids::random_generator()();
        m_path = fs::temp_directory_path() / ("crealityprint-timelapse-" + boost::uuids::to_string(id) + ".mp4");
    }

    ~TemporaryVideo()
    {
        Remove(m_path);
        Remove(fs::path(m_path.string() + ".part"));
    }

    TemporaryVideo(const TemporaryVideo&) = delete;
    TemporaryVideo& operator=(const TemporaryVideo&) = delete;

    const fs::path& path() const { return m_path; }

private:
    static void Remove(const fs::path& path) noexcept
    {
        boost::system::error_code ec;
        fs::remove(path, ec);
    }

private:
    fs::path m_path;
};

} // namespace

struct TimeLapseShareManager::Impl
{
    struct Job
    {
        explicit Job(ShareRequest value) : request(std::move(value)) {}

        ShareRequest request;
        std::atomic<bool> cancel_requested {false};
        bool started {false};
        bool terminal {false};
        std::mutex operation_mutex;
        UploadFile* cloud_info_provider {nullptr};
        VideoOssUploader* uploader {nullptr};
        std::chrono::steady_clock::time_point last_progress_time {};
        std::string last_progress_stage;
        int last_progress_percentage {-2};
        std::uint64_t last_progress_transferred {0};
    };

    explicit Impl(EventCallback callback, AccountSessionProvider provider)
        : event_callback(std::move(callback))
        , account_session_provider(std::move(provider))
        , worker([this] { WorkerLoop(); })
    {
    }

    StartResult Start(ShareRequest request)
    {
        StartResult result;
        if (!request.IsValid()) {
            result.error_code = "invalid_request";
            result.error_message = "requestId, address, userId, video and videoid are required";
            return result;
        }
        if (!IsSafeObjectKeyComponent(request.user_id) || !IsSafeObjectKeyComponent(request.video_id)) {
            result.error_code = "invalid_object_identity";
            result.error_message = "userId and videoid must not contain control characters or path separators";
            return result;
        }

        auto job = std::make_shared<Job>(std::move(request));
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (stopping) {
                result.error_code = "manager_stopped";
                result.error_message = "The time-lapse share manager is shutting down";
                return result;
            }
            if (jobs.find(job->request.request_id) != jobs.end()) {
                result.error_code = "duplicate_request";
                result.error_message = "A time-lapse share task with this requestId already exists";
                return result;
            }
            if (jobs.size() >= MAX_QUEUED_JOBS) {
                result.error_code = "queue_full";
                result.error_message = "Too many time-lapse share tasks are already queued";
                return result;
            }
            jobs.emplace(job->request.request_id, job);
            queue.emplace_back(job);
        }
        condition.notify_one();
        result.accepted = true;
        return result;
    }

    bool Cancel(const std::string& request_id)
    {
        std::shared_ptr<Job> job;
        bool cancelled_while_queued = false;
        {
            std::lock_guard<std::mutex> lock(mutex);
            const auto it = jobs.find(request_id);
            if (it == jobs.end())
                return false;
            job = it->second;
            if (job->terminal)
                return false;
            job->cancel_requested.store(true, std::memory_order_release);
            if (!job->started) {
                const auto queue_it = std::find(queue.begin(), queue.end(), job);
                if (queue_it != queue.end())
                    queue.erase(queue_it);
                job->terminal = true;
                jobs.erase(it);
                cancelled_while_queued = true;
            }
        }

        if (cancelled_while_queued) {
            EmitCancelledEvent(*job);
            condition.notify_one();
            return true;
        }

        CancelActiveOperations(job);
        condition.notify_one();
        return true;
    }

    static void CancelActiveOperations(const std::shared_ptr<Job>& job)
    {
        std::lock_guard<std::mutex> operation_lock(job->operation_mutex);
        if (job->cloud_info_provider != nullptr)
            job->cloud_info_provider->setCancel(true);
        if (job->uploader != nullptr)
            job->uploader->cancel();
    }

    void Shutdown()
    {
        std::vector<std::shared_ptr<Job>> pending_jobs;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!stopping) {
                stopping = true;
                pending_jobs.reserve(jobs.size());
                for (auto& item : jobs) {
                    if (item.second->terminal)
                        continue;
                    item.second->cancel_requested.store(true, std::memory_order_release);
                    pending_jobs.emplace_back(item.second);
                }
            }
        }

        for (const auto& job : pending_jobs)
            CancelActiveOperations(job);
        condition.notify_all();
        if (worker.joinable())
            worker.join();
    }

    void WorkerLoop()
    {
        for (;;) {
            std::shared_ptr<Job> job;
            {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock, [this] { return stopping || !queue.empty(); });
                if (queue.empty()) {
                    if (stopping)
                        break;
                    continue;
                }
                job = queue.front();
                queue.pop_front();
                job->started = true;
            }

            try {
                Process(job);
            } catch (const std::exception& exception) {
                EmitError(*job, "share_task_exception", exception.what());
            } catch (...) {
                EmitError(*job, "share_task_exception", "Unknown time-lapse share task error");
            }

            std::lock_guard<std::mutex> lock(mutex);
            jobs.erase(job->request.request_id);
        }
    }

    void Process(const std::shared_ptr<Job>& job)
    {
        EmitProgress(*job, "prepare", 0, 0, 0, true);
        if (IsCancelled(*job)) {
            EmitCancelled(*job);
            return;
        }
        if (!EnsureCurrentAccount(*job))
            return;

        CloudOssConfig oss_config;
        std::string oss_config_error;
        {
            UploadFile cloud_info_provider(job->request.request_headers);
            {
                std::lock_guard<std::mutex> operation_lock(job->operation_mutex);
                job->cloud_info_provider = &cloud_info_provider;
                if (IsCancelled(*job))
                    cloud_info_provider.setCancel(true);
            }
            struct CloudInfoRegistration {
                std::shared_ptr<Job> job;
                UploadFile* provider;
                ~CloudInfoRegistration()
                {
                    std::lock_guard<std::mutex> lock(job->operation_mutex);
                    if (job->cloud_info_provider == provider)
                        job->cloud_info_provider = nullptr;
                }
            } cloud_info_registration {job, &cloud_info_provider};

            try {
                if (cloud_info_provider.getCloudUploadInfo(oss_config) != 0 || !oss_config.valid_for_video())
                    oss_config_error = "Unable to obtain a complete OSS video upload configuration";
            } catch (const ErrorCodeException& exception) {
                oss_config_error = exception.what();
            } catch (const std::exception& exception) {
                oss_config_error = exception.what();
            } catch (...) {
                oss_config_error = "Unknown error while obtaining OSS upload configuration";
            }
        }

        if (IsCancelled(*job)) {
            EmitCancelled(*job);
            return;
        }
        if (!EnsureCurrentAccount(*job))
            return;
        if (!oss_config_error.empty()) {
            EmitError(*job, "oss_config_failed", oss_config_error);
            return;
        }

        const std::string object_key = "cp/timeapse/" + job->request.user_id + '_' + job->request.video_id + ".mp4";
        VideoOssUploader uploader(oss_config);
        {
            std::lock_guard<std::mutex> operation_lock(job->operation_mutex);
            job->uploader = &uploader;
            if (IsCancelled(*job))
                uploader.cancel();
        }
        struct UploaderRegistration {
            std::shared_ptr<Job> job;
            VideoOssUploader* uploader;
            ~UploaderRegistration()
            {
                std::lock_guard<std::mutex> lock(job->operation_mutex);
                if (job->uploader == uploader)
                    job->uploader = nullptr;
            }
        } uploader_registration {job, &uploader};

        EmitProgress(*job, "check", 0, 0, 0, true);
        const VideoOssUploader::Result check_result = uploader.check_object(object_key);
        if (IsCancelled(*job)) {
            EmitCancelled(*job);
            return;
        }
        if (!EnsureCurrentAccount(*job))
            return;
        if (check_result.already_exists()) {
            if (!EnsureCurrentAccount(*job))
                return;
            EmitComplete(*job, check_result, true, true);
            return;
        }
        if (check_result.status != VideoOssUploader::Status::NotFound) {
            EmitUploadError(*job, check_result, "oss_check_failed");
            return;
        }

        TemporaryVideo temporary_video;
        DownloadRequest download_request;
        download_request.address = job->request.address;
        download_request.video_path = job->request.video;
        download_request.output_path = temporary_video.path().string();
        download_request.ca_file = job->request.ca_file;
        download_request.secure_connection = job->request.secure_connection;

        EmitProgress(*job, "download", 0, 0, 0, true);
        const DownloadResult download_result = DeviceVideoDownloader::Download(
            download_request,
            job->cancel_requested,
            [this, job](const DownloadProgress& progress) {
                EmitProgress(*job,
                             "download",
                             progress.percentage,
                             progress.bytes_downloaded,
                             progress.total_bytes);
            });

        if (download_result.cancelled || IsCancelled(*job)) {
            EmitCancelled(*job);
            return;
        }
        if (!download_result.success) {
            EmitError(*job, ToString(download_result.error_code), download_result.error_message);
            return;
        }

        if (IsCancelled(*job)) {
            EmitCancelled(*job);
            return;
        }
        if (!EnsureCurrentAccount(*job))
            return;

        EmitProgress(*job, "upload", 0, 0, download_result.bytes_written, true);
        const VideoOssUploader::Result upload_result = uploader.upload(
            temporary_video.path().string(),
            object_key,
            [this, job, &uploader](const VideoOssUploader::Progress& progress) {
                if (IsCancelled(*job)) {
                    uploader.cancel();
                    return;
                }
                EmitProgress(*job,
                             "upload",
                             static_cast<int>(progress.percentage),
                             progress.transferred_bytes,
                             progress.total_bytes);
            });

        if (upload_result.uploaded()) {
            if (!EnsureCurrentAccount(*job))
                return;
            EmitComplete(*job, upload_result, false, false);
            return;
        }
        if (upload_result.canceled() || IsCancelled(*job)) {
            EmitCancelled(*job);
            return;
        }
        if (!EnsureCurrentAccount(*job))
            return;
        if (!upload_result.success()) {
            EmitUploadError(*job, upload_result, "oss_upload_failed");
            return;
        }
        EmitComplete(*job, upload_result, upload_result.already_exists(), true);
    }

    static bool IsCancelled(const Job& job)
    {
        return job.cancel_requested.load(std::memory_order_acquire);
    }

    bool EnsureCurrentAccount(Job& job)
    {
        bool matches = false;
        try {
            matches = account_session_provider &&
                      account_session_provider() == job.request.account_session;
        } catch (...) {
        }
        if (matches)
            return true;

        EmitError(job,
                  "account_changed",
                  "The active Creality Cloud account changed while sharing the time-lapse video");
        return false;
    }

    bool ClaimTerminal(Job& job,
                       EventType requested_type,
                       bool cancellation_wins,
                       EventType& resolved_type)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (job.terminal)
            return false;

        resolved_type = requested_type;
        if (cancellation_wins && requested_type != EventType::Cancelled && IsCancelled(job))
            resolved_type = EventType::Cancelled;
        job.terminal = true;
        return true;
    }

    void EmitProgress(Job& job,
                      const std::string& stage,
                      int percentage,
                      std::uint64_t transferred,
                      std::uint64_t total,
                      bool force = false)
    {
        if (IsCancelled(job))
            return;

        const auto now = std::chrono::steady_clock::now();
        const bool stage_changed = stage != job.last_progress_stage;
        const bool percentage_changed = percentage != job.last_progress_percentage;
        const bool bytes_changed = transferred != job.last_progress_transferred;
        const bool interval_elapsed = job.last_progress_time.time_since_epoch().count() == 0 ||
                                      now - job.last_progress_time >= std::chrono::milliseconds(150);
        if (!force && !stage_changed && percentage != 100) {
            const bool value_changed = percentage >= 0 ? percentage_changed : bytes_changed;
            if (!value_changed || !interval_elapsed)
                return;
        }

        job.last_progress_stage = stage;
        job.last_progress_percentage = percentage;
        job.last_progress_transferred = transferred;
        job.last_progress_time = now;

        ShareEvent event;
        event.type = EventType::Progress;
        event.request_id = job.request.request_id;
        event.stage = stage;
        event.percentage = percentage;
        event.transferred_bytes = transferred;
        event.total_bytes = total;
        Emit(event);
    }

    void EmitComplete(Job& job,
                      const VideoOssUploader::Result& result,
                      bool already_existed,
                      bool cancellation_wins)
    {
        EventType resolved_type;
        if (!ClaimTerminal(job, EventType::Complete, cancellation_wins, resolved_type))
            return;
        if (resolved_type == EventType::Cancelled) {
            EmitCancelledEvent(job);
            return;
        }

        ShareEvent event;
        event.type = EventType::Complete;
        event.request_id = job.request.request_id;
        event.stage = "complete";
        event.percentage = 100;
        event.object_key = result.object_key;
        event.file_key = result.file_key;
        event.title = ShareTitle(job.request);
        event.already_existed = already_existed;
        Emit(event);
    }

    void EmitUploadError(Job& job,
                         const VideoOssUploader::Result& result,
                         const std::string& fallback_code)
    {
        std::string message = result.error_message.empty() ? "OSS video operation failed" : result.error_message;
        if (!result.abort_error_message.empty())
            message += "; abort failed: " + result.abort_error_message;
        EmitError(job, result.error_code.empty() ? fallback_code : result.error_code, message);
    }

    void EmitError(Job& job, const std::string& code, const std::string& message)
    {
        EventType resolved_type;
        if (!ClaimTerminal(job, EventType::Error, true, resolved_type))
            return;
        if (resolved_type == EventType::Cancelled) {
            EmitCancelledEvent(job);
            return;
        }

        ShareEvent event;
        event.type = EventType::Error;
        event.request_id = job.request.request_id;
        event.stage = "error";
        event.error_code = code;
        event.error_message = message;
        Emit(event);
    }

    void EmitCancelled(Job& job)
    {
        EventType resolved_type;
        if (!ClaimTerminal(job, EventType::Cancelled, true, resolved_type))
            return;
        EmitCancelledEvent(job);
    }

    void EmitCancelledEvent(const Job& job)
    {
        ShareEvent event;
        event.type = EventType::Cancelled;
        event.request_id = job.request.request_id;
        event.stage = "cancelled";
        Emit(event);
    }

    void Emit(const ShareEvent& event) const noexcept
    {
        if (!event_callback)
            return;
        try {
            event_callback(event);
        } catch (...) {
        }
    }

    EventCallback event_callback;
    AccountSessionProvider account_session_provider;
    std::mutex mutex;
    std::condition_variable condition;
    std::deque<std::shared_ptr<Job>> queue;
    std::unordered_map<std::string, std::shared_ptr<Job>> jobs;
    bool stopping {false};
    std::thread worker;
};

TimeLapseShareManager::TimeLapseShareManager(EventCallback event_callback,
                                             AccountSessionProvider account_session_provider)
    : m_impl(new Impl(std::move(event_callback), std::move(account_session_provider)))
{
}

TimeLapseShareManager::~TimeLapseShareManager()
{
    Shutdown();
}

TimeLapseShareManager::StartResult TimeLapseShareManager::Start(ShareRequest request)
{
    if (!m_impl) {
        StartResult result;
        result.error_code = "manager_stopped";
        result.error_message = "The time-lapse share manager is not available";
        return result;
    }
    return m_impl->Start(std::move(request));
}

bool TimeLapseShareManager::Cancel(const std::string& request_id)
{
    return m_impl != nullptr && m_impl->Cancel(request_id);
}

void TimeLapseShareManager::Shutdown()
{
    if (m_impl)
        m_impl->Shutdown();
}

}}} // namespace Slic3r::GUI::TimeLapseShare
