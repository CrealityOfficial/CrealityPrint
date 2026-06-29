#ifndef slic3r_SlicerBridge_hpp_
#define slic3r_SlicerBridge_hpp_

#include "SlicerAction.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Slic3r { class ModelObject; }

namespace Slic3r {
namespace GUI {
namespace Bridge {

/// Result of lightweight overhang analysis on a model object.
struct OverhangResult {
    double overhang_ratio         = 0.0;  ///< Legacy alias of unsupported_area_ratio.
    double unsupported_area_ratio = 0.0;  ///< 0.0 ~ 1.0  downward-facing area with visible clearance to the bed.
    double downward_area_ratio    = 0.0;  ///< 0.0 ~ 1.0  all downward-facing area above the configured threshold.
    double bottom_contact_ratio   = 0.0;  ///< 0.0 ~ 1.0  surface area near the object's bottom used as bed contact proxy.
    double bottom_contact_area_mm2 = 0.0;
    double max_overhang_angle     = 0.0;  ///< degrees (0 = horizontal bottom, 90 = vertical wall)
};

/// SlicerBridge �C Central registry and executor for slicer actions.
///
/// Holds the catalogue of available actions with metadata (name, params,
/// requires_confirm) and executes them against the running slicer.
///
/// Usage from MCPChatPanel:
///   auto& bridge = SlicerBridge::Instance();
///   nlohmann::json result = bridge.Execute("start_slice", params);
///
class SlicerBridge
{
public:
    using ActionExecutor = std::function<nlohmann::json(const nlohmann::json&)>;

    static SlicerBridge& Instance();

    // ---- Registry queries ----

    /// Return all registered action definitions.
    const std::vector<ActionDef>& GetActionList() const { return m_actions; }

    /// Look up a single action by id.  Returns nullptr if not found.
    const ActionDef* FindAction(const std::string& id) const;

    /// Serialise the full action catalogue as JSON (for the front-end).
    nlohmann::json GetActionListJSON() const;

    /// Export the action catalogue as local-tool metadata for Dify/MCP-style orchestration.
    nlohmann::json GetAvailableToolsJSON() const;

    /// Generate a Dify-ready system prompt describing every registered action.
    std::string GenerateSystemPrompt() const;

    // ---- Execution ----

    /// Execute an action by id.  Returns a JSON object with at least
    /// { "success": bool, "action": string, "message": string, ... }
    nlohmann::json Execute(const std::string& action_id,
                           const nlohmann::json& params = nlohmann::json::object());

    void SetSendToPrinterDelegate(void* owner, ActionExecutor delegate);
    void ClearSendToPrinterDelegate(void* owner);

    void ClearPendingModelSearchCache();

    // ---- Dev tools ----

    /// Build the user-facing slicer parameter schema array (same content the
    /// "get_config_options" action returns under the "options" field).
    /// Pure function over print_config_def — no GUI/runtime state required.
    static nlohmann::json BuildConfigSchemaArray();

    /// Dev-only helper: dump the schema array as pretty-printed JSON to the
    /// given UTF-8 file path. Returns true on success; on failure populates
    /// out_message (when provided) with the error description.
    /// Used by the "Export AI Config Schema" menu item to produce
    /// CxAgent/sagent/data/config_schema.json without going through the
    /// runtime CP → agent push channel.
    static bool ExportConfigSchemaToFile(const std::string& utf8_path,
                                         std::string* out_message = nullptr);

private:
    SlicerBridge();
    ~SlicerBridge() = default;
    SlicerBridge(const SlicerBridge&) = delete;
    SlicerBridge& operator=(const SlicerBridge&) = delete;

    // ---- Internal registration ----
    void RegisterAllActions();
    void RegisterAction(ActionDef def, ActionExecutor executor);

    // ---- Helpers ----
    static OverhangResult AnalyzeOverhang(const ModelObject& obj, double threshold_deg);

