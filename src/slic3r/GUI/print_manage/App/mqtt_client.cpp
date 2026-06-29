#include "mqtt_client.h"

#include <chrono>

namespace {
constexpr auto MQTT_CONNECT_WAIT_TIMEOUT = std::chrono::seconds(8);
constexpr auto MQTT_OPERATION_WAIT_TIMEOUT = std::chrono::seconds(5);
constexpr auto MQTT_DISCONNECT_WAIT_TIMEOUT = std::chrono::seconds(2);

bool WaitForToken(
    const mqtt::token_ptr& token,
    const std::chrono::milliseconds& timeout,
    const std::string& operation,
    const std::string& target,
    std::string& last_error)
{
    if (!token) {
        last_error = operation + " failed: missing MQTT action token";
        return false;
    }

    if (!token->wait_for(timeout)) {
        last_error = operation + " timed out after " + std::to_string(timeout.count()) + "ms";
        if (!target.empty())
            last_error += " (" + target + ")";
        return false;
    }
    return true;
}
}

// ActionCallback implementation
MQTTClient::ActionCallback::ActionCallback(MQTTClient& client) 
    : client_(client) {}

void MQTTClient::ActionCallback::connection_lost(const std::string& cause) {
    client_.last_error_ = cause.empty() ? "MQTT connection lost." : ("MQTT connection lost: " + cause);
    std::cerr << "Connection lost: " << (cause.empty() ? "unknown reason" : cause) << std::endl;
    if (client_.connection_lost_callback_) {
        client_.connection_lost_callback_(cause);
    }
    if (client_.connection_callback_) {
        client_.connection_callback_(false);
    }
}

void MQTTClient::ActionCallback::message_arrived(mqtt::const_message_ptr msg) {
    std::string topic = msg->get_topic();
    // Preserve the raw bytes from the payload; some messages are not UTF-8 encoded.
    const auto& payload_ref = msg->get_payload_ref();
    std::string payload;
    if (!payload_ref.empty())
        payload.assign(payload_ref.data(), payload_ref.size());
    
    // Find a callback for the exact topic.
    auto it = client_.topic_callbacks_.find(topic);
    if (it != client_.topic_callbacks_.end() && it->second) {
        it->second(topic, payload);
    } else {
        // Fall back to a simple wildcard prefix match.
        for (const auto& pair : client_.topic_callbacks_) {
            // This is a lightweight wildcard match; broker-side matching remains authoritative.
            if (pair.first.find('#') != std::string::npos || 
                pair.first.find('+') != std::string::npos) {
                // Full MQTT wildcard matching can be added here if needed.
                // If the topic starts with the wildcard prefix, invoke the callback.
                size_t wildcard_pos = pair.first.find_first_of("#+");
                if (wildcard_pos != std::string::npos) {
                    std::string prefix = pair.first.substr(0, wildcard_pos);
                    if (topic.find(prefix) == 0 && pair.second) {
                        pair.second(topic, payload);
                    }
                }
            }
        }
    }
}

void MQTTClient::ActionCallback::delivery_complete(mqtt::delivery_token_ptr token) {
    // Delivery completion is not used.
}
MQTTClient::ActionCallback::~ActionCallback()
{
}
// MQTTClient implementation
MQTTClient::MQTTClient(const std::string& server_address, const std::string& client_id)
    : server_address_(server_address),
      client_id_(client_id),
      connection_callback_(nullptr),
      connection_lost_callback_(nullptr) {
    
    try {
        client_ = std::make_unique<mqtt::async_client>(server_address_, client_id_);
        callback_ = std::make_unique<ActionCallback>(*this);
        client_->set_callback(*callback_);
    } catch (const mqtt::exception& exc) {
        std::cerr << "Error creating MQTT client: " << exc.what() << std::endl;
        throw;
    }
}

MQTTClient::~MQTTClient() {
    try {
        if (isConnected()) {
            disconnect();
        }
    } catch (...) {
        // Do not throw from destructor.
    }
}

