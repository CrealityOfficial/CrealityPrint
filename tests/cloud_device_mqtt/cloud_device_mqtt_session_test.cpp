#include "CloudDeviceMqttSession.hpp"
#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using Session = Slic3r::GUI::CloudDeviceMqttSession;
using namespace std::chrono_literals;

struct Harness {
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::string> operations;
    std::vector<Session::Event> events;
    std::vector<Session::Transport::Lost> losses;
    std::vector<Session::Transport::Receive> receivers;
    int connect_failures = 0, subscribe_failures = 0, monitor_failures = 0;
    bool auth_failure = false, block_connect = false, release = false;
    int alive = 0, maximum_alive = 0;
    void Wait(std::function<bool()> predicate) {
        std::unique_lock<std::mutex> lock(mutex);
        if (!cv.wait_for(lock, 3s, predicate)) { std::cerr << "Timed out\n"; std::abort(); }
    }
    size_t Count(const std::string& state) {
        size_t n = 0; for (const auto& e : events) n += e.state == state; return n;
    }
};
struct FakeTransport : Session::Transport {
    std::shared_ptr<Harness> h;
    FakeTransport(std::shared_ptr<Harness> h) : h(h) {
        std::lock_guard<std::mutex> lock(h->mutex);
        h->maximum_alive = std::max(h->maximum_alive, ++h->alive);
    }
    ~FakeTransport() override { std::lock_guard<std::mutex> lock(h->mutex); --h->alive; h->cv.notify_all(); }
    Session::Result Connect(const Session::Config&, Receive receive, Lost lost) override {
        std::unique_lock<std::mutex> lock(h->mutex);
        h->operations.push_back("connect"); h->receivers.push_back(receive); h->losses.push_back(lost); h->cv.notify_all();
        if (h->block_connect) h->cv.wait(lock, [&] { return h->release; });
        if (h->auth_failure) return {false, true, "connect_auth"};
        if (h->connect_failures-- > 0) return {false, false, "connect_timeout"};
        return {};
    }
    Session::Result Subscribe() override {
        std::lock_guard<std::mutex> lock(h->mutex); h->operations.push_back("subscribe");
        if (h->subscribe_failures-- > 0) return {false, false, "subscribe_timeout"}; return {};
    }
    Session::Result Monitor(const std::string& dn, bool enabled) override {
        std::lock_guard<std::mutex> lock(h->mutex); h->operations.push_back((enabled ? "add:" : "del:") + dn);
        if (enabled && h->monitor_failures-- > 0) return {false, false, "monitor_timeout"}; return {};
    }
    Session::Result Acknowledge(const std::string&) override { return {}; }
};
Session::Config Config() { return {"broker", "client", "username", "token", "user", "China"}; }
std::unique_ptr<Session> Start(std::shared_ptr<Harness> h, Session::DataTimeouts timeouts = {}) {
    return std::make_unique<Session>([h] { return std::make_unique<FakeTransport>(h); },
        [h](Session::Event e) { std::lock_guard<std::mutex> lock(h->mutex); h->events.push_back(e); h->cv.notify_all(); },
        [](unsigned) { return 1ms; }, timeouts);
}
int main() {
    for (int failure = 0; failure < 3; ++failure) {
        auto h = std::make_shared<Harness>();
        if (failure == 0) h->connect_failures = 1;
        if (failure == 1) h->subscribe_failures = 1;
        if (failure == 2) h->monitor_failures = 1;
        auto session = Start(h); session->SetTarget(Config(), "A");
        h->Wait([&] { return h->Count("monitoring") == 1; });
        assert(h->Count("retrying") == 1);
        assert(session->Shutdown().wait_for(1s) == std::future_status::ready);
        assert(h->maximum_alive == 1); assert(h->alive == 0);
    }
    {
        auto h = std::make_shared<Harness>(); auto session = Start(h);
        session->SetTarget(Config(), "A"); h->Wait([&] { return h->Count("monitoring") == 1; });
        auto lost = h->losses.back(); auto old_receive = h->receivers.back();
        lost(); lost(); lost();
        h->Wait([&] { return h->Count("monitoring") == 2; });
        session->SetTarget(Config(), "B"); h->Wait([&] { return h->Count("monitoring") == 3; });
        old_receive({"topic", "old", "A"});
        h->receivers.back()({"topic", "new", "B"});
        h->Wait([&] { return h->Count("message") == 1; });
        assert(h->events.back().payload == "new");
        assert(session->Shutdown().wait_for(1s) == std::future_status::ready);
        assert(h->maximum_alive == 1);
    }
    {
        auto h = std::make_shared<Harness>(); h->auth_failure = true; auto session = Start(h);
        session->SetTarget(Config(), "A"); h->Wait([&] { return h->Count("auth_required") == 1; });
        assert(h->Count("retrying") == 0);
        { std::lock_guard<std::mutex> lock(h->mutex); h->auth_failure = false; }
        auto config = Config(); config.password = "new-token"; session->SetTarget(config, "A");
        h->Wait([&] { return h->Count("monitoring") == 1; });
        assert(session->Shutdown().wait_for(1s) == std::future_status::ready);
    }
    {
        auto h = std::make_shared<Harness>(); h->block_connect = true; auto session = Start(h);
        session->SetTarget(Config(), "A"); h->Wait([&] { return !h->operations.empty(); });
        session->SetTarget(Config(), "B");
        { std::lock_guard<std::mutex> lock(h->mutex); h->release = true; h->cv.notify_all(); }
        h->Wait([&] { return h->Count("monitoring") == 1; });
        assert(h->events.back().address == "B");
        for (const auto& op : h->operations) assert(op != "add:A");
        auto generation = h->events.back().generation;
        assert(session->Shutdown().wait_for(1s) == std::future_status::ready);
        assert(!session->IsCurrent(generation));
    }
    {
        auto h = std::make_shared<Harness>(); h->block_connect = true; auto session = Start(h);
        session->SetTarget(Config(), "A"); h->Wait([&] { return !h->operations.empty(); });
        auto done = session->Shutdown();
        session.reset(); // The UI owner can disappear while connect is still in flight.
        { std::lock_guard<std::mutex> lock(h->mutex); h->release = true; h->cv.notify_all(); }
        assert(done.wait_for(1s) == std::future_status::ready);
        assert(h->Count("monitoring") == 0); assert(h->alive == 0);
    }
    // Logout must invalidate old deliveries without permanently stopping the session.
    for (bool change_account : {false, true}) {
        auto h = std::make_shared<Harness>(); auto session = Start(h);
        session->SetTarget(Config(), "A");
        h->Wait([&] { return h->Count("monitoring") == 1; });
        auto old_receive = h->receivers.back();
        auto old_generation = h->events.back().generation;
        session->SetTarget({}, {});
        assert(!session->IsCurrent(old_generation));
        h->Wait([&] { return h->Count("stopped") == 1; });
        auto config = Config();
        if (change_account) { config.user_id = "other-user"; config.password = "other-token"; }
        session->SetTarget(config, "A");
        h->Wait([&] { return h->Count("monitoring") == 2; });
        old_receive({"topic", "old-account", "A"});
        h->receivers.back()({"topic", "new-account", "A"});
        h->Wait([&] { return h->Count("message") == 1; });
        assert(session->Shutdown().wait_for(1s) == std::future_status::ready);
        assert(h->Count("message") == 1);
        assert(h->events.back().payload == "new-account");
        assert(h->events.back().user_id == config.user_id);
        assert(h->maximum_alive == 1);
    }
    // Recover without a connection_lost callback, including when unrelated RPCs continue.
    for (int scenario = 0; scenario < 3; ++scenario) {
        const bool was_live = scenario == 2;
        auto h = std::make_shared<Harness>(); auto session = Start(h, {200ms, 200ms});
        session->SetTarget(Config(), "A");
        h->Wait([&] { return h->Count("monitoring") >= 1; });
        Session::Transport::Receive receive;
        { std::lock_guard<std::mutex> lock(h->mutex); receive = h->receivers.back(); }
        if (was_live) {
            receive({"topic", "first", "A"});
            h->Wait([&] { return h->Count("live") == 1; });
        }
        if (scenario != 0) {
            const auto limit = std::chrono::steady_clock::now() + 2s;
            while (std::chrono::steady_clock::now() < limit) {
                // Same target/config from the UI timer must not reset the deadline either.
                session->SetTarget(Config(), "A");
                receive({"topic", "other-device", "B"});
                std::unique_lock<std::mutex> lock(h->mutex);
                if (h->cv.wait_for(lock, 20ms, [&] { return h->Count("retrying") != 0; })) break;
            }
        }
        h->Wait([&] { return h->Count("monitoring") >= 2; });
        assert(session->Shutdown().wait_for(1s) == std::future_status::ready);
        auto retry = std::find_if(h->events.begin(), h->events.end(),
            [](const Session::Event& event) { return event.state == "retrying"; });
        assert(retry != h->events.end());
        assert(retry->error == (was_live ? "device_data_timeout" : "first_message_timeout"));
        assert(h->Count("message") == (was_live ? 1u : 0u));
        assert(std::count(h->operations.begin(), h->operations.end(), "subscribe") >= 2);
        assert(std::count(h->operations.begin(), h->operations.end(), "add:A") >= 2);
        assert(h->maximum_alive == 1);
    }
    // Matching telemetry renews the idle deadline; recovery starts only after it stops.
    {
        auto h = std::make_shared<Harness>(); auto session = Start(h, {1s, 500ms});
        session->SetTarget(Config(), "A");
        h->Wait([&] { return h->Count("monitoring") == 1; });
        auto receive = h->receivers.back();
        for (size_t n = 1; n <= 8; ++n) {
            receive({"topic", "fresh", "A"});
            h->Wait([&] { return h->Count("message") >= n; });
            std::unique_lock<std::mutex> lock(h->mutex);
            assert(!h->cv.wait_for(lock, 100ms, [&] { return h->Count("retrying") != 0; }));
        }
        h->Wait([&] { return h->Count("retrying") >= 1; });
        assert(session->Shutdown().wait_for(1s) == std::future_status::ready);
        assert(h->Count("live") == 1);
    }
    std::cout << "Cloud MQTT session tests passed\n";
}
