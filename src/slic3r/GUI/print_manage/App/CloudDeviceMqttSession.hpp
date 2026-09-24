#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <string>

namespace Slic3r { namespace GUI {

// One worker owns all transport operations. Callbacks only enqueue notifications.
class CloudDeviceMqttSession final {
public:
    struct Config {
        std::string broker, client_id, username, password, user_id, region;
        bool operator==(const Config& other) const;
    };
    struct Result {
        bool ok = true;
        bool authentication_failed = false;
        std::string error; // Stable operation/error code, never credentials or payloads.
    };
    struct Message { std::string topic, payload, address; };
    struct Event {
        std::uint64_t generation = 0;
        std::string address, user_id, region, state, error, payload;
        unsigned attempt = 0;
        std::chrono::milliseconds retry_delay{0};
    };
    class Transport {
    public:
        using Receive = std::function<void(Message)>;
        using Lost = std::function<void()>;
        virtual ~Transport() = default;
        virtual Result Connect(const Config&, Receive, Lost) = 0;
        virtual Result Subscribe() = 0;
        virtual Result Monitor(const std::string& address, bool enable) = 0;
        virtual Result Acknowledge(const std::string& topic) = 0;
    };
    using Factory = std::function<std::unique_ptr<Transport>()>;
    using Sink = std::function<void(Event)>;
    using RetryDelay = std::function<std::chrono::milliseconds(unsigned)>;

    CloudDeviceMqttSession(Factory, Sink, RetryDelay = {});
    ~CloudDeviceMqttSession();
    CloudDeviceMqttSession(const CloudDeviceMqttSession&) = delete;
    CloudDeviceMqttSession& operator=(const CloudDeviceMqttSession&) = delete;
    void SetTarget(Config config, std::string address);
    bool IsCurrent(std::uint64_t generation) const;
    // Nonblocking for the UI. The worker retains its state until bounded I/O ends.
    std::shared_future<void> Shutdown();

private:
    struct Impl;
    std::shared_ptr<Impl> m_impl;
};

std::unique_ptr<CloudDeviceMqttSession::Transport> MakeCloudDeviceMqttTransport();

}} // namespace Slic3r::GUI