bool MQTTClient::connect(const std::string& username, 
                        const std::string& password, 
                        bool clean_session,
                        int keep_alive) {
    try {
        if (isConnected()) {
            return true;
        }
        
        // Configure connection options.
        conn_opts_ = mqtt::connect_options();
        conn_opts_.set_clean_session(clean_session);
        conn_opts_.set_keep_alive_interval(keep_alive);
        conn_opts_.set_connect_timeout(MQTT_CONNECT_WAIT_TIMEOUT);
        conn_opts_.set_automatic_reconnect(false);
        conn_opts_.set_max_inflight(20);                               // Improve concurrent handling.
        conn_opts_.set_mqtt_version(MQTTVERSION_3_1_1);
        
        
        if (!username.empty()) {
            conn_opts_.set_user_name(username);
        }
        if (!password.empty()) {
            conn_opts_.set_password(password);
        }
        
        // Connect to broker.
        if (!WaitForToken(
                client_->connect(conn_opts_),
                MQTT_CONNECT_WAIT_TIMEOUT,
                "MQTT connect",
                server_address_,
                last_error_)) {
            std::cerr << last_error_ << std::endl;
            return false;
        }
        last_error_.clear();
        
        if (connection_callback_) {
            connection_callback_(true);
        }
        
        return true;
    } catch (const mqtt::exception& exc) {
        last_error_ = exc.what();
        std::cerr << "Error connecting to MQTT server: " << exc.what() << std::endl;
        return false;
    }
}

void MQTTClient::disconnect() {
    try {
        if (isConnected()) {
            if (!WaitForToken(
                    client_->disconnect(),
                    MQTT_DISCONNECT_WAIT_TIMEOUT,
                    "MQTT disconnect",
                    server_address_,
                    last_error_)) {
                std::cerr << last_error_ << std::endl;
                return;
            }
        }
        last_error_.clear();
    } catch (const mqtt::exception& exc) {
        last_error_ = exc.what();
        std::cerr << "Error disconnecting from MQTT server: " << exc.what() << std::endl;
    }
}

bool MQTTClient::publish(const std::string& topic, 
                        const std::string& payload, 
                        int qos, 
                        bool retained) {
    try {
        if (!isConnected()) {
            last_error_ = "Not connected to MQTT server";
            std::cerr << "Not connected to MQTT server" << std::endl;
            return false;
        }
        
        auto msg = mqtt::make_message(topic, payload, qos, retained);
        if (!WaitForToken(
                client_->publish(msg),
                MQTT_OPERATION_WAIT_TIMEOUT,
                "MQTT publish",
                topic,
                last_error_)) {
            std::cerr << last_error_ << std::endl;
            return false;
        }
        last_error_.clear();
        return true;
    } catch (const mqtt::exception& exc) {
        last_error_ = exc.what();
        std::cerr << "Error publishing message: " << exc.what() << std::endl;
        return false;
    }
}

bool MQTTClient::subscribe(const std::string& topic, 
                          int qos, 
                          MessageCallback callback) {
    try {
        if (!isConnected()) {
            last_error_ = "Not connected to MQTT server";
            std::cerr << "Not connected to MQTT server" << std::endl;
            return false;
        }
        
        if (!WaitForToken(
                client_->subscribe(topic, qos),
                MQTT_OPERATION_WAIT_TIMEOUT,
                "MQTT subscribe",
                topic,
                last_error_)) {
            std::cerr << last_error_ << std::endl;
            return false;
        }
        
        // Store callback.
        if (callback) {
            topic_callbacks_[topic] = callback;
        }
        last_error_.clear();
        
        return true;
    } catch (const mqtt::exception& exc) {
        last_error_ = exc.what();
        std::cerr << "Error subscribing to topic: " << exc.what() << std::endl;
        return false;
    }
}

bool MQTTClient::unsubscribe(const std::string& topic) {
    try {
        if (!isConnected()) {
            last_error_ = "Not connected to MQTT server";
            std::cerr << "Not connected to MQTT server" << std::endl;
            return false;
        }
        
        if (!WaitForToken(
                client_->unsubscribe(topic),
                MQTT_OPERATION_WAIT_TIMEOUT,
                "MQTT unsubscribe",
                topic,
                last_error_)) {
            std::cerr << last_error_ << std::endl;
            return false;
        }
        
        // Remove callback.
        topic_callbacks_.erase(topic);
        last_error_.clear();
        
        return true;
    } catch (const mqtt::exception& exc) {
        last_error_ = exc.what();
        std::cerr << "Error unsubscribing from topic: " << exc.what() << std::endl;
        return false;
    }
}

void MQTTClient::setConnectionCallback(ConnectionCallback callback) {
    connection_callback_ = callback;
}

void MQTTClient::setConnectionLostCallback(ConnectionLostCallback callback) {
    connection_lost_callback_ = callback;
}

bool MQTTClient::isConnected() const {
    return client_ && client_->is_connected();
}

std::string MQTTClient::getLastError() const {
    return last_error_;
}
