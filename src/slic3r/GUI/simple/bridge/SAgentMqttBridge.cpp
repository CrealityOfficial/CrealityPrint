#include "SAgentMqttBridge.hpp"
#include "slic3r/GUI/print_manage/App/mqtt_client.h"

#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>

#include <algorithm>
#include <chrono>
#include <sstream>
#include <utility>

namespace Slic3r {
namespace GUI {
namespace Bridge {

namespace {

std::string trim_copy(const std::string& value)
{
    return boost::algorithm::trim_copy(value);
}

std::string build_broker_url(const std::string& host, int port)
{
    if (host.rfind("tcp://", 0) == 0 || host.rfind("ssl://", 0) == 0)
        return host;
    return "tcp://" + host + ":" + std::to_string(port > 0 ? port : 1883);
}

std::string build_tool_topic(const SAgentMqttBridge::Config& config, const std::string& kind)
{
    return config.topic_prefix + "/" + config.tenant_id + "/" + config.client_id + "/tool/" + kind;
}

std::string build_workflow_update_topic(const SAgentMqttBridge::Config& config)
{
    return config.topic_prefix + "/" + config.tenant_id + "/" + config.client_id + "/workflow/update";
}

std::string build_bridge_error_message(const std::string& prefix, const std::string& detail)
{
    const std::string normalized_detail = trim_copy(detail);
    if (normalized_detail.empty())
        return prefix;
    return prefix + ": " + normalized_detail;
}

std::string normalize_disconnect_cause(const std::string& cause)
{
    const std::string normalized = trim_copy(cause);
    return normalized.empty() ? "unknown" : normalized;
}

std::string format_reported_error(
    const std::string& code,
    const std::string& message,
    const std::string& disconnect_cause = std::string())
{
    std::ostringstream stream;
    if (!trim_copy(code).empty())
        stream << "[" << trim_copy(code) << "] ";
    stream << trim_copy(message);
    if (!trim_copy(disconnect_cause).empty())
        stream << " (cause=" << trim_copy(disconnect_cause) << ")";
    return trim_copy(stream.str());
}

} // namespace

SAgentMqttBridge::SAgentMqttBridge() = default;

SAgentMqttBridge::~SAgentMqttBridge()
{
    Stop();
}

void SAgentMqttBridge::SetMessageHandler(MessageHandler handler)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_message_handler = std::move(handler);
}

void SAgentMqttBridge::SetStatusHandler(StatusHandler handler)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_status_handler = std::move(handler);
}

bool SAgentMqttBridge::Start(const Config& config)
{
    Stop();

    Config normalized = config;
    normalized.host = trim_copy(normalized.host);
    normalized.topic_prefix = NormalizeTopicPart(normalized.topic_prefix, "sagent");
    normalized.tenant_id = NormalizeTopicPart(normalized.tenant_id, "default");
    normalized.client_id = trim_copy(normalized.client_id);
    normalized.session_id = trim_copy(normalized.session_id);

    if (normalized.host.empty()) {
        SetError("Missing SAgent MQTT host.", "mqtt_invalid_config");
        return false;
    }
    if (normalized.client_id.empty()) {
        SetError("Missing SAgent MQTT client_id.", "mqtt_invalid_config");
        return false;
    }

    const std::string broker_url = build_broker_url(normalized.host, normalized.port);

    BOOST_LOG_TRIVIAL(warning)
        << "[SAgentMQTT][BridgeStart] broker_url=" << broker_url
        << " tenant_id=" << normalized.tenant_id
        << " client_id=" << normalized.client_id
        << " topic_prefix=" << normalized.topic_prefix
        << " session_id=" << normalized.session_id
        << " username=" << normalized.username;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config = normalized;
        m_stop_reconnect = false;
        m_status.connected = false;
        m_status.connecting = true;
        m_status.broker_url = broker_url;
        m_status.topic_prefix = normalized.topic_prefix;
        m_status.tenant_id = normalized.tenant_id;
        m_status.client_id = normalized.client_id;
        m_status.session_id = normalized.session_id;
        m_status.request_topic = build_tool_topic(normalized, "request");
        m_status.workflow_update_topic = build_workflow_update_topic(normalized);
        m_status.last_error.clear();
        m_status.last_error_code.clear();
        m_status.last_disconnect_cause.clear();
        m_status.reconnect_attempt = 0;
        m_status.reconnect_scheduled = false;
    }
    EmitStatus();

