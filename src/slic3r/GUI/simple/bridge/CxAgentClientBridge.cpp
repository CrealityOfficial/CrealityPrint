#include "CxAgentClientBridge.hpp"
#include "SlicerBridge.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/log/trivial.hpp>

#include <algorithm>
#include <atomic>
#include <thread>

namespace Slic3r {
namespace GUI {
namespace Bridge {

namespace {

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

struct ParsedWsUrl {
    std::string host;
    std::string port;
    std::string target = "/";
};

ParsedWsUrl parse_ws_url(const std::string& raw_url)
{
    ParsedWsUrl parsed;
    std::string tmp = raw_url;
    if (tmp.rfind("ws://", 0) == 0)
        tmp = tmp.substr(5);
    else if (tmp.rfind("wss://", 0) == 0)
        tmp = tmp.substr(6);

    std::string hostport = tmp;
    const std::size_t slash = tmp.find('/');
    if (slash != std::string::npos) {
        hostport = tmp.substr(0, slash);
        parsed.target = tmp.substr(slash);
        if (parsed.target.empty())
            parsed.target = "/";
    }

    const std::size_t colon = hostport.find(':');
    if (colon == std::string::npos) {
        parsed.host = hostport;
        parsed.port = "80";
    } else {
        parsed.host = hostport.substr(0, colon);
        parsed.port = hostport.substr(colon + 1);
        if (parsed.port.empty())
            parsed.port = "80";
    }
    return parsed;
}

} // namespace

struct CxAgentClientBridge::Session : public std::enable_shared_from_this<CxAgentClientBridge::Session>
{
    explicit Session(CxAgentClientBridge* owner_ptr)
        : owner(owner_ptr)
        , ioc(std::make_shared<boost::asio::io_context>())
    {
    }

    ~Session()
    {
        stop();
    }

    void start(const std::string& url_value)
    {
        url = url_value;
        worker = std::thread([self = shared_from_this()]() { self->run(); });
    }

    void stop()
    {
        if (closed.exchange(true))
            return;
        boost::system::error_code ec;
        if (ws)
            ws->close(websocket::close_code::normal, ec);
        if (ioc)
            ioc->stop();
        if (worker.joinable())
            worker.join();
    }

    void send_json(const nlohmann::json& payload)
    {
        auto self = shared_from_this();
        boost::asio::post(*ioc, [self, text = payload.dump()]() {
            const bool write_in_progress = !self->outbox.empty();
            self->outbox.push_back(text);
            if (!write_in_progress)
                self->do_write();
        });
    }

    void run()
    {
        try {
            ParsedWsUrl parsed = parse_ws_url(url);
            tcp::resolver resolver(*ioc);
            ws.reset(new websocket::stream<tcp::socket>(*ioc));
            auto results = resolver.resolve(parsed.host, parsed.port);
            boost::asio::connect(ws->next_layer(), results.begin(), results.end());
            ws->handshake(parsed.host, parsed.target);
            owner->SetConnected(true);
            owner->PushClientHello();
            owner->SendHeartbeat("idle");
            do_read();
            ioc->run();
        } catch (const std::exception& e) {
            owner->SetError(e.what());
            owner->SetConnected(false);
        }
    }

    void do_read()
    {
        auto self = shared_from_this();
        ws->async_read(buffer, [self](const boost::system::error_code& ec, std::size_t) {
            if (ec) {
                self->owner->SetError(ec.message());
                self->owner->SetConnected(false);
                return;
            }

            std::string data = boost::beast::buffers_to_string(self->buffer.data());
            self->buffer.consume(self->buffer.size());
            try {
                const auto parsed = nlohmann::json::parse(data);
                const std::string type = parsed.value("type", "");
                if (type == "planner_update") {
                    BOOST_LOG_TRIVIAL(warning)
                        << "[CxAgentClientBridge] received planner_update task_id="
                        << parsed.value("task_id", std::string())
                        << " tool=" << parsed.value("planner_tool_name", std::string())
                        << " reason=" << parsed.value("planner_reason_code", std::string());
                } else if (type == "tool_call") {
                    BOOST_LOG_TRIVIAL(warning)
                        << "[CxAgentClientBridge] received tool_call request_id="
                        << parsed.value("request_id", std::string())
                        << " tool=" << parsed.value("tool", std::string())
                        << " replay=" << (parsed.value("replay", false) ? "true" : "false");
                }
                self->owner->HandleIncomingMessage(parsed);
            } catch (const std::exception& e) {
                self->owner->HandleMessageError(e.what());
            }
            self->do_read();
        });
    }

