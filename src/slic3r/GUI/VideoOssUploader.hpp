#ifndef slic3r_VideoOssUploader_hpp_
#define slic3r_VideoOssUploader_hpp_

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

namespace AlibabaCloud { namespace OSS { class OssClient; }}

namespace Slic3r {
namespace GUI {

struct CloudOssConfig;

class VideoOssUploader
{
public:
    enum class Status {
        Uploaded,
        AlreadyExists,
        NotFound,
        Canceled,
        Failed
    };

    struct Options {
        std::uint64_t part_size_bytes         = 5ULL * 1024ULL * 1024ULL;
        long          connect_timeout_ms      = 10L * 1000L;
        long          head_request_timeout_ms = 15L * 1000L;
        long          abort_request_timeout_ms = 15L * 1000L;
        long          request_timeout_ms      = 60L * 1000L;
    };

    struct Progress {
        std::uint64_t transferred_bytes = 0;
        std::uint64_t total_bytes       = 0;
        int           current_part      = 0;
        int           total_parts       = 0;
        double        percentage        = 0.0;
    };

    struct Result {
        Status      status = Status::Failed;
        std::string object_key;
        std::string file_key;
        std::string etag;
        std::string error_code;
        std::string error_message;
        std::string request_id;
        std::string abort_error_code;
        std::string abort_error_message;

        bool success() const
        {
            return status == Status::Uploaded || status == Status::AlreadyExists;
        }

        bool uploaded() const { return status == Status::Uploaded; }
        bool already_exists() const { return status == Status::AlreadyExists; }
        bool canceled() const { return status == Status::Canceled; }
    };

    using ProgressCallback = std::function<void(const Progress &)>;

    explicit VideoOssUploader(const CloudOssConfig &config);
    VideoOssUploader(const CloudOssConfig &config, const Options &options);
    ~VideoOssUploader() = default;

    VideoOssUploader(const VideoOssUploader &) = delete;
    VideoOssUploader &operator=(const VideoOssUploader &) = delete;
    VideoOssUploader(VideoOssUploader &&) = delete;
    VideoOssUploader &operator=(VideoOssUploader &&) = delete;

    Result upload(const std::string &local_path,
                  const std::string &object_key,
                  const ProgressCallback &progress_callback = ProgressCallback());

    // Performs the same authoritative HEAD check used by upload(), without
    // reading the local video.  NotFound means the caller may proceed to
    // download and upload the object.
    Result check_object(const std::string &object_key);

    void cancel();
    bool is_uploading() const;

private:
    class ActiveClientGuard;

    void set_active_client(AlibabaCloud::OSS::OssClient &client);
    void clear_active_client(AlibabaCloud::OSS::OssClient &client);
    void finish_upload();

private:
    std::string m_token;
    std::string m_access_key_id;
    std::string m_secret_access_key;
    std::string m_endpoint;
    std::string m_video_bucket;
    Options     m_options;

    mutable std::mutex m_state_mutex;
    bool               m_uploading = false;
    AlibabaCloud::OSS::OssClient *m_active_client = nullptr;
    std::atomic<bool>  m_cancel_requested {false};
};

} // namespace GUI
} // namespace Slic3r

#endif
