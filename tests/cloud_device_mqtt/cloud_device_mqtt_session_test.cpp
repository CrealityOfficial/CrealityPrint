#include "CloudDeviceMqttSession.hpp"
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
std::unique_ptr<Session> Start(std::shared_ptr<Harness> h) {
    return std::make_unique<Session>([h] { return std::make_unique<FakeTransport>(h); },
        [h](Session::Event e) { std::lock_guard<std::mutex> lock(h->mutex); h->events.push_back(e); h->cv.notify_all(); },
        [](unsigned) { return 1ms; });
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
    std::cout << "Cloud MQTT session tests passed\n";
}