    void do_write()
    {
        if (!ws || outbox.empty())
            return;

        auto self = shared_from_this();
        ws->async_write(
            boost::asio::buffer(outbox.front()),
            [self](const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    self->owner->SetError(ec.message());
                    self->owner->SetConnected(false);
                    return;
                }
                self->outbox.erase(self->outbox.begin());
                if (!self->outbox.empty())
                    self->do_write();
            }
        );
    }

    CxAgentClientBridge* owner = nullptr;
    std::shared_ptr<boost::asio::io_context> ioc;
    std::unique_ptr<websocket::stream<tcp::socket>> ws;
    boost::beast::flat_buffer buffer;
    std::vector<std::string> outbox;
    std::thread worker;
    std::atomic<bool> closed {false};
    std::string url;
};

CxAgentClientBridge::CxAgentClientBridge() = default;
CxAgentClientBridge::~CxAgentClientBridge()
{
    Stop();
}

void CxAgentClientBridge::SetMessageHandler(MessageHandler handler)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_message_handler = std::move(handler);
}

void CxAgentClientBridge::SetStatusHandler(StatusHandler handler)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_status_handler = std::move(handler);
}

bool CxAgentClientBridge::Start(const std::string& http_base_url,
                                const std::string& client_id,
                                const std::string& session_id)
{
    Stop();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.http_base_url = http_base_url;
        m_status.client_id = client_id;
        m_status.session_id = session_id;
        m_status.ws_url = BuildWsUrl(http_base_url, client_id);
        m_status.connecting = true;
        m_status.connected = false;
        m_status.last_error.clear();
        m_status.last_client_seq = 0;
        m_status.last_seen_server_seq = 0;
        m_status.in_flight_request_ids.clear();
        m_session = std::make_shared<Session>(this);
    }

    EmitStatus();
    m_session->start(GetStatus().ws_url);
    return true;
}

void CxAgentClientBridge::Stop()
{
    std::shared_ptr<Session> session;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        session = m_session;
        m_session.reset();
        m_status.connected = false;
        m_status.connecting = false;
    }
    if (session)
        session->stop();
    EmitStatus();
}

CxAgentClientBridge::StatusSnapshot CxAgentClientBridge::GetStatus() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
}

void CxAgentClientBridge::SendHeartbeat(const std::string& status, const std::string& active_request_id)
{
    Json payload = {
        {"type", "heartbeat"},
        {"client_seq", NextClientSeq()},
        {"session_id", GetStatus().session_id},
        {"status", status},
    };
    if (!active_request_id.empty())
        payload["active_request_id"] = active_request_id;
    QueueJson(payload);
}

void CxAgentClientBridge::SendContextUpdate(const Json& project_context)
{
    const auto status = GetStatus();
    BOOST_LOG_TRIVIAL(warning)
        << "[CxAgentClientBridge] context_update session_id=" << status.session_id
        << " client_id=" << status.client_id
        << " connected=" << (status.connected ? "true" : "false");

    QueueJson({
        {"type", "context_update"},
        {"client_seq", NextClientSeq()},
        {"session_id", status.session_id},
        {"project_context", project_context},
    });
}

void CxAgentClientBridge::SendToolProgress(const std::string& request_id,
                                           int progress,
                                           const std::string& message,
                                           const std::string& stage,
                                           const std::string& status)
{
    QueueJson({
        {"type", "tool_progress"},
        {"client_seq", NextClientSeq()},
        {"request_id", request_id},
        {"status", status},
        {"progress", progress},
        {"message", message},
        {"stage", stage},
    });
}

void CxAgentClientBridge::SendToolResult(const std::string& request_id,
                                         bool ok,
                                         const Json& result_or_error)
{
    const auto status = GetStatus();
    const int client_seq = NextClientSeq();
    Json payload = {
        {"type", "tool_result"},
        {"client_seq", client_seq},
        {"request_id", request_id},
        {"ok", ok},
    };
    if (ok)
        payload["result"] = result_or_error;
    else
        payload["error"] = result_or_error;
    BOOST_LOG_TRIVIAL(warning)
        << "[CxAgentClientBridge] send tool_result"
        << " session_id=" << status.session_id
        << " client_id=" << status.client_id
        << " request_id=" << request_id
        << " client_seq=" << client_seq
        << " ok=" << (ok ? "true" : "false")
        << " await_context_update="
        << ((ok && result_or_error.is_object() && result_or_error.value("await_context_update", false)) ? "true" : "false");
    QueueJson(payload);
}

void CxAgentClientBridge::MarkRequestStarted(const std::string& request_id)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find(m_status.in_flight_request_ids.begin(), m_status.in_flight_request_ids.end(), request_id);
        if (it == m_status.in_flight_request_ids.end())
            m_status.in_flight_request_ids.push_back(request_id);
    }
    EmitStatus();
}

