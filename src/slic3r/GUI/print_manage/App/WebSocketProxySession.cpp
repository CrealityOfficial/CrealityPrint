#include "WebSocketProxy.hpp"
#include "DeviceMessageFilter.h"

#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/DeviceTlsPolicy.hpp"
#include "nlohmann/json.hpp"
#include "slic3r/GUI/print_manage/Utils.hpp"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/log/trivial.hpp>

#include <wx/app.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <deque>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Slic3r {
namespace GUI {
namespace WebSocketProxy {

namespace asio      = boost::asio;
namespace beast     = boost::beast;
namespace ssl       = asio::ssl;
namespace websocket = beast::websocket;
using tcp            = asio::ip::tcp;
using Strand         = asio::strand<asio::io_context::executor_type>;

namespace {

constexpr std::size_t MAX_SESSIONS             = 1024;
constexpr std::size_t MAX_URL_SIZE              = 2 * 1024;
constexpr std::size_t MAX_PAGE_TOKEN_SIZE       = 128;
constexpr std::size_t MAX_SOCKET_TOKEN_SIZE     = 128;
constexpr std::size_t MAX_CLOSE_REASON_SIZE     = 123;
constexpr std::size_t MAX_OUTGOING_MESSAGE_SIZE = 1 * 1024 * 1024;
constexpr std::size_t MAX_QUEUED_MESSAGES       = 128;
constexpr std::size_t MAX_QUEUED_BYTES          = 4 * 1024 * 1024;
constexpr std::size_t MAX_INCOMING_MESSAGE_SIZE = 4 * 1024 * 1024;
constexpr std::size_t MAX_UI_EVENTS             = 1024;
constexpr std::size_t MAX_UI_EVENT_BYTES        = 8 * 1024 * 1024;
constexpr std::size_t MAX_UI_BATCH_EVENTS       = 64;
constexpr std::size_t MAX_UI_BATCH_BYTES        = 256 * 1024;

constexpr auto RESOLVE_TIMEOUT   = std::chrono::seconds(5);
constexpr auto CONNECT_TIMEOUT   = std::chrono::seconds(5);
constexpr auto TLS_TIMEOUT       = std::chrono::seconds(8);
constexpr auto WS_TIMEOUT        = std::chrono::seconds(8);
constexpr auto WRITE_TIMEOUT     = std::chrono::seconds(8);
constexpr auto CLOSE_TIMEOUT     = std::chrono::seconds(3);
constexpr auto SHUTDOWN_WAIT     = std::chrono::seconds(5);

struct ParsedWsUrl
{
    bool        secure{false};
    std::string host;
    std::string port;
    std::string host_header;
    std::string target{"/"};
};

bool has_invalid_host_character(const std::string& value)
{
    return value.empty() ||
           std::any_of(value.begin(), value.end(), [](unsigned char ch) {
               return std::iscntrl(ch) != 0 || std::isspace(ch) != 0 ||
                      ch == '/' || ch == '?' || ch == '#';
           });
}

bool is_ip_literal(const std::string& value)
{
    boost::system::error_code error;
    asio::ip::make_address(value, error);
    return !error;
}

bool is_ipv4_literal(const std::string& value)
{
    boost::system::error_code error;
    const auto address = asio::ip::make_address(value, error);
    return !error && address.is_v4();
}

bool is_device_proxy_endpoint(const ParsedWsUrl& endpoint)
{
    return endpoint.target == "/" &&
           is_ipv4_literal(endpoint.host) &&
           (endpoint.secure || endpoint.port == "9999");
}

bool parse_port(const std::string& text)
{
    if (text.empty() || text.size() > 5)
        return false;

    unsigned int value = 0;
    for (unsigned char ch : text) {
        if (!std::isdigit(ch))
            return false;
        value = value * 10 + static_cast<unsigned int>(ch - '0');
    }
    return value > 0 && value <= 65535;
}

std::optional<ParsedWsUrl> parse_ws_url(const std::string& raw_url)
{
    if (raw_url.empty() || raw_url.size() > MAX_URL_SIZE ||
        raw_url.find('#') != std::string::npos) {
        return std::nullopt;
    }

    ParsedWsUrl result;
    std::size_t scheme_size = 0;
    if (raw_url.rfind("ws://", 0) == 0) {
        scheme_size = 5;
        result.port = "80";
    } else if (raw_url.rfind("wss://", 0) == 0) {
        scheme_size = 6;
        result.secure = true;
        result.port = "443";
    } else {
        return std::nullopt;
    }

    const std::size_t authority_end = raw_url.find_first_of("/?", scheme_size);
    const std::string authority = raw_url.substr(
        scheme_size,
        authority_end == std::string::npos ? std::string::npos : authority_end - scheme_size);
    if (authority.empty() || authority.find('@') != std::string::npos)
        return std::nullopt;

    bool explicit_port = false;
    bool bracketed_host = false;
    if (authority.front() == '[') {
        bracketed_host = true;
        const std::size_t close_bracket = authority.find(']');
        if (close_bracket == std::string::npos || close_bracket == 1)
            return std::nullopt;

        result.host = authority.substr(1, close_bracket - 1);
        const std::string remainder = authority.substr(close_bracket + 1);
        if (!remainder.empty()) {
            if (remainder.front() != ':' || remainder.size() == 1)
                return std::nullopt;
            result.port = remainder.substr(1);
            explicit_port = true;
        }
    } else {
        const std::size_t first_colon = authority.find(':');
        const std::size_t last_colon = authority.rfind(':');
        if (first_colon != std::string::npos && first_colon != last_colon)
            return std::nullopt;

        if (last_colon == std::string::npos) {
            result.host = authority;
        } else {
            result.host = authority.substr(0, last_colon);
            result.port = authority.substr(last_colon + 1);
            explicit_port = true;
        }
    }

    if (has_invalid_host_character(result.host) ||
        (explicit_port && !parse_port(result.port))) {
        return std::nullopt;
    }

    const std::string default_port = result.secure ? "443" : "80";
    result.host_header = bracketed_host ? "[" + result.host + "]" : result.host;
    if (result.port != default_port)
        result.host_header += ":" + result.port;

    if (authority_end != std::string::npos) {
        result.target = raw_url[authority_end] == '?'
            ? "/" + raw_url.substr(authority_end)
            : raw_url.substr(authority_end);
    }
    if (result.target.empty())
        result.target = "/";

    return result;
}

enum class CommandType
{
    Reset,
    Open,
    Send,
    Close
};

struct ProxyCommand
{
    CommandType type{CommandType::Reset};
    std::string page_token;
    std::string socket_token;
    int         id{-1};
    std::string url;
    std::string data;
    int         close_code{0};
    std::string close_reason;
};

bool read_int(const nlohmann::json& value, int& result)
{
    if (!value.is_number_integer() && !value.is_number_unsigned())
        return false;

    try {
        const std::int64_t number = value.get<std::int64_t>();
        if (number < std::numeric_limits<int>::min() ||
            number > std::numeric_limits<int>::max()) {
            return false;
        }
        result = static_cast<int>(number);
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<ProxyCommand> parse_command(const nlohmann::json& payload)
{
    if (!payload.is_object())
        return std::nullopt;

    const auto type_it = payload.find("type");
    const auto token_it = payload.find("pageToken");
    if (type_it == payload.end() || !type_it->is_string() ||
        token_it == payload.end() || !token_it->is_string()) {
        return std::nullopt;
    }

    ProxyCommand command;
    command.page_token = token_it->get<std::string>();
    if (command.page_token.empty() || command.page_token.size() > MAX_PAGE_TOKEN_SIZE)
        return std::nullopt;

    const std::string type = type_it->get<std::string>();
    if (type == "reset") {
        command.type = CommandType::Reset;
        return command;
    }

    const auto id_it = payload.find("id");
    if (id_it == payload.end() || !read_int(*id_it, command.id) || command.id <= 0)
        return std::nullopt;

    const auto socket_token_it = payload.find("socketToken");
    if (socket_token_it == payload.end() || !socket_token_it->is_string())
        return std::nullopt;
    command.socket_token = socket_token_it->get<std::string>();
    if (command.socket_token.empty() ||
        command.socket_token.size() > MAX_SOCKET_TOKEN_SIZE) {
        return std::nullopt;
    }

    if (type == "open") {
        const auto url_it = payload.find("url");
        if (url_it == payload.end() || !url_it->is_string())
            return std::nullopt;
        command.type = CommandType::Open;
        command.url = url_it->get<std::string>();
        return command;
    }

    if (type == "send") {
        const auto data_it = payload.find("data");
        if (data_it == payload.end() || !data_it->is_string())
            return std::nullopt;
        command.type = CommandType::Send;
        command.data = data_it->get<std::string>();
        return command;
    }

    if (type == "close") {
        command.type = CommandType::Close;
        const auto code_it = payload.find("code");
        if (code_it != payload.end() && !code_it->is_null() &&
            !read_int(*code_it, command.close_code)) {
            return std::nullopt;
        }

        const auto reason_it = payload.find("reason");
        if (reason_it != payload.end() && !reason_it->is_null()) {
            if (!reason_it->is_string())
                return std::nullopt;
            command.close_reason = reason_it->get<std::string>();
        }
        return command;
    }

    return std::nullopt;
}

struct ProxyEvent
{
    int           id{-1};
    std::uint64_t generation{0};
    std::string   page_token;
    std::string   socket_token;
    std::string   event;
    std::string   data;
    int           code{0};
    std::string   reason;
    bool          was_clean{false};
};

std::string build_event_payload(const ProxyEvent& event)
{
    nlohmann::json payload;
    payload["id"] = event.id;
    payload["pageToken"] = event.page_token;
    payload["socketToken"] = event.socket_token;
    payload["event"] = event.event;
    if (event.event == "message")
        payload["data"] = event.data;
    if (event.code != 0)
        payload["code"] = event.code;
    if (!event.reason.empty())
        payload["reason"] = event.reason;
    if (event.event == "error")
        payload["message"] = event.reason;
    payload["wasClean"] = event.was_clean;

    // Serialize as an ASCII-safe JavaScript object literal, without URL encoding.
    return payload.dump(-1, ' ', true);
}

std::string build_batch_script(const std::vector<std::string>& payloads)
{
    // Each payload is already serialized JSON. Join the object literals directly.
    std::string script = "window.__nativeWebSocketCallbackBatch([";
    bool first = true;
    for (const std::string& payload : payloads) {
        if (!first)
            script += ',';
        first = false;
        script += payload;
    }
    script += "]);";
    return script;
}

class UiGate : public std::enable_shared_from_this<UiGate>
{
public:
    enum class PostResult
    {
        Posted,
        Inactive,
        Overflow
    };

    explicit UiGate(ScriptSink script_sink)
        : m_script_sink(std::move(script_sink))
    {
    }

    void SetGeneration(std::uint64_t generation)
    {
        m_generation.store(generation);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pending.clear();
        m_pending_bytes = 0;
    }

    PostResult Post(const ProxyEvent& event)
    {
        if (!m_active.load() || event.generation != m_generation.load())
            return PostResult::Inactive;

        PendingEvent pending{event.generation, build_event_payload(event)};
        const bool is_message = event.event == "message";
        bool schedule = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_active.load() || pending.generation != m_generation.load())
                return PostResult::Inactive;

            const bool byte_limit_exceeded =
                pending.payload.size() > MAX_UI_EVENT_BYTES ||
                m_pending_bytes > MAX_UI_EVENT_BYTES - pending.payload.size();
            // State transitions must remain deliverable so JS cannot stay stuck.
            if (is_message &&
                (m_pending.size() >= MAX_UI_EVENTS || byte_limit_exceeded)) {
                BOOST_LOG_TRIVIAL(error) << "WebSocket proxy UI event queue is full";
                return PostResult::Overflow;
            }

            m_pending_bytes += pending.payload.size();
            m_pending.emplace_back(std::move(pending));
            if (!m_callback_scheduled) {
                m_callback_scheduled = true;
                schedule = true;
            }
        }

        if (!schedule)
            return PostResult::Posted;

        if (!ScheduleDrain()) {
            ClearPending();
            return PostResult::Inactive;
        }
        return PostResult::Posted;
    }

    void Invalidate()
    {
        m_active.store(false);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_script_sink = nullptr;
        m_pending.clear();
        m_pending_bytes = 0;
    }

private:
    struct PendingEvent
    {
        std::uint64_t generation;
        std::string   payload;
    };

    bool ScheduleDrain()
    {
        wxApp* app = wxTheApp;
        if (!app)
            return false;

        std::weak_ptr<UiGate> weak_self = shared_from_this();
        try {
            app->CallAfter([weak_self]() {
                if (auto self = weak_self.lock())
                    self->Drain();
            });
        } catch (...) {
            return false;
        }
        return true;
    }

    void ClearPending()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callback_scheduled = false;
        m_pending.clear();
        m_pending_bytes = 0;
    }

