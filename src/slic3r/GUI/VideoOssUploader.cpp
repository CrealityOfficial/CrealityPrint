#include "VideoOssUploader.hpp"

#include "UploadFile.hpp"

#include <alibabacloud/oss/OssClient.h>

#include <boost/nowide/fstream.hpp>

#include <algorithm>
#include <cctype>
#include <exception>
#include <limits>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

namespace Slic3r {
namespace GUI {

namespace {

constexpr std::uint64_t MIN_PART_SIZE_BYTES = 100ULL * 1024ULL;
constexpr int           MAX_PART_COUNT      = 10000;

std::string make_file_key(const std::string &object_key)
{
    const std::string::size_type slash = object_key.find_last_of("/\\");
    std::string                  file_key = object_key.substr(slash == std::string::npos ? 0 : slash + 1);

    if (file_key.size() >= 4) {
        std::string extension = file_key.substr(file_key.size() - 4);
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (extension == ".mp4")
            file_key.resize(file_key.size() - 4);
    }

    return file_key;
}

bool is_not_found_error(const AlibabaCloud::OSS::OssError &error)
{
    const std::string &code = error.Code();
    return code == "NoSuchKey" || code == "NoSuchObject" || code == "NotFound" ||
           code == "404" || code == "ServerError:404" ||
           (code.size() > 4 && code.compare(code.size() - 4, 4, ":404") == 0);
}

void set_sdk_error(VideoOssUploader::Result       &result,
                   const char                     *operation,
                   const AlibabaCloud::OSS::OssError &error)
{
    result.status        = VideoOssUploader::Status::Failed;
    result.error_code    = error.Code();
    result.error_message = std::string(operation) + " failed";
    if (!error.Message().empty())
        result.error_message += ": " + error.Message();
    result.request_id = error.RequestId();
}

void set_local_error(VideoOssUploader::Result &result,
                     std::string              code,
                     std::string              message)
{
    result.status        = VideoOssUploader::Status::Failed;
    result.error_code    = std::move(code);
    result.error_message = std::move(message);
}

void set_canceled(VideoOssUploader::Result &result)
{
    result.status        = VideoOssUploader::Status::Canceled;
    result.error_code    = "Canceled";
    result.error_message = "Video OSS upload was canceled.";
}

void abort_multipart_upload(AlibabaCloud::OSS::OssClient &client,
                            const std::string             &bucket,
                            const std::string             &object_key,
                            const std::string             &upload_id,
                            VideoOssUploader::Result      &result) noexcept
{
    if (upload_id.empty())
        return;

    try {
        AlibabaCloud::OSS::AbortMultipartUploadRequest request(bucket, object_key, upload_id);
        auto outcome = client.AbortMultipartUpload(request);
        if (!outcome.isSuccess()) {
            result.abort_error_code    = outcome.error().Code();
            result.abort_error_message = outcome.error().Message();
        }
    } catch (const std::exception &exception) {
        result.abort_error_code    = "AbortException";
        result.abort_error_message = exception.what();
    } catch (...) {
        result.abort_error_code    = "AbortException";
        result.abort_error_message = "Unknown exception while aborting multipart upload.";
    }
}

class MultipartAbortGuard
{
public:
    MultipartAbortGuard(AlibabaCloud::OSS::OssClient &client,
                        const std::string             &bucket,
                        const std::string             &object_key,
                        const std::string             &upload_id,
                        VideoOssUploader::Result      &result)
        : m_client(client)
        , m_bucket(bucket)
        , m_object_key(object_key)
        , m_upload_id(upload_id)
        , m_result(result)
    {
    }

    ~MultipartAbortGuard() { abort(); }

    MultipartAbortGuard(const MultipartAbortGuard &) = delete;
    MultipartAbortGuard &operator=(const MultipartAbortGuard &) = delete;

    void abort() noexcept
    {
        if (!m_active)
            return;
        m_active = false;
        abort_multipart_upload(m_client, m_bucket, m_object_key, m_upload_id, m_result);
    }