void CxAgentClientBridge::MarkRequestFinished(const std::string& request_id)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& ids = m_status.in_flight_request_ids;
        ids.erase(std::remove(ids.begin(), ids.end(), request_id), ids.end());
    }
    EmitStatus();
}

std::string CxAgentClientBridge::BuildWsUrl(const std::string& http_base_url, const std::string& client_id) const
{
    std::string base = http_base_url;
    std::string scheme = "ws://";
    if (base.rfind("https://", 0) == 0) {
        scheme = "wss://";
        base = base.substr(8);
    } else if (base.rfind("http://", 0) == 0) {
        base = base.substr(7);
    }

    const std::size_t slash = base.find('/');
    std::string host = slash == std::string::npos ? base : base.substr(0, slash);
    std::string path = slash == std::string::npos ? "" : base.substr(slash);
    if (path.empty())
        path = "/api";
    else if (path.size() < 4 || path.substr(path.size() - 4) != "/api")
        path += "/api";
    return scheme + host + path + "/ws/clients/" + client_id;
}

CxAgentClientBridge::Json CxAgentClientBridge::BuildCapabilitiesPayload() const
{
    static const char* kBlockingErrorSchemaVersion = "1.0.0";
    static const std::vector<std::string> kSupportedBlockingErrorCodes = {
        "NO_MODEL_LOADED",
        "NO_PRINTABLE_INSTANCES",
        "MODEL_OUT_OF_BOUNDS",
        "MODEL_EXCEEDS_HEIGHT_LIMIT",
        "FILAMENT_MAPPING_REQUIRED",
        "CRITICAL_SLICE_WARNING",
        "SLICE_RESULT_INVALIDATED",
        "PLATE_SELECTION_INVALID",
        "UNKNOWN_BLOCKING_ERROR",
    };
    static const std::vector<std::pair<std::string, std::string>> tool_action_map = {
        {"get_project_context", ActionID::GET_SLICER_STATE},
        {"get_current_slice_params", ActionID::GET_EDITED_CONFIG},
        {"list_presets", ActionID::GET_PRESETS},
        {"list_printers", ActionID::LIST_PRINTERS},
        {"get_config_schema", ActionID::GET_CONFIG_OPTIONS},
        {"capture_model_views", ActionID::CAPTURE_MODEL_VIEWS},
        {"select_objects", ActionID::SELECT_OBJECTS},
        {"move_object", ActionID::MOVE_OBJECT},
        {"rotate_object", ActionID::ROTATE_OBJECT},
        {"scale_object", ActionID::SCALE_OBJECT},
        {"delete_model", ActionID::DELETE_MODEL},
        {"clone_model", ActionID::CLONE_MODEL},
        {"import_model", ActionID::IMPORT_MODEL},
        {"recommend_model", ActionID::RECOMMEND_MODEL},
        {"import_model_from_search", "import_model_from_search"},
        {"open_model_library", "open_model_library"},
        {"smart_model_search", "smart_model_search"},
        {"fill_bed", ActionID::FILL_BED},
        {"apply_preset", ActionID::SELECT_PRESET},
        {"apply_param_patch", ActionID::APPLY_CONFIG},
        {"reset_config", "reset_config"},
        {"get_slice_result", "get_slice_result"},
        {"analyze_model_geometry", ActionID::GET_SLICER_STATE},
        {"auto_orient_model", ActionID::AUTO_ORIENT},
        {"repair_mesh", ActionID::REPAIR_MESH},
        {"fix_model", ActionID::REPAIR_MESH},
        {"repair_model", ActionID::REPAIR_MESH},
        {"simplify_model", ActionID::SIMPLIFY_MODEL},
        {"add_test_model", ActionID::ADD_TEST_MODEL},
        {"auto_arrange", ActionID::AUTO_ARRANGE},
        {"arrange_current_plate", ActionID::ARRANGE_SINGLE_PLATE},
        {"arrange_all_plates", ActionID::ARRANGE_ALL_PLATES},
        {"undo", ActionID::UNDO},
        {"redo", ActionID::REDO},
        {"open_filament_mapping", "open_filament_mapping"},
        {"auto_map_filaments", ActionID::AUTO_MAP_FILAMENTS},
        {"run_slice", ActionID::START_SLICE},
        {"send_to_printer", ActionID::SEND_TO_PRINTER},
        {"select_printer", ActionID::SELECT_PRINTER},
        {"switch_printer", ActionID::SELECT_PRINTER},
        {"change_printer", ActionID::SELECT_PRINTER},
        {"select_device", ActionID::SELECT_PRINTER},
        {"edit_plate_name", ActionID::EDIT_PLATE_NAME},
        {"add_plate", ActionID::ADD_PLATE},
        {"delete_plate", ActionID::DELETE_PLATE},
        {"toggle_preview_lite_mode", ActionID::TOGGLE_PREVIEW_LITE_MODE},
    };

    auto contract_metadata = [](const std::string& name) -> Json {
        if (name == "import_model") {
            return {
                {"requires", Json::array()},
                {"provides", Json::array({"project.has_model", "plate.current.has_objects"})},
                {"invalidates", Json::array()},
                {"waits_for_context_update", false},
            };
        }
        if (name == "move_object" || name == "rotate_object" || name == "scale_object") {
            return {
                {"requires", Json::array({"project.has_model", "plate.current.has_objects"})},
                {"provides", Json::array({
                    "project.has_model",
                    "plate.current.has_objects",
                    "plate.current.has_printable_instances",
                    "scene.layout.valid"
                })},
                {"invalidates", Json::array({
                    "plate.current.slice_completed",
                    "plate.current.slice_ready_for_print",
                    "plate.current.gcode_available"
                })},
                {"waits_for_context_update", false},
                {"remediations", Json::object()},
            };
        }
        if (name == "repair_mesh") {
            return {
                {"requires", Json::array({"project.has_model", "plate.current.has_objects"})},
                {"provides", Json::array({
                    "project.has_model",
                    "plate.current.has_objects",
                    "scene.mesh.repaired"
                })},
                {"invalidates", Json::array({
                    "plate.current.slice_completed",
                    "plate.current.slice_ready_for_print",
                    "plate.current.gcode_available"
                })},
                {"waits_for_context_update", false},
                {"remediations", Json::object()},
            };
        }
        if (name == "open_model_library") {
            return {
                {"requires", Json::array()},
                {"provides", Json::array({"project.has_model", "plate.current.has_objects"})},
                {"invalidates", Json::array()},
                {"waits_for_context_update", false},
                {"remediations", Json::object()},
            };
        }
        if (name == "recommend_model") {
            return {
                {"requires", Json::array({"network.connected", "model_library.available"})},
                {"provides", Json::array()},
                {"invalidates", Json::array()},
                {"waits_for_context_update", false},
                {"remediations", Json::object({
                    {"network.connected", Json::array({
                        {{"message", "Please check the network connection and try again"}}
                    })}
                })},
            };
        }
        if (name == "smart_model_search") {
            return {
                {"requires", Json::array({"network.connected", "model_library.available"})},
                {"provides", Json::array({"project.has_model", "plate.current.has_objects"})},
                {"invalidates", Json::array()},
                {"waits_for_context_update", true},
                {"remediations", Json::object({
                    {"network.connected", Json::array({
                        {{"message", "Please check the network connection and try again"}}
                    })}
                })},
            };
        }
        if (name == "auto_arrange") {
            return {
                {"requires", Json::array({"project.has_model"})},
                {"provides", Json::array({"scene.layout.valid"})},
                {"invalidates", Json::array({"plate.current.slice_ready_for_print", "plate.current.gcode_available"})},
                {"waits_for_context_update", false},
            };
        }
        if (name == "fix_model" || name == "repair_model") {
            return {
                {"requires", Json::array({"project.has_model"})},
                {"provides", Json::array()},
                {"invalidates", Json::array({"plate.current.slice_ready_for_print", "plate.current.gcode_available"})},
                {"waits_for_context_update", true},
                {"remediations", Json::object()},
            };
        }
        if (name == "simplify_model") {
            return {
                {"requires", Json::array({"project.has_model"})},
                {"provides", Json::array()},
                {"invalidates", Json::array({"plate.current.slice_ready_for_print", "plate.current.gcode_available"})},
                {"waits_for_context_update", true},
                {"remediations", Json::object()},
            };
        }
        if (name == "add_test_model") {
            return {
                {"requires", Json::array()},
                {"provides", Json::array({"project.has_model", "plate.current.has_objects"})},
                {"invalidates", Json::array()},
                {"waits_for_context_update", false},
                {"remediations", Json::object()},
            };
        }
        if (name == "open_filament_mapping") {
            return {
                {"requires", Json::array({"project.has_model"})},
                {"provides", Json::array({"scene.filament_mapping.valid"})},
                {"invalidates", Json::array()},
                {"waits_for_context_update", false},
            };
        }
        if (name == "auto_map_filaments") {
            return {
                {"requires", Json::array({"project.has_model", "device.current.valid"})},
                {"provides", Json::array({"scene.filament_mapping.valid"})},
                {"invalidates", Json::array({"plate.current.slice_ready_for_print", "plate.current.gcode_available"})},
                {"waits_for_context_update", true},
                {"remediations", Json::object({
                    {"scene.filament_mapping.valid", Json::array({
                        Json::object({
                            {"tools", Json::array({"auto_map_filaments", "open_filament_mapping"})},
                            {"policy", "fallback"},
                            {"reason_code", "FILAMENT_MAPPING_INCOMPLETE"},
                            {"user_action_required", true}
                        })
                    })}
                })},
            };
        }
        if (name == "run_slice") {
            return {
                {"requires", Json::array({
                    "project.has_model",
                    "plate.current.has_printable_instances",
                    "scene.no_blocking_errors",
                    "scene.filament_mapping.valid",
                    "scene.layout.valid",
                    "plate.current.no_critical_warnings"
                })},
                {"provides", Json::array({
                    "plate.current.slice_completed",
                    "plate.current.slice_ready_for_print",
                    "plate.current.gcode_available"
                })},
                {"invalidates", Json::array()},
                {"waits_for_context_update", false},
                {"remediations", Json::object({
                    {"project.has_model", Json::array({
                        Json::object({
                            {"tools", Json::array({"open_model_library", "import_model"})},
                            {"policy", "first"},
                            {"reason_code", "NO_MODEL_LOADED"},
                            {"user_action_required", true}
                        })
                    })},
                    {"scene.layout.valid", Json::array({
                        Json::object({
                            {"tools", Json::array({"auto_arrange"})},
                            {"policy", "first"},
                            {"reason_code", "OUT_OF_BOUNDS"}
                        })
                    })},
                    {"scene.filament_mapping.valid", Json::array({
                        Json::object({
                            {"tools", Json::array({"auto_map_filaments", "open_filament_mapping"})},
                            {"policy", "first"},
                            {"reason_code", "FILAMENT_MAPPING_REQUIRED"},
                            {"user_action_required", true}
                        })
                    })}
                })},
            };
        }
        if (name == "get_slice_result") {
            return {
                {"requires", Json::array({"plate.current.slice_completed"})},
                {"provides", Json::array({"plate.current.gcode_available", "plate.current.slice_ready_for_print"})},
                {"invalidates", Json::array()},
                {"waits_for_context_update", false},
            };
        }
        if (name == "send_to_printer") {
            return {
                {"requires", Json::array({
                    "project.has_model",
                    "plate.current.has_printable_instances",
                    "plate.current.slice_ready_for_print",
                    "plate.current.gcode_available",
                    "scene.no_blocking_errors",
                    "device.current.valid",
                    "device.current.online",
                    "device.current.idle"
                })},
                {"provides", Json::array({"print.job_submitted"})},
                {"invalidates", Json::array()},
                {"waits_for_context_update", false},
                {"remediations", Json::object({
                    {"project.has_model", Json::array({
                        Json::object({
                            {"tools", Json::array({"open_model_library", "import_model"})},
                            {"policy", "first"},
                            {"reason_code", "NO_MODEL_LOADED"},
                            {"user_action_required", true}
                        })
                    })},
                    {"scene.no_blocking_errors", Json::array({
                        Json::object({
                            {"tools", Json::array({"auto_arrange", "auto_map_filaments", "open_filament_mapping"})},
                            {"policy", "first"},
                            {"reason_code", "SCENE_BLOCKED"}
                        })
                    })},
                    {"plate.current.slice_ready_for_print", Json::array({
                        Json::object({
                            {"tools", Json::array({"run_slice"})},
                            {"policy", "first"},
                            {"reason_code", "SLICE_REQUIRED"}
                        })
                    })},
                    {"plate.current.gcode_available", Json::array({
                        Json::object({
                            {"tools", Json::array({"get_slice_result", "run_slice"})},
                            {"policy", "first"},
                            {"reason_code", "GCODE_REQUIRED"}
                        })
                    })}
                })},
            };
        }
        if (name == "list_printers") {
            return {
                {"requires", Json::array({"device.available"})},
                {"provides", Json::array({"device.list.available"})},
                {"invalidates", Json::array()},
                {"waits_for_context_update", false},
                {"remediations", Json::object()},
            };
        }
        return {
            {"requires", Json::array()},
            {"provides", Json::array()},
            {"invalidates", Json::array()},
            {"waits_for_context_update", false},
            {"remediations", Json::object()},
        };
    };

    auto custom_schema = [&contract_metadata](const std::string& name) -> Json {
        if (name == "reset_config") {
            return {
                {"name", name},
                {"description", "Reset print configuration parameters to defaults. NOTE: This feature is not yet supported. When the user asks to reset parameters, call this tool and inform the user that the feature is not yet available."},
                {"requires_confirmation", false},
                {"params", Json::array()},
                {"requires", contract_metadata(name)["requires"]},
                {"provides", contract_metadata(name)["provides"]},
                {"invalidates", contract_metadata(name)["invalidates"]},
                {"remediations", contract_metadata(name)["remediations"]},
                {"waits_for_context_update", contract_metadata(name)["waits_for_context_update"]},
            };
        }
        if (name == "get_slice_result") {
            return {
                {"name", name},
                {"description", "Return the current valid slice result for the active plate if available."},
                {"requires_confirmation", false},
                {"params", Json::array()},
                {"requires", contract_metadata(name)["requires"]},
                {"provides", contract_metadata(name)["provides"]},
                {"invalidates", contract_metadata(name)["invalidates"]},
                {"remediations", contract_metadata(name)["remediations"]},
                {"waits_for_context_update", contract_metadata(name)["waits_for_context_update"]},
            };
        }
        if (name == "open_filament_mapping") {
            return {
                {"name", name},
                {"description", "Open the local filament mapping dialog and wait for a later context update after the user finishes mapping."},
                {"requires_confirmation", false},
                {"params", Json::array()},
                {"requires", contract_metadata(name)["requires"]},
                {"provides", contract_metadata(name)["provides"]},
                {"invalidates", contract_metadata(name)["invalidates"]},
                {"remediations", contract_metadata(name)["remediations"]},
                {"waits_for_context_update", contract_metadata(name)["waits_for_context_update"]},
            };
        }
        if (name == "open_model_library") {
            return {
                {"name", name},
                {"description", "Open the built-in online model library so the user can search and select a printable model."},
                {"requires_confirmation", false},
                {"params", Json::array({
                    {
                        {"name", "query"},
                        {"type", "string"},
                        {"required", false},
                        {"description", "Optional model search keyword. When omitted, open the default model library page."},
                    },
                })},
                {"requires", contract_metadata(name)["requires"]},
                {"provides", contract_metadata(name)["provides"]},
                {"invalidates", contract_metadata(name)["invalidates"]},
                {"remediations", contract_metadata(name)["remediations"]},
                {"waits_for_context_update", contract_metadata(name)["waits_for_context_update"]},
            };
        }
        if (name == "recommend_model") {
            return {
                {"name", name},
                {"description", "Recommend one or more random trending online models that have importable 3mf files."},
                {"requires_confirmation", false},
                {"params", Json::array({
                    {
                        {"name", "count"},
                        {"type", "integer"},
                        {"required", false},
                        {"description", "Number of recommended models to return. Default is 1."},
                    },
                    {
                        {"name", "random_seed"},
                        {"type", "integer"},
                        {"required", false},
                        {"description", "Optional deterministic seed for repeatable recommendations."},
                    },
                    {
                        {"name", "candidate_pool_size"},
                        {"type", "integer"},
                        {"required", false},
                        {"description", "How many trending candidates to sample from before random selection. Default is 30."},
                    },
                })},
                {"requires", contract_metadata(name)["requires"]},
                {"provides", contract_metadata(name)["provides"]},
                {"invalidates", contract_metadata(name)["invalidates"]},
                {"remediations", contract_metadata(name)["remediations"]},
                {"waits_for_context_update", contract_metadata(name)["waits_for_context_update"]},
            };
        }
        if (name == "smart_model_search") {
            return {
                {"name", name},
                {"description", "Search the online model library and return the best matched model for user confirmation before import."},
                {"requires_confirmation", false},
                {"params", Json::array({
                    {
                        {"name", "keyword"},
                        {"type", "string"},
                        {"required", true},
                        {"description", "Search keyword for the model, e.g., 'labubu', 'dragon egg'."},
                    },
                    {
                        {"name", "sort_by"},
                        {"type", "string"},
                        {"required", false},
                        {"description", "Sort rule: 'likes' (most liked), 'downloads' (most downloaded), 'date' (newest), 'score' (highest rated). Default is 'downloads'."},
                    },
                    {
                        {"name", "limit"},
                        {"type", "integer"},
                        {"required", false},
                        {"description", "Number of results to return. Default is 1 (top result only)."},
                    },
                })},
                {"requires", contract_metadata(name)["requires"]},
                {"provides", contract_metadata(name)["provides"]},
                {"invalidates", contract_metadata(name)["invalidates"]},
                {"remediations", contract_metadata(name)["remediations"]},
                {"waits_for_context_update", contract_metadata(name)["waits_for_context_update"]},
            };
        }
        if (name == "fix_model" || name == "repair_model") {
            return {
                {"name", name},
                {"description", "Repair the currently selected model(s) with non-manifold edges or other mesh errors using Netfabb. Use this when the user asks to fix or repair a broken model."},
                {"requires_confirmation", true},
                {"params", Json::array()},
                {"requires", contract_metadata(name)["requires"]},
                {"provides", contract_metadata(name)["provides"]},
                {"invalidates", contract_metadata(name)["invalidates"]},
                {"remediations", contract_metadata(name)["remediations"]},
                {"waits_for_context_update", contract_metadata(name)["waits_for_context_update"]},
            };
        }
        if (name == "simplify_model") {
            return {
                {"name", name},
                {"description", "Simplify (reduce triangle count of) the selected or specified model. This is useful for models with very high polygon counts (>1M triangles) to improve performance. Use a ratio parameter (0.05-0.95, default 0.6) to control how many triangles to keep."},
                {"requires_confirmation", true},
                {"params", Json::array({
                    {
                        {"name", "ratio"},
                        {"type", "number"},
                        {"required", false},
                        {"description", "Fraction of original triangles to retain (0.05-0.95). Default 0.6 keeps 60% of triangles."},
                        {"default_value", 0.6},
                    },
                    {
                        {"name", "object_index"},
                        {"type", "integer"},
                        {"required", false},
                        {"description", "Index of the model object to simplify. Defaults to the currently selected object."},
                    },
                    {
                        {"name", "object_name"},
                        {"type", "string"},
                        {"required", false},
                        {"description", "Name of the model object to simplify."},
                    },
                })},
                {"requires", contract_metadata(name)["requires"]},
                {"provides", contract_metadata(name)["provides"]},
                {"invalidates", contract_metadata(name)["invalidates"]},
                {"remediations", contract_metadata(name)["remediations"]},
                {"waits_for_context_update", contract_metadata(name)["waits_for_context_update"]},
            };
        }
        if (name == "add_test_model") {
            return {
                {"name", name},
                {"description", "Add a test model to the build plate. Supports procedural shapes (Cube, Sphere, Cylinder, Cone, Truncated Cone, Torus, Pyramid, Prism, Disc) and file-based test models (Block20XY, 3DBenchy, Complex, Overhang, Square columns Z axis, Square prism Z axis)."},
                {"requires_confirmation", false},
                {"params", Json::array({
                    {
                        {"name", "type_name"},
                        {"type", "string"},
                        {"required", true},
                        {"description", "Test model type name. e.g. Cube, Sphere, Cylinder, 3DBenchy, etc."},
                    },
                    {
                        {"name", "name"},
                        {"type", "string"},
                        {"required", false},
                        {"description", "Alias of type_name."},
                    },
                })},
                {"requires", contract_metadata(name)["requires"]},
                {"provides", contract_metadata(name)["provides"]},
                {"invalidates", contract_metadata(name)["invalidates"]},
                {"remediations", contract_metadata(name)["remediations"]},
                {"waits_for_context_update", contract_metadata(name)["waits_for_context_update"]},
            };
        }
        if (name == "send_to_printer") {
            return {
                {"name", name},
                {"description", "Send the current plate to the currently selected printer after physical safety is confirmed. The printer must be bound, online, and idle."},
                {"requires_confirmation", true},
                {"params", Json::array({
                    {
                        {"name", "safety_confirmed"},
                        {"type", "bool"},
                        {"required", false},
                        {"description", "Set true when the user already confirmed the printer platform is physically safe for printing."},
                        {"default_value", false},
                    },
                    {
                        {"name", "skip_local_confirmation"},
                        {"type", "bool"},
                        {"required", false},
                        {"description", "Set true to skip the local native confirmation because confirmation already happened in chat."},
                        {"default_value", false},
                    },
                })},
                {"requires", contract_metadata(name)["requires"]},
                {"provides", contract_metadata(name)["provides"]},
                {"invalidates", contract_metadata(name)["invalidates"]},
                {"remediations", contract_metadata(name)["remediations"]},
                {"waits_for_context_update", contract_metadata(name)["waits_for_context_update"]},
            };
        }
        if (name == "select_printer") {
            return {
                {"requires", Json::array({"device.available"})},
                {"provides", Json::array({
                    "device.current.valid",
                    "printer.preset.selected"
                })},
                {"invalidates", Json::array({
                    "plate.current.slice_ready_for_print",
                    "plate.current.gcode_available",
                    "scene.filament_mapping.valid"
                })},
                {"waits_for_context_update", true},
                {"remediations", Json::object()},
            };
        }

        Json schema = {
            {"name", name},
            {"description", "Client tool capability."},
            {"requires_confirmation", false},
            {"params", Json::array()},
        };
        Json metadata = contract_metadata(name);
        schema["requires"] = metadata["requires"];
        schema["provides"] = metadata["provides"];
        schema["invalidates"] = metadata["invalidates"];
        schema["remediations"] = metadata["remediations"];
        schema["waits_for_context_update"] = metadata["waits_for_context_update"];
        return schema;
    };

    Json tools = Json::array();
    Json schemas = Json::array();
    Json supported_blocking_error_codes = Json::array();
    auto& bridge = SlicerBridge::Instance();

    for (const auto& code : kSupportedBlockingErrorCodes)
        supported_blocking_error_codes.push_back(code);

    for (const auto& entry : tool_action_map) {
        tools.push_back(entry.first);

        const ActionDef* def = bridge.FindAction(entry.second);
        if (!def) {
            schemas.push_back(custom_schema(entry.first));
            continue;
        }

        Json params = Json::array();
        for (const auto& param : def->params) {
            params.push_back({
                {"name", param.name},
                {"type", param.type},
                {"description", param.description},
                {"required", param.required},
                {"default_value", param.default_value},
            });
        }

        Json schema = {
            {"name", entry.first},
            {"action_id", def->id},
            {"name_en", def->name_en},
            {"name_zh", def->name_zh},
            {"description", def->description},
            {"requires_confirmation", def->requires_confirm},
            {"params", params},
        };
        Json metadata = contract_metadata(entry.first);
        schema["requires"] = metadata["requires"];
        schema["provides"] = metadata["provides"];
        schema["invalidates"] = metadata["invalidates"];
        schema["remediations"] = metadata["remediations"];
        schema["waits_for_context_update"] = metadata["waits_for_context_update"];
        schemas.push_back(std::move(schema));
    }

    return {
        {"tools", tools},
        {"tool_schemas", schemas},
        {"blocking_error_schema_version", kBlockingErrorSchemaVersion},
        {"supported_blocking_error_codes", supported_blocking_error_codes},
        {"progress_events", true},
        {"context_update", true},
    };
}

