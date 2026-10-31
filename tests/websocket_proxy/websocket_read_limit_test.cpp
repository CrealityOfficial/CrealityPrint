#include "slic3r/GUI/print_manage/App/WebSocketReadLimitPolicy.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using Slic3r::GUI::WebSocketProxy::WebSocketReadLimitPolicy;

static void check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::abort();
    }
}

int main()
{
    constexpr std::size_t limit = 4 * 1024 * 1024;
    const std::string oversized = "{\"retMaterials\":\"" + std::string(5 * 1024 * 1024, 'x') + "\"}";
    WebSocketReadLimitPolicy policy(limit);
    asio::io_context server_context;
    tcp::acceptor acceptor(server_context, {asio::ip::address_v4::loopback(), 0});
    const auto port = acceptor.local_endpoint().port();
    const std::string endpoint = "ws://127.0.0.1:" + std::to_string(port) + "/";

    for (int attempt = 0; attempt < 2; ++attempt) {
        const auto read_limit = policy.Limit(endpoint);
        check(read_limit == (attempt == 0 ? limit : 0), "Only the retry should disable the limit");
        std::thread server([&] {
            websocket::stream<tcp::socket> ws(acceptor.accept());
            ws.accept();
            ws.text(true);
            const std::string small = "{\"state\":1}";
            for (int i = 0; i < 3; ++i)
                ws.write(asio::buffer(small));
            if (attempt == 0) {
                // A declared oversized frame fails before its payload arrives.
                std::array<unsigned char, 10> header{0x81, 0x7f};
                for (int i = 0; i < 8; ++i)
                    header[9 - i] = static_cast<unsigned char>(static_cast<std::uint64_t>(oversized.size()) >> (8 * i));
                asio::write(ws.next_layer(), asio::buffer(header));
            } else {
                ws.auto_fragment(true);
                ws.write_buffer_bytes(4096);
                ws.write(asio::buffer(oversized));
                for (const std::string data : {"{\"boxsInfo\":{}}", "{\"videoToken\":\"test\"}", "{\"nozzleTemp\":25}"})
                    ws.write(asio::buffer(data));
            }
            beast::flat_buffer buffer;
            beast::error_code ec;
            ws.read(buffer, ec); // Reply to the client's close handshake.
            check(ec == websocket::error::closed, "Expected a WebSocket close handshake");
        });

        asio::io_context context;
        websocket::stream<beast::tcp_stream> ws(context);
        ws.read_message_max(read_limit);
        ws.next_layer().connect({asio::ip::address_v4::loopback(), port});
        beast::flat_buffer buffer;
        beast::error_code result;
        std::vector<std::size_t> sizes;
        bool completed = false;
        std::function<void()> read_next;
        read_next = [&] {
            ws.next_layer().expires_after(std::chrono::seconds(10));
            ws.async_read(buffer, [&](beast::error_code ec, std::size_t bytes) {
                if (ec) {
                    result = ec;
                    if (ec == websocket::error::message_too_big)
                        policy.Exceeded(endpoint);
                    completed = true;
                    return;
                }
                check(bytes == buffer.size(), "Log sizes must represent a complete message");
                sizes.push_back(buffer.size());
                if (sizes.size() == 4)
                    check(beast::buffers_to_string(buffer.data()) == oversized, "Retry payload must be intact");
                buffer.consume(buffer.size());
                if (sizes.size() == 7)
                    completed = true;
                else
                    read_next();
            });
        };
        ws.next_layer().expires_after(std::chrono::seconds(10));
        ws.async_handshake("127.0.0.1", "/", [&](beast::error_code ec) {
            check(!ec, "Handshake must succeed");
            check(ws.read_message_max() == read_limit, "Handshake must retain the configured limit");
            read_next();
        });
        context.run();
        check(completed, "Read must complete");
        if (attempt == 0) {
            check(result == websocket::error::message_too_big && sizes.size() == 3,
                  "First connection must fail on the fourth message");
            check(buffer.size() == 0, "Declared length must not be reported as received payload bytes");
        } else {
            check(!result && sizes.size() == 7, "Retry must receive the large message and subsequent updates");
            check(sizes[3] == oversized.size(), "Full oversized payload size must be retained");
            std::cout << "Retry received_bytes=" << sizes[3] << ", subsequent_messages=3\n";
            beast::error_code ec;
            ws.close(websocket::close_code::normal, ec);
            check(!ec, "Close must succeed");
        }
        server.join();
    }
    check(policy.Limit("ws://127.0.0.2:9999/") == limit, "Other devices must keep their limit");
    check(policy.Limit("wss://127.0.0.1:" + std::to_string(port) + "/") == limit,
          "Transport endpoints must have separate fallback state");
    policy.Clear();
    check(policy.Limit(endpoint) == limit, "Page reset must restore the original limit");
    std::cout << "WebSocket read-limit fallback tests passed\n";
}