    void dismiss() noexcept { m_active = false; }

private:
    AlibabaCloud::OSS::OssClient &m_client;
    const std::string            &m_bucket;
    const std::string            &m_object_key;
    const std::string            &m_upload_id;
    VideoOssUploader::Result     &m_result;
    bool                          m_active = true;
};

} // namespace

class VideoOssUploader::ActiveClientGuard final
{
public:
    ActiveClientGuard(VideoOssUploader &uploader, AlibabaCloud::OSS::OssClient &client)
        : m_uploader(uploader)
        , m_client(client)
    {
        m_uploader.set_active_client(m_client);
    }

    ~ActiveClientGuard() { m_uploader.clear_active_client(m_client); }

    ActiveClientGuard(const ActiveClientGuard &) = delete;
    ActiveClientGuard &operator=(const ActiveClientGuard &) = delete;

private:
    VideoOssUploader              &m_uploader;
    AlibabaCloud::OSS::OssClient &m_client;
};

VideoOssUploader::VideoOssUploader(const CloudOssConfig &config)
    : VideoOssUploader(config, Options())
{
}

VideoOssUploader::VideoOssUploader(const CloudOssConfig &config, const Options &options)
    : m_token(config.token)
    , m_access_key_id(config.access_key_id)
    , m_secret_access_key(config.secret_access_key)
    , m_endpoint(config.endpoint)
    , m_video_bucket(config.video_bucket)
    , m_options(options)
{
}

VideoOssUploader::Result VideoOssUploader::check_object(const std::string &object_key)
{
    Result result;
    result.object_key = object_key;
    result.file_key = make_file_key(object_key);

    if (m_access_key_id.empty() || m_secret_access_key.empty() || m_token.empty() ||
        m_endpoint.empty() || m_video_bucket.empty()) {
        set_local_error(result, "InvalidConfig", "The OSS video upload configuration is incomplete.");
        return result;
    }
    if (object_key.empty() || result.file_key.empty()) {
        set_local_error(result, "InvalidObjectKey", "The OSS object key is empty or has no file name.");
        return result;
    }
    if (m_options.connect_timeout_ms <= 0 || m_options.head_request_timeout_ms <= 0) {
        set_local_error(result, "InvalidTimeout", "OSS HEAD connect and request timeouts must be positive.");
        return result;
    }
    if (m_cancel_requested.load(std::memory_order_acquire)) {
        set_canceled(result);
        return result;
    }

    OssSdkGuard sdk_guard;
    AlibabaCloud::OSS::ClientConfiguration client_configuration;
    client_configuration.scheme = AlibabaCloud::OSS::Http::Scheme::HTTPS;
    client_configuration.connectTimeoutMs = m_options.connect_timeout_ms;
    client_configuration.requestTimeoutMs = m_options.head_request_timeout_ms;

    try {
        AlibabaCloud::OSS::OssClient client(m_endpoint,
                                             m_access_key_id,
                                             m_secret_access_key,
                                             m_token,
                                             client_configuration);
        const auto outcome = [&] {
            ActiveClientGuard active_client(*this, client);
            return client.HeadObject(m_video_bucket, object_key);
        }();
        if (m_cancel_requested.load(std::memory_order_acquire)) {
            set_canceled(result);
            return result;
        }
        if (outcome.isSuccess()) {
            result.status = Status::AlreadyExists;
            result.etag = outcome.result().ETag();
            return result;
        }
        if (is_not_found_error(outcome.error())) {
            result.status = Status::NotFound;
            result.error_code = outcome.error().Code();
            result.error_message = outcome.error().Message();
            result.request_id = outcome.error().RequestId();
            return result;
        }
        set_sdk_error(result, "HeadObject", outcome.error());
    } catch (const std::exception &exception) {
        if (m_cancel_requested.load(std::memory_order_acquire))
            set_canceled(result);
        else
            set_local_error(result, "HeadObjectException", exception.what());
    } catch (...) {
        if (m_cancel_requested.load(std::memory_order_acquire))
            set_canceled(result);
        else
            set_local_error(result, "HeadObjectException", "Unknown exception during OSS object check.");
    }
    return result;
}

VideoOssUploader::Result VideoOssUploader::upload(const std::string      &local_path,
                                                  const std::string      &object_key,
                                                  const ProgressCallback &progress_callback)
{
    Result result;
    result.object_key = object_key;
    result.file_key   = make_file_key(object_key);

    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        if (m_uploading) {
            set_local_error(result, "Busy", "A video OSS upload is already running on this uploader.");
            return result;
        }
        m_uploading = true;
    }