void CxAgentClientBridge::PushClientHello()
{
    Json payload = {
        {"type", "client_hello"},
        {"client_seq", NextClientSeq()},
        {"session_id", GetStatus().session_id},
        {"schema_version", "1.0.0"},
        {"client", {
            {"name", "C3DSlicer"},
            {"version", "0.1.0"},
        }},
        {"capabilities", BuildCapabilitiesPayload()},
        {"in_flight_request_ids", GetStatus().in_flight_request_ids},
        {"last_seen_server_seq", GetStatus().last_seen_server_seq},
    };
    QueueJson(payload);
}

void CxAgentClientBridge::QueueJson(const Json& payload)
{
    std::shared_ptr<Session> session;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        session = m_session;
    }
    if (session)
        session->send_json(payload);
}

int CxAgentClientBridge::NextClientSeq()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return ++m_status.last_client_seq;
}

void CxAgentClientBridge::SetConnected(bool connected)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.connected = connected;
        m_status.connecting = false;
        if (connected)
            m_status.last_error.clear();
    }
    EmitStatus();
}

void CxAgentClientBridge::SetError(const std::string& message)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.last_error = message;
        m_status.connected = false;
        m_status.connecting = false;
    }
    EmitStatus();
}

void CxAgentClientBridge::HandleTransportError(const std::string& message)
{
    std::uint64_t generation = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.last_error = message;
        m_status.connected = false;
        m_status.connecting = false;
        generation = m_connection_generation;
    }
    EmitStatus();
    ScheduleReconnect(generation);
}

