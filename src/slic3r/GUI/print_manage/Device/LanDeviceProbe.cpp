#include "LanDeviceProbe.hpp"

#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/Http.hpp"

#include <algorithm>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <cctype>
#include <chrono>
#include <limits>
#include <thread>
#include <utility>

namespace RemotePrint {
namespace {

struct PortAvailability
{
    bool https_open {false};
    bool http_open {false};
};

const char* const INVALID_PORT_DETAIL = "wssPort and videoPort must be valid ports";
const char* const CANCELLED_DETAIL = "probe cancelled";

bool probe_cancelled(const ProbeOptions& options)
{
    return options.cancelled && options.cancelled();
}

ProbeAttempt cancelled_attempt(ProbeSource source)
{
    ProbeAttempt attempt;
    attempt.source = source;
    attempt.error = ProbeError::Cancelled;
    attempt.detail = CANCELLED_DETAIL;
    return attempt;
}

ProbeResult cancelled_result()
{
    ProbeResult result;
    result.error = ProbeError::Cancelled;
    result.detail = CANCELLED_DETAIL;
    return result;
}

bool is_valid_ipv4(const std::string& address)
{
    boost::system::error_code error;
    const auto parsed = boost::asio::ip::address::from_string(address, error);
    return !error && parsed.is_v4();
}

int parse_port(const nlohmann::json& info, const char* key)
{
    const auto it = info.find(key);
    if (it == info.end())
        return 0;

    try {
        long long value = 0;
        if (it->is_number_integer() || it->is_number_unsigned()) {
            value = it->get<long long>();
        } else if (it->is_string()) {
            const std::string text = it->get<std::string>();
            std::size_t parsed_count = 0;
            value = std::stoll(text, &parsed_count);
            if (parsed_count != text.size())
                return 0;
        } else {
            return 0;
        }

        return value > 0 && value <= std::numeric_limits<std::uint16_t>::max()
                   ? static_cast<int>(value)
                   : 0;
    } catch (...) {
        return 0;
    }
}

bool has_required_info(const nlohmann::json& info)
{
    const auto model = info.find("model");
    const auto mac = info.find("mac");
    return model != info.end() && model->is_string() && !model->get_ref<const std::string&>().empty() &&
           mac != info.end() && mac->is_string() && !mac->get_ref<const std::string&>().empty();
}

std::string lower_ascii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

ProbeError classify_request_error(const std::string& detail, unsigned http_status)
{
    if (http_status != 0)
        return ProbeError::HttpError;

    const std::string lower = lower_ascii(detail);
    if (lower.find("timeout") != std::string::npos || lower.find("timed out") != std::string::npos)
        return ProbeError::Timeout;
    if (lower.find("cert") != std::string::npos || lower.find("ssl") != std::string::npos ||
        lower.find("verify") != std::string::npos || lower.find("host") != std::string::npos)
        return ProbeError::CertError;
    return ProbeError::NetworkError;
}

ProbeAttempt request_info(const std::string& address, ProbeSource source, const ProbeOptions& options)
{
    if (probe_cancelled(options))
        return cancelled_attempt(source);

    ProbeAttempt attempt;
    attempt.source = source;

    const bool https = source == ProbeSource::Https;
    const std::string url = std::string(https ? "https://" : "http://") + address +
                            (https ? ":443/info" : ":80/info");
    std::string response_body;
    std::string response_error;

    try {
        auto request = Slic3r::Http::get(url);
        request.noproxy("*")
            .size_limit(options.response_size_limit)
            .timeout_connect(options.connect_timeout_seconds)
            .timeout_max(https ? options.https_timeout_seconds : options.http_timeout_seconds)
            .on_complete([&](std::string body, unsigned http_status) {
                response_body = std::move(body);
                attempt.http_status = http_status;
            })
            .on_error([&](std::string body, std::string error, unsigned http_status) {
                response_body = std::move(body);
                response_error = std::move(error);
                attempt.http_status = http_status;
            });

        if (https) {
            request.ca_file(Slic3r::resources_dir() + "/cert/ca.crt")
                .ssl_verify_peer(true)
                .ssl_verify_host(false).ssl_ignore_certificate_time(true);
        }

        request.perform_sync();
    } catch (const std::exception& exception) {
        response_error = exception.what();
    } catch (...) {
        response_error = "unknown request error";
    }

    if (probe_cancelled(options))
        return cancelled_attempt(source);

    if (attempt.http_status != 200 || response_body.empty()) {
        attempt.error = classify_request_error(response_error, attempt.http_status);
        attempt.detail = response_error.empty() ? "request failed" : response_error;
        return attempt;
    }

    try {
        attempt.info = nlohmann::json::parse(response_body);
    } catch (const std::exception& exception) {
        attempt.error = ProbeError::ParseError;
        attempt.detail = exception.what();
    }

    return attempt;
}

std::optional<LanConnectionProfile> normalize_info(const std::string& address, ProbeAttempt& attempt)
{
    if (attempt.error != ProbeError::None || !attempt.info)
        return std::nullopt;

    const nlohmann::json& info = *attempt.info;
    if (!has_required_info(info)) {
        attempt.error = ProbeError::InvalidInfo;
        attempt.detail = "model and mac are required";
        return std::nullopt;
    }

    LanConnectionProfile profile;
    profile.address = address;
    profile.model = info["model"].get<std::string>();
    profile.mac = info["mac"].get<std::string>();
    profile.source = attempt.source;
    profile.raw_info = info;

    if (attempt.source == ProbeSource::Https) {
        const int wss_port = parse_port(info, "wssPort");
        const int video_port = parse_port(info, "videoPort");
        if (wss_port == 0 || video_port == 0) {
            attempt.error = ProbeError::InvalidInfo;
            attempt.detail = INVALID_PORT_DETAIL;
            return std::nullopt;
        }

        profile.secure_connection = true;
        profile.wss_port = static_cast<std::uint16_t>(wss_port);
        profile.video_port = static_cast<std::uint16_t>(video_port);
    }

    return profile;
}

bool probe_tcp_port(const std::string& address, unsigned short port, int timeout_ms)
{
    if (timeout_ms <= 0 || !is_valid_ipv4(address))
        return false;

    try {
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::socket socket(io_service);
        boost::asio::steady_timer timer(io_service);
        const boost::asio::ip::tcp::endpoint endpoint(
            boost::asio::ip::address::from_string(address), port);
        bool finished = false;
        bool connected = false;

        socket.async_connect(endpoint, [&](const boost::system::error_code& error) {
            if (finished)
                return;
            finished = true;
            connected = !error;
            boost::system::error_code ignored;
            timer.cancel(ignored);
        });

        timer.expires_after(std::chrono::milliseconds(timeout_ms));
        timer.async_wait([&](const boost::system::error_code& error) {
            if (error || finished)
                return;
            finished = true;
            boost::system::error_code ignored;
            socket.close(ignored);
        });

        io_service.run();
        boost::system::error_code ignored;
        socket.close(ignored);
        return connected;
    } catch (...) {
        return false;
    }
}

PortAvailability probe_ports(const std::string& address,
                             ProbePolicy policy,
                             const ProbeOptions& options)
{
    PortAvailability ports;
    if (probe_cancelled(options))
        return ports;

    if (policy != ProbePolicy::HttpOnly)
        ports.https_open = probe_tcp_port(address, 443, options.port_probe_timeout_ms);
    if (!probe_cancelled(options) && policy != ProbePolicy::SecureOnly)
        ports.http_open = probe_tcp_port(address, 80, options.port_probe_timeout_ms);
    return ports;
}

std::vector<PortAvailability> probe_ports_many(const std::vector<std::string>& addresses,
                                               ProbePolicy policy,
                                               const ProbeOptions& options)
{
    std::vector<PortAvailability> results(addresses.size());
    if (addresses.empty())
        return results;

    const std::size_t worker_count = std::min(
        std::max<std::size_t>(1, options.port_probe_concurrency), addresses.size());
    std::atomic<std::size_t> next_index {0};
    std::vector<std::thread> workers;
    workers.reserve(worker_count);

    for (std::size_t worker = 0; worker < worker_count; ++worker) {
        workers.emplace_back([&] {
            while (true) {
                if (probe_cancelled(options))
                    break;
                const std::size_t index = next_index.fetch_add(1);
                if (index >= addresses.size())
                    break;
                results[index] = probe_ports(addresses[index], policy, options);
            }
        });
    }

    for (auto& worker : workers)
        worker.join();
    return results;
}

ProbeAttempt closed_port_attempt(ProbeSource source)
{
    ProbeAttempt attempt;
    attempt.source = source;
    attempt.error = ProbeError::NetworkError;
    attempt.detail = "port is closed";
    return attempt;
}

ProbeResult probe_impl(const std::string& address,
                       ProbePolicy policy,
                       const ProbeOptions& options,
                       const std::optional<PortAvailability>& availability)
{
    ProbeResult result;
    if (probe_cancelled(options))
        return cancelled_result();

    if (!is_valid_ipv4(address)) {
        result.error = ProbeError::InvalidAddress;
        result.detail = "address must be a numeric IPv4 address";
        return result;
    }

    auto try_source = [&](ProbeSource source, bool port_open) -> std::optional<LanConnectionProfile> {
        ProbeAttempt attempt = probe_cancelled(options)
                                   ? cancelled_attempt(source)
                                   : (port_open ? request_info(address, source, options)
                                                : closed_port_attempt(source));
        auto profile = normalize_info(address, attempt);
        result.attempts.push_back(std::move(attempt));
        return profile;
    };

    if (policy != ProbePolicy::HttpOnly) {
        const bool https_open = !availability || availability->https_open;
        if (auto profile = try_source(ProbeSource::Https, https_open)) {
            result.profile = std::move(profile);
            return result;
        }
    }

    if (probe_cancelled(options)) {
        result.error = ProbeError::Cancelled;
        result.detail = CANCELLED_DETAIL;
        return result;
    }

    if (policy != ProbePolicy::SecureOnly) {
        const bool http_open = !availability || availability->http_open;
        if (auto profile = try_source(ProbeSource::Http, http_open)) {
            result.profile = std::move(profile);
            return result;
        }
    }

    if (!result.attempts.empty()) {
        const ProbeAttempt& last = result.attempts.back();
        result.error = last.error;
        result.http_status = last.http_status;
        result.detail = last.detail;
    } else {
        result.error = ProbeError::NetworkError;
        result.detail = "no probe attempt was made";
    }
    return result;
}

class ProbeTaskExecutor
{
public:
    void submit(std::function<void()> task)
    {
        boost::asio::post(m_pool, std::move(task));
    }

private:
    boost::asio::thread_pool m_pool {4};
};

ProbeTaskExecutor& probe_task_executor()
{
    static ProbeTaskExecutor executor;
    return executor;
}

} // namespace

const char* probe_error_name(ProbeError error) noexcept
{
    switch (error) {
    case ProbeError::None: return "none";
    case ProbeError::InvalidAddress: return "invalid_address";
    case ProbeError::Timeout: return "timeout";
    case ProbeError::NetworkError: return "network_error";
    case ProbeError::CertError: return "cert_error";
    case ProbeError::HttpError: return "http_error";
    case ProbeError::ParseError: return "parse_error";
    case ProbeError::InvalidInfo: return "invalid_device_info";
    case ProbeError::Cancelled: return "cancelled";
    }
    return "network_error";
}

const char* probe_source_name(ProbeSource source) noexcept
{
    switch (source) {
    case ProbeSource::Https: return "https";
    case ProbeSource::Http: return "http";
    case ProbeSource::None: return "none";
    }
    return "none";
}

nlohmann::json LanConnectionProfile::to_json() const
{
    nlohmann::json result = raw_info.is_object() ? raw_info : nlohmann::json::object();
    result["address"] = address;
    result["mac"] = mac;
    result["model"] = model;
    result["secureConnection"] = secure_connection;
    result["wssPort"] = secure_connection ? wss_port : 0;
    result["videoPort"] = secure_connection ? video_port : 0;
    result["source"] = probe_source_name(source);
    return result;
}

nlohmann::json ProbeAttempt::to_json() const
{
    return {
        {"source", probe_source_name(source)},
        {"errorType", probe_error_name(error)},
        {"httpStatus", http_status}
    };
}

nlohmann::json ProbeResult::to_json() const
{
    nlohmann::json result;
    if (ok()) {
        result["status"] = 0;
        result["result"] = profile->to_json();
    } else {
        result["status"] = 1;
        result["errorType"] = probe_error_name(error);
        result["error"] = detail;
        result["httpStatus"] = http_status;
    }

    result["attempts"] = nlohmann::json::array();
    for (const ProbeAttempt& attempt : attempts)
        result["attempts"].push_back(attempt.to_json());
    return result;
}

ProbeResult LanDeviceProbe::probe(const std::string& address,
                                          ProbePolicy policy,
                                          const ProbeOptions& options) const
{
    return probe_impl(address, policy, options, std::nullopt);
}

std::vector<ProbeResult> LanDeviceProbe::probe_many(
    const std::vector<std::string>& addresses,
    ProbePolicy policy,
    const ProbeOptions& options) const
{
    std::vector<ProbeResult> results(addresses.size());
    if (addresses.empty())
        return results;

    if (probe_cancelled(options)) {
        std::fill(results.begin(), results.end(), cancelled_result());
        return results;
    }

    std::vector<PortAvailability> port_results;
    if (options.preflight_ports)
        port_results = probe_ports_many(addresses, policy, options);

    if (probe_cancelled(options)) {
        std::fill(results.begin(), results.end(), cancelled_result());
        return results;
    }

    const std::size_t worker_count = std::min(
        std::max<std::size_t>(1, options.request_concurrency), addresses.size());
    std::atomic<std::size_t> next_index {0};
    std::vector<std::thread> workers;
    workers.reserve(worker_count);

    for (std::size_t worker = 0; worker < worker_count; ++worker) {
        workers.emplace_back([&] {
            while (true) {
                if (probe_cancelled(options))
                    break;
                const std::size_t index = next_index.fetch_add(1);
                if (index >= addresses.size())
                    break;
                const std::optional<PortAvailability> availability = options.preflight_ports
                    ? std::optional<PortAvailability>(port_results[index])
                    : std::nullopt;
                results[index] = probe_impl(addresses[index], policy, options, availability);
            }
        });
    }

    for (auto& worker : workers)
        worker.join();

    if (probe_cancelled(options)) {
        for (ProbeResult& result : results) {
            if (!result.ok() && result.attempts.empty())
                result = cancelled_result();
        }
    }
    return results;
}

void submit_lan_device_probe_task(std::function<void()> task)
{
    probe_task_executor().submit(std::move(task));
}

} // namespace RemotePrint