    void Drain()
    {
        std::vector<std::string> payloads;
        std::size_t batch_bytes = 0;
        std::uint64_t batch_generation = 0;
        bool schedule_next = false;
        ScriptSink script_sink;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_active.load()) {
                m_callback_scheduled = false;
                m_pending.clear();
                m_pending_bytes = 0;
                return;
            }

            batch_generation = m_generation.load();
            while (!m_pending.empty()) {
                PendingEvent& next = m_pending.front();
                const bool batch_full = payloads.size() >= MAX_UI_BATCH_EVENTS;
                const bool byte_limit_reached =
                    !payloads.empty() &&
                    (next.payload.size() > MAX_UI_BATCH_BYTES ||
                     batch_bytes > MAX_UI_BATCH_BYTES - next.payload.size());
                if (batch_full || byte_limit_reached)
                    break;

                const std::size_t payload_size = next.payload.size();
                m_pending_bytes = payload_size <= m_pending_bytes
                    ? m_pending_bytes - payload_size
                    : 0;
                if (next.generation == batch_generation) {
                    batch_bytes += payload_size;
                    payloads.emplace_back(next.payload);
                }
                m_pending.pop_front();
            }

            schedule_next = !m_pending.empty();
            if (!schedule_next)
                m_callback_scheduled = false;
            script_sink = m_script_sink;
        }

