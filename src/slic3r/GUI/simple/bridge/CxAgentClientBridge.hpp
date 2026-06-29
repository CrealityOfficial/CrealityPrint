#ifndef slic3r_GUI_simple_bridge_CxAgentClientBridge_hpp_
#define slic3r_GUI_simple_bridge_CxAgentClientBridge_hpp_

#include "nlohmann/json.hpp"

#include <functional>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Slic3r {
namespace GUI {
namespace Bridge {

class CxAgentClientBridge
{
public:
    struct StatusSnapshot {
        bool connected = false;
        bool connecting = false;
        std::string http_base_url;
        std::string ws_url;
        std::string client_id;
        std::string session_id;
        std::string last_error;
        int last_client_seq = 0;
        int last_seen_server_seq = 0;
        std::vector<std::string> in_flight_request_ids;
    };

    using Json = nlohmann::json;
    using MessageHandler = std::function<void(const Json&)>;
    using StatusHandler = std::function<void(const StatusSnapshot&)>;

    CxAgentClientBridge();
    ~CxAgentClientBridge();

    void SetMessageHandler(MessageHandler handler);
    void SetStatusHandler(StatusHandler handler);

    bool Start(const std::string& http_base_url,
               const std::string& client_id,
               const std::string& session_id);
    void Stop();

    StatusSnapshot GetStatus() const;

    void SendHeartbeat(const std::string& status, const std::string& active_request_id = std::string());
    void SendContextUpdate(const Json& project_context);
    void SendToolProgress(const std::string& request_id,
                          int progress,
                          const std::string& message,
                          const std::string& stage,
                          const std::string& status = "running");
    void SendToolResult(const std::string& request_id,
                        bool ok,
                        const Json& result_or_error);

    void MarkRequestStarted(const std::string& request_id);
    void MarkRequestFinished(const std::string& request_id);

private:
    struct Session;

    std::string BuildWsUrl(const std::string& http_base_url, const std::string& client_id) const;
    Json BuildCapabilitiesPayload() const;
    void PushClientHello();
    void QueueJson(const Json& payload);
    int NextClientSeq();
    void SetConnected(bool connected);
    void SetError(const std::string& message);
    void HandleTransportError(const std::string& message);
    void HandleMessageError(const std::string& message);
    void ScheduleReconnect(std::uint64_t generation);
    void HandleIncomingMessage(const Json& msg);
    void EmitStatus();

private:
    mutable std::mutex m_mutex;
    std::shared_ptr<Session> m_session;
    MessageHandler m_message_handler;
    StatusHandler m_status_handler;
    StatusSnapshot m_status;
    bool m_reconnect_scheduled = false;
    std::uint64_t m_connection_generation = 0;
};

} // namespace Bridge
} // namespace GUI
} // namespace Slic3r

#endif