    struct UploadStateGuard {
        VideoOssUploader *uploader;
        ~UploadStateGuard() { uploader->finish_upload(); }
    } upload_state_guard {this};

    if (m_access_key_id.empty() || m_secret_access_key.empty() || m_token.empty() ||
        m_endpoint.empty() || m_video_bucket.empty()) {
        set_local_error(result, "InvalidConfig", "The OSS video upload configuration is incomplete.");
        return result;
    }
    if (object_key.empty() || result.file_key.empty()) {
        set_local_error(result, "InvalidObjectKey", "The OSS object key is empty or has no file name.");
        return result;
    }
    if (m_options.part_size_bytes < MIN_PART_SIZE_BYTES ||
        m_options.part_size_bytes > static_cast<std::uint64_t>((std::numeric_limits<std::streamsize>::max)()) ||
        m_options.part_size_bytes > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
        set_local_error(result, "InvalidPartSize", "The multipart size must be at least 100 KiB and fit in streamsize.");
        return result;
    }
    if (m_options.connect_timeout_ms <= 0 || m_options.head_request_timeout_ms <= 0 ||
        m_options.abort_request_timeout_ms <= 0 || m_options.request_timeout_ms <= 0) {
        set_local_error(result, "InvalidTimeout", "OSS connect and request timeouts must be positive.");
        return result;
    }
    if (m_cancel_requested.load(std::memory_order_acquire)) {
        set_canceled(result);
        return result;
    }

    OssSdkGuard sdk_guard;

    AlibabaCloud::OSS::ClientConfiguration head_client_configuration;
    head_client_configuration.scheme           = AlibabaCloud::OSS::Http::Scheme::HTTPS;
    head_client_configuration.connectTimeoutMs = m_options.connect_timeout_ms;
    head_client_configuration.requestTimeoutMs = m_options.head_request_timeout_ms;