        if (script_sink && !payloads.empty() && m_active.load() &&
            batch_generation == m_generation.load()) {
            try {
                script_sink(build_batch_script(payloads));
            } catch (...) {
            }
        }

        if (schedule_next && !ScheduleDrain())
            ClearPending();
    }

private:
    std::atomic<bool>          m_active{true};
    std::atomic<std::uint64_t> m_generation{0};
    std::mutex                 m_mutex;
    ScriptSink                 m_script_sink;
    std::deque<PendingEvent>   m_pending;
    std::size_t                m_pending_bytes{0};
    bool                       m_callback_scheduled{false};
};

class Runtime
{
public:
    Runtime()
        : m_ssl_context(ssl::context::tls_client)
        , m_strand(asio::make_strand(m_io_context))
        , m_work_guard(asio::make_work_guard(m_io_context))
        , m_worker_exited(m_worker_exit.get_future())
    {
        InitializeSsl();
        m_worker = std::thread([this]() {
            for (;;) {
                try {
                    m_io_context.run();
                    break;
                } catch (const std::exception& e) {
                    BOOST_LOG_TRIVIAL(error) << "WebSocket proxy handler failed: "
                                             << e.what();
                } catch (...) {
                    BOOST_LOG_TRIVIAL(error) << "WebSocket proxy handler failed with unknown error";
                }
            }
            try {
                m_worker_exit.set_value();
            } catch (...) {
            }
        });
    }

