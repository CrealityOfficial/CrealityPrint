#include "TimeLapseShareTypes.hpp"

namespace Slic3r { namespace GUI { namespace TimeLapseShare {

const char* ToString(DownloadErrorCode error_code) noexcept
{
    switch (error_code) {
    case DownloadErrorCode::None: return "none";
    case DownloadErrorCode::InvalidRequest: return "invalid_request";
    case DownloadErrorCode::Cancelled: return "cancelled";
    case DownloadErrorCode::CurlGlobalInitFailed: return "curl_global_init_failed";
    case DownloadErrorCode::CurlEasyInitFailed: return "curl_easy_init_failed";
    case DownloadErrorCode::UrlEncodingFailed: return "url_encoding_failed";
    case DownloadErrorCode::TlsCaFileMissing: return "tls_ca_file_missing";
    case DownloadErrorCode::OutputDirectoryCreateFailed: return "output_directory_create_failed";
    case DownloadErrorCode::DiskSpaceCheckFailed: return "disk_space_check_failed";
    case DownloadErrorCode::InsufficientDiskSpace: return "insufficient_disk_space";
    case DownloadErrorCode::PartFileCleanupFailed: return "part_file_cleanup_failed";
    case DownloadErrorCode::PartFileOpenFailed: return "part_file_open_failed";
    case DownloadErrorCode::CurlConfigurationFailed: return "curl_configuration_failed";
    case DownloadErrorCode::ProgressCallbackFailed: return "progress_callback_failed";
    case DownloadErrorCode::FileWriteFailed: return "file_write_failed";
    case DownloadErrorCode::Timeout: return "timeout";
    case DownloadErrorCode::TlsVerificationFailed: return "tls_verification_failed";
    case DownloadErrorCode::NetworkError: return "network_error";
    case DownloadErrorCode::UnexpectedHttpStatus: return "unexpected_http_status";
    case DownloadErrorCode::FileFinalizeFailed: return "file_finalize_failed";
    }

    return "unknown";
}

bool ShareRequest::IsValid() const noexcept
{
    return !request_id.empty() && !address.empty() && !user_id.empty() && !account_session.empty() &&
           !video.empty() && !video_id.empty();
}

const char* EventCommand(EventType type) noexcept
{
    switch (type) {
    case EventType::Progress: return "time_lapse_share_progress";
    case EventType::Complete: return "time_lapse_share_complete";
    case EventType::Error: return "time_lapse_share_error";
    case EventType::Cancelled: return "time_lapse_share_cancelled";
    }
    return "time_lapse_share_error";
}

}}} // namespace Slic3r::GUI::TimeLapseShare