void CxAgentClientBridge::HandleMessageError(const std::string& message)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.last_error = message;
    }
    EmitStatus();
}

void CxAgentClientBridge::ScheduleReconnect(std::uint64_t generation)
{
    std::string http_base_url;
    std::string client_id;
    std::string session_id;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_reconnect_scheduled)
            return;
        if (m_status.http_base_url.empty() || m_status.client_id.empty() || m_status.session_id.empty())
            return;
        m_reconnect_scheduled = true;
        http_base_url = m_status.http_base_url;
        client_id = m_status.client_id;
        session_id = m_status.session_id;
    }

    std::thread([this, generation, http_base_url, client_id, session_id]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        std::shared_ptr<Session> session;
        std::string ws_url;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (generation != m_connection_generation) {
                m_reconnect_scheduled = false;
                return;
            }
            if (m_status.connected || m_status.connecting) {
                m_reconnect_scheduled = false;
                return;
            }
            m_status.http_base_url = http_base_url;
            m_status.client_id = client_id;
            m_status.session_id = session_id;
            m_status.ws_url = BuildWsUrl(http_base_url, client_id);
            m_status.connecting = true;
            m_reconnect_scheduled = false;
            session = std::make_shared<Session>(this);
            m_session = session;
            ws_url = m_status.ws_url;
        }
        EmitStatus();
        session->start(ws_url);
    }).detach();
}

void CxAgentClientBridge::HandleIncomingMessage(const Json& msg)
{
    if (msg.contains("server_seq")) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.last_seen_server_seq = msg.value("server_seq", m_status.last_seen_server_seq);
    }
    EmitStatus();

    MessageHandler handler;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        handler = m_message_handler;
    }
    if (handler)
        handler(msg);
}

void CxAgentClientBridge::EmitStatus()
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
