#ifndef slic3r_GUI_TimeLapseShareTypes_hpp_
#define slic3r_GUI_TimeLapseShareTypes_hpp_

#include <cstdint>
#include <functional>
#include <map>
#include <string>

namespace Slic3r { namespace GUI { namespace TimeLapseShare {

enum class DownloadErrorCode {
    None = 0,
    InvalidRequest,
    Cancelled,
    CurlGlobalInitFailed,
    CurlEasyInitFailed,
    UrlEncodingFailed,
    TlsCaFileMissing,
    OutputDirectoryCreateFailed,
    DiskSpaceCheckFailed,
    InsufficientDiskSpace,
    PartFileCleanupFailed,
    PartFileOpenFailed,
    CurlConfigurationFailed,
    ProgressCallbackFailed,
    FileWriteFailed,
    Timeout,
    TlsVerificationFailed,
    NetworkError,
    UnexpectedHttpStatus,
    FileFinalizeFailed
};

const char* ToString(DownloadErrorCode error_code) noexcept;

struct ShareRequest
{
    std::string request_id;
    std::string address;
    std::string user_id;
    std::string account_session;
    std::map<std::string, std::string> request_headers;
    std::string video;
    std::string video_id;
    std::string gcode_name;
    std::string video_name;
    std::string ca_file;
    bool        secure_connection {false};

    bool IsValid() const noexcept;
};

enum class EventType
{
    Progress,
    Complete,
    Error,
    Cancelled
};

struct ShareEvent
{
    EventType   type {EventType::Progress};
    std::string request_id;
    std::string stage;
    int         percentage {-1};
    std::uint64_t transferred_bytes {0};
    std::uint64_t total_bytes {0};

    std::string object_key;
    std::string file_key;
    std::string title;
    bool        already_existed {false};

    std::string error_code;
    std::string error_message;
};

const char* EventCommand(EventType type) noexcept;

struct DownloadRequest
{
    std::string   address;
    std::string   video_path;
    std::string   output_path;
    std::string   ca_file;
    bool          secure_connection{false};
    std::uint16_t http_port{80};
    std::uint16_t https_port{443};
    long          connect_timeout_seconds{10};
    long          total_timeout_seconds{600};
};

struct DownloadProgress
{
    std::uint64_t bytes_downloaded{0};
    std::uint64_t total_bytes{0};
    // -1 means that the device did not provide a content length.
    int percentage{-1};
};

using ProgressCallback = std::function<void(const DownloadProgress&)>;

struct DownloadResult
{
    bool              success{false};
    bool              cancelled{false};
    long              http_status{0};
    std::uint64_t     bytes_written{0};
    DownloadErrorCode error_code{DownloadErrorCode::None};
    std::string       error_message;
};

}}} // namespace Slic3r::GUI::TimeLapseShare

#endif /* slic3r_GUI_TimeLapseShareTypes_hpp_ */