    ~Runtime()
    {
        Stop();
    }

    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    Strand executor()
    {
        return m_strand;
    }

    ssl::context& ssl_context()
    {
        return m_ssl_context;
    }

    const std::string& ssl_error() const
    {
        return m_ssl_error;
    }

    void Stop() noexcept
    {
        if (m_stopped.exchange(true))
            return;

        m_work_guard.reset();
        if (m_worker.joinable()) {
            if (m_worker_exited.wait_for(SHUTDOWN_WAIT) ==
                std::future_status::timeout) {
                BOOST_LOG_TRIVIAL(error)
                    << "WebSocket proxy I/O drain timed out; forcing stop";
                m_io_context.stop();
            }
            m_worker.join();
        }
    }

private:
    void InitializeSsl()
    {
        beast::error_code ec;
        m_ssl_context.set_options(
            ssl::context::default_workarounds |
            ssl::context::no_sslv2 |
            ssl::context::no_sslv3,
            ec);
        if (!ec)
            m_ssl_context.set_verify_mode(ssl::verify_peer, ec);
        if (!ec)
            m_ssl_context.load_verify_file(Slic3r::resources_dir() + "/cert/ca.crt", ec);
        if (ec) {
            m_ssl_error = ec.message();
            BOOST_LOG_TRIVIAL(error) << "WebSocket proxy SSL initialization failed: "
                                     << m_ssl_error;
        }
    }

private:
    ssl::context                                          m_ssl_context;
    asio::io_context                                      m_io_context;
    Strand                                                m_strand;
    asio::executor_work_guard<asio::io_context::executor_type> m_work_guard;
    std::promise<void>                                    m_worker_exit;
    std::future<void>                                     m_worker_exited;
    std::thread                                           m_worker;
    std::atomic<bool>                                     m_stopped{false};
    std::string                                           m_ssl_error;
};

class Session;
using SessionPtr = std::shared_ptr<Session>;

class Session : public std::enable_shared_from_this<Session>
{
public:
    using EventHandler = std::function<void(const SessionPtr&, ProxyEvent)>;
    using SessionHandler = std::function<void(const SessionPtr&)>;

    Session(Strand executor,
            ssl::context& ssl_context,
            std::string ssl_error,
            int id,
            std::uint64_t generation,
            std::string page_token,
            std::string socket_token,
            ParsedWsUrl endpoint,
            EventHandler event_handler,
            SessionHandler terminal_handler)
        : m_executor(std::move(executor))
        , m_ssl_context(ssl_context)
        , m_resolver(m_executor)
        , m_timer(m_executor)
        , m_ssl_error(std::move(ssl_error))
        , m_id(id)
        , m_generation(generation)
        , m_page_token(std::move(page_token))
        , m_socket_token(std::move(socket_token))
        , m_endpoint(std::move(endpoint))
        , m_event_handler(std::move(event_handler))
        , m_terminal_handler(std::move(terminal_handler))
    {
    }

    int id() const
    {
        return m_id;
    }

    std::uint64_t generation() const
    {
        return m_generation;
    }

    const std::string& socket_token() const
    {
        return m_socket_token;
    }

    const std::string& host() const
    {
        return m_endpoint.host;
    }

    bool is_device_connection() const
    {
        return m_endpoint.target == "/" &&
               is_ip_literal(m_endpoint.host) &&
               !m_endpoint.port.empty();
    }

    bool CanStart() const
    {
        return m_state == State::Created && !m_finished;
    }

    void Start()
    {
        if (!CanStart())
            return;

        try {
            if (m_endpoint.secure) {
                if (!m_ssl_error.empty()) {
                    Fail("SSL initialization failed: " + m_ssl_error);
                    return;
                }
                m_wss = std::make_unique<SecureStream>(m_executor, m_ssl_context);
                m_wss->read_message_max(MAX_INCOMING_MESSAGE_SIZE);
                if (is_device_proxy_endpoint(m_endpoint)) {
                    std::string policy_error;
                    if (!Slic3r::DeviceTlsPolicy::ignore_certificate_time(
                            m_wss->next_layer().native_handle(), policy_error)) {
                        Fail("Failed to configure device TLS policy: " + policy_error);
                        return;
                    }
                }
            } else {
                m_ws = std::make_unique<PlainStream>(m_executor);
                m_ws->read_message_max(MAX_INCOMING_MESSAGE_SIZE);
            }
        } catch (const std::exception& e) {
            Fail(e.what());
            return;
        }

        m_state = State::Resolving;
        ArmTimer(RESOLVE_TIMEOUT, "resolve");
        const SessionPtr self = shared_from_this();
        m_resolver.async_resolve(
            m_endpoint.host,
            m_endpoint.port,
            [self](const beast::error_code& ec, tcp::resolver::results_type results) {
                self->OnResolve(ec, std::move(results));
            });
    }