    unsigned long long generation = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connect_running = true;
        generation = ++m_connect_generation;
    }
    m_connect_thread = std::thread(&SAgentMqttBridge::InitialConnect, this, generation);
    return true;
}

void SAgentMqttBridge::Stop()
{
    StopConnectThread();
    StopReconnectLoop();

    std::unique_ptr<MQTTClient> client;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop_reconnect = true;
        client = std::move(m_client);
        m_config = Config();
        m_status.connected = false;
        m_status.connecting = false;
        m_status.last_error.clear();
        m_status.last_error_code.clear();
        m_status.last_disconnect_cause.clear();
        m_status.reconnect_attempt = 0;
        m_status.reconnect_scheduled = false;
    }

    if (client) {
        try {
            if (client->isConnected())
                client->disconnect();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning) << "[SAgentMqttBridge] disconnect failed: " << e.what();
        }
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop_reconnect = false;
    }
    EmitStatus();
}

bool SAgentMqttBridge::PublishToolResponse(const std::string& kind, const Json& payload, int qos)
{
    if (!IsAllowedToolResponseKind(kind)) {
        SetError("Rejected SAgent MQTT tool response kind: " + kind);
        return false;
    }
    return Publish(BuildToolTopic(kind), payload, qos);
}

bool SAgentMqttBridge::Publish(const std::string& topic, const Json& payload, int qos)
{
    MQTTClient* client = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        client = m_client.get();
    }

    if (!client || !client->isConnected()) {
        SetError("SAgent MQTT bridge is not connected.", "mqtt_not_connected");
        return false;
    }
    if (!IsAllowedPublishTopic(topic)) {
        SetError("Rejected SAgent MQTT publish topic: " + topic, "mqtt_publish_topic_rejected");
        return false;
    }

    try {
        BOOST_LOG_TRIVIAL(warning)
            << "[SAgentMQTT][BridgePublish] topic=" << topic
            << " qos=" << qos
            << " payload=" << payload.dump();
        if (client->publish(topic, payload.dump(), qos, false))
            return true;
        SetError(
            build_bridge_error_message("Failed to publish SAgent MQTT message", client->getLastError()),
            ClassifyErrorCode(client->getLastError()));
        return false;
    } catch (const std::exception& e) {
        SetError(e.what(), ClassifyErrorCode(e.what()));
        return false;
    }
}

SAgentMqttBridge::StatusSnapshot SAgentMqttBridge::GetStatus() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
}

std::string SAgentMqttBridge::ClassifyErrorCode(const std::string& message)
{
    const std::string normalized = boost::algorithm::to_lower_copy(trim_copy(message));
    if (normalized.empty())
        return "mqtt_error";
    if (normalized.find("not authorized") != std::string::npos ||
        normalized.find("bad user name or password") != std::string::npos ||
        normalized.find("identifier rejected") != std::string::npos ||
        normalized.find("auth") != std::string::npos ||
        normalized.find("password") != std::string::npos ||
        normalized.find("signed_password_expire") != std::string::npos ||
        normalized.find("signed_password_expired") != std::string::npos ||
        normalized.find("forbidden") != std::string::npos) {
        return "mqtt_auth_failed";
    }
    if (normalized.find("timed out") != std::string::npos ||
        normalized.find("timeout") != std::string::npos) {
        return "mqtt_timeout";
    }
    if (normalized.find("server unavailable") != std::string::npos ||
        normalized.find("service unavailable") != std::string::npos ||
        normalized.find("connection refused") != std::string::npos ||
        normalized.find("refused") != std::string::npos) {
        return "mqtt_server_unavailable";
    }
    if (normalized.find("subscribe") != std::string::npos) {
        return "mqtt_subscribe_failed";
    }
    if (normalized.find("publish") != std::string::npos) {
        return "mqtt_publish_failed";
    }
    if (normalized.find("connection lost") != std::string::npos ||
        normalized.find("broken pipe") != std::string::npos ||
        normalized.find("network") != std::string::npos ||
        normalized.find("reset by peer") != std::string::npos) {
        return "mqtt_connection_lost";
    }
    return "mqtt_error";
}

