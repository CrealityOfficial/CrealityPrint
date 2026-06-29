#ifndef slic3r_AIProcessSwitchService_hpp_
#define slic3r_AIProcessSwitchService_hpp_

#include "AIProcessPresetIntentResolver.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <vector>

namespace Slic3r {
namespace GUI {

struct AIProcessApplyResult {
    bool                      success = false;
    bool                      changed = false;
    bool                      reslice_expected = false;
    std::string               code;
    std::string               message;
    AIProcessIntent           intent = AIProcessIntent::Direct;
    AIProcessIntentResolution resolution;
    nlohmann::json            bridge_result = nlohmann::json::object();
};

class AIProcessSwitchService
{
public:
    AIProcessSwitchService() = default;

    AIProcessApplyResult ApplyIntent(AIProcessIntent intent) const;
    AIProcessResolveContext BuildContext() const;

private:
    nlohmann::json get_slicer_state() const;
    nlohmann::json get_presets() const;

    std::string resolve_default_print_preset_name(
        const nlohmann::json& slicer_state,
        const nlohmann::json& presets_result) const;
    std::vector<std::string> collect_print_preset_names(
        const AIProcessResolveContext& context,
        const nlohmann::json& presets_result) const;
    AIProcessApplyResult apply_print_preset(
        AIProcessIntent intent,
        const AIProcessIntentResolution& resolution) const;
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_AIProcessSwitchService_hpp_
