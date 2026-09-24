#include "DeviceVideoDownloader.hpp"

#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/DeviceTlsPolicy.hpp"

#include <boost/filesystem.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/system/errc.hpp>
#include <curl/curl.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <utility>

namespace Slic3r { namespace GUI { namespace TimeLapseShare {
namespace {

namespace fs = boost::filesystem;

std::once_flag g_curl_init_once;
CURLcode       g_curl_init_code = CURLE_OK;

struct CurlHandleDeleter
{
    void operator()(CURL* handle) const noexcept
    {
        if (handle != nullptr)
            curl_easy_cleanup(handle);
    }
};

using CurlHandle = std::unique_ptr<CURL, CurlHandleDeleter>;

class FileGuard final
{
public:
    explicit FileGuard(std::FILE* file = nullptr) noexcept : m_file(file) {}

    ~FileGuard()
    {
        if (m_file != nullptr)
            std::fclose(m_file);
    }

    FileGuard(const FileGuard&)            = delete;
    FileGuard& operator=(const FileGuard&) = delete;

    std::FILE* Get() const noexcept { return m_file; }

    bool Close() noexcept
    {
        if (m_file == nullptr)
            return true;

        std::FILE* file = m_file;
        m_file          = nullptr;
        return std::fclose(file) == 0;
    }

private:
    std::FILE* m_file;
};

struct TransferContext
{
    std::FILE*               file{nullptr};
    const std::atomic<bool>* cancel{nullptr};
    const ProgressCallback*  progress_callback{nullptr};
    std::uint64_t            bytes_written{0};
    std::uint64_t            available_bytes{0};
    curl_off_t               last_downloaded{-1};
    curl_off_t               last_total{-1};
    bool                     insufficient_disk_space{false};
    bool                     write_failed{false};
    bool                     progress_callback_failed{false};
    std::string              callback_error;
    std::string              disk_space_error;
    std::string              write_error;
};

DownloadResult MakeFailure(DownloadErrorCode error_code, std::string message, bool cancelled = false)
{
    DownloadResult result;
    result.cancelled     = cancelled;
    result.error_code    = error_code;
    result.error_message = std::move(message);
    return result;
}

bool IsCancelled(const std::atomic<bool>& cancel) noexcept { return cancel.load(std::memory_order_relaxed); }

std::string TrimAsciiWhitespace(const std::string& value)
{
    constexpr const char* whitespace = " \t\r\n";
    const std::size_t     first      = value.find_first_not_of(whitespace);
    if (first == std::string::npos)
        return {};

    const std::size_t last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

bool NormalizeHost(const std::string& address, std::string& host, std::string& error)
{
    host = TrimAsciiWhitespace(address);
    if (host.empty()) {
        error = "Device address is empty";
        return false;
    }

    if (host.find("://") != std::string::npos || host.find_first_of("/?#@\\") != std::string::npos ||
        std::any_of(host.begin(), host.end(), [](unsigned char ch) { return ch <= 0x20; })) {
        error = "Device address must be a host or IP address without a scheme, port, or path";
        return false;
    }

    if (host.front() == '[') {
        if (host.size() < 3 || host.back() != ']') {
            error = "Invalid bracketed IPv6 device address";
            return false;
        }
        return true;
    }

    const std::size_t colon_count = static_cast<std::size_t>(std::count(host.begin(), host.end(), ':'));
    if (colon_count == 1) {
        error = "Device address must not contain a port; use http_port or https_port";
        return false;
    }
    if (colon_count > 1)
        host = '[' + host + ']';

    return true;
}

bool IsPathNotFoundError(const boost::system::error_code& ec)
{
    return ec.default_error_condition() ==
           boost::system::errc::make_error_condition(boost::system::errc::no_such_file_or_directory);
}

bool RemovePartFile(const fs::path& part_path, std::string& error)
{
    boost::system::error_code ec;
    const bool                exists = fs::exists(part_path, ec);
    if (ec) {
        if (IsPathNotFoundError(ec))
            return true;
        error = "Unable to inspect partial file '" + part_path.string() + "': " + ec.message();
        return false;
    }
    if (!exists)
        return true;

    if (fs::is_directory(part_path, ec)) {
        error = "Partial download path is a directory: " + part_path.string();
        return false;
    }
    if (ec) {
        error = "Unable to inspect partial file type '" + part_path.string() + "': " + ec.message();
        return false;
    }

    fs::remove(part_path, ec);
    if (ec) {
        error = "Unable to remove partial file '" + part_path.string() + "': " + ec.message();
        return false;
    }
    return true;
}

void AppendCleanupError(std::string& message, const fs::path& part_path)
{
    std::string cleanup_error;
    if (!RemovePartFile(part_path, cleanup_error)) {
        if (!message.empty())
            message += "; ";
        message += cleanup_error;
    }
}

std::uint64_t ToUnsignedBytes(curl_off_t value) noexcept { return value > 0 ? static_cast<std::uint64_t>(value) : 0; }

int CalculatePercentage(curl_off_t downloaded, curl_off_t total) noexcept
{
    if (total <= 0)
        return -1;
    if (downloaded <= 0)
        return 0;
    if (downloaded >= total)
        return 100;

    const long double ratio = static_cast<long double>(downloaded) / static_cast<long double>(total);
    return static_cast<int>(ratio * 100.0L);
}

bool ReportProgress(TransferContext& context, curl_off_t downloaded, curl_off_t total)
{
    if (context.progress_callback == nullptr || !*context.progress_callback)
        return true;
    if (downloaded == context.last_downloaded && total == context.last_total)
        return true;

    context.last_downloaded = downloaded;
    context.last_total      = total;

    DownloadProgress progress;
    progress.bytes_downloaded = ToUnsignedBytes(downloaded);
    progress.total_bytes      = ToUnsignedBytes(total);
    progress.percentage       = CalculatePercentage(downloaded, total);

    try {
        (*context.progress_callback)(progress);
        return true;
    } catch (const std::exception& e) {
        context.callback_error = e.what();
    } catch (...) {
        context.callback_error = "Unknown exception from download progress callback";
    }

    context.progress_callback_failed = true;
    return false;
}

size_t CurlWriteCallback(char* data, size_t size, size_t count, void* user_data)
{
    auto* context = static_cast<TransferContext*>(user_data);
    if (context == nullptr || context->file == nullptr)
        return 0;
    if (context->cancel != nullptr && IsCancelled(*context->cancel))
        return 0;
    if (size != 0 && count > (std::numeric_limits<size_t>::max)() / size) {
        context->write_failed = true;
        context->write_error  = "Download chunk size overflow";
        return 0;
    }

    const size_t bytes = size * count;
    if (bytes == 0)
        return 0;

    if (context->bytes_written > context->available_bytes ||
        static_cast<std::uint64_t>(bytes) > context->available_bytes - context->bytes_written) {
        context->insufficient_disk_space = true;
        context->disk_space_error        = "The device video is larger than the available temporary disk space";
        return 0;
    }

    errno                = 0;
    const size_t written = std::fwrite(data, 1, bytes, context->file);
    context->bytes_written += static_cast<std::uint64_t>(written);
    if (written != bytes) {
        context->write_failed = true;
        context->write_error  = errno != 0 ? std::strerror(errno) : "Short write to partial download file";
        return 0;
    }

    return bytes;
}

int CurlXferInfoCallback(
    void* user_data, curl_off_t download_total, curl_off_t download_now, curl_off_t /* upload_total */, curl_off_t /* upload_now */)
{
    auto* context = static_cast<TransferContext*>(user_data);
    if (context == nullptr)
        return 1;
    if (context->cancel != nullptr && IsCancelled(*context->cancel))
        return 1;
    if (download_total > 0 && static_cast<std::uint64_t>(download_total) > context->available_bytes) {
        context->insufficient_disk_space = true;
        context->disk_space_error        = "The device video is larger than the available temporary disk space";
        return 1;
    }
    if (!ReportProgress(*context, download_now, download_total))
        return 1;
    if (context->cancel != nullptr && IsCancelled(*context->cancel))
        return 1;
    return 0;
}


CURLcode CurlSslContextCallback(CURL*, void* ssl_context, void*)
{
    std::string error;
    if (!Slic3r::DeviceTlsPolicy::ignore_certificate_time_from_curl(ssl_context, error))
        return CURLE_SSL_CERTPROBLEM;
    return CURLE_OK;
}
std::string CurlErrorMessage(CURLcode code, const char* error_buffer)
{
    std::ostringstream message;
    message << curl_easy_strerror(code) << " (curl " << static_cast<int>(code) << ')';
    if (error_buffer != nullptr && error_buffer[0] != '\0')
        message << ": " << error_buffer;
    return message.str();
}

DownloadErrorCode ClassifyCurlError(CURLcode code, const TransferContext& context)
{
    if (context.insufficient_disk_space)
        return DownloadErrorCode::InsufficientDiskSpace;
    if (context.progress_callback_failed)
        return DownloadErrorCode::ProgressCallbackFailed;
    if (context.write_failed)
        return DownloadErrorCode::FileWriteFailed;
    if (code == CURLE_OPERATION_TIMEDOUT)
        return DownloadErrorCode::Timeout;
    if (code == CURLE_PEER_FAILED_VERIFICATION || code == CURLE_SSL_CONNECT_ERROR || code == CURLE_SSL_CACERT_BADFILE)
        return DownloadErrorCode::TlsVerificationFailed;
    return DownloadErrorCode::NetworkError;
}

} // namespace

DownloadResult DeviceVideoDownloader::Download(const DownloadRequest&   request,
                                               const std::atomic<bool>& cancel,
                                               ProgressCallback         progress_callback)
{
    if (request.video_path.empty())
        return MakeFailure(DownloadErrorCode::InvalidRequest, "Device video path is empty");
    if (request.output_path.empty())
        return MakeFailure(DownloadErrorCode::InvalidRequest, "Output path is empty");
    const std::uint16_t selected_port = request.secure_connection ? request.https_port : request.http_port;
    if (selected_port == 0)
        return MakeFailure(DownloadErrorCode::InvalidRequest, "Download port must be greater than zero");
    if (request.connect_timeout_seconds <= 0 || request.total_timeout_seconds <= 0)
        return MakeFailure(DownloadErrorCode::InvalidRequest, "Connect and total timeouts must be greater than zero");
    if (request.video_path.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
        return MakeFailure(DownloadErrorCode::InvalidRequest, "Device video path is too long");
    if (IsCancelled(cancel))
        return MakeFailure(DownloadErrorCode::Cancelled, "Download cancelled before it started", true);

    std::string host;
    std::string validation_error;
    if (!NormalizeHost(request.address, host, validation_error))
        return MakeFailure(DownloadErrorCode::InvalidRequest, std::move(validation_error));

    const fs::path output_path(request.output_path);
    if (output_path.filename().empty())
        return MakeFailure(DownloadErrorCode::InvalidRequest, "Output path must include a file name");

    const fs::path output_directory = output_path.parent_path();
    if (!output_directory.empty()) {
        boost::system::error_code ec;
        fs::create_directories(output_directory, ec);
        if (ec) {
            return MakeFailure(DownloadErrorCode::OutputDirectoryCreateFailed,
                               "Unable to create output directory '" + output_directory.string() + "': " + ec.message());
        }
    }

    fs::path    part_path(request.output_path + ".part");
    std::string cleanup_error;
    if (!RemovePartFile(part_path, cleanup_error))
        return MakeFailure(DownloadErrorCode::PartFileCleanupFailed, std::move(cleanup_error));

    boost::system::error_code space_error;
    const fs::path space_path = output_directory.empty() ? fs::current_path(space_error) : output_directory;
    if (space_error) {
        return MakeFailure(DownloadErrorCode::DiskSpaceCheckFailed,
                           "Unable to resolve the temporary download directory: " + space_error.message());
    }
    const fs::space_info space_info = fs::space(space_path, space_error);
    if (space_error) {
        return MakeFailure(DownloadErrorCode::DiskSpaceCheckFailed,
                           "Unable to query temporary disk space for '" + space_path.string() + "': " + space_error.message());
    }
    if (space_info.available == 0) {
        return MakeFailure(DownloadErrorCode::InsufficientDiskSpace,
                           "No temporary disk space is available for the device video");
    }
    const std::uint64_t available_bytes = static_cast<std::uint64_t>(std::min<std::uintmax_t>(
        space_info.available, (std::numeric_limits<std::uint64_t>::max)()));

    std::call_once(g_curl_init_once, [] { g_curl_init_code = curl_global_init(CURL_GLOBAL_DEFAULT); });
    if (g_curl_init_code != CURLE_OK) {
        return MakeFailure(DownloadErrorCode::CurlGlobalInitFailed,
                           "Unable to initialize libcurl: " + CurlErrorMessage(g_curl_init_code, nullptr));
    }

    CurlHandle curl(curl_easy_init());
    if (!curl)
        return MakeFailure(DownloadErrorCode::CurlEasyInitFailed, "Unable to create a libcurl easy handle");

    char* encoded_video_path = curl_easy_escape(curl.get(), request.video_path.data(), static_cast<int>(request.video_path.size()));
    if (encoded_video_path == nullptr)
        return MakeFailure(DownloadErrorCode::UrlEncodingFailed, "Unable to URL-encode the device video path");
    const std::string escaped_video_path(encoded_video_path);
    curl_free(encoded_video_path);

    const std::string scheme = request.secure_connection ? "https" : "http";
    const std::string url    = scheme + "://" + host + ':' + std::to_string(selected_port) + "/downloads/video/" + escaped_video_path;

    std::string ca_file;
    if (request.secure_connection) {
        ca_file = request.ca_file.empty() ? Slic3r::resources_dir() + "/cert/ca.crt" : request.ca_file;
        boost::system::error_code ec;
        if (ca_file.empty() || !fs::is_regular_file(fs::path(ca_file), ec) || ec) {
            std::string message = "TLS CA file does not exist or is not a regular file: " + ca_file;
            if (ec)
                message += " (" + ec.message() + ')';
            return MakeFailure(DownloadErrorCode::TlsCaFileMissing, std::move(message));
        }
    }

    std::FILE* raw_file = boost::nowide::fopen(part_path.string().c_str(), "wb");
    if (raw_file == nullptr) {
        std::string message = "Unable to open partial download file '" + part_path.string() + "'";
        if (errno != 0)
            message += ": " + std::string(std::strerror(errno));
        AppendCleanupError(message, part_path);
        return MakeFailure(DownloadErrorCode::PartFileOpenFailed, std::move(message));
    }
    FileGuard file(raw_file);

    TransferContext context;
    context.file              = raw_file;
    context.cancel            = &cancel;
    context.progress_callback = &progress_callback;
    context.available_bytes   = available_bytes;

    auto fail_download = [&](DownloadErrorCode error_code, std::string message, bool cancelled = false) {
        DownloadResult result = MakeFailure(error_code, std::move(message), cancelled);
        result.bytes_written  = context.bytes_written;
        file.Close();
        AppendCleanupError(result.error_message, part_path);
        return result;
    };

    char        error_buffer[CURL_ERROR_SIZE] = {};
    CURLcode    option_error                  = CURLE_OK;
    std::string option_error_name;
    auto        set_option = [&](CURLoption option, auto value, const char* option_name) {
        if (option_error != CURLE_OK)
            return;
        option_error = curl_easy_setopt(curl.get(), option, value);
        if (option_error != CURLE_OK)
            option_error_name = option_name;
    };

    set_option(CURLOPT_ERRORBUFFER, error_buffer, "CURLOPT_ERRORBUFFER");
    set_option(CURLOPT_URL, url.c_str(), "CURLOPT_URL");
    set_option(CURLOPT_HTTPGET, 1L, "CURLOPT_HTTPGET");
    set_option(CURLOPT_FOLLOWLOCATION, 0L, "CURLOPT_FOLLOWLOCATION");
    set_option(CURLOPT_FAILONERROR, 0L, "CURLOPT_FAILONERROR");
    set_option(CURLOPT_NOSIGNAL, 1L, "CURLOPT_NOSIGNAL");
    set_option(CURLOPT_TCP_NODELAY, 1L, "CURLOPT_TCP_NODELAY");
    set_option(CURLOPT_NOPROXY, "*", "CURLOPT_NOPROXY");
    set_option(CURLOPT_PATH_AS_IS, 1L, "CURLOPT_PATH_AS_IS");
    set_option(CURLOPT_CONNECTTIMEOUT, request.connect_timeout_seconds, "CURLOPT_CONNECTTIMEOUT");
    set_option(CURLOPT_TIMEOUT, request.total_timeout_seconds, "CURLOPT_TIMEOUT");
    set_option(CURLOPT_WRITEFUNCTION, &CurlWriteCallback, "CURLOPT_WRITEFUNCTION");
    set_option(CURLOPT_WRITEDATA, static_cast<void*>(&context), "CURLOPT_WRITEDATA");
    set_option(CURLOPT_NOPROGRESS, 0L, "CURLOPT_NOPROGRESS");
    set_option(CURLOPT_XFERINFOFUNCTION, &CurlXferInfoCallback, "CURLOPT_XFERINFOFUNCTION");
    set_option(CURLOPT_XFERINFODATA, static_cast<void*>(&context), "CURLOPT_XFERINFODATA");

    if (request.secure_connection) {
        // Device certificates are signed by the bundled CA but do not necessarily
        // contain the printer IP as a hostname. This matches the existing secure
        // device HTTP clients: verify the certificate chain without HTTP fallback.
        set_option(CURLOPT_CAINFO, ca_file.c_str(), "CURLOPT_CAINFO");
        set_option(CURLOPT_SSL_VERIFYPEER, 1L, "CURLOPT_SSL_VERIFYPEER");
        set_option(CURLOPT_SSL_VERIFYHOST, 0L, "CURLOPT_SSL_VERIFYHOST");
        set_option(CURLOPT_SSL_CTX_FUNCTION, &CurlSslContextCallback, "CURLOPT_SSL_CTX_FUNCTION");
    }

    if (option_error != CURLE_OK) {
        return fail_download(DownloadErrorCode::CurlConfigurationFailed,
                             "Unable to set " + option_error_name + ": " + CurlErrorMessage(option_error, error_buffer));
    }

    const CURLcode perform_code = curl_easy_perform(curl.get());

    long http_status = 0;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_status);

    if (IsCancelled(cancel)) {
        DownloadResult result = fail_download(DownloadErrorCode::Cancelled, "Download cancelled", true);
        result.http_status    = http_status;
        return result;
    }

    if (perform_code != CURLE_OK) {
        DownloadErrorCode error_code = ClassifyCurlError(perform_code, context);
        std::string       message;
        if (context.insufficient_disk_space)
            message = context.disk_space_error.empty() ? "Insufficient temporary disk space" : context.disk_space_error;
        else if (context.progress_callback_failed)
            message = context.callback_error.empty() ? "Download progress callback failed" : context.callback_error;
        else if (context.write_failed)
            message = context.write_error.empty() ? "Unable to write the partial download file" : context.write_error;
        else
            message = CurlErrorMessage(perform_code, error_buffer);

        DownloadResult result = fail_download(error_code, std::move(message));
        result.http_status    = http_status;
        return result;
    }

    if (http_status != 200) {
        DownloadResult result = fail_download(DownloadErrorCode::UnexpectedHttpStatus,
                                              "Device video download returned HTTP " + std::to_string(http_status));
        result.http_status    = http_status;
        return result;
    }

    if (std::fflush(raw_file) != 0) {
        const std::string message = errno != 0 ? std::strerror(errno) : "Unable to flush the partial download file";
        DownloadResult    result  = fail_download(DownloadErrorCode::FileWriteFailed, message);
        result.http_status        = http_status;
        return result;
    }

    if (!ReportProgress(context, static_cast<curl_off_t>(context.bytes_written), static_cast<curl_off_t>(context.bytes_written))) {
        DownloadResult result = fail_download(DownloadErrorCode::ProgressCallbackFailed, context.callback_error.empty() ?
                                                                                             "Download progress callback failed" :
                                                                                             context.callback_error);
        result.http_status    = http_status;
        return result;
    }

    if (!file.Close()) {
        const std::string message = errno != 0 ? std::strerror(errno) : "Unable to close the partial download file";
        DownloadResult    result  = fail_download(DownloadErrorCode::FileWriteFailed, message);
        result.http_status        = http_status;
        return result;
    }

    if (IsCancelled(cancel)) {
        DownloadResult result = fail_download(DownloadErrorCode::Cancelled, "Download cancelled before finalizing the file", true);
        result.http_status    = http_status;
        return result;
    }

    const std::error_code rename_error = Slic3r::rename_file(part_path.string(), output_path.string());
    if (rename_error) {
        DownloadResult result = fail_download(DownloadErrorCode::FileFinalizeFailed,
                                              "Unable to replace output file '" + output_path.string() + "': " + rename_error.message());
        result.http_status    = http_status;
        return result;
    }

    DownloadResult result;
    result.success       = true;
    result.http_status   = http_status;
    result.bytes_written = context.bytes_written;
    return result;
}

}}} // namespace Slic3r::GUI::TimeLapseShare