    void EnqueueText(std::string text)
    {
        if (m_finished || m_close_requested || m_state == State::Closing)
            return;

        if (text.size() > MAX_OUTGOING_MESSAGE_SIZE) {
            Fail("outgoing WebSocket message is too large");
            return;
        }

        const std::size_t message_count =
            m_write_queue.size() + (m_write_in_progress ? 1 : 0);
        if (message_count >= MAX_QUEUED_MESSAGES ||
            m_queued_bytes + text.size() > MAX_QUEUED_BYTES) {
            Fail("WebSocket send queue is full");
            return;
        }

        m_queued_bytes += text.size();
        m_write_queue.emplace_back(
            std::make_shared<const std::string>(std::move(text)));
        if (m_state == State::Open)
            PumpWrite();
    }

    void RequestClose(int code, std::string reason)
    {
        if (m_finished || m_close_requested)
            return;

        m_close_requested = true;
        m_close_code = NormalizeCloseCode(code);
        m_close_reason = TruncateCloseReason(std::move(reason));

        if (m_state != State::Open) {
            Finish(1006, std::string(), false, true);
            return;
        }

        if (!m_write_in_progress && m_write_queue.empty())
            BeginClose();
    }

    void Abort()
    {
        if (!m_finished)
            Finish(1006, std::string(), false, false);
    }

    void FailEventDelivery()
    {
        Fail("WebSocket proxy UI event queue is full");
    }

private:
    using PlainStream = websocket::stream<beast::tcp_stream>;
    using SecureStream = websocket::stream<beast::ssl_stream<beast::tcp_stream>>;

    enum class State
    {
        Created,
        Resolving,
        Connecting,
        TlsHandshaking,
        WsHandshaking,
        Open,
        Closing,
        Closed
    };

    static int NormalizeCloseCode(int code)
    {
        if (code == 0)
            return 1000;
        if (code == 1000 || (code >= 3000 && code <= 4999)) {
            return code;
        }
        return 1000;
    }

    static std::string TruncateCloseReason(std::string reason)
    {
        if (reason.size() <= MAX_CLOSE_REASON_SIZE)
            return reason;

        std::size_t end = MAX_CLOSE_REASON_SIZE;
        while (end > 0 &&
               (static_cast<unsigned char>(reason[end]) & 0xc0) == 0x80) {
            --end;
        }
        reason.resize(end);
        return reason;
    }

    void OnResolve(const beast::error_code& ec, tcp::resolver::results_type results)
    {
        if (m_finished || m_state != State::Resolving)
            return;

        CancelTimer();
        if (ec) {
            Fail("resolve failed: " + ec.message());
            return;
        }

        m_state = State::Connecting;
        beast::tcp_stream& stream = LowestLayer();
        stream.expires_after(CONNECT_TIMEOUT);
        const SessionPtr self = shared_from_this();
        stream.async_connect(
            results,
            [self](const beast::error_code& connect_ec, const tcp::endpoint&) {
                self->OnConnect(connect_ec);
            });
    }

    void OnConnect(const beast::error_code& ec)
    {
        if (m_finished || m_state != State::Connecting)
            return;

        LowestLayer().expires_never();
        if (ec) {
            Fail("connect failed: " + ec.message());
            return;
        }

        if (m_wss) {
            m_state = State::TlsHandshaking;
            ArmTimer(TLS_TIMEOUT, "TLS handshake");
            const SessionPtr self = shared_from_this();
            m_wss->next_layer().async_handshake(
                ssl::stream_base::client,
                [self](const beast::error_code& handshake_ec) {
                    self->OnTlsHandshake(handshake_ec);
                });
            return;
        }

        StartWsHandshake();
    }

    void OnTlsHandshake(const beast::error_code& ec)
    {
        if (m_finished || m_state != State::TlsHandshaking)
            return;

        CancelTimer();
        if (ec) {
            Fail("TLS handshake failed: " + ec.message());
            return;
        }
        StartWsHandshake();
    }

    void StartWsHandshake()
    {
        m_state = State::WsHandshaking;
        ArmTimer(WS_TIMEOUT, "WebSocket handshake");
        const SessionPtr self = shared_from_this();
        auto handler = [self](const beast::error_code& ec) {
            self->OnWsHandshake(ec);
        };
        if (m_wss)
            m_wss->async_handshake(m_endpoint.host_header, m_endpoint.target, handler);
        else
            m_ws->async_handshake(m_endpoint.host_header, m_endpoint.target, handler);
    }

    void OnWsHandshake(const beast::error_code& ec)
    {
        if (m_finished || m_state != State::WsHandshaking)
            return;

        CancelTimer();
        if (ec) {
            Fail("WebSocket handshake failed: " + ec.message());
            return;
        }

        m_state = State::Open;
        if (m_wss)
            m_wss->text(true);
        else
            m_ws->text(true);

        Emit("open", std::string(), 0, std::string(), true);
        DoRead();
        PumpWrite();
    }

    void DoRead()
    {
        if (m_finished || m_state != State::Open || m_read_in_progress)
            return;

        m_read_in_progress = true;
        const SessionPtr self = shared_from_this();
        auto handler = [self](const beast::error_code& ec, std::size_t) {
            self->OnRead(ec);
        };
        if (m_wss)
            m_wss->async_read(m_read_buffer, handler);
        else
            m_ws->async_read(m_read_buffer, handler);
    }

