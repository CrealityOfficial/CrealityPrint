#include "CloudDeviceMqttSession.hpp"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>

namespace Slic3r { namespace GUI {

bool CloudDeviceMqttSession::Config::operator==(const Config& o) const
{
    return broker == o.broker && client_id == o.client_id && username == o.username &&
        password == o.password && user_id == o.user_id && region == o.region;
}

struct CloudDeviceMqttSession::Impl : std::enable_shared_from_this<Impl> {
    struct Inbox { bool lost = false; std::deque<Message> messages; };
    std::mutex mutex;
    std::condition_variable cv;
    Config desired;
    std::string address;
    std::uint64_t generation = 0;
    bool stopped = false;
    Factory factory;
    Sink sink;
    RetryDelay delay;
    std::promise<void> completion;
    std::shared_future<void> done = completion.get_future().share();

    Impl(Factory f, Sink s, RetryDelay d) : factory(std::move(f)), sink(std::move(s)), delay(std::move(d)) {}

    bool Current(std::uint64_t g) {
        std::lock_guard<std::mutex> lock(mutex);
        return !stopped && generation == g;
    }
    void Emit(std::uint64_t g, const Config& config, const std::string& dn,
              const std::string& state, const Result& result = {}, unsigned attempt = 0,
              std::chrono::milliseconds retry = {}, std::string payload = {}) {
        if (!Current(g)) return;
        try { sink(Event{g, dn, config.user_id, config.region, state, result.error,
                         std::move(payload), attempt, retry}); } catch (...) {}
    }

    void Run() noexcept {
        try {
            std::uint64_t last = 0;
            for (;;) {
                Config config;
                std::string dn;
                std::uint64_t g;
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    cv.wait(lock, [&] { return stopped || generation != last; });
                    if (stopped) break;
                    g = last = generation;
                    config = desired;
                    dn = address;
                }
                if (dn.empty() || config.user_id.empty() || config.password.empty()) {
                    Emit(g, config, dn, "stopped");
                    continue;
                }
                unsigned attempt = 0;
                while (Current(g)) {
                    auto inbox = std::make_shared<Inbox>();
                    std::weak_ptr<Impl> weak = shared_from_this();
                    Result result;
                    bool monitored = false;
                    std::unique_ptr<Transport> transport;
                    auto stage = [&](const char* state, auto operation) {
                        if (!Current(g) || !result.ok) return;
                        Emit(g, config, dn, state);
                        result = operation();
                    };
                    try {
                        transport = factory();
                        if (!transport) throw std::runtime_error("missing transport");
                        stage("connecting", [&] {
                            return transport->Connect(config,
                                [weak, inbox, g](Message message) {
                                    if (auto self = weak.lock()) {
                                        std::lock_guard<std::mutex> lock(self->mutex);
                                        if (self->stopped || self->generation != g) return;
                                        // Recover rather than grow without bound during a slow broker ACK.
                                        if (inbox->messages.size() >= 256) inbox->lost = true;
                                        else inbox->messages.push_back(std::move(message));
                                        self->cv.notify_all();
                                    }
                                }, [weak, inbox, g] {
                                    if (auto self = weak.lock()) {
                                        std::lock_guard<std::mutex> lock(self->mutex);
                                        if (self->stopped || self->generation != g) return;
                                        inbox->lost = true;
                                        self->cv.notify_all();
                                    }
                                });
                        });
                        stage("subscribing", [&] { return transport->Subscribe(); });
                        stage("restoring", [&] {
                            auto r = transport->Monitor(dn, true);
                            monitored = r.ok;
                            return r;
                        });
                        if (result.ok && Current(g)) {
                            // QoS 0 publish completion is not confirmation of live device data.
                            Emit(g, config, dn, "monitoring");
                            bool live = false;
                            while (Current(g)) {
                                Message message;
                                {
                                    std::unique_lock<std::mutex> lock(mutex);
                                    cv.wait(lock, [&] { return stopped || generation != g ||
                                                               inbox->lost || !inbox->messages.empty(); });
                                    if (stopped || generation != g) break;
                                    if (inbox->lost) { result = {false, false, "connection_lost"}; break; }
                                    message = std::move(inbox->messages.front());
                                    inbox->messages.pop_front();
                                }
                                if (message.address == dn) {
                                    if (!live) {
                                        live = true;
                                        attempt = 0;
                                        Emit(g, config, dn, "live");
                                    }
                                    Emit(g, config, dn, "message", {}, 0, {}, std::move(message.payload));
                                }
                                if (!Current(g)) break;
                                result = transport->Acknowledge(message.topic);
                                if (!result.ok) break;
                            }
                        }
                    } catch (...) { result = {false, false, "transport_exception"}; }
                    // Each attempt has its own callbacks/inbox. Delayed callbacks cannot affect its successor.
                    if (transport && monitored) {
                        try { transport->Monitor(dn, false); } catch (...) {}
                    }
                    transport.reset();
                    if (!Current(g)) break;
                    if (result.authentication_failed) {
                        Emit(g, config, dn, "auth_required", result);
                        break; // A changed credential snapshot starts a fresh generation.
                    }
                    const auto wait = delay(std::min(attempt, 30u));
                    attempt = std::min(attempt + 1, 30u);
                    Emit(g, config, dn, "retrying", result, attempt, wait);
                    std::unique_lock<std::mutex> lock(mutex);
                    cv.wait_for(lock, wait, [&] { return stopped || generation != g; });
                }
            }
        } catch (...) {
            // Keep shutdown completion observable even if allocation or worker setup fails.
        }
        completion.set_value();
    }
};

CloudDeviceMqttSession::CloudDeviceMqttSession(Factory factory, Sink sink, RetryDelay delay)
{
    if (!delay) delay = [](unsigned attempt) {
        thread_local std::mt19937 random(std::random_device{}());
        const int base = std::min(30000, 1000 * (1 << std::min(attempt, 5u)));
        return std::chrono::milliseconds(std::min(30000, base + std::uniform_int_distribution<int>(0, base / 5)(random)));
    };
    m_impl = std::make_shared<Impl>(std::move(factory), std::move(sink), std::move(delay));
    // The detached worker owns Impl, never the view. Shutdown invalidates all UI delivery immediately.
    std::thread([impl = m_impl] { impl->Run(); }).detach();
}
CloudDeviceMqttSession::~CloudDeviceMqttSession() { Shutdown(); }
void CloudDeviceMqttSession::SetTarget(Config config, std::string address)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (m_impl->stopped || (m_impl->desired == config && m_impl->address == address)) return;
    m_impl->desired = std::move(config);
    m_impl->address = std::move(address);
    ++m_impl->generation;
    m_impl->cv.notify_all();
}
bool CloudDeviceMqttSession::IsCurrent(std::uint64_t generation) const { return m_impl->Current(generation); }
std::shared_future<void> CloudDeviceMqttSession::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->stopped = true;
    ++m_impl->generation;
    m_impl->cv.notify_all();
    return m_impl->done;
}

}} // namespace Slic3r::GUI