void SAgentMqttBridge::InitialConnect(unsigned long long generation)
{
    std::unique_ptr<MQTTClient> next_client;
    std::string error_message;
    std::string error_code;
    if (ConnectClient(&next_client, &error_message, &error_code)) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stop_reconnect || generation != m_connect_generation) {
                m_connect_running = false;
                return;
            }
            m_client = std::move(next_client);
            m_connect_running = false;
        }
        SetConnected(true);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop_reconnect || generation != m_connect_generation) {
            m_connect_running = false;
            return;
        }
        m_connect_running = false;
    }

    StartReconnectLoop(
        error_message.empty() ? "Failed to connect to SAgent MQTT broker." : error_message,
        error_code,
        std::string());
}

void SAgentMqttBridge::StopConnectThread()
{
    std::thread thread_to_join;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop_reconnect = true;
        ++m_connect_generation;
        if (m_connect_thread.joinable())
            thread_to_join = std::move(m_connect_thread);
        m_connect_running = false;
    }

    m_reconnect_cv.notify_all();
    if (thread_to_join.joinable())
        thread_to_join.join();
}
bool SAgentMqttBridge::ConnectClient(
    std::unique_ptr<MQTTClient>* out_client,
    std::string* error_message,
    std::string* error_code)
{
    if (out_client == nullptr)
        return false;

    Config config;
    std::string broker_url;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        config = m_config;
        broker_url = m_status.broker_url;
    }

    try {
        std::unique_ptr<MQTTClient> next_client(new MQTTClient(broker_url, config.client_id));
        next_client->setConnectionLostCallback([this](const std::string& cause) {
            HandleConnectionLost(cause);
        });

        if (!next_client->connect(config.username, config.password)) {
            const std::string detail = next_client->getLastError();
            const std::string message = build_bridge_error_message(
                "Failed to connect to SAgent MQTT broker",
                detail);
            if (error_message)
                *error_message = message;
            if (error_code)
                *error_code = ClassifyErrorCode(detail.empty() ? message : detail);
            return false;
        }

        if (!SubscribeClient(next_client.get(), error_message, error_code))
            return false;

        *out_client = std::move(next_client);
        return true;
    } catch (const std::exception& e) {
        if (error_message)
            *error_message = build_bridge_error_message("Failed to initialize SAgent MQTT client", e.what());
        if (error_code)
            *error_code = ClassifyErrorCode(e.what());
        return false;
    }
}

bool SAgentMqttBridge::SubscribeClient(
    MQTTClient* client,
    std::string* error_message,
    std::string* error_code)
{
    if (client == nullptr) {
        if (error_message)
            *error_message = "SAgent MQTT client is not initialized.";
        if (error_code)
            *error_code = "mqtt_client_missing";
        return false;
    }

    const StatusSnapshot status = GetStatus();
    const std::string request_topic = status.request_topic;
    const std::string workflow_update_topic = status.workflow_update_topic;
    BOOST_LOG_TRIVIAL(warning)
        << "[SAgentMQTT][Subscribe] request_topic=" << request_topic
        << " workflow_update_topic=" << workflow_update_topic;

    if (!client->subscribe(request_topic, 1, [this](const std::string& topic, const std::string& payload) {
            HandleIncomingMessage(topic, payload);
        })) {
        const std::string detail = client->getLastError();
        const std::string message = build_bridge_error_message(
            "Failed to subscribe SAgent MQTT request topic",
            detail);
        if (error_message)
            *error_message = message;
        if (error_code)
            *error_code = ClassifyErrorCode(detail.empty() ? message : detail);
        return false;
    }

    if (!workflow_update_topic.empty() &&
        !client->subscribe(workflow_update_topic, 1, [this](const std::string& topic, const std::string& payload) {
            HandleIncomingMessage(topic, payload);
        })) {
        const std::string detail = client->getLastError();
        const std::string message = build_bridge_error_message(
            "Failed to subscribe SAgent MQTT workflow update topic",
            detail);
        if (error_message)
            *error_message = message;
        if (error_code)
            *error_code = ClassifyErrorCode(detail.empty() ? message : detail);
        return false;
    }

    return true;
}

void SAgentMqttBridge::HandleConnectionLost(const std::string& cause)
{
    const std::string normalized_cause = normalize_disconnect_cause(cause);
    StartReconnectLoop(
        build_bridge_error_message("SAgent MQTT connection lost", normalized_cause),
        ClassifyErrorCode(cause.empty() ? "connection lost" : cause),
        normalized_cause);
}

