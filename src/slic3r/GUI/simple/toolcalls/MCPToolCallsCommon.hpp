#ifndef slic3r_MCPToolCallsCommon_hpp_
#define slic3r_MCPToolCallsCommon_hpp_

#include "nlohmann/json.hpp"

#include <initializer_list>
#include <string>
#include <vector>

namespace Slic3r {
namespace GUI {

class NotificationManager;

namespace ToolCalls {

enum class NativeExecutionMode
{
    BridgeImmediateOrPayloadDeferred,
    DeferredSliceResult,
    DeferredSceneSettle,
    CustomDeferred,
};

struct BridgeToolRouteSpec
{
    const char* tool = "";
    const char* action_id = "";
    bool register_handler = false;
    bool native_supported = false;
    const char* progress_stage = "";
    const char* progress_start_message = "";
    const char* progress_done_message = "";
    const char* error_code = "";
    const char* error_message = "";
    bool normalize_apply_config = false;
    bool attach_project_context = false;
    NativeExecutionMode execution_mode = NativeExecutionMode::BridgeImmediateOrPayloadDeferred;
};

const std::vector<BridgeToolRouteSpec>& GetBridgeToolRouteSpecs();
const BridgeToolRouteSpec* FindBridgeToolRouteSpec(const std::string& tool);
const char* NativeExecutionModeName(NativeExecutionMode mode);
bool IsDeferredNativeExecution(NativeExecutionMode mode);

nlohmann::json BuildBlockingErrorsPayload(const nlohmann::json& state);
nlohmann::json BuildExplicitFactsFromState(const nlohmann::json& state);
void AttachExplicitFactsFromState(nlohmann::json& payload, const nlohmann::json& state);

nlohmann::json BuildGeometryAnalysisFromState(const nlohmann::json& state);
nlohmann::json BuildVisualRecommendationGeometryFromState(const nlohmann::json& state);

nlohmann::json NormalizeApplyParamPatchArgs(const nlohmann::json& args);

std::string SafeJsonDumpForLog(const nlohmann::json& value);
void LogAISendPanelStage(const std::string& stage,
                         const std::string& card_id,
                         const std::string& request_id,
                         const std::string& message);

void ClearWebViewLogFile();
void AppendWebViewLogLine(const nlohmann::json& payload);
void DismissNotificationEntry(Slic3r::GUI::NotificationManager* notification_manager,
                              const nlohmann::json& entry);

} // namespace ToolCalls
} // namespace GUI
} // namespace Slic3r

#endif