    void OnRead(const beast::error_code& ec)
    {
        m_read_in_progress = false;
        if (m_finished)
            return;

        if (ec == websocket::error::closed) {
            const websocket::close_reason reason =
                m_wss ? m_wss->reason() : m_ws->reason();
            const int code = reason.code == websocket::close_code::none
                ? 1005
                : static_cast<int>(reason.code);
            const std::string text(reason.reason.data(), reason.reason.size());
            Finish(code, text, true, true);
            return;
        }
        if (ec) {
            Fail("read failed: " + ec.message());
            return;
        }

        const std::string data = beast::buffers_to_string(m_read_buffer.data());
        m_read_buffer.consume(m_read_buffer.size());
        Emit("message", data, 0, std::string(), true);
        if (m_state == State::Open)
            DoRead();
    }

    void PumpWrite()
    {
        if (m_finished || m_state != State::Open || m_write_in_progress)
            return;
        if (m_write_queue.empty()) {
            if (m_close_requested)
                BeginClose();
            return;
        }

        m_current_write = m_write_queue.front();
        m_write_queue.pop_front();
        m_write_in_progress = true;
        ArmTimer(WRITE_TIMEOUT, "write");

        const SessionPtr self = shared_from_this();
        const auto payload = m_current_write;
        auto handler = [self, payload](const beast::error_code& ec, std::size_t) {
            self->OnWrite(ec, payload->size());
        };
        if (m_wss)
            m_wss->async_write(asio::buffer(*payload), handler);
        else
            m_ws->async_write(asio::buffer(*payload), handler);
    }

    void OnWrite(const beast::error_code& ec, std::size_t message_size)
    {
        CancelTimer();
        m_write_in_progress = false;
        m_current_write.reset();
        m_queued_bytes = message_size <= m_queued_bytes
            ? m_queued_bytes - message_size
            : 0;

        if (m_finished)
            return;
        if (ec) {
            Fail("write failed: " + ec.message());
            return;
        }

        PumpWrite();
    }

    void BeginClose()
    {
        if (m_finished || m_state != State::Open || m_write_in_progress ||
            !m_write_queue.empty()) {
            return;
        }

        m_state = State::Closing;
        websocket::close_reason close_reason;
        close_reason.code =
            static_cast<websocket::close_code>(m_close_code);
        close_reason.reason = m_close_reason;

        ArmTimer(CLOSE_TIMEOUT, "close");
        const SessionPtr self = shared_from_this();
        auto handler = [self](const beast::error_code& ec) {
            self->OnClose(ec);
        };
        if (m_wss)
            m_wss->async_close(close_reason, handler);
        else
            m_ws->async_close(close_reason, handler);
    }

    void OnClose(const beast::error_code& ec)
    {
        CancelTimer();
        if (m_finished)
            return;
        if (ec) {
            Fail("close failed: " + ec.message());
            return;
        }
        const websocket::close_reason reason =
            m_wss ? m_wss->reason() : m_ws->reason();
        const int code = reason.code == websocket::close_code::none
            ? 1005
            : static_cast<int>(reason.code);
        const std::string text(reason.reason.data(), reason.reason.size());
        Finish(code, text, true, true);
    }

    template<class Rep, class Period>
    void ArmTimer(std::chrono::duration<Rep, Period> timeout,
                  const char* stage)
    {
        const std::uint64_t timer_generation = ++m_timer_generation;
        m_timer.expires_after(timeout);
        const SessionPtr self = shared_from_this();
        m_timer.async_wait(
            [self, timer_generation, stage](const beast::error_code& ec) {
                if (ec || self->m_finished ||
                    timer_generation != self->m_timer_generation) {
                    return;
                }
                self->Fail(std::string(stage) + " timeout");
            });
    }

    void CancelTimer()
    {
        ++m_timer_generation;
        beast::error_code ignored_ec;
        m_timer.cancel(ignored_ec);
    }

    beast::tcp_stream& LowestLayer()
    {
        return m_wss
            ? beast::get_lowest_layer(*m_wss)
            : beast::get_lowest_layer(*m_ws);
    }

    void HardClose()
    {
        m_resolver.cancel();
        CancelTimer();
        try {
            if (m_wss)
                beast::get_lowest_layer(*m_wss).close();
            else if (m_ws)
                beast::get_lowest_layer(*m_ws).close();
        } catch (...) {
        }
    }

    void Fail(const std::string& message)
    {
        if (m_finished)
            return;
        BOOST_LOG_TRIVIAL(error) << "WebSocket proxy session failed: id="
                                 << m_id << ", error=" << message;
        Emit("error", std::string(), 0, message, false);
        Finish(1006, message, false, true);
    }

    void Finish(int code,
                const std::string& reason,
                bool was_clean,
                bool emit_close)
    {
        if (m_finished)
            return;

        m_finished = true;
        m_state = State::Closed;
        HardClose();
        m_write_queue.clear();
        m_queued_bytes = 0;

        if (emit_close)
            Emit("close", std::string(), code, reason, was_clean);

        const SessionPtr self = shared_from_this();
        if (m_terminal_handler)
            m_terminal_handler(self);
    }