void SAgentMqttBridge::StartReconnectLoop(
    const std::string& message,
    const std::string& code,
    const std::string& disconnect_cause)
{
    std::thread thread_to_join;
    bool should_start = false;
    unsigned long long generation = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop_reconnect)
            return;
        const std::string normalized_code = code.empty() ? ClassifyErrorCode(message) : code;
        m_status.connected = false;
        m_status.connecting = true;
        m_status.last_error = format_reported_error(normalized_code, message, disconnect_cause);
        m_status.last_error_code = normalized_code;
        if (!disconnect_cause.empty())
            m_status.last_disconnect_cause = disconnect_cause;
        m_status.reconnect_scheduled = true;
        if (!m_reconnect_running) {
            if (m_reconnect_thread.joinable())
                thread_to_join = std::move(m_reconnect_thread);
            m_reconnect_running = true;
            generation = ++m_reconnect_generation;
            should_start = true;
        }
    }

    EmitStatus();

    if (thread_to_join.joinable())
        thread_to_join.join();

    if (should_start)
        m_reconnect_thread = std::thread(&SAgentMqttBridge::ReconnectLoop, this, generation);
}

void SAgentMqttBridge::ReconnectLoop(unsigned long long generation)
{
    int attempt = 0;

    while (true) {
        const auto wait_delay = std::chrono::milliseconds(std::min(30000, 2000 * (attempt + 1)));
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_stop_reconnect || generation != m_reconnect_generation)
                break;
            m_status.connected = false;
            m_status.connecting = true;
            m_status.reconnect_scheduled = true;
            m_status.reconnect_attempt = attempt + 1;
            if (m_reconnect_cv.wait_for(lock, wait_delay, [this, generation]() {
                    return m_stop_reconnect || generation != m_reconnect_generation;
                })) {
                break;
            }
        }

        std::unique_ptr<MQTTClient> next_client;
        std::string error_message;
        std::string error_code;
        if (ConnectClient(&next_client, &error_message, &error_code)) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_stop_reconnect || generation != m_reconnect_generation)
                    break;
                m_client = std::move(next_client);
                m_reconnect_running = false;
                m_status.reconnect_attempt = 0;
                m_status.reconnect_scheduled = false;
            }
            BOOST_LOG_TRIVIAL(warning)
                << "[SAgentMQTT][Reconnect] recovered generation=" << generation;
            SetConnected(true);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stop_reconnect || generation != m_reconnect_generation)
                break;
            const std::string normalized_message = error_message.empty()
                ? "Failed to reconnect SAgent MQTT bridge."
                : error_message;
            const std::string normalized_code = error_code.empty()
                ? ClassifyErrorCode(normalized_message)
                : error_code;
            m_status.connected = false;
            m_status.connecting = true;
            m_status.last_error = format_reported_error(
                normalized_code,
                normalized_message,
                m_status.last_disconnect_cause);
            m_status.last_error_code = normalized_code;
            m_status.reconnect_scheduled = true;
            m_status.reconnect_attempt = attempt + 1;
        }
        BOOST_LOG_TRIVIAL(warning)
            << "[SAgentMQTT][Reconnect] attempt=" << (attempt + 1)
            << " error=" << (error_message.empty() ? "unknown" : error_message);
        EmitStatus();
        ++attempt;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (generation == m_reconnect_generation) {
            m_reconnect_running = false;
            m_status.reconnect_scheduled = false;
            if (m_stop_reconnect) {
                m_status.connecting = false;
                m_status.reconnect_attempt = 0;
            }
        }
    }
}

void SAgentMqttBridge::StopReconnectLoop()
{
    std::thread thread_to_join;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop_reconnect = true;
        ++m_reconnect_generation;
        if (m_reconnect_thread.joinable())
            thread_to_join = std::move(m_reconnect_thread);
        m_reconnect_running = false;
    }

    m_reconnect_cv.notify_all();
    if (thread_to_join.joinable())
        thread_to_join.join();
}

std::string SAgentMqttBridge::NormalizeTopicPart(const std::string& value, const std::string& fallback)
{
    std::string normalized = trim_copy(value);
    while (!normalized.empty() && normalized.front() == '/')
        normalized.erase(normalized.begin());
    while (!normalized.empty() && normalized.back() == '/')
        normalized.pop_back();
    return normalized.empty() ? fallback : normalized;
}

bool SAgentMqttBridge::IsAllowedToolResponseKind(const std::string& kind)
{
    return kind == "ack" || kind == "progress" || kind == "result";
}

std::string SAgentMqttBridge::BuildToolTopic(const std::string& kind) const
{
    return m_status.topic_prefix + "/" + m_status.tenant_id + "/" + m_status.client_id + "/tool/" + kind;
}

