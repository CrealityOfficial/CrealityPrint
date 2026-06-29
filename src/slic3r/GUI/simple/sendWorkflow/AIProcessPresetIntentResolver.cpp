#include "AIProcessPresetIntentResolver.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <utility>

namespace Slic3r {
namespace GUI {

namespace {

struct PresetCandidate {
    std::string name;
    std::string normalized;
    double      layer_height = -1.0;
    bool        is_standard = false;
};

std::string trim_copy(std::string value)
{
    auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
    while (!value.empty() && is_space(static_cast<unsigned char>(value.front())))
        value.erase(value.begin());
    while (!value.empty() && is_space(static_cast<unsigned char>(value.back())))
        value.pop_back();
    return value;
}

std::string normalize_copy(const std::string& value)
{
    std::string out = trim_copy(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return out;
}

bool contains_any(const std::string& normalized, const std::vector<std::string>& tags)
{
    for (const std::string& tag : tags) {
        if (!tag.empty() && normalized.find(normalize_copy(tag)) != std::string::npos)
            return true;
    }
    return false;
}

double parse_layer_height_mm(const std::string& preset_name)
{
    const std::size_t mm_pos = preset_name.find("mm");
    if (mm_pos == std::string::npos)
        return -1.0;

    std::size_t begin = 0;
    while (begin < mm_pos && !std::isdigit(static_cast<unsigned char>(preset_name[begin])) && preset_name[begin] != '.')
        ++begin;
    if (begin >= mm_pos)
        return -1.0;

    const std::string number = preset_name.substr(begin, mm_pos - begin);
    char* end_ptr = nullptr;
    const double value = std::strtod(number.c_str(), &end_ptr);
    if (end_ptr == number.c_str())
        return -1.0;
    return value;
}

std::vector<PresetCandidate> build_candidates(const AIProcessResolveContext& context)
{
    std::vector<PresetCandidate> candidates;
    candidates.reserve(context.available_print_preset_names.size());
    for (const std::string& name : context.available_print_preset_names) {
        if (name.empty())
            continue;

        PresetCandidate candidate;
        candidate.name = name;
        candidate.normalized = normalize_copy(name);
        candidate.layer_height = parse_layer_height_mm(name);
        candidate.is_standard = candidate.normalized.find("standard") != std::string::npos;
        candidates.push_back(std::move(candidate));
    }
    return candidates;
}

std::string effective_baseline_preset(const AIProcessResolveContext& context)
{
    if (!context.current_print_preset_name.empty())
        return context.current_print_preset_name;
    return context.default_print_preset_name;
}

double effective_baseline_height(const AIProcessResolveContext& context)
{
    const double current = parse_layer_height_mm(context.current_print_preset_name);
    if (current > 0.0)
        return current;
    return parse_layer_height_mm(context.default_print_preset_name);
}

std::string determine_machine_group(const std::vector<PresetCandidate>& candidates)
{
    for (const PresetCandidate& candidate : candidates) {
        if (contains_any(candidate.normalized, {"strength", "structural", "high quality", "fine", "optimal"}))
            return "explicit_semantic";
    }
    return "layer_height_gradient";
}

AIProcessIntentResolution make_keep_current(
    const AIProcessResolveContext& context,
    AIProcessIntent intent,
    const std::string& strategy,
    const std::string& summary,
    const std::string& fallback_reason = {})
{
    AIProcessIntentResolution resolution;
    resolution.success = true;
    resolution.intent = intent;
    resolution.resolved_preset_name = effective_baseline_preset(context);
    resolution.requires_change = false;
    resolution.strategy = strategy;
    resolution.summary_text = summary;
    resolution.fallback_reason = fallback_reason;
    return resolution;
}

AIProcessIntentResolution make_switch(
    const AIProcessResolveContext& context,
    AIProcessIntent intent,
    const std::string& preset_name,
    const std::string& strategy,
    const std::string& summary)
{
    AIProcessIntentResolution resolution;
    resolution.success = !preset_name.empty();
    resolution.intent = intent;
    resolution.resolved_preset_name = preset_name;
    resolution.requires_change = !preset_name.empty() && preset_name != context.current_print_preset_name;
    resolution.strategy = strategy;
    resolution.summary_text = summary;
    return resolution;
}

const PresetCandidate* find_named_candidate(
    const std::vector<PresetCandidate>& candidates,
    const std::vector<std::string>& tags)
{
    for (const PresetCandidate& candidate : candidates) {
        if (contains_any(candidate.normalized, tags))
            return &candidate;
    }
    return nullptr;
}

const PresetCandidate* find_nearest_standard(
    const std::vector<PresetCandidate>& candidates,
    double baseline_height,
    bool smaller)
{
    const PresetCandidate* best = nullptr;
    double best_delta = std::numeric_limits<double>::max();

    for (const PresetCandidate& candidate : candidates) {
        if (!candidate.is_standard || candidate.layer_height <= 0.0)
            continue;

        if (baseline_height > 0.0) {
            if (smaller && candidate.layer_height >= baseline_height)
                continue;
            if (!smaller && candidate.layer_height <= baseline_height)
                continue;
        }

        const double delta = baseline_height > 0.0 ? std::fabs(candidate.layer_height - baseline_height) : candidate.layer_height;
        if (delta < best_delta) {
            best_delta = delta;
            best = &candidate;
        }
    }

    return best;
}

AIProcessIntentResolution resolve_direct(const AIProcessResolveContext& context)
{
    const std::string baseline = effective_baseline_preset(context);
    return make_keep_current(
        context,
        AIProcessIntent::Direct,
        "keep_current",
        baseline.empty() ? "Keep the current process preset." : "Keep the current process preset.");
}

AIProcessIntentResolution resolve_appearance(const AIProcessResolveContext& context,
                                             const std::vector<PresetCandidate>& candidates)
{
    if (const PresetCandidate* semantic = find_named_candidate(candidates, {"high quality", "fine", "optimal"})) {
        return make_switch(
            context,
            AIProcessIntent::Appearance,
            semantic->name,
            "named_quality",
            "Switch to an appearance-oriented process preset.");
    }

    if (const PresetCandidate* smaller = find_nearest_standard(candidates, effective_baseline_height(context), true)) {
        return make_switch(
            context,
            AIProcessIntent::Appearance,
            smaller->name,
            "smaller_standard",
            "Switch to a finer standard preset for better appearance.");
    }

    return make_keep_current(
        context,
        AIProcessIntent::Appearance,
        "keep_current",
        "No finer appearance-oriented process preset is available.",
        "Current printer has no finer appearance-oriented process preset.");
}

AIProcessIntentResolution resolve_speed(const AIProcessResolveContext& context,
                                        const std::vector<PresetCandidate>& candidates)
{
    if (const PresetCandidate* semantic = find_named_candidate(candidates, {"draft", "fast", "extra draft"})) {
        return make_switch(
            context,
            AIProcessIntent::Speed,
            semantic->name,
            "named_speed",
            "Switch to a speed-oriented process preset.");
    }

    if (const PresetCandidate* larger = find_nearest_standard(candidates, effective_baseline_height(context), false)) {
        return make_switch(
            context,
            AIProcessIntent::Speed,
            larger->name,
            "larger_standard",
            "Switch to a faster standard preset.");
    }

    return make_keep_current(
        context,
        AIProcessIntent::Speed,
        "keep_current",
        "No faster process preset is available.",
        "Current printer has no faster process preset.");
}

AIProcessIntentResolution resolve_strength(const AIProcessResolveContext& context,
                                           const std::vector<PresetCandidate>& candidates)
{
    if (const PresetCandidate* semantic = find_named_candidate(candidates, {"strength", "structural"})) {
        AIProcessIntentResolution resolution = make_switch(
            context,
            AIProcessIntent::Strength,
            semantic->name,
            "named_strength",
            "Switch to a dedicated strength-oriented process preset.");
        resolution.true_strength_supported = true;
        return resolution;
    }

    return make_keep_current(
        context,
        AIProcessIntent::Strength,
        "keep_current",
        "This printer has no dedicated strength process preset.",
        "Current printer has no dedicated strength process preset.");
}

} // namespace

std::string to_string(AIProcessIntent intent)
{
    switch (intent) {
    case AIProcessIntent::Direct: return "direct";
    case AIProcessIntent::Speed: return "speed";
    case AIProcessIntent::Appearance: return "appearance";
    case AIProcessIntent::Strength: return "strength";
    }
    return "direct";
}

bool parse_process_intent(const std::string& raw, AIProcessIntent& out)
{
    const std::string normalized = normalize_copy(raw);
    if (normalized == "direct" || normalized == "direct_print") {
        out = AIProcessIntent::Direct;
        return true;
    }
    if (normalized == "speed") {
        out = AIProcessIntent::Speed;
        return true;
    }
    if (normalized == "appearance" || normalized == "quality") {
        out = AIProcessIntent::Appearance;
        return true;
    }
    if (normalized == "strength") {
        out = AIProcessIntent::Strength;
        return true;
    }
    return false;
}

AIProcessIntentResolution AIProcessPresetIntentResolver::Resolve(
    const AIProcessResolveContext& context,
    AIProcessIntent intent)
{
    const std::vector<PresetCandidate> candidates = build_candidates(context);
    AIProcessIntentResolution resolution;

    if (candidates.empty()) {
        resolution.intent = intent;
        resolution.summary_text = "No print presets are available for resolution.";
        resolution.fallback_reason = "No print presets are available.";
        return resolution;
    }

    switch (intent) {
    case AIProcessIntent::Direct:
        resolution = resolve_direct(context);
        break;
    case AIProcessIntent::Speed:
        resolution = resolve_speed(context, candidates);
        break;
    case AIProcessIntent::Appearance:
        resolution = resolve_appearance(context, candidates);
        break;
    case AIProcessIntent::Strength:
        resolution = resolve_strength(context, candidates);
        break;
    }

    resolution.machine_group = determine_machine_group(candidates);
    if (resolution.summary_text.empty())
        resolution.summary_text = "Process resolution completed.";
    return resolution;
}

} // namespace GUI
} // namespace Slic3r
