#include "DeviceImageProxy.hpp"

#include "libslic3r/Utils.hpp"
#include "slic3r/Utils/DeviceTlsPolicy.hpp"

#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/log/trivial.hpp>

#include <chrono>
#include <cstdint>
#include <deque>
#include <utility>

namespace Slic3r { namespace GUI {

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace ssl   = asio::ssl;
using tcp       = asio::ip::tcp;

namespace {

constexpr std::size_t MAX_ACTIVE_REQUESTS = 4;  //4个异步
constexpr std::size_t MAX_QUEUED_REQUESTS = 64;  //64个排队
constexpr std::uint64_t MAX_IMAGE_BODY_SIZE = 10ULL * 1024ULL * 1024ULL; // 10 MB

constexpr auto CONNECT_TIMEOUT   = std::chrono::seconds(3);   // TCP 连接3秒连接超时
constexpr auto HANDSHAKE_TIMEOUT = std::chrono::seconds(5);   // TLS 握手5秒握手超时
constexpr auto WRITE_TIMEOUT     = std::chrono::seconds(5);   // 5秒写入超时
constexpr auto READ_TIMEOUT      = std::chrono::seconds(10);  // 10秒读取超时
constexpr auto QUEUE_TIMEOUT     = std::chrono::seconds(10);  // 10秒排队超时

bool is_timeout_error(const beast::error_code& ec)
{
    return ec == beast::error::timeout || ec == asio::error::timed_out;
}

//负责网络通信
class DeviceImageOperation : public std::enable_shared_from_this<DeviceImageOperation>
{
public:
    using Completion = DeviceImageProxy::Completion;

    DeviceImageOperation(asio::io_context& io_context,
                         ssl::context& ssl_context,
                         DeviceImageRequest request,
                         Completion completion)
        : m_device_request(std::move(request))
        , m_completion(std::move(completion))
    {
        if (m_device_request.secure)
            m_https_stream = std::make_unique<SecureStream>(io_context, ssl_context);
        else
            m_http_stream = std::make_unique<beast::tcp_stream>(io_context);
        m_parser.body_limit(MAX_IMAGE_BODY_SIZE);
    }

    void run()
    {
        beast::error_code ec;
        const asio::ip::address address = asio::ip::make_address(m_device_request.address, ec);
        if (ec || !address.is_v4()) {
            DeviceImageResult result;
            result.status_code = 400;
            result.error = "Invalid device IP address";
            complete(std::move(result));
            return;
        }

        m_request.method(http::verb::get);
        m_request.version(11);
        m_request.target(m_device_request.target_path);
        m_request.set(http::field::host, m_device_request.address);
        m_request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        m_request.set(http::field::accept, "image/*");
        m_request.set(http::field::connection, "close");

        lowest_layer().expires_after(CONNECT_TIMEOUT);
        const auto self = shared_from_this();
        lowest_layer().async_connect(
            tcp::endpoint(address, m_device_request.secure ? 443 : 80),
            [self](const beast::error_code& connect_ec) {
                self->on_connect(connect_ec);
            });
    }

private:
    void on_connect(const beast::error_code& ec)
    {
        if (ec) {
            fail("connect", ec);
            return;
        }

        if (!m_device_request.secure) {
            write_request();
            return;
        }

        // verify_peer checks the certificate chain. No host/IP verifier is installed.
        beast::error_code verify_ec;
        m_https_stream->set_verify_mode(ssl::verify_peer, verify_ec);
        if (verify_ec) {
            fail("configure TLS verification", verify_ec);
            return;
        }
        lowest_layer().expires_after(HANDSHAKE_TIMEOUT);

        const auto self = shared_from_this();
        m_https_stream->async_handshake(ssl::stream_base::client,
            [self](const beast::error_code& handshake_ec) {
                self->on_handshake(handshake_ec);
            });
    }

    void on_handshake(const beast::error_code& ec)
    {
        if (ec) {
            fail("TLS handshake", ec);
            return;
        }

        write_request();
    }

    void write_request()
    {
        lowest_layer().expires_after(WRITE_TIMEOUT);
        const auto self = shared_from_this();
        const auto handler = [self](const beast::error_code& write_ec, std::size_t) {
            self->on_write(write_ec);
        };
        if (m_https_stream)
            http::async_write(*m_https_stream, m_request, handler);
        else
            http::async_write(*m_http_stream, m_request, handler);
    }

    void on_write(const beast::error_code& ec)
    {
        if (ec) {
            fail("write request", ec);
            return;
        }

        lowest_layer().expires_after(READ_TIMEOUT);
        const auto self = shared_from_this();
        const auto handler = [self](const beast::error_code& read_ec, std::size_t) {
            self->on_read(read_ec);
        };
        if (m_https_stream)
            http::async_read(*m_https_stream, m_buffer, m_parser, handler);
        else
            http::async_read(*m_http_stream, m_buffer, m_parser, handler);
    }

    void on_read(const beast::error_code& ec)
    {
        if (ec) {
            fail("read response", ec);
            return;
        }

        auto response = m_parser.release();

        DeviceImageResult result;
        result.status_code = response.result_int();

        const auto content_type = response.find(http::field::content_type);
        if (content_type != response.end()) {
            const auto value = content_type->value();
            result.content_type.assign(value.data(), value.size());
        }

        if (response.result() != http::status::ok) {
            result.error = "Device returned HTTP status " + std::to_string(result.status_code);
            complete(std::move(result));
            return;
        }

        result.body = std::move(response.body());
        if (result.body.empty()) {
            result.status_code = 502;
            result.error = "Device returned an empty image";
        }

        complete(std::move(result));
    }

