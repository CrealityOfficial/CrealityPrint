#ifndef slic3r_GUI_simple_bridge_SAgentMqttBridge_hpp_
#define slic3r_GUI_simple_bridge_SAgentMqttBridge_hpp_

#include "nlohmann/json.hpp"

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class MQTTClient;

namespace Slic3r {
namespace GUI {
namespace Bridge {

class SAgentMqttBridge
{
public:
    struct Config {
        std::string host;
        int port = 1883;
        std::string username;
        std::string password;
        std::string topic_prefix = "sagent";
        std::string tenant_id = "default";
        std::string client_id;
        std::string session_id;
    };

    struct StatusSnapshot {
        bool connected = false;
        bool connecting = false;
        std::string broker_url;
        std::string request_topic;
        std::string workflow_update_topic;
        std::string topic_prefix = "sagent";
        std::string tenant_id = "default";
        std::string client_id;
        std::string session_id;
        std::string last_error;
        std::string last_error_code;
        std::string last_disconnect_cause;
        int reconnect_attempt = 0;
        bool reconnect_scheduled = false;
    };

    using Json = nlohmann::json;
    using MessageHandler = std::function<void(const Json&)>;
    using StatusHandler = std::function<void(const StatusSnapshot&)>;

    SAgentMqttBridge();
    ~SAgentMqttBridge();

    void SetMessageHandler(MessageHandler handler);
    void SetStatusHandler(StatusHandler handler);

    bool Start(const Config& config);
    void Stop();
    bool Publish(const std::string& topic, const Json& payload, int qos = 1);
    bool PublishToolResponse(const std::string& kind, const Json& payload, int qos = 1);
    StatusSnapshot GetStatus() const;

private:
    static std::string NormalizeTopicPart(const std::string& value, const std::string& fallback);
    static bool IsAllowedToolResponseKind(const std::string& kind);
    static std::string ClassifyErrorCode(const std::string& message);

    std::string BuildToolTopic(const std::string& kind) const;
    std::string BuildWorkflowUpdateTopic() const;
    bool IsAllowedPublishTopic(const std::string& topic) const;
    bool ConnectClient(std::unique_ptr<MQTTClient>* out_client, std::string* error_message, std::string* error_code);
    bool SubscribeClient(MQTTClient* client, std::string* error_message, std::string* error_code);
    void InitialConnect(unsigned long long generation);
    void StopConnectThread();
    void HandleIncomingMessage(const std::string& topic, const std::string& payload);
    void HandleConnectionLost(const std::string& cause);
    void StartReconnectLoop(const std::string& message, const std::string& code, const std::string& disconnect_cause);
    void ReconnectLoop(unsigned long long generation);
    void StopReconnectLoop();
    void SetConnected(bool connected);
    void SetError(
        const std::string& message,
        const std::string& code = std::string(),
        const std::string& disconnect_cause = std::string());
    void EmitStatus();

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_reconnect_cv;
    std::unique_ptr<MQTTClient> m_client;
    std::thread m_connect_thread;
    std::thread m_reconnect_thread;
    Config m_config;
    MessageHandler m_message_handler;
    StatusHandler m_status_handler;
    StatusSnapshot m_status;
    bool m_stop_reconnect = false;
    bool m_connect_running = false;
    bool m_reconnect_running = false;
    unsigned long long m_connect_generation = 0;
    unsigned long long m_reconnect_generation = 0;
};

} // namespace Bridge
} // namespace GUI
} // namespace Slic3r

#endif
