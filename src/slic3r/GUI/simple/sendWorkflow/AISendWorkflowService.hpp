#ifndef slic3r_AISendWorkflowService_hpp_
#define slic3r_AISendWorkflowService_hpp_

#include "nlohmann/json.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Slic3r {
namespace GUI {

class EasyPrintSender;
class ImGuiFilamentPanel;

class AISendWorkflowService
{
public:
    using EventCallback = std::function<void(const nlohmann::json&)>;

    struct OpenResult {
        bool        success = false;
        std::string code;
        std::string message;
        std::string card_id;
    };

    AISendWorkflowService();
    ~AISendWorkflowService();

    void SetSnapshotCallback(EventCallback callback);
    void SetProgressCallback(EventCallback callback);
    void SetResultCallback(EventCallback callback);
    void SetErrorCallback(EventCallback callback);

    OpenResult OpenCard(const std::string& request_id,
                        const nlohmann::json& args = nlohmann::json::object());

    bool StartSendOnly(const std::string& card_id);
    bool StartSendAndPrint(const std::string& card_id);
    bool SelectPlate(const std::string& card_id, int plate_index);
    bool AutoMatch(const std::string& card_id);
    bool UpdateMapping(const std::string& card_id, const nlohmann::json& payload);
    bool ApplyMapping(const std::string& card_id);
    bool ApplyProcessIntent(const std::string& card_id, const std::string& intent_key);
    bool Cancel(const std::string& card_id);
    std::string FindCardIdByRequestId(const std::string& request_id);

    std::vector<nlohmann::json> GetActiveSnapshots() const;
    void RefreshActiveSnapshots();
    void RefreshMappingForCurrentDevice(bool auto_match);

private:
    struct Session {
        std::string                  card_id;
        std::string                  request_id;
        nlohmann::json               open_args = nlohmann::json::object();
        nlohmann::json               last_state = nlohmann::json::object();
        nlohmann::json               last_snapshot = nlohmann::json::object();
        std::shared_ptr<EasyPrintSender> sender;
        std::string                  status = "ready";
        std::string                  status_text = "Ready to send";
        std::string                  last_action = "open";
        int                          revision = 0;
        int                          progress = 0;
        bool                         waiting_user_action = true;
        bool                         in_progress = false;
        bool                         terminal = false;
        bool                         last_start_print = false;
        bool                         cloud_workflow_active = false;
        bool                         mapping_sync_retry_active = false;
        std::uint64_t                mapping_sync_retry_token = 0;
        std::unordered_map<int, std::string> draft_selection_tokens;
        int                          selected_plate_index = -1;
        nlohmann::json               mapping_items = nlohmann::json::array();
        int                          mapping_items_scope_plate_index = -2;
        bool                         mapping_dirty = false;
        bool                         mapping_applied_to_scene = false;
        std::string                  selected_process_intent = "direct";
        std::string                  current_print_preset_name;
        std::string                  resolved_process_preset_name;
        std::string                  process_summary_text = "Keep the current process preset.";
        std::string                  process_status = "idle";
        std::string                  process_status_text;
        bool                         process_switch_in_progress = false;
        bool                         process_reslice_expected = false;
    };

    OpenResult open_card_locked(const std::string& request_id, const nlohmann::json& args);
    bool start_send_internal(const std::string& card_id, bool start_print);

    bool refresh_state_locked(Session& session, std::string& code, std::string& message);
    bool refresh_process_context_locked(Session& session, std::string& code, std::string& message);
    bool can_send_locked(const Session& session, std::string& message) const;

    ImGuiFilamentPanel* get_filament_panel() const;
    int resolve_selected_plate_index_locked(const Session& session) const;
    std::pair<std::string, std::string> resolve_gcode_file(const Session& session) const;
    nlohmann::json build_print_data(const Session& session, const std::string& upload_name) const;
    nlohmann::json build_send_info_locked(const Session& session) const;
    nlohmann::json build_mapping_items(const Session& session) const;
    nlohmann::json build_effective_mapping_items_for_send_locked(const Session& session) const;
    nlohmann::json build_mapping_option_groups() const;
    int desired_mapping_scope_plate_index_locked(const Session& session) const;
    nlohmann::json filter_source_mapping_items_to_plate_locked(
        const nlohmann::json& source_mapping_items,
        int preferred_plate_index) const;
    bool rebuild_mapping_items_for_open_locked(Session& session, bool auto_match);
    bool ensure_mapping_items_locked(Session& session, bool auto_match, bool allow_restore_last_applied = true);
    bool restore_last_applied_mapping_locked(Session& session);
    void sync_mapping_dirty_locked(Session& session) const;
    void ensure_original_source_snapshot_locked(Session& session);
    void invalidate_current_plate_mapping_preview_bases_locked(const Session& session) const;
    nlohmann::json merge_original_source_into_effective_mapping_items(
        const Session& session,
        const nlohmann::json& effective_items) const;
    bool is_mapping_loading_snapshot(const nlohmann::json& snapshot) const;
    void schedule_pending_mapping_refresh(const std::string& card_id);

    nlohmann::json build_snapshot_envelope_locked(Session& session);
    nlohmann::json build_progress_envelope_locked(
        Session& session,
        int progress,
        const std::string& message,
        const std::string& stage,
        const std::string& state,
        double speed = 0.0);
    nlohmann::json build_result_envelope_locked(
        Session& session,
        const std::string& result_type,
        const std::string& message,
        const nlohmann::json& details = nlohmann::json::object());
    nlohmann::json build_error_envelope_locked(
        Session& session,
        const std::string& code,
        const std::string& message,
        const nlohmann::json& details = nlohmann::json::object());
    void update_last_snapshot_status_locked(Session& session);

    void on_upload_progress(const std::string& card_id, float progress, double speed);
    void on_upload_status(const std::string& card_id, int status_code);
    void on_upload_complete(const std::string& card_id, bool start_print, const std::string& body);
    void on_cloud_print_progress(const std::string& card_id,
                                 int progress,
                                 const std::string& stage,
                                 const std::string& message);
    void on_cloud_print_success(const std::string& card_id, const nlohmann::json& result);
    void on_cloud_print_error(const std::string& card_id,
                              const std::string& code,
                              const std::string& message);

    void emit_snapshot(const nlohmann::json& envelope) const;
    void emit_progress(const nlohmann::json& envelope) const;
    void emit_result(const nlohmann::json& envelope) const;
    void emit_error(const nlohmann::json& envelope) const;

    static long long current_timestamp_ms();
    static bool is_upload_successful(const std::string& body);

private:
    mutable std::mutex                        m_mutex;
    std::unordered_map<std::string, Session>  m_sessions;
    std::uint64_t                             m_next_card_id = 1;
    nlohmann::json                            m_last_applied_mapping_items = nlohmann::json::array();
    int                                       m_last_applied_plate_index = -1;
    EventCallback                             m_snapshot_callback;
    EventCallback                             m_progress_callback;
    EventCallback                             m_result_callback;
    EventCallback                             m_error_callback;
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_AISendWorkflowService_hpp_
