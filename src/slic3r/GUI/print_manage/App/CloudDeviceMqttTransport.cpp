#include "CloudDeviceMqttSession.hpp"
#include <mqtt/async_client.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <algorithm>

namespace Slic3r { namespace GUI {
namespace {
using Session = CloudDeviceMqttSession;
constexpr const char* request_prefix = "v1/devices/me/rpc/request/";

class PahoTransport final : public Session::Transport, private mqtt::callback {
    Receive receive;
    Lost lost;
    std::unique_ptr<mqtt::async_client> client;

    template<class Operation>
    Session::Result Perform(const char* operation, Operation run, int seconds = 5) {
        try {
            auto token = run();
            if (!token || !token->wait_for(std::chrono::seconds(seconds)))
                return {false, false, std::string(operation) + "_timeout"};
            return {};
        } catch (const mqtt::exception& e) {
            const int code = e.get_reason_code();
            // MQTT 3.1.1 CONNACK: 4=bad credentials, 5=not authorized.
            const bool auth = std::string(operation) == "connect" &&
                (code == 4 || code == 5 || e.get_return_code() == 4 || e.get_return_code() == 5);
            return {false, auth, std::string(operation) + "_failed_" + std::to_string(code)};
        } catch (...) { return {false, false, std::string(operation) + "_failed"}; }
    }
    void connection_lost(const std::string&) override { if (lost) lost(); }
    void message_arrived(mqtt::const_message_ptr message) override {
        try {
            const auto topic = message->get_topic();
            if (topic.rfind(request_prefix, 0) != 0) return;
            const auto payload = message->get_payload_str();
            auto json = nlohmann::json::parse(payload, nullptr, false);
            std::string address;
            if (json.is_object() && json.contains("params") && json["params"].is_object()) {
                const auto& params = json["params"];
                if (params.contains("deviceIoTchanged") && params["deviceIoTchanged"].is_object()) {
                    const auto& change = params["deviceIoTchanged"];
                    if (change.contains("dn") && change["dn"].is_string() &&
                        (change.contains("timeseries") || change.contains("attributes")))
                        address = change["dn"].get<std::string>();
                }
            }
            if (receive) receive({topic, payload, address});
        } catch (...) { if (lost) lost(); }
    }
public:
    ~PahoTransport() override {
        if (!client) return;
        // Destroy Paho while callback members still exist; callbacks never access the view.
        try {
            client->disable_callbacks();
            if (client->is_connected()) Perform("disconnect", [&] { return client->disconnect(); }, 2);
        } catch (...) {}
        client.reset();
    }
    Session::Result Connect(const Session::Config& config, Receive r, Lost l) override {
        receive = std::move(r);
        lost = std::move(l);
        client = std::make_unique<mqtt::async_client>(config.broker, config.client_id);
        client->set_callback(*this);
        mqtt::connect_options options;
        options.set_clean_session(true);
        options.set_keep_alive_interval(20);
        options.set_connect_timeout(std::chrono::seconds(8));
        options.set_automatic_reconnect(false);
        options.set_mqtt_version(MQTTVERSION_3_1_1);
        options.set_user_name(config.username);
        options.set_password(config.password);
        return Perform("connect", [&] { return client->connect(options); }, 8);
    }
    Session::Result Subscribe() override {
        return Perform("subscribe", [&] { return client->subscribe(std::string(request_prefix) + "+", 0); });
    }
    Session::Result Monitor(const std::string& address, bool enable) override {
        static std::atomic<std::uint64_t> sequence{0};
        nlohmann::json body;
        body["method"] = "set";
        body["params"][enable ? "addMonitorDevice" : "delMonitorDevice"]["dn"] = {address};
        const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        auto previous = sequence.load();
        std::uint64_t id;
        do { id = std::max(static_cast<std::uint64_t>(stamp), previous + 1); }
        while (!sequence.compare_exchange_weak(previous, id));
        return Perform("monitor", [&] { return client->publish(mqtt::make_message(
            "v1/devices/me/attributes/" + std::to_string(id), body.dump(), 0, false)); });
    }
    Session::Result Acknowledge(const std::string& topic) override {
        if (topic.rfind(request_prefix, 0) != 0) return {};
        return Perform("ack", [&] { return client->publish(mqtt::make_message(
            "v1/devices/me/rpc/response/" + topic.substr(std::char_traits<char>::length(request_prefix)), "{\"code\":0}", 0, false)); });
    }
};
}
std::unique_ptr<CloudDeviceMqttSession::Transport> MakeCloudDeviceMqttTransport()
{
    return std::make_unique<PahoTransport>();
}
}} // namespace Slic3r::GUI