    void Emit(const std::string& event,
              const std::string& data,
              int code,
              const std::string& reason,
              bool was_clean)
    {
        if (!m_event_handler)
            return;

        ProxyEvent proxy_event;
        proxy_event.id = m_id;
        proxy_event.generation = m_generation;
        proxy_event.page_token = m_page_token;
        proxy_event.socket_token = m_socket_token;
        proxy_event.event = event;
        proxy_event.data = data;
        proxy_event.code = code;
        proxy_event.reason = reason;
        proxy_event.was_clean = was_clean;
        m_event_handler(shared_from_this(), std::move(proxy_event));
    }

private:
    Strand                              m_executor;
    ssl::context&                       m_ssl_context;
    tcp::resolver                       m_resolver;
    asio::steady_timer                  m_timer;
    std::unique_ptr<PlainStream>        m_ws;
    std::unique_ptr<SecureStream>       m_wss;
    beast::flat_buffer                  m_read_buffer;
    std::deque<std::shared_ptr<const std::string>> m_write_queue;
    std::shared_ptr<const std::string>  m_current_write;
    std::string                         m_ssl_error;
    int                                 m_id;
    std::uint64_t                       m_generation;
    std::string                         m_page_token;
    std::string                         m_socket_token;
    ParsedWsUrl                         m_endpoint;
    EventHandler                        m_event_handler;
    SessionHandler                      m_terminal_handler;
    State                               m_state{State::Created};
    std::uint64_t                       m_timer_generation{0};
    std::size_t                         m_queued_bytes{0};
    int                                 m_close_code{1000};
    std::string                         m_close_reason;
    bool                                m_read_in_progress{false};
    bool                                m_write_in_progress{false};
    bool                                m_close_requested{false};
    bool                                m_finished{false};
};

} // namespace

struct Manager::Impl : public std::enable_shared_from_this<Manager::Impl>
{
    Impl(ScriptSink script_sink)
        : ui_gate(std::make_shared<UiGate>(std::move(script_sink)))
    {
    }

    void Submit(ProxyCommand command)
    {
        if (!accepting.load())
            return;

        std::weak_ptr<Impl> weak_self = shared_from_this();
        asio::post(runtime.executor(),
            [weak_self, command = std::move(command)]() mutable {
                if (auto self = weak_self.lock())
                    self->HandleCommandOnExecutor(std::move(command));
            });
    }

    void SubmitDeviceDetailState(DeviceDetailState state)
    {
        if (!accepting.load())
            return;

        std::weak_ptr<Impl> weak_self = shared_from_this();
        asio::post(runtime.executor(),
            [weak_self, state = std::move(state)]() mutable {
                if (auto self = weak_self.lock())
                    self->device_detail_state = std::move(state);
            });
    }

    void Shutdown() noexcept
    {
        if (!accepting.exchange(false))
            return;

        ui_gate->Invalidate();
        auto done = std::make_shared<std::promise<void>>();
        std::future<void> completed = done->get_future();
        try {
            asio::post(runtime.executor(), [this, done]() {
                ShutdownOnExecutor();
                done->set_value();
            });
            if (completed.wait_for(SHUTDOWN_WAIT) == std::future_status::timeout) {
                BOOST_LOG_TRIVIAL(error) << "WebSocket proxy shutdown timed out";
            }
        } catch (...) {
        }
        runtime.Stop();
    }

private:
    void HandleCommandOnExecutor(ProxyCommand command)
    {
        if (shutting_down)
            return;

        switch (command.type) {
        case CommandType::Reset:
            HandleReset(command.page_token);
            break;
        case CommandType::Open:
            HandleOpen(command);
            break;
        case CommandType::Send:
            HandleSend(command);
            break;
        case CommandType::Close:
            HandleClose(command);
            break;
        }
    }

    void HandleReset(const std::string& new_page_token)
    {
        if (page_token == new_page_token)
            return;

        page_token = new_page_token;
        ++generation;
        device_detail_state = {};
        ui_gate->SetGeneration(generation);

        std::vector<SessionPtr> old_sessions;
        old_sessions.reserve(sessions.size());
        for (auto& item : sessions)
            old_sessions.emplace_back(std::move(item.second));
        sessions.clear();

        for (const SessionPtr& session : old_sessions) {
            if (session)
                session->Abort();
        }
    }

    void HandleOpen(const ProxyCommand& command)
    {
        if (!IsCurrentPage(command.page_token))
            return;

        const std::optional<ParsedWsUrl> endpoint = parse_ws_url(command.url);
        const bool endpoint_allowed = endpoint &&
            is_device_proxy_endpoint(*endpoint);
        if (!endpoint_allowed) {
            const auto existing_it = sessions.find(command.id);
            if (existing_it != sessions.end()) {
                SessionPtr old_session = std::move(existing_it->second);
                sessions.erase(existing_it);
                if (old_session)
                    old_session->Abort();
            }
            EmitProtocolFailure(command.id, command.socket_token,
                                endpoint ? "WebSocket URL is not allowed by proxy policy"
                                         : "invalid WebSocket URL");
            return;
        }

        const auto existing_it = sessions.find(command.id);
        if (existing_it == sessions.end() && sessions.size() >= MAX_SESSIONS) {
            EmitProtocolFailure(command.id, command.socket_token,
                                "WebSocket session limit reached");
            return;
        }

        std::weak_ptr<Impl> weak_self = shared_from_this();
        SessionPtr session = std::make_shared<Session>(
            runtime.executor(),
            runtime.ssl_context(),
            runtime.ssl_error(),
            command.id,
            generation,
            page_token,
            command.socket_token,
            *endpoint,
            [weak_self](const SessionPtr& source, ProxyEvent event) {
                if (auto self = weak_self.lock())
                    self->OnSessionEvent(source, std::move(event));
            },
            [weak_self](const SessionPtr& source) {
                if (auto self = weak_self.lock())
                    self->OnSessionTerminal(source);
            });

        SessionPtr old_session;
        if (existing_it != sessions.end())
            old_session = existing_it->second;
        sessions[command.id] = session;

        if (old_session)
            old_session->Abort();
        session->Start();
    }