    // ---- Action executor implementations ----
    nlohmann::json DoGetPresets(const nlohmann::json& params);
    nlohmann::json DoSelectPreset(const nlohmann::json& params);
    nlohmann::json DoListPrinters(const nlohmann::json& params);
    nlohmann::json DoSelectPrinter(const nlohmann::json& params);
    nlohmann::json DoMovePrintHead(const nlohmann::json& params);
    nlohmann::json DoPrintControl(const nlohmann::json& params);
    nlohmann::json DoApplyConfig(const nlohmann::json& params);
    nlohmann::json DoSetObjectColor(const nlohmann::json& params);
    nlohmann::json DoGetEditedConfig(const nlohmann::json& params);
    nlohmann::json DoGetSlicerState(const nlohmann::json& params);
    nlohmann::json DoGetSceneDiagnostics(const nlohmann::json& params);
    nlohmann::json DoCaptureModelViews(const nlohmann::json& params);
    nlohmann::json DoImportModel(const nlohmann::json& params);
    nlohmann::json DoImportModelFromSearch(const nlohmann::json& params);
    nlohmann::json DoOpenModelLibrary(const nlohmann::json& params);
    nlohmann::json DoRecommendModel(const nlohmann::json& params);
    nlohmann::json DoSmartModelSearch(const nlohmann::json& params);
    nlohmann::json DoAutoOrient(const nlohmann::json& params);
    nlohmann::json DoRepairMesh(const nlohmann::json& params);
    nlohmann::json DoAutoArrange(const nlohmann::json& params);
    nlohmann::json DoUndo(const nlohmann::json& params);
    nlohmann::json DoRedo(const nlohmann::json& params);
    nlohmann::json DoStartSlice(const nlohmann::json& params);
    nlohmann::json DoSendToPrinter(const nlohmann::json& params);
    nlohmann::json DoExportGcode(const nlohmann::json& params);
    nlohmann::json DoGetConfigOptions(const nlohmann::json& params);
    nlohmann::json DoAddFilament(const nlohmann::json& params);
    nlohmann::json DoDeleteFilament(const nlohmann::json& params);
    nlohmann::json DoSetFilamentType(const nlohmann::json& params);
    nlohmann::json DoAutoMapFilaments(const nlohmann::json& params);
    nlohmann::json DoMoveObject(const nlohmann::json& params);
    nlohmann::json DoRotateObject(const nlohmann::json& params);
    nlohmann::json DoScaleObject(const nlohmann::json& params);
    nlohmann::json DoSelectObjects(const nlohmann::json& params);
    nlohmann::json DoDeleteModel(const nlohmann::json& params);
    nlohmann::json DoCloneModel(const nlohmann::json& params);
    nlohmann::json DoArrangeSinglePlate(const nlohmann::json& params);
    nlohmann::json DoArrangeAllPlates(const nlohmann::json& params);
    nlohmann::json DoFillBed(const nlohmann::json& params);
    nlohmann::json DoGetSceneWarnings(const nlohmann::json& params);
    nlohmann::json DoRenamePlate(const nlohmann::json& params);
    nlohmann::json DoAddPlate(const nlohmann::json& params);
    nlohmann::json DoDeletePlate(const nlohmann::json& params);
    nlohmann::json DoTogglePreviewLiteMode(const nlohmann::json& params);
    nlohmann::json DoSimplifyModel(const nlohmann::json& params);
    nlohmann::json DoSplitModel(const nlohmann::json& params);
    nlohmann::json DoAddTestModel(const nlohmann::json& params);

    struct CachedModelSearchResult {
        std::string model_id;
        std::string model_name;
        std::string download_url;
        std::string author;
        std::string keyword;
        std::string sort_by;
        std::string sort_label;
        int likes = 0;
        int downloads = 0;
    };

    // ---- Data ----
    std::vector<ActionDef> m_actions;
    std::unordered_map<std::string, ActionExecutor> m_executors;
    void* m_send_to_printer_delegate_owner = nullptr;
    ActionExecutor m_send_to_printer_delegate;
    CachedModelSearchResult m_cached_model_search_result;
};

} // namespace Bridge
} // namespace GUI
} // namespace Slic3r

#endif // slic3r_SlicerBridge_hpp_
