#ifndef slic3r_AIProcessPresetIntentResolver_hpp_
#define slic3r_AIProcessPresetIntentResolver_hpp_

#include <string>
#include <vector>

namespace Slic3r {
namespace GUI {

enum class AIProcessIntent {
    Direct = 0,
    Speed,
    Appearance,
    Strength
};

struct AIProcessResolveContext {
    std::string              printer_preset_name;
    std::string              printer_model;
    std::string              nozzle_diameter;
    std::string              current_print_preset_name;
    std::string              default_print_preset_name;
    std::vector<std::string> available_print_preset_names;
};

struct AIProcessIntentResolution {
    bool            success = false;
    bool            requires_change = false;
    bool            true_strength_supported = false;
    AIProcessIntent intent = AIProcessIntent::Direct;
    std::string     resolved_preset_name;
    std::string     fallback_reason;
    std::string     summary_text;
    std::string     machine_group;
    std::string     strategy;
};

std::string to_string(AIProcessIntent intent);
bool parse_process_intent(const std::string& raw, AIProcessIntent& out);

class AIProcessPresetIntentResolver
{
public:
    static AIProcessIntentResolution Resolve(
        const AIProcessResolveContext& context,
        AIProcessIntent intent);
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_AIProcessPresetIntentResolver_hpp_