    void HandleSend(const ProxyCommand& command)
    {
        const SessionPtr session = FindCurrentSession(command);
        if (session)
            session->EnqueueText(command.data);
    }

    void HandleClose(const ProxyCommand& command)
    {
        const SessionPtr session = FindCurrentSession(command);
        if (session)
            session->RequestClose(command.close_code, command.close_reason);
    }

    SessionPtr FindCurrentSession(const ProxyCommand& command)
    {
        if (!IsCurrentPage(command.page_token))
            return {};

        const auto it = sessions.find(command.id);
        if (it == sessions.end() ||
            it->second->socket_token() != command.socket_token) {
            return {};
        }
        return it->second;
    }

    bool IsCurrentPage(const std::string& token) const
    {
        return !page_token.empty() && token == page_token;
    }

    void OnSessionEvent(const SessionPtr& source, ProxyEvent event)
    {
        if (!source || event.generation != generation ||
            event.page_token != page_token) {
            return;
        }

        const auto it = sessions.find(event.id);
        if (it == sessions.end() || it->second.get() != source.get())
            return;

        if (event.event == "message" && source->is_device_connection()) {
            const DeviceIdentity source_device{source->host(), {}};
            DeviceMessageFilterResult filter_result = FilterDeviceMessage(
                device_detail_state, source_device, event.data);
            if (filter_result.action == DeviceMessageFilterAction::Drop)
                return;
            if (filter_result.action == DeviceMessageFilterAction::UseModified)
                event.data = std::move(filter_result.modified_payload);
        }

        const UiGate::PostResult result = ui_gate->Post(event);
        if (result == UiGate::PostResult::Overflow &&
            event.event == "message") {
            source->FailEventDelivery();
        }
    }

    void OnSessionTerminal(const SessionPtr& source)
    {
        if (!source)
            return;

        const auto it = sessions.find(source->id());
        if (it != sessions.end() && it->second.get() == source.get() &&
            source->generation() == generation) {
            sessions.erase(it);
        }
    }

    void EmitProtocolFailure(int id,
                             const std::string& socket_token,
                             const std::string& message)
    {
        ProxyEvent error_event;
        error_event.id = id;
        error_event.generation = generation;
        error_event.page_token = page_token;
        error_event.socket_token = socket_token;
        error_event.event = "error";
        error_event.reason = message;
        ui_gate->Post(error_event);

        ProxyEvent close_event;
        close_event.id = id;
        close_event.generation = generation;
        close_event.page_token = page_token;
        close_event.socket_token = socket_token;
        close_event.event = "close";
        close_event.code = 1006;
        close_event.reason = message;
        ui_gate->Post(close_event);
    }

    void ShutdownOnExecutor()
    {
        if (shutting_down)
            return;
        shutting_down = true;
        ++generation;
        page_token.clear();
        ui_gate->SetGeneration(generation);

        std::vector<SessionPtr> old_sessions;
        old_sessions.reserve(sessions.size());
        for (auto& item : sessions)
            old_sessions.emplace_back(std::move(item.second));
        sessions.clear();

        for (const SessionPtr& session : old_sessions) {
            if (session)
                session->Abort();
        }
    }

private:
    std::shared_ptr<UiGate>                   ui_gate;
    Runtime                                   runtime;
    DeviceDetailState                         device_detail_state;
    std::string                               page_token;
    std::uint64_t                             generation{0};
    std::unordered_map<int, SessionPtr>       sessions;
    std::atomic<bool>                         accepting{true};
    bool                                      shutting_down{false};
};

Manager::Manager(ScriptSink script_sink)
    : m_impl(std::make_shared<Impl>(std::move(script_sink)))
{
}

Manager::~Manager()
{
    Shutdown();
}

void Manager::HandleCommand(const nlohmann::json& payload)
{
    const std::shared_ptr<Impl> impl = m_impl;
    if (!impl)
        return;

    try {
        const std::optional<ProxyCommand> command = parse_command(payload);
        if (!command) {
            BOOST_LOG_TRIVIAL(warning) << "Ignored invalid WebSocket proxy command";
            return;
        }
        impl->Submit(*command);
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Failed to parse WebSocket proxy command: "
                                 << e.what();
    } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "Failed to parse WebSocket proxy command";
    }
}

void Manager::SetDeviceDetailState(DeviceDetailState state)
{
    const std::shared_ptr<Impl> impl = m_impl;
    if (impl)
        impl->SubmitDeviceDetailState(std::move(state));
}

void Manager::Shutdown() noexcept
{
    const std::shared_ptr<Impl> impl = m_impl;
    if (impl)
        impl->Shutdown();
}

} // namespace WebSocketProxy
} // namespace GUI
} // namespace Slic3r