    try {
        AlibabaCloud::OSS::OssClient head_client(m_endpoint,
                                                  m_access_key_id,
                                                  m_secret_access_key,
                                                  m_token,
                                                  head_client_configuration);

        auto head_outcome = [&] {
            ActiveClientGuard active_client(*this, head_client);
            return head_client.HeadObject(m_video_bucket, object_key);
        }();
        if (m_cancel_requested.load(std::memory_order_acquire)) {
            set_canceled(result);
            return result;
        }
        if (head_outcome.isSuccess()) {
            result.status = Status::AlreadyExists;
            result.etag   = head_outcome.result().ETag();
            if (progress_callback) {
                try {
                    progress_callback(Progress {0, 0, 0, 0, 100.0});
                } catch (...) {
                }
            }
            return result;
        }
        if (!is_not_found_error(head_outcome.error())) {
            set_sdk_error(result, "HeadObject", head_outcome.error());
            return result;
        }

        boost::nowide::ifstream input(local_path, std::ios::binary | std::ios::ate);
        if (!input.is_open()) {
            set_local_error(result, "FileOpenFailed", "Failed to open the local MP4 file: " + local_path);
            return result;
        }

        const std::streamoff file_size_value = input.tellg();
        if (file_size_value <= 0) {
            set_local_error(result, "InvalidFile", "The local MP4 file is empty or its size cannot be determined.");
            return result;
        }
        const std::uint64_t file_size = static_cast<std::uint64_t>(file_size_value);
        const std::uint64_t part_count_value =
            (file_size + m_options.part_size_bytes - 1) / m_options.part_size_bytes;
        if (part_count_value == 0 || part_count_value > MAX_PART_COUNT) {
            set_local_error(result, "TooManyParts", "The MP4 file requires more than 10000 multipart segments.");
            return result;
        }
        const int total_parts = static_cast<int>(part_count_value);
        input.seekg(0, std::ios::beg);

        AlibabaCloud::OSS::ClientConfiguration client_configuration;
        client_configuration.scheme           = AlibabaCloud::OSS::Http::Scheme::HTTPS;
        client_configuration.connectTimeoutMs = m_options.connect_timeout_ms;
        client_configuration.requestTimeoutMs = m_options.request_timeout_ms;
        AlibabaCloud::OSS::OssClient client(m_endpoint,
                                             m_access_key_id,
                                             m_secret_access_key,
                                             m_token,
                                             client_configuration);
        AlibabaCloud::OSS::ClientConfiguration abort_client_configuration;
        abort_client_configuration.scheme           = AlibabaCloud::OSS::Http::Scheme::HTTPS;
        abort_client_configuration.connectTimeoutMs = m_options.connect_timeout_ms;
        abort_client_configuration.requestTimeoutMs = m_options.abort_request_timeout_ms;
        AlibabaCloud::OSS::OssClient abort_client(m_endpoint,
                                                   m_access_key_id,
                                                   m_secret_access_key,
                                                   m_token,
                                                   abort_client_configuration);

        auto report_progress = [&](std::uint64_t transferred_bytes,
                                   int           current_part,
                                   double        percentage,
                                   std::string  &callback_error) -> bool {
            if (!progress_callback)
                return true;
            try {
                progress_callback(Progress {transferred_bytes,
                                            file_size,
                                            current_part,
                                            total_parts,
                                            std::max(0.0, std::min(100.0, percentage))});
                return true;
            } catch (const std::exception &exception) {
                callback_error = exception.what();
            } catch (...) {
                callback_error = "The progress callback threw an unknown exception.";
            }
            return false;
        };

        std::string callback_error;
        if (!report_progress(0, 0, 0.0, callback_error)) {
            set_local_error(result, "ProgressCallbackError", callback_error);
            return result;
        }

        AlibabaCloud::OSS::ObjectMetaData metadata;
        metadata.setContentType("video/mp4");
        AlibabaCloud::OSS::InitiateMultipartUploadRequest initiate_request(m_video_bucket,
                                                                            object_key,
                                                                            metadata);
        auto initiate_outcome = [&] {
            ActiveClientGuard active_client(*this, client);
            return client.InitiateMultipartUpload(initiate_request);
        }();
        if (!initiate_outcome.isSuccess()) {
            if (m_cancel_requested.load(std::memory_order_acquire))
                set_canceled(result);
            else
                set_sdk_error(result, "InitiateMultipartUpload", initiate_outcome.error());
            return result;
        }

        const std::string upload_id = initiate_outcome.result().UploadId();
        MultipartAbortGuard abort_guard(abort_client, m_video_bucket, object_key, upload_id, result);
        if (m_cancel_requested.load(std::memory_order_acquire)) {
            set_canceled(result);
            abort_guard.abort();
            return result;
        }
        AlibabaCloud::OSS::PartList parts;
        parts.reserve(static_cast<std::size_t>(total_parts));

        std::uint64_t completed_bytes = 0;
        for (int part_number = 1; part_number <= total_parts; ++part_number) {
            if (m_cancel_requested.load(std::memory_order_acquire)) {
                set_canceled(result);
                abort_guard.abort();
                return result;
            }

            const std::uint64_t remaining = file_size - completed_bytes;
            const std::uint64_t part_size = std::min(m_options.part_size_bytes, remaining);
            std::string         part_data(static_cast<std::size_t>(part_size), '\0');
            input.read(&part_data[0], static_cast<std::streamsize>(part_size));
            if (input.gcount() != static_cast<std::streamsize>(part_size)) {
                set_local_error(result, "FileReadFailed", "Failed to read the complete MP4 multipart segment.");
                abort_guard.abort();
                return result;
            }

            auto content = std::make_shared<std::stringstream>(std::ios::in |
                                                                std::ios::out |
                                                                std::ios::binary);
            content->write(part_data.data(), static_cast<std::streamsize>(part_data.size()));
            content->seekg(0, std::ios::beg);

            AlibabaCloud::OSS::UploadPartRequest part_request(m_video_bucket,
                                                               object_key,
                                                               part_number,
                                                               upload_id,
                                                               content);
            part_request.setContentLength(part_size);

            bool        transfer_callback_failed = false;
            std::string transfer_callback_error;
            AlibabaCloud::OSS::TransferProgress transfer_progress;
            transfer_progress.UserData = nullptr;
            transfer_progress.Handler = [&, completed_bytes, part_number, part_size](
                                             std::size_t,
                                             std::int64_t transferred,
                                             std::int64_t,
                                             void *) {
                if (transfer_callback_failed || m_cancel_requested.load(std::memory_order_acquire))
                    return;

                const std::uint64_t part_transferred = transferred <= 0 ? 0 :
                    std::min<std::uint64_t>(static_cast<std::uint64_t>(transferred), part_size);
                const std::uint64_t overall_transferred = completed_bytes + part_transferred;
                const double percentage = std::min(99.0,
                    100.0 * static_cast<double>(overall_transferred) / static_cast<double>(file_size));
                if (!report_progress(overall_transferred,
                                     part_number,
                                     percentage,
                                     transfer_callback_error)) {
                    transfer_callback_failed = true;
                }
            };
            part_request.setTransferProgress(transfer_progress);

            auto part_outcome = [&] {
                ActiveClientGuard active_client(*this, client);
                return client.UploadPart(part_request);
            }();
            if (transfer_callback_failed) {
                set_local_error(result, "ProgressCallbackError", transfer_callback_error);
                abort_guard.abort();
                return result;
            }
            if (m_cancel_requested.load(std::memory_order_acquire)) {
                set_canceled(result);
                abort_guard.abort();
                return result;
            }
            if (!part_outcome.isSuccess()) {
                set_sdk_error(result, "UploadPart", part_outcome.error());
                abort_guard.abort();
                return result;
            }

            parts.emplace_back(part_number, part_outcome.result().ETag());
            completed_bytes += part_size;
            if (!report_progress(completed_bytes,
                                 part_number,
                                 std::min(99.0,
                                     100.0 * static_cast<double>(completed_bytes) /
                                         static_cast<double>(file_size)),
                                 callback_error)) {
                set_local_error(result, "ProgressCallbackError", callback_error);
                abort_guard.abort();
                return result;
            }
        }

        if (m_cancel_requested.load(std::memory_order_acquire)) {
            set_canceled(result);
            abort_guard.abort();
            return result;
        }

        AlibabaCloud::OSS::CompleteMultipartUploadRequest complete_request(m_video_bucket,
                                                                            object_key,
                                                                            parts,
                                                                            upload_id);
        auto complete_outcome = [&] {
            ActiveClientGuard active_client(*this, client);
            return client.CompleteMultipartUpload(complete_request);
        }();
        if (!complete_outcome.isSuccess()) {
            if (m_cancel_requested.load(std::memory_order_acquire))
                set_canceled(result);
            else
                set_sdk_error(result, "CompleteMultipartUpload", complete_outcome.error());
            abort_guard.abort();
            return result;
        }

        abort_guard.dismiss();
        result.status = Status::Uploaded;
        result.etag   = complete_outcome.result().ETag();
        (void) report_progress(file_size, total_parts, 100.0, callback_error);
        return result;
    } catch (const std::exception &exception) {
        if (m_cancel_requested.load(std::memory_order_acquire))
            set_canceled(result);
        else
            set_local_error(result, "UploadException", exception.what());
        return result;
    } catch (...) {
        if (m_cancel_requested.load(std::memory_order_acquire))
            set_canceled(result);
        else
            set_local_error(result, "UploadException", "Unknown exception during video OSS upload.");
        return result;
    }
}

void VideoOssUploader::cancel()
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_cancel_requested.store(true, std::memory_order_release);
    if (m_active_client != nullptr) {
        try {
            m_active_client->DisableRequest();
        } catch (...) {
        }
    }
}

bool VideoOssUploader::is_uploading() const
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    return m_uploading;
}

void VideoOssUploader::finish_upload()
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_active_client = nullptr;
    m_uploading = false;
}

void VideoOssUploader::set_active_client(AlibabaCloud::OSS::OssClient &client)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_active_client = &client;
    if (m_cancel_requested.load(std::memory_order_acquire)) {
        try {
            client.DisableRequest();
        } catch (...) {
        }
    }
}

void VideoOssUploader::clear_active_client(AlibabaCloud::OSS::OssClient &client)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    if (m_active_client == &client)
        m_active_client = nullptr;
}

} // namespace GUI
} // namespace Slic3r
