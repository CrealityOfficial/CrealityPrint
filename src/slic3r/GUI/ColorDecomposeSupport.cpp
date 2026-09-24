#include "ColorDecomposeSupport.hpp"
#include "MixedFilamentDialog.hpp"
#include "GUI_App.hpp"
#include "MsgDialog.hpp"
#include "I18N.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Utils.hpp"
#include "cr_km_recipe.h"

#include <algorithm>
#include <cctype>

namespace Slic3r { namespace GUI {

std::string decompose_normalize_color_hex(std::string color)
{
    if (color.size() >= 7)
        color = color.substr(0, 7);
    std::transform(color.begin(), color.end(), color.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return color;
}

const char* decompose_base_color_en(DecomposeBaseColor color)
{
    switch (color) {
    case DecomposeBaseColor::Cyan:    return "Cyan";
    case DecomposeBaseColor::Magenta: return "Magenta";
    case DecomposeBaseColor::Yellow:  return "Yellow";
    case DecomposeBaseColor::White:   return "White";
    case DecomposeBaseColor::Red:     return "Red";
    case DecomposeBaseColor::Green:   return "Green";
    case DecomposeBaseColor::Blue:    return "Blue";
    default:                          return "";
    }
}

wxString decompose_base_color_display(DecomposeBaseColor color)
{
    switch (color) {
    case DecomposeBaseColor::Cyan:    return _L("Cyan");
    case DecomposeBaseColor::Magenta: return _L("Magenta");
    case DecomposeBaseColor::Yellow:  return _L("Yellow");
    case DecomposeBaseColor::White:   return _L("White");
    case DecomposeBaseColor::Red:     return _L("Red");
    case DecomposeBaseColor::Green:   return _L("Green");
    case DecomposeBaseColor::Blue:    return _L("Blue");
    default:                          return wxString();
    }
}

std::string decompose_basic_filament_id(const std::string& basic_type)
{
    // C3D does not have Bambu's official preset system. Return a synthetic id
    // that is non-empty so callers can use it as a stable key when matching
    // existing project filaments.
    if (basic_type == kDecomposePetgBasicType)
        return "PETG_BASIC";
    return "PLA_BASIC";
}

DecomposeOfficialComponent lookup_decompose_official_component(
    const std::string& basic_type,
    DecomposeMode mode,
    DecomposeBaseColor base_color,
    const wxColour& fallback)
{
    DecomposeOfficialComponent result;
    result.base_color  = base_color;
    result.filament_id = decompose_basic_filament_id(basic_type);

    // Prefer the official_hex shipped with base_color_spectra.json so the
    // UI swatch matches what the K-M engine treats as "100% of this base".
    // The closed-source library returns "#000000" only on resource failure;
    // in that case fall back to the caller's hard-coded RGB.
    const char* en_name = decompose_base_color_en(base_color);
    if (en_name && *en_name) {
        const int abi_mode = (mode == DecomposeMode::RYBW)
            ? C3D_RECIPE_MODE_RYBW
            : C3D_RECIPE_MODE_CMYW;
        const char* official = cr_close_get_base_official_hex(
            abi_mode, basic_type.c_str(), en_name);
        if (official && *official && std::strcmp(official, "#000000") != 0) {
            result.color_hex = decompose_normalize_color_hex(std::string(official));
            return result;
        }
    }
    result.color_hex = decompose_normalize_color_hex(
        fallback.GetAsString(wxC2S_HTML_SYNTAX).ToStdString());
    return result;
}

std::string find_decompose_standard_preset_name(size_t /*source_config_idx*/, const std::string& /*basic_type*/)
{
    // C3D does not ship Bambu's official presets; this lookup is a no-op
    // until a custom preset naming convention is introduced.
    return {};
}

std::string official_basic_type_from_preset_name(const std::string& preset_name)
{
    if (preset_name.find(std::string(kDecomposeBambuPresetPrefix) + kDecomposePlaBasicType) != std::string::npos)
        return kDecomposePlaBasicType;
    if (preset_name.find(std::string(kDecomposeBambuPresetPrefix) + kDecomposePetgBasicType) != std::string::npos)
        return kDecomposePetgBasicType;
    return {};
}

std::string filament_type_for_color_decompose(Preset* preset)
{
    if (!preset)
        return kDecomposePlaShortType;

    std::string display_type;
    std::string ft = preset->config.get_filament_type(display_type);
    const std::string basic = official_basic_type_from_preset_name(preset->name);
    if (!basic.empty())
        ft = basic;
    if (ft.empty())
        ft = kDecomposePlaShortType;
    return ft;
}

int find_existing_decompose_component(
    const DecomposeOfficialComponent& component,
    const std::vector<std::string>& physical_colors,
    const std::vector<std::string>& physical_types,
    const std::vector<size_t>& physical_config_indices,
    size_t source_config_idx,
    const std::string& required_type)
{
    const size_t num_physical = physical_colors.size();
    for (size_t i = 0; i < num_physical && i < physical_config_indices.size(); ++i) {
        const size_t config_idx = physical_config_indices[i];
        if (config_idx == source_config_idx)
            continue;
        // CMYW/RYBW requires the existing physical to match BOTH color AND
        // material type (Hyper PLA). A Generic PLA slot with the same color
        // does NOT count as a reusable base color — the user explicitly
        // requested Hyper PLA decomposition, so only Hyper PLA physicals
        // qualify. When required_type is empty (MaterialList mode) we skip
        // the type check and only match by color.
        if (!required_type.empty()) {
            if (i >= physical_types.size() || physical_types[i] != required_type)
                continue;
        }
        const std::string slot_color = decompose_normalize_color_hex(physical_colors[i]);
        if (slot_color == component.color_hex)
            return static_cast<int>(i + 1);  // Return 1-based physical slot index, not config index
    }
    return -1;
}

bool prepare_decompose_mixed_result(
    const ColorDecomposeResult& result,
    size_t /*source_config_idx*/,
    size_t /*source_physical_idx*/,
    const std::vector<std::string>& physical_colors,
    const std::vector<std::string>& physical_types,
    const std::vector<size_t>& physical_config_indices,
    MixedFilamentResult& out_result,
    std::vector<DecomposeMissingComponent>& missing)
{
    out_result = MixedFilamentResult{};
    missing.clear();
    if (result.components.empty())
        return false;

    const bool standard_mode = result.mode == DecomposeMode::CMYW || result.mode == DecomposeMode::RYBW;
    // Standard modes require Hyper PLA physicals; MaterialList accepts any type.
    const std::string required_type = standard_mode ? kDecomposeHyperPlaType : std::string{};

    // CMYW/RYBW 单色（目标色本身是某个标准 base color、100% 该 base）场景：
    // try_build_single_base_result 会返回 1 个 component，base_color != None
    // 但 filament_index == -1（该 base 还没有对应的物理耗材）。这里仍然走
    // “缺耗材”流程：找到 missing、加到 missing 列表，但不创建 mixed filament
    // （mixed_result.components 留空，调用方应跳过 Step 2 的 add_custom_filament）。
    for (size_t i = 0; i < result.components.size(); ++i) {
        const DecomposeComponent& comp = result.components[i];
        out_result.ratios.push_back(comp.ratio);
        if (!standard_mode) {
            if (comp.filament_index <= 0)
                return false;
            const size_t physical_idx = static_cast<size_t>(comp.filament_index - 1);
            if (physical_idx >= physical_config_indices.size())
                return false;
            out_result.components.push_back(static_cast<unsigned int>(physical_idx + 1));  // 1-based physical slot index
            continue;
        }

        if (comp.base_color == DecomposeBaseColor::None)
            return false;
        DecomposeOfficialComponent official_component =
            lookup_decompose_official_component(kDecomposeHyperPlaType, result.mode, comp.base_color, comp.colour);
        int existing_idx = find_existing_decompose_component(official_component, physical_colors,
                                                             physical_types,
                                                             physical_config_indices, 0,
                                                             required_type);
        if (existing_idx > 0) {
            // 单色且已存在：表明不需要创建 mixed filament，但可以可选地
            // 仍然输出 components 以便调用方知道。现状是单色 + 已存在
            // 场景下 mixed_result.components 也保持为空。
            if (result.components.size() >= 2)
                out_result.components.push_back(static_cast<unsigned int>(existing_idx));
            continue;
        }

        DecomposeMissingComponent missing_comp;
        missing_comp.component_idx = out_result.components.size();
        missing_comp.official_component = official_component;
        // Include material type in display name for clarity (e.g., "Yellow Hyper PLA")
        missing_comp.display_name = decompose_base_color_display(comp.base_color) +
            (required_type.empty() ? wxString() : wxString(" ") + wxString::FromUTF8(required_type));
        missing.push_back(std::move(missing_comp));
        out_result.components.push_back(0);
    }

    // 单色场景返回 true（以让调用方处理 missing），但 components 为空
    // 表示不需要创建 mixed filament。
    return out_result.components.size() == out_result.ratios.size();
}

size_t count_decompose_new_physical_filaments(
    const ColorDecomposeResult& result,
    const std::vector<std::string>& physical_colors,
    const std::vector<std::string>& physical_types,
    size_t /*source_physical_idx*/,
    const std::vector<size_t>* physical_config_indices)
{
    // MaterialList 模式：不检查、不增加。用户列表里的耗材已足够拆解用。
    if (result.mode == DecomposeMode::MaterialList)
        return 0;

    std::vector<size_t> fallback_indices;
    const std::vector<size_t>* indices = physical_config_indices;
    if (!indices) {
        fallback_indices.resize(physical_colors.size());
        for (size_t i = 0; i < fallback_indices.size(); ++i)
            fallback_indices[i] = i;
        indices = &fallback_indices;
    }

    // CMYW/RYBW requires Hyper PLA physicals; same color in Generic PLA
    // does NOT count as reusable. 单色 (components.size() == 1) 也会走这里：
    // 例如目标色本身是一个标准 base color (如纯 Cyan/Magenta/Yellow 等) 时
    // try_build_single_base_result 会返回 1 个 component (filament_index == -1)，
    // 如果该 base color 不在用户的 Hyper PLA 物理列表里，依然计入 missing。
    const std::string required_type = kDecomposeHyperPlaType;

    size_t missing_count = 0;
    for (const DecomposeComponent& comp : result.components) {
        if (comp.base_color == DecomposeBaseColor::None)
            continue;
        DecomposeOfficialComponent official_component =
            lookup_decompose_official_component(kDecomposeHyperPlaType, result.mode, comp.base_color, comp.colour);
        int existing_idx = find_existing_decompose_component(official_component, physical_colors,
                                                             physical_types, *indices, 0,
                                                             required_type);
        if (existing_idx <= 0)
            ++missing_count;
    }
    return missing_count;
}

bool confirm_create_decompose_missing_components(wxWindow* parent, const std::vector<DecomposeMissingComponent>& missing)
{
    if (missing.empty())
        return true;

    static const char* config_key = "not_show_color_decompose_missing_component_tip";
    if (wxGetApp().app_config->get(config_key) == "1") {
        return true;
    }

    wxString missing_text;
    for (size_t i = 0; i < missing.size(); ++i) {
        if (i > 0)
            missing_text += _L(", ");
        missing_text += missing[i].display_name;
    }

    wxString message = _L("The current filament list does not contain ") + missing_text +
        _L(". A project filament required by the mixed filament will be created automatically after decomposition.");

    MessageDialog dlg(parent, message, _L("Tip"), wxOK | wxCANCEL | wxICON_INFORMATION);
    dlg.show_dsa_button();
    int res = dlg.ShowModal();
    if (res == wxID_OK && dlg.get_checkbox_state())
        wxGetApp().app_config->set(config_key, "1");
    return res == wxID_OK;
}

}} // namespace Slic3r::GUI