    void fail(const char* stage, const beast::error_code& ec)
    {
        if (ec != asio::error::operation_aborted) {
            BOOST_LOG_TRIVIAL(error) << "Device image "
                                     << (m_device_request.secure ? "HTTPS " : "HTTP ") << stage
                                     << " failed: address=" << m_device_request.address
                                     << ", target=" << m_device_request.target_path
                                     << ", error=" << ec.message();
        }

        DeviceImageResult result;
        result.status_code = is_timeout_error(ec) ? 504 : 502;
        result.error = std::string(stage) + " failed: " + ec.message();
        complete(std::move(result));
    }

    void complete(DeviceImageResult result)
    {
        if (m_completed)
            return;
        m_completed = true;

        beast::error_code ignored_ec;
        auto& socket = lowest_layer().socket();
        socket.shutdown(tcp::socket::shutdown_both, ignored_ec);
        socket.close(ignored_ec);

        auto completion = std::move(m_completion);
        if (completion)
            completion(std::move(result));
    }

private:
    using SecureStream = beast::ssl_stream<beast::tcp_stream>;

    beast::tcp_stream& lowest_layer()
    {
        return m_https_stream ? beast::get_lowest_layer(*m_https_stream) : *m_http_stream;
    }

    std::unique_ptr<beast::tcp_stream>                           m_http_stream;
    std::unique_ptr<SecureStream>                                m_https_stream;
    beast::flat_buffer                                           m_buffer;
    http::request<http::empty_body>                              m_request;
    http::response_parser<http::vector_body<unsigned char>>      m_parser;
    DeviceImageRequest                                           m_device_request;
    Completion                                                   m_completion;
    bool                                                         m_completed{false};
};

} // namespace

//负责管理多个请求
class DeviceImageProxy::Impl
{
public:
    explicit Impl(asio::io_context& io_context)
        : m_io_context(io_context)
        , m_ssl_context(ssl::context::tls_client)
    {
        beast::error_code ec;
        m_ssl_context.set_options(ssl::context::default_workarounds |
                                  ssl::context::no_sslv2 |
                                  ssl::context::no_sslv3,
                                  ec);
        if (!ec)
            m_ssl_context.set_verify_mode(ssl::verify_peer, ec);
        if (!ec)
            m_ssl_context.load_verify_file(Slic3r::resources_dir() + "/cert/ca.crt", ec);

        if (!ec) {
            std::string policy_error;
            if (!Slic3r::DeviceTlsPolicy::ignore_certificate_time(m_ssl_context.native_handle(), policy_error)) {
                m_initialization_error = "Failed to configure device TLS policy: " + policy_error;
                BOOST_LOG_TRIVIAL(error) << "Device image proxy TLS policy initialization failed: "
                                          << policy_error;
            }
        }

        if (ec) {
            m_initialization_error = ec.message();
            BOOST_LOG_TRIVIAL(error) << "Device image proxy SSL initialization failed: "
                                     << m_initialization_error;
        }
    }

    void async_get(DeviceImageRequest request, Completion completion)
    {
        if (!completion)
            return;

        if (request.secure && !m_initialization_error.empty()) {
            DeviceImageResult result;
            result.status_code = 502;
            result.error = "SSL initialization failed: " + m_initialization_error;
            completion(std::move(result));
            return;
        }

        PendingRequest pending{
            std::move(request),
            std::move(completion),
            std::chrono::steady_clock::now()
        };

        if (m_active_requests < MAX_ACTIVE_REQUESTS) {
            start_request(std::move(pending));
            return;
        }

        if (m_pending_requests.size() >= MAX_QUEUED_REQUESTS) {
            DeviceImageResult result;
            result.status_code = 503;
            result.error = "Device image request queue is full";
            pending.completion(std::move(result));
            return;
        }

        m_pending_requests.emplace_back(std::move(pending));
    }

private:
    struct PendingRequest
    {
        DeviceImageRequest                 request;
        Completion                         completion;
        std::chrono::steady_clock::time_point queued_at;
    };

    void start_request(PendingRequest pending)
    {
        ++m_active_requests;

        auto completion = std::move(pending.completion);
        auto operation = std::make_shared<DeviceImageOperation>(
            m_io_context,
            m_ssl_context,
            std::move(pending.request),
            [this, completion = std::move(completion)](DeviceImageResult result) mutable {
                if (m_active_requests > 0)
                    --m_active_requests;
                start_next_request();
                completion(std::move(result));
            });
        operation->run();
    }

    void start_next_request()
    {
        while (m_active_requests < MAX_ACTIVE_REQUESTS && !m_pending_requests.empty()) {
            PendingRequest pending = std::move(m_pending_requests.front());
            m_pending_requests.pop_front();

            if (std::chrono::steady_clock::now() - pending.queued_at > QUEUE_TIMEOUT) {
                DeviceImageResult result;
                result.status_code = 504;
                result.error = "Device image request queue timeout";
                pending.completion(std::move(result));
                continue;
            }

            start_request(std::move(pending));
        }
    }

private:
    asio::io_context&           m_io_context;
    ssl::context                m_ssl_context;
    std::deque<PendingRequest>  m_pending_requests;
    std::size_t                 m_active_requests{0};
    std::string                 m_initialization_error;
};

DeviceImageProxy::DeviceImageProxy(asio::io_context& io_context)
    : m_impl(std::make_unique<Impl>(io_context))
{}

DeviceImageProxy::~DeviceImageProxy() = default;

void DeviceImageProxy::async_get(DeviceImageRequest request, Completion completion)
{
    m_impl->async_get(std::move(request), std::move(completion));
}

}} // namespace Slic3r::GUI