std::string SAgentMqttBridge::BuildWorkflowUpdateTopic() const
{
    return m_status.topic_prefix + "/" + m_status.tenant_id + "/" + m_status.client_id + "/workflow/update";
}

bool SAgentMqttBridge::IsAllowedPublishTopic(const std::string& topic) const
{
    StatusSnapshot status = GetStatus();
    const std::string prefix = status.topic_prefix + "/" + status.tenant_id + "/" + status.client_id + "/tool/";
    if (topic.rfind(prefix, 0) != 0)
        return false;
    const std::string kind = topic.substr(prefix.size());
    return IsAllowedToolResponseKind(kind);
}

void SAgentMqttBridge::HandleIncomingMessage(const std::string& topic, const std::string& payload)
{
    StatusSnapshot status = GetStatus();
    std::string message_kind;
    if (topic == status.request_topic)
        message_kind = "tool_request";
    else if (topic == status.workflow_update_topic)
        message_kind = "workflow_update";
    else {
        BOOST_LOG_TRIVIAL(warning)
            << "[SAgentMqttBridge] ignored unexpected topic=" << topic
            << " request_topic=" << status.request_topic
            << " workflow_update_topic=" << status.workflow_update_topic;
        return;
    }

    Json data = Json::parse(payload, nullptr, false);
    if (data.is_discarded() || !data.is_object()) {
        SetError("Received invalid SAgent MQTT tool request JSON.", "mqtt_invalid_payload");
        return;
    }

    if (message_kind == "workflow_update") {
        BOOST_LOG_TRIVIAL(warning)
            << "[SAgentMQTT][WorkflowUpdate] workflow_id=" << data.value("workflow_id", std::string())
            << " stage=" << data.value("stage", std::string())
            << " current_card_type="
            << (data.contains("current_card") && data["current_card"].is_object()
                    ? data["current_card"].value("card_type", std::string())
                    : std::string())
            << " current_card_task_id="
            << (data.contains("current_card") && data["current_card"].is_object()
                    ? data["current_card"].value("task_id", std::string())
                    : std::string())
            << " pending_tool="
            << (data.contains("pending_tool") && data["pending_tool"].is_object()
                    ? data["pending_tool"].value("tool", std::string())
                    : std::string())
            << " pending_request_id="
            << (data.contains("pending_tool") && data["pending_tool"].is_object()
                    ? data["pending_tool"].value("request_id", std::string())
                    : std::string())
            << " session_id=" << data.value("session_id", std::string());
    }
    data["tenant_id"] = data.value("tenant_id", status.tenant_id);
    data["client_id"] = data.value("client_id", status.client_id);
    data["message_kind"] = message_kind;
    if (!status.session_id.empty())
        data["session_id"] = data.value("session_id", status.session_id);

    MessageHandler handler;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        handler = m_message_handler;
    }
    if (handler)
        handler(data);
}

void SAgentMqttBridge::SetConnected(bool connected)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.connected = connected;
        m_status.connecting = false;
        if (connected) {
            m_status.last_error.clear();
            m_status.last_error_code.clear();
            m_status.last_disconnect_cause.clear();
            m_status.reconnect_attempt = 0;
            m_status.reconnect_scheduled = false;
        }
    }
    BOOST_LOG_TRIVIAL(warning)
        << "[SAgentMQTT][Connected] connected=" << (connected ? "true" : "false");
    EmitStatus();
}

void SAgentMqttBridge::SetError(
    const std::string& message,
    const std::string& code,
    const std::string& disconnect_cause)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.connected = false;
        m_status.connecting = false;
        const std::string normalized_code = code.empty() ? ClassifyErrorCode(message) : code;
        m_status.last_error = format_reported_error(normalized_code, message, disconnect_cause);
        m_status.last_error_code = normalized_code;
        if (!disconnect_cause.empty())
            m_status.last_disconnect_cause = disconnect_cause;
        m_status.reconnect_scheduled = false;
    }
    BOOST_LOG_TRIVIAL(warning) << "[SAgentMqttBridge] " << message;
    EmitStatus();
}

void SAgentMqttBridge::EmitStatus()
{
    StatusHandler handler;
    StatusSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        handler = m_status_handler;
        snapshot = m_status;
    }
    if (handler)
        handler(snapshot);
}

} // namespace Bridge
} // namespace GUI
} // namespace Slic3r
