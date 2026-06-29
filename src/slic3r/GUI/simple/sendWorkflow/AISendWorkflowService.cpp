#include "AISendWorkflowService.hpp"

#include "AIProcessPresetIntentResolver.hpp"
#include "AIProcessSwitchService.hpp"
#include "EasyPrintSender.hpp"
#include "../../GLCanvas3D.hpp"
#include "../../GUI_App.hpp"
#include "../filamentMapping/ImGuiFilamentPanel.hpp"
#include "../filamentMapping/FilamentMappingService.hpp"
#include "../../PartPlate.hpp"
#include "../../Plater.hpp"
#include "../filamentMapping/ResidentFilamentMappingAdapter.hpp"
#include "../filamentMapping/ThumbnailDataRecolor.hpp"
#include "../bridge/SlicerBridge.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/MixedFilament.hpp"
#include "../../print_manage/data/DataCenter.hpp"

#include <boost/beast/core/detail/base64.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <wx/mstream.h>

// Local testing switch:
// Enable this macro when you want AI simple-mode sends to force the same-MAC cloud device.
// AI cloud sends use the cloud closed-loop flow by default now.
// #define C3D_AI_SIMPLE_FORCE_CLOUD_DEVICE_FOR_TEST
// #define C3D_AI_SIMPLE_FORCE_CLOUD_CLOSED_LOOP_FOR_TEST

namespace Slic3r {
namespace GUI {

using json = nlohmann::json;

namespace {

constexpr int kAllSceneMappingItemsScopePlateIndex = -2;

std::string normalize_upload_name(const std::string& file_path)
{
    std::string upload_name = boost::filesystem::path(file_path).filename().string();
    if (!upload_name.empty() && upload_name.front() == '.') {
        upload_name.erase(upload_name.begin());
        if (upload_name.empty())
            upload_name = "AI_Send_" + std::to_string(static_cast<long long>(std::time(nullptr))) + ".gcode";
    }
    if (upload_name.empty())
        upload_name = "AI_Send_" + std::to_string(static_cast<long long>(std::time(nullptr))) + ".gcode";
    return upload_name;
}

void replace_upload_name_illegal_chars(std::string& name)
{
    static const std::string illegal_characters = "\\/:*?\"'<>|";
    for (char& ch : name) {
        if (illegal_characters.find(ch) != std::string::npos)
            ch = '_';
    }
}

std::vector<std::string> collect_professional_mode_filament_types()
{
    std::vector<std::string> filament_types;

    if (wxGetApp().preset_bundle == nullptr)
        return filament_types;

    const std::vector<std::string> filament_presets = wxGetApp().preset_bundle->filament_presets;
    filament_types.reserve(filament_presets.size());

    for (const auto& preset_name : filament_presets) {
        std::string filament_type;
        Slic3r::Preset* preset = wxGetApp().preset_bundle->filaments.find_preset(preset_name);
        if (preset != nullptr) {
            preset->get_filament_type(filament_type);
            filament_types.emplace_back(std::move(filament_type));
        }
    }

    const auto& mixed = wxGetApp().preset_bundle->mixed_filaments.mixed_filaments();
    const size_t num_physical = filament_types.size();
    for (const auto& mf : mixed) {
        if (!mf.enabled || mf.deleted)
            continue;

        std::string filament_type = "PLA";
        if (mf.component_a >= 1 && static_cast<size_t>(mf.component_a) <= num_physical)
            filament_type = filament_types[mf.component_a - 1];
        filament_types.emplace_back(std::move(filament_type));
    }

    return filament_types;
}

std::string build_professional_mode_upload_name(Plater* plater,
                                                PartPlate* plate,
                                                int plate_index,
                                                const std::string& file_path)
{
    if (plater == nullptr)
        return normalize_upload_name(file_path);

    std::string upload_name;
    if (plater->only_gcode_mode()) {
        upload_name = boost::filesystem::path(file_path).stem().string();
        if (!upload_name.empty())
            upload_name += ".gcode";
    } else if (plate != nullptr) {
        std::vector<int> plate_extruders = plate->get_extruders(true);
        ModelObjectPtrs plate_objects = plate->get_objects_on_this_plate();
        std::string obj0_name;
        if (!plate_objects.empty() && plate_objects[0] != nullptr)
            obj0_name = plate_objects[0]->name;
        replace_upload_name_illegal_chars(obj0_name);

        const auto* slice_result = plate->get_slice_result();
        const std::vector<std::string> filament_types = collect_professional_mode_filament_types();
        if (slice_result != nullptr && !plate_extruders.empty()) {
            const int extruder_index = plate_extruders[0] - 1;
            const auto& plate_print_statistics = slice_result->print_statistics;
            const PrintEstimatedStatistics::Mode& plate_time_mode =
                plate_print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)];

            if (extruder_index >= 0 && extruder_index < static_cast<int>(filament_types.size())) {
                upload_name = obj0_name + "_" + filament_types[extruder_index] + "_" +
                              get_bbl_time_dhms(plate_time_mode.model_time_s()) + ".gcode";
            } else {
                upload_name = obj0_name + "_" + get_bbl_time_dhms(plate_time_mode.model_time_s()) + ".gcode";
            }
        } else {
            upload_name = "plate" + std::to_string(plate_index + 1) + ".gcode";
        }
    }

    replace_upload_name_illegal_chars(upload_name);
    if (upload_name.empty())
        upload_name = normalize_upload_name(file_path);
    return upload_name;
}

std::string color_to_hex(const ImVec4& color)
{
    auto to_channel = [](float value) {
        const float clamped = std::max(0.0f, std::min(1.0f, value));
        return static_cast<int>(std::round(clamped * 255.0f));
    };

    std::ostringstream oss;
    oss << '#'
        << std::uppercase << std::hex << std::setfill('0')
        << std::setw(2) << to_channel(color.x)
        << std::setw(2) << to_channel(color.y)
        << std::setw(2) << to_channel(color.z);
    return oss.str();
}

std::string thumbnail_to_data_url(const ThumbnailData& thumbnail)
{
    wxImage image(thumbnail.width, thumbnail.height);
    image.InitAlpha();
    for (unsigned int row = 0; row < thumbnail.height; ++row) {
        const unsigned int flipped_row = (thumbnail.height - 1 - row) * thumbnail.width;
        for (unsigned int col = 0; col < thumbnail.width; ++col) {
            unsigned char* px = (unsigned char*)thumbnail.pixels.data() + 4 * (flipped_row + col);
            image.SetRGB((int)col, (int)row, px[0], px[1], px[2]);
            image.SetAlpha((int)col, (int)row, px[3]);
        }
    }

    wxMemoryOutputStream mem_stream;
    if (!image.SaveFile(mem_stream, wxBITMAP_TYPE_PNG))
        return {};

    const size_t png_size = mem_stream.GetSize();
    std::vector<unsigned char> png_bytes(png_size);
    mem_stream.CopyTo(png_bytes.data(), png_size);

    std::string encoded(boost::beast::detail::base64::encoded_size(png_size), '\0');
    boost::beast::detail::base64::encode(&encoded[0], png_bytes.data(), png_size);
    return "data:image/png;base64," + encoded;
}

bool ensure_current_plate_thumbnail()
{
    auto* plater = wxGetApp().plater();
    if (plater == nullptr)
        return false;

    auto& plate_list = plater->get_partplate_list();
    PartPlate* plate = plate_list.get_curr_plate();
    if (plate == nullptr)
        return false;

    if (plate->thumbnail_data.is_valid())
        return true;

    GLCanvas3D* canvas = plater->get_view3D_canvas3D();
    if (canvas == nullptr || !canvas->make_current_for_postinit())
        return false;

    const int plate_index = plate_list.get_curr_plate_index();
    const ThumbnailsParams thumbnail_params = {{}, false, true, true, true, plate_index};
    canvas->render_thumbnail(
        plate->thumbnail_data,
        (unsigned int)plate->plate_thumbnail_width,
        (unsigned int)plate->plate_thumbnail_height,
        thumbnail_params,
        Camera::EType::Ortho);
    return plate->thumbnail_data.is_valid();
}

bool ensure_current_plate_preview_bases()
{
    auto* plater = wxGetApp().plater();
    if (plater == nullptr)
        return false;

    auto& plate_list = plater->get_partplate_list();
    PartPlate* plate = plate_list.get_curr_plate();
    if (plate == nullptr)
        return false;

    GLCanvas3D* canvas = plater->get_view3D_canvas3D();
    if (canvas == nullptr || !canvas->make_current_for_postinit())
        return false;

    const int plate_index = plate_list.get_curr_plate_index();
    const ThumbnailsParams thumbnail_params = {{}, false, true, true, true, plate_index};

    if (!plate->thumbnail_data.is_valid()) {
        canvas->render_thumbnail(
            plate->thumbnail_data,
            (unsigned int)plate->plate_thumbnail_width,
            (unsigned int)plate->plate_thumbnail_height,
            thumbnail_params,
            Camera::EType::Ortho,
            false,
            false,
            false,
            "iso");
    }

    if (!plate->no_light_thumbnail_data.is_valid()) {
        canvas->render_thumbnail(
            plate->no_light_thumbnail_data,
            (unsigned int)plate->plate_thumbnail_width,
            (unsigned int)plate->plate_thumbnail_height,
            thumbnail_params,
            Camera::EType::Ortho,
            false,
            false,
            true,
            "iso");
    }

    return plate->thumbnail_data.is_valid() && plate->no_light_thumbnail_data.is_valid();
}

RGB8 hex_to_rgb8(const std::string& value)
{
    std::string hex = value;
    if (!hex.empty() && hex.front() == '#')
        hex.erase(hex.begin());
    if (hex.size() != 6)
        return {};
    auto parse_channel = [&](size_t offset) -> std::uint8_t {
        unsigned int channel = 0;
        std::stringstream ss;
        ss << std::hex << hex.substr(offset, 2);
        ss >> channel;
        return static_cast<std::uint8_t>(channel);
    };
    return RGB8{
        parse_channel(0),
        parse_channel(2),
        parse_channel(4)
    };
}
std::string resolve_effective_mapping_item_color(const json& item)
{
    if (!item.is_object())
        return std::string();
    if (item.value("mapped", false)) {
        const std::string match_color = item.value("matchColor", std::string());
        if (!match_color.empty())
            return match_color;
    }
    return item.value("sourceColor", std::string());
}
std::vector<std::string> build_effective_physical_colors(
    const json& source_mapping_items,
    const json& mapping_items,
    size_t num_physical)
{
    std::vector<std::string> physical_colors(num_physical);
    if (source_mapping_items.is_array()) {
        for (const auto& item : source_mapping_items) {
            if (!item.is_object())
                continue;
            const int item_index = item.value("item_index", -1);
            if (item_index < 0 || static_cast<size_t>(item_index) >= num_physical)
                continue;
            physical_colors[static_cast<size_t>(item_index)] = item.value("sourceColor", std::string());
        }
    }
    if (mapping_items.is_array()) {
        for (const auto& item : mapping_items) {
            if (!item.is_object())
                continue;
            const int item_index = item.value("item_index", -1);
            if (item_index < 0 || static_cast<size_t>(item_index) >= num_physical)
                continue;
            physical_colors[static_cast<size_t>(item_index)] = resolve_effective_mapping_item_color(item);
        }
    }
    return physical_colors;
}
MixedFilamentManager build_effective_mixed_filament_manager(
    const json& source_mapping_items,
    const json& mapping_items)
{
    auto* preset_bundle = wxGetApp().preset_bundle;
    if (preset_bundle == nullptr)
        return MixedFilamentManager();
    MixedFilamentManager mixed_manager = preset_bundle->mixed_filaments;
    const size_t num_physical = preset_bundle->filament_presets.size();
    if (num_physical == 0)
        return mixed_manager;
    mixed_manager.refresh_display_colors(
        build_effective_physical_colors(source_mapping_items, mapping_items, num_physical));
    return mixed_manager;
}
std::vector<int> build_equal_component_percents(size_t count)
{
    std::vector<int> percents(count, 0);
    if (count == 0)
        return percents;
    const int base = 100 / int(count);
    const int remainder = 100 % int(count);
    for (size_t i = 0; i < count; ++i)
        percents[i] = base + (int(i) < remainder ? 1 : 0);
    return percents;
}
std::vector<int> normalize_component_weights_to_percents(
    const std::vector<int>& weights,
    size_t expected_count)
{
    if (expected_count == 0)
        return {};
    std::vector<int> normalized = weights;
    if (normalized.size() != expected_count)
        normalized.resize(expected_count, 0);
    int total = 0;
    for (int& weight : normalized) {
        weight = std::max(0, weight);
        total += weight;
    }
    if (total <= 0)
        return build_equal_component_percents(expected_count);
    std::vector<int> percents(expected_count, 0);
    std::vector<double> remainders(expected_count, 0.0);
    int assigned = 0;
    for (size_t i = 0; i < expected_count; ++i) {
        const double exact = 100.0 * double(normalized[i]) / double(total);
        percents[i] = int(std::floor(exact));
        remainders[i] = exact - double(percents[i]);
        assigned += percents[i];
    }
    int missing = std::max(0, 100 - assigned);
    while (missing > 0) {
        size_t best_index = 0;
        double best_remainder = -1.0;
        for (size_t i = 0; i < remainders.size(); ++i) {
            if (remainders[i] > best_remainder) {
                best_remainder = remainders[i];
                best_index = i;
            }
        }
        ++percents[best_index];
        remainders[best_index] = 0.0;
        --missing;
    }
    return percents;
}
std::vector<unsigned int> decode_gradient_component_ids_for_display(const std::string& value)
{
    std::vector<unsigned int> component_ids;
    bool seen[10] = {false};
    for (const char token : value) {
        if (token < '1' || token > '9')
            continue;
        const unsigned int component_id = static_cast<unsigned int>(token - '0');
        if (seen[component_id])
            continue;
        seen[component_id] = true;
        component_ids.push_back(component_id);
    }
    return component_ids;
}
std::vector<int> decode_gradient_component_weights_for_display(
    const std::string& value,
    size_t expected_count)
{
    std::vector<int> weights;
    std::string token;
    for (const char ch : value) {
        if (ch >= '0' && ch <= '9') {
            token.push_back(ch);
            continue;
        }
        if (token.empty())
            continue;
        try {
            weights.push_back(std::stoi(token));
        } catch (...) {
        }
        token.clear();
    }
    if (!token.empty()) {
        try {
            weights.push_back(std::stoi(token));
        } catch (...) {
        }
    }
    return normalize_component_weights_to_percents(weights, expected_count);
}
std::vector<int> build_mixed_component_percents(
    const MixedFilament& mixed,
    const std::vector<unsigned int>& component_ids)
{
    if (component_ids.empty())
        return {};
    const std::vector<unsigned int> gradient_ids =
        decode_gradient_component_ids_for_display(mixed.gradient_component_ids);
    if (gradient_ids.size() >= 3) {
        const std::vector<int> gradient_percents =
            decode_gradient_component_weights_for_display(
                mixed.gradient_component_weights,
                gradient_ids.size());
        std::unordered_map<unsigned int, int> percent_by_id;
        for (size_t i = 0; i < gradient_ids.size(); ++i)
            percent_by_id[gradient_ids[i]] = gradient_percents[i];
        std::vector<int> ordered_percents;
        ordered_percents.reserve(component_ids.size());
        for (const unsigned int component_id : component_ids) {
            const auto it = percent_by_id.find(component_id);
            ordered_percents.push_back(it != percent_by_id.end() ? it->second : 0);
        }
        return normalize_component_weights_to_percents(ordered_percents, component_ids.size());
    }
    const std::string normalized_pattern =
        MixedFilamentManager::normalize_manual_pattern(mixed.manual_pattern);
    if (!normalized_pattern.empty()) {
        std::unordered_map<unsigned int, int> counts_by_id;
        for (const char token : normalized_pattern) {
            unsigned int component_id = 0;
            if (token == '1' || token == 'A' || token == 'a')
                component_id = mixed.component_a;
            else if (token == '2' || token == 'B' || token == 'b')
                component_id = mixed.component_b;
            else if (token >= '3' && token <= '9')
                component_id = static_cast<unsigned int>(token - '0');
            if (component_id == 0)
                continue;
            ++counts_by_id[component_id];
        }
        std::vector<int> ordered_counts;
        ordered_counts.reserve(component_ids.size());
        for (const unsigned int component_id : component_ids) {
            const auto it = counts_by_id.find(component_id);
            ordered_counts.push_back(it != counts_by_id.end() ? it->second : 0);
        }
        return normalize_component_weights_to_percents(ordered_counts, component_ids.size());
    }
    if (component_ids.size() == 1)
        return {100};
    std::vector<int> pair_weights;
    pair_weights.reserve(component_ids.size());
    for (const unsigned int component_id : component_ids) {
        if (component_id == mixed.component_a)
            pair_weights.push_back(std::max(0, 100 - std::clamp(mixed.mix_b_percent, 0, 100)));
        else if (component_id == mixed.component_b)
            pair_weights.push_back(std::clamp(mixed.mix_b_percent, 0, 100));
        else
            pair_weights.push_back(0);
    }
    return normalize_component_weights_to_percents(pair_weights, component_ids.size());
}
std::vector<RGB8> build_preview_match_colors(
    const json& mapping_items,
    const MixedFilamentManager* mixed_manager)
{
    int max_item_index = -1;
    if (mapping_items.is_array()) {
        for (const auto& item : mapping_items) {
            if (!item.is_object())
                continue;
            max_item_index = std::max(max_item_index, item.value("item_index", -1));
        }
    }
    auto* plater = wxGetApp().plater();
    auto* preset_bundle = wxGetApp().preset_bundle;
    PartPlate* plate = plater != nullptr ? plater->get_partplate_list().get_curr_plate() : nullptr;
    const size_t num_physical = preset_bundle != nullptr ? preset_bundle->filament_presets.size() : 0;
    if (mixed_manager != nullptr && plate != nullptr && num_physical > 0) {
        const std::vector<int> used_extruders = plate->get_model_volume_extruders();
        for (const int extruder_id : used_extruders) {
            if (extruder_id > 0)
                max_item_index = std::max(max_item_index, extruder_id - 1);
        }
    }
    std::vector<RGB8> colors;
    if (max_item_index < 0)
        return colors;
    colors.resize(static_cast<size_t>(max_item_index + 1));
    for (const auto& item : mapping_items) {
        if (!item.is_object())
            continue;
        const int item_index = item.value("item_index", -1);
        if (item_index < 0 || item_index >= static_cast<int>(colors.size()))
            continue;
        colors[static_cast<size_t>(item_index)] = hex_to_rgb8(resolve_effective_mapping_item_color(item));
    }
    if (mixed_manager != nullptr && plate != nullptr && num_physical > 0) {
        const std::vector<int> used_extruders = plate->get_model_volume_extruders();
        for (const int extruder_id : used_extruders) {
            if (extruder_id <= 0 || static_cast<size_t>(extruder_id) <= num_physical)
                continue;
            const MixedFilament* mixed =
                mixed_manager->mixed_filament_from_id(static_cast<unsigned int>(extruder_id), num_physical);
            if (mixed == nullptr)
                continue;
            const int item_index = extruder_id - 1;
            if (item_index < 0 || item_index >= static_cast<int>(colors.size()))
                continue;
            colors[static_cast<size_t>(item_index)] = hex_to_rgb8(mixed->display_color);
        }
    }
    return colors;
}
std::string resolve_current_plate_preview_image(
    const json& mapping_items,
    const MixedFilamentManager* mixed_manager)
{
    auto* plater = wxGetApp().plater();
    if (plater == nullptr)
        return {};
    PartPlate* plate = plater->get_partplate_list().get_curr_plate();
    if (plate == nullptr)
        return {};
    if (!ensure_current_plate_preview_bases())
        return {};
    ThumbnailData recolored_thumbnail;
    const auto match_colors = build_preview_match_colors(mapping_items, mixed_manager);
    if (!match_colors.empty() &&
        recolor_thumbnail_with_no_light(
            recolored_thumbnail,
            plate->thumbnail_data,
            plate->no_light_thumbnail_data,
            match_colors,
            ThumbnailRecolorParams{})) {
        return thumbnail_to_data_url(recolored_thumbnail);
    }
    return thumbnail_to_data_url(plate->thumbnail_data);
}

std::pair<std::string, std::string> resolve_current_plate_display_info()
{
    auto* plater = wxGetApp().plater();
    if (plater == nullptr)
        return {};

    auto& plate_list = plater->get_partplate_list();
    PartPlate* plate = plate_list.get_curr_plate();
    auto* current_result = plate_list.get_current_slice_result();
    const auto& current_print_statistics = plate_list.get_current_fff_print().print_statistics();

    std::string print_time;
    if (plate != nullptr && plate->get_slice_result() != nullptr) {
        print_time = wxString::Format(
            "%s",
            short_time(get_time_dhms(plate->get_slice_result()->print_statistics.modes[0].model_time_s()))).ToStdString();
    } else if (current_result != nullptr) {
        print_time = wxString::Format(
            "%s",
            short_time(get_time_dhms(current_result->print_statistics.modes[0].model_time_s()))).ToStdString();
    }

    double total_weight = current_print_statistics.total_weight;
    if (total_weight <= 0.0 && current_result != nullptr) {
        total_weight = 0.0;
        for (const auto& role_entry : current_result->print_statistics.used_filaments_per_role)
            total_weight += role_entry.second.second;
    }

    std::string total_weight_text;
    if (total_weight > 0.0) {
        char weight_buffer[64];
        const bool use_inches = wxGetApp().app_config != nullptr && wxGetApp().app_config->get("use_inches") == "1";
        if (use_inches)
            std::snprintf(weight_buffer, sizeof(weight_buffer), "%.2f oz", total_weight * 0.035274);
        else
            std::snprintf(weight_buffer, sizeof(weight_buffer), "%.2f g", total_weight);
        total_weight_text = weight_buffer;
    }

    return {print_time, total_weight_text};
}

int resolve_active_plate_index(int preferred_plate_index)
{
    if (preferred_plate_index >= 0)
        return preferred_plate_index;

    auto* plater = wxGetApp().plater();
    if (plater == nullptr)
        return -1;

    return plater->get_partplate_list().get_curr_plate_index();
}

json build_mixed_filament_display_items(
    const json& source_mapping_items,
    const json& effective_mapping_items,
    const MixedFilamentManager& mixed_manager,
    int preferred_plate_index)
{
    json mixed_items = json::array();
    auto* plater = wxGetApp().plater();
    auto* preset_bundle = wxGetApp().preset_bundle;
    if (plater == nullptr || preset_bundle == nullptr)
        return mixed_items;
    PartPlateList& plate_list = plater->get_partplate_list();
    const int plate_index = resolve_active_plate_index(preferred_plate_index);
    PartPlate* plate = plate_index >= 0 ? plate_list.get_plate(plate_index) : nullptr;
    if (plate == nullptr)
        return mixed_items;
    const size_t num_physical = preset_bundle->filament_presets.size();
    if (num_physical == 0)
        return mixed_items;
    std::unordered_map<int, json> source_by_index;
    if (source_mapping_items.is_array()) {
        for (const auto& source_item : source_mapping_items) {
            if (!source_item.is_object())
                continue;
            const int item_index = source_item.value("item_index", -1);
            if (item_index < 0)
                continue;
            source_by_index[item_index] = source_item;
        }
    }
    std::unordered_map<int, json> effective_by_index;
    if (effective_mapping_items.is_array()) {
        for (const auto& effective_item : effective_mapping_items) {
            if (!effective_item.is_object())
                continue;
            const int item_index = effective_item.value("item_index", -1);
            if (item_index < 0)
                continue;
            effective_by_index[item_index] = effective_item;
        }
    }
    std::vector<int> used_extruders = plate->get_model_volume_extruders();
    for (const int extruder_id : used_extruders) {
        if (extruder_id <= 0 || static_cast<size_t>(extruder_id) <= num_physical)
            continue;
        const MixedFilament* mixed = mixed_manager.mixed_filament_from_id(static_cast<unsigned int>(extruder_id), num_physical);
        if (mixed == nullptr)
            continue;
        const std::vector<unsigned int> component_ids = FilamentMappingService::resolve_physical_source_filament_ids(static_cast<unsigned int>(extruder_id), num_physical);
        if (component_ids.empty())
            continue;
        json component_items = json::array();
        json component_labels = json::array();
        json component_colors = json::array();
        const std::vector<int> component_percents =
            build_mixed_component_percents(*mixed, component_ids);
        for (unsigned int component_id : component_ids) {
            json component_item = json::object();
            const int component_index = int(component_id) - 1;
            const auto effective_it = effective_by_index.find(component_index);
            if (effective_it != effective_by_index.end())
                component_item = effective_it->second;
            else {
                const auto source_it = source_by_index.find(component_index);
                if (source_it != source_by_index.end())
                    component_item = source_it->second;
            }
            component_item["item_index"] = int(component_id) - 1;
            component_item["extruderId"] = int(component_id);
            component_item["sourceKind"] = "mixed_component";
            component_item["readonly"] = true;
            component_items.push_back(component_item);
            const std::string component_label = component_item.value("presetDisplay", std::string());
            component_labels.push_back(component_label.empty() ? std::to_string(component_id) : component_label);
            component_colors.push_back(resolve_effective_mapping_item_color(component_item));
        }
        std::string component_summary;
        for (const auto& label_value : component_labels) {
            const std::string label = label_value.get<std::string>();
            if (label.empty())
                continue;
            if (!component_summary.empty())
                component_summary += " + ";
            component_summary += label;
        }
        if (component_summary.empty())
            component_summary = std::string("Virtual ") + std::to_string(extruder_id);
        json mixed_item;
        mixed_item["item_index"] = extruder_id - 1;
        mixed_item["extruderId"] = extruder_id;
        mixed_item["mapped"] = true;
        mixed_item["readonly"] = true;
        mixed_item["mixedSummary"] = true;
        mixed_item["sourceKind"] = "mixed";
        mixed_item["is_virtual_mixed"] = true;
        mixed_item["virtual_filament_id"] = extruder_id;
        mixed_item["sourceColor"] = mixed->display_color;
        mixed_item["presetDisplay"] = std::string("Virtual ") + std::to_string(extruder_id);
        mixed_item["sourceFilamentPreset"] = std::string();
        mixed_item["extruderFilamentType"] = std::string("Mixed");
        mixed_item["mixed_component_ids"] = component_ids;
        mixed_item["mixed_component_labels"] = component_labels;
        mixed_item["mixed_component_colors"] = component_colors;
        mixed_item["mixed_component_percents"] = component_percents;
        mixed_item["mixed_component_items"] = component_items;
        mixed_item["mixed_component_summary"] = component_summary;
        mixed_item["slotLabel"] = std::string();
        mixed_item["matchColor"] = mixed->display_color;
        mixed_item["boxType"] = -1;
        mixed_item["boxId"] = -1;
        mixed_item["materialId"] = -1;
        mixed_item["materialType"] = std::string();
        mixed_item["materialName"] = std::string();
        mixed_item["selection_token"] = std::string();
        mixed_item["sharedSlot"] = false;
        mixed_item["autoReused"] = false;
        mixed_item["mappingWarning"] = std::string();
        mixed_item["source_snapshot"] = true;
        mixed_items.push_back(std::move(mixed_item));
    }
    return mixed_items;
}

void filter_draft_selection_tokens_to_plate(
    std::unordered_map<int, std::string>& draft_selection_tokens,
    int item_count,
    int preferred_plate_index)
{
    if (draft_selection_tokens.empty())
        return;

    const auto plate_item_indices = FilamentMappingService::collect_plate_item_indices(item_count, preferred_plate_index);
    if (plate_item_indices.empty()) {
        draft_selection_tokens.clear();
        return;
    }

    for (auto it = draft_selection_tokens.begin(); it != draft_selection_tokens.end();) {
        if (plate_item_indices.count(it->first) == 0)
            it = draft_selection_tokens.erase(it);
        else
            ++it;
    }
}


json filter_mapping_items_to_plate(const json& mapping_items, int preferred_plate_index)
{
    if (!mapping_items.is_array())
        return json::array();

    int item_count_hint = 0;
    for (const auto& item : mapping_items) {
        if (!item.is_object())
            continue;

        const int item_index = item.value("item_index", -1);
        if (item_index < 0)
            continue;

        item_count_hint = std::max(item_count_hint, item_index + 1);
    }

    const auto plate_item_indices = FilamentMappingService::collect_plate_item_indices(item_count_hint, preferred_plate_index);
    if (plate_item_indices.empty())
        return mapping_items;

    json filtered = json::array();
    for (const auto& item : mapping_items) {
        if (!item.is_object())
            continue;

        const int item_index = item.value("item_index", -1);
        if (plate_item_indices.count(item_index) == 0)
            continue;

        filtered.push_back(item);
    }
    return filtered;
}

constexpr std::size_t kDefaultVisibleMappingCount = 5;

bool parse_selection_token(const std::string& selection_token, int& box_type, int& box_id, int& material_id)
{
    box_type = -1;
    box_id = -1;
    material_id = -1;

    std::stringstream ss(selection_token);
    std::string segment;
    std::vector<int> parts;
    while (std::getline(ss, segment, ':')) {
        if (segment.empty())
            return false;
        try {
            parts.push_back(std::stoi(segment));
        } catch (...) {
            return false;
        }
    }

    if (parts.size() != 3)
        return false;

    box_type = parts[0];
    box_id = parts[1];
    material_id = parts[2];
    return true;
}

std::string extract_mapping_selection_token(const json& item)
{
    if (!item.is_object())
        return {};

    std::string selection_token = item.value("selection_token", std::string());
    if (selection_token.empty())
        selection_token = item.value("selectionToken", std::string());
    return selection_token;
}

const ResidentFilamentMappingAdapter::PopupOptionSeed* find_popup_option_seed(
    const ResidentFilamentMappingAdapter::PopupOptionCatalog& popup_catalog,
    const std::string& selection_token)
{
    if (selection_token.empty())
        return nullptr;

    for (const auto& group : popup_catalog.groups) {
        for (const auto& option : group.options) {
            if (option.selection_token == selection_token)
                return &option;
        }
    }
    return nullptr;
}

void prune_draft_selection_tokens(
    std::unordered_map<int, std::string>& draft_selection_tokens,
    const json& base_mapping_items,
    const ResidentFilamentMappingAdapter::PopupOptionCatalog& popup_catalog)
{
    if (draft_selection_tokens.empty())
        return;

    std::unordered_map<int, std::string> base_selection_tokens;
    if (base_mapping_items.is_array()) {
        for (const auto& item : base_mapping_items) {
            if (!item.is_object())
                continue;
            const int item_index = item.value("item_index", -1);
            if (item_index < 0)
                continue;
            base_selection_tokens[item_index] = extract_mapping_selection_token(item);
        }
    }

    for (auto it = draft_selection_tokens.begin(); it != draft_selection_tokens.end();) {
        const auto base_it = base_selection_tokens.find(it->first);
        const bool valid_token = find_popup_option_seed(popup_catalog, it->second) != nullptr;
        const bool same_as_base = base_it != base_selection_tokens.end() && base_it->second == it->second;
        if (base_it == base_selection_tokens.end() || !valid_token || same_as_base)
            it = draft_selection_tokens.erase(it);
        else
            ++it;
    }
}

json apply_draft_selection_tokens_to_mapping_items(
    const json& base_mapping_items,
    const std::unordered_map<int, std::string>& draft_selection_tokens,
    const ResidentFilamentMappingAdapter::PopupOptionCatalog& popup_catalog)
{
    if (!base_mapping_items.is_array() || draft_selection_tokens.empty())
        return base_mapping_items;

    json effective_items = base_mapping_items;
    for (auto& item : effective_items) {
        if (!item.is_object())
            continue;

        const int item_index = item.value("item_index", -1);
        const auto draft_it = draft_selection_tokens.find(item_index);
        if (draft_it == draft_selection_tokens.end())
            continue;

        const auto* option = find_popup_option_seed(popup_catalog, draft_it->second);
        if (option == nullptr)
            continue;

        int box_type = -1;
        int box_id = -1;
        int material_id = -1;
        parse_selection_token(option->selection_token, box_type, box_id, material_id);

        item["mapped"] = true;
        item["slotLabel"] = option->slot_label;
        item["matchColor"] = color_to_hex(option->material_color);
        item["materialType"] = option->material_match_key;
        item["materialName"] = option->material_label;
        item["selection_token"] = option->selection_token;
        item["boxType"] = box_type;
        item["boxId"] = box_id;
        item["materialId"] = material_id;
    }

    return effective_items;
}

std::string resolve_entry_mode(const json& open_args)
{
    const std::string entry_mode = open_args.value("entry_mode", std::string());
    if (entry_mode == "mapping_only")
        return entry_mode;
    if (entry_mode == "process_intent")
        return entry_mode;
    return "send_workflow";
}

bool is_mapping_only_entry_mode(const json& open_args)
{
    return resolve_entry_mode(open_args) == "mapping_only";
}

bool is_process_intent_entry_mode(const json& open_args)
{
    return resolve_entry_mode(open_args) == "process_intent";
}

std::string build_mapping_only_status_text(const json& state, bool mapping_required, bool mapping_complete)
{
    if (!state.value("has_model", false))
        return _u8L("No model is loaded for consumable mapping.");
    if (!mapping_required)
        return _u8L("No consumable mapping is required for the current scene.");
    if (mapping_complete)
        return _u8L("Consumable mapping is ready. You can review the current plate preview result.");
    return _u8L("Please confirm the consumable mapping for the current scene.");
}

std::string build_process_intent_status_text(const json& state)
{
    if (!state.value("has_model", false))
        return _u8L("No model is loaded. Please place a model before slicing.");

    const json current_plate = state.value("current_plate", json::object());
    if (!current_plate.value("has_printable_instances", false))
        return _u8L("The current plate has no printable instances.");

    return _u8L("Choose a print preference before slicing.");
}

bool is_mapping_data_loading(const json& state, const json& all_mapping_items)
{
    return state.value("has_model", false) &&
           all_mapping_items.is_array() &&
           all_mapping_items.empty();
}

bool mapping_items_have_mapped_items(const json& mapping_items)
{
    if (!mapping_items.is_array())
        return false;

    for (const auto& item : mapping_items) {
        if (item.is_object() && item.value("mapped", false))
            return true;
    }
    return false;
}

bool diagnostics_have_ignored_color_mismatch(const json& diagnostics)
{
    const json items = diagnostics.value("items", json::array());
    if (!items.is_array())
        return false;

    for (const auto& item : items) {
        if (item.is_object() && item.value("color_mismatch_ignored", false))
            return true;
    }

    return false;
}

std::string build_mapping_loading_text()
{
    return _u8L("Synchronizing consumable mapping for the current scene...");
}

bool should_enable_cloud_closed_loop()
{
#ifdef C3D_AI_SIMPLE_FORCE_CLOUD_CLOSED_LOOP_FOR_TEST
    return true;
#else
    if (const char* value = std::getenv("C3D_AI_SIMPLE_CLOUD_CLOSED_LOOP")) {
        const std::string flag = value;
        if (flag == "1" || flag == "true" || flag == "TRUE" || flag == "on" || flag == "ON")
            return true;
        if (flag == "0" || flag == "false" || flag == "FALSE" || flag == "off" || flag == "OFF")
            return false;
    }
    return true;
#endif
}

bool is_cloud_device_type(int device_type)
{
    return device_type == 1;
}

bool should_force_cloud_device_for_test()
{
#ifdef C3D_AI_SIMPLE_FORCE_CLOUD_DEVICE_FOR_TEST
    return true;
#else
    return false;
#endif
}

std::string describe_device(const DM::Device& device)
{
    std::ostringstream oss;
    oss << "{valid=" << (device.valid ? "true" : "false")
        << ", name=" << device.name
        << ", address=" << device.address
        << ", mac=" << device.mac
        << ", tbId=" << device.tbId
        << ", online=" << (device.online ? "true" : "false")
        << ", deviceType=" << device.deviceType
        << "}";
    return oss.str();
}

std::string normalize_cloud_cid(std::string cid)
{
    cid.erase(std::remove_if(cid.begin(), cid.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }), cid.end());

    std::transform(cid.begin(), cid.end(), cid.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });

    return cid;
}

std::string build_cloud_cid_from_slot_label(const std::string& slot_label)
{
    const std::string normalized = normalize_cloud_cid(slot_label);
    if (normalized.size() < 3)
        return {};

    size_t digit_begin = 0;
    if (normalized.front() == 'T')
        digit_begin = 1;

    size_t digit_end = digit_begin;
    while (digit_end < normalized.size() && std::isdigit(static_cast<unsigned char>(normalized[digit_end])) != 0)
        ++digit_end;

    if (digit_end == digit_begin || digit_end != normalized.size() - 1)
        return {};

    if (std::isalpha(static_cast<unsigned char>(normalized.back())) == 0)
        return {};

    return normalized.front() == 'T' ? normalized : ("T" + normalized);
}
std::string resolve_box_color_cid(const DM::Device& device, int box_id, int material_id)
{
    for (const auto& box_color_info : device.boxColorInfos) {
        if (box_color_info.boxId == box_id &&
            box_color_info.materialId == material_id &&
            !box_color_info.cId.empty()) {
            return normalize_cloud_cid(box_color_info.cId);
        }
    }
    return {};
}

void backfill_cloud_device_color_match_info(json& color_match_info, const DM::Device& target_device)
{
    if (!color_match_info.is_array())
        return;

    for (auto& item : color_match_info) {
        if (!item.is_object())
            continue;

        std::string resolved_cid = normalize_cloud_cid(item.value("cId", item.value("c_id", std::string())));
        if (resolved_cid.empty() && target_device.valid) {
            const int box_id = item.value("boxId", item.value("box_id", -1));
            const int material_id = item.value("materialId", item.value("material_id", -1));
            if (box_id >= 0 && material_id >= 0)
                resolved_cid = resolve_box_color_cid(target_device, box_id, material_id);
        }

        if (resolved_cid.empty()) {
            const std::string slot_label = item.value("slotLabel", item.value("slot_label", std::string()));
            resolved_cid = build_cloud_cid_from_slot_label(slot_label);
        }

        if (!resolved_cid.empty()) {
            item["cId"] = resolved_cid;
            item["c_id"] = resolved_cid;
        }
    }
}
DM::Device resolve_ai_send_target_device(bool* used_forced_cloud = nullptr, std::string* reason = nullptr)
{
    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    if (used_forced_cloud)
        *used_forced_cloud = false;

    if (!should_force_cloud_device_for_test()) {
        if (reason)
            *reason = "force_cloud_macro_disabled";
        return current_device;
    }

    if (!current_device.valid) {
        if (reason)
            *reason = "current_device_invalid";
        return current_device;
    }

    if (current_device.mac.empty()) {
        if (reason)
            *reason = "current_device_mac_empty";
        return current_device;
    }

    if (is_cloud_device_type(current_device.deviceType)) {
        if (reason)
            *reason = "current_device_already_cloud";
        return current_device;
    }

    try {
        const json& data = DM::DataCenter::Ins().get_data();
        if (data.contains("data") && data["data"].contains("printerList")) {
            for (const auto& group : data["data"]["printerList"]) {
                if (!group.contains("list") || !group["list"].is_array())
                    continue;
                for (const auto& printer : group["list"]) {
                    if (!printer.is_object())
                        continue;
                    if (printer.value("mac", std::string()) != current_device.mac)
                        continue;
                    if (printer.value("deviceType", -1) != 1)
                        continue;
                    if (!printer.value("online", false))
                        continue;

                    json printer_copy = printer;
                    DM::Device cloud_device = DM::Device::deserialize(printer_copy);
                    if (cloud_device.valid) {
                        if (used_forced_cloud)
                            *used_forced_cloud = true;
                        if (reason)
                            *reason = "forced_same_mac_cloud_device";
                        return cloud_device;
                    }
                }
            }
        }
    } catch (...) {
        if (reason)
            *reason = "force_cloud_lookup_exception";
        return current_device;
    }

    if (reason)
        *reason = "same_mac_cloud_device_not_found";
    return current_device;
}

std::string safe_json_dump(const json& value)
{
    try {
        return value.dump();
    } catch (...) {
        return "<json_dump_failed>";
    }
}

void append_ai_send_debug_log(const std::string& tag, const std::string& content)
{
    std::ofstream log_file("D:/log.txt", std::ios::app);
    if (!log_file.is_open())
        return;

    std::time_t now = std::time(nullptr);
    char        buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    log_file << "[" << buf << "][" << tag << "] " << content << "\n";
}

void log_ai_send_stage(const std::string& stage,
                       const std::string& card_id,
                       const std::string& request_id,
                       const std::string& message)
{
    const std::string full_message =
        "[AISendWorkflowService] stage=" + stage +
        " card_id=" + card_id +
        " request_id=" + request_id +
        " " + message;

    BOOST_LOG_TRIVIAL(info)
        << full_message;
    append_ai_send_debug_log("AISendWorkflowService", full_message);
}

} // namespace

AISendWorkflowService::AISendWorkflowService() = default;
AISendWorkflowService::~AISendWorkflowService() = default;

void AISendWorkflowService::SetSnapshotCallback(EventCallback callback) { m_snapshot_callback = std::move(callback); }
void AISendWorkflowService::SetProgressCallback(EventCallback callback) { m_progress_callback = std::move(callback); }
void AISendWorkflowService::SetResultCallback(EventCallback callback) { m_result_callback = std::move(callback); }
void AISendWorkflowService::SetErrorCallback(EventCallback callback) { m_error_callback = std::move(callback); }

AISendWorkflowService::OpenResult AISendWorkflowService::OpenCard(const std::string& request_id, const json& args)
{
    json snapshot;
    OpenResult result;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        result = open_card_locked(request_id, args);
        if (!result.success)
            return result;

        auto it = m_sessions.find(result.card_id);
        if (it != m_sessions.end())
            snapshot = build_snapshot_envelope_locked(it->second);
    }

    log_ai_send_stage(
        "open_card",
        result.card_id,
        request_id,
        "entry_mode=" + args.value("entry_mode", std::string("send_workflow")));

    if (snapshot.is_object())
        emit_snapshot(snapshot);
    if (snapshot.is_object() && is_mapping_loading_snapshot(snapshot))
        schedule_pending_mapping_refresh(result.card_id);
    return result;
}

bool AISendWorkflowService::StartSendOnly(const std::string& card_id)
{
    return start_send_internal(card_id, false);
}

bool AISendWorkflowService::StartSendAndPrint(const std::string& card_id)
{
    return start_send_internal(card_id, true);
}

bool AISendWorkflowService::SelectPlate(const std::string& card_id, int plate_index)
{
    json snapshot_envelope;
    json error_envelope;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return false;

        Session& session = it->second;
        if (session.terminal)
            return true;

        if (session.in_progress) {
            error_envelope = build_error_envelope_locked(
                session,
                "SEND_ALREADY_RUNNING",
                "Plate switching is disabled while AI send workflow is running.");
        } else {
            auto* plater = wxGetApp().plater();
            if (plater == nullptr) {
                error_envelope = build_error_envelope_locked(
                    session,
                    "PLATER_NOT_AVAILABLE",
                    "Plater is not available for plate switching.");
            } else if (plater->select_plate(plate_index, false) != 0) {
                error_envelope = build_error_envelope_locked(
                    session,
                    "SELECT_PLATE_FAILED",
                    "Failed to switch to the requested plate.",
                    {{"plate_index", plate_index}});
            } else {
                session.selected_plate_index = plate_index;

                if (is_mapping_only_entry_mode(session.open_args) &&
                    !ensure_mapping_items_locked(session, true, false)) {
                    error_envelope = build_error_envelope_locked(
                        session,
                        "MAPPING_DATA_NOT_AVAILABLE",
                        "Consumable mapping data is not available for the selected plate.");
                } else {
                    std::string code;
                    std::string message;
                    if (!refresh_state_locked(session, code, message))
                        error_envelope = build_error_envelope_locked(session, code, message);
                    else
                        snapshot_envelope = session.last_snapshot;
                }
            }
        }

        if (error_envelope.is_object()) {
            session.waiting_user_action = true;
            error_envelope["finish_tool_call"] = false;
            error_envelope["data"]["finish_tool_call"] = false;
            update_last_snapshot_status_locked(session);
        }
    }

    if (error_envelope.is_object()) {
        emit_error(error_envelope);
        return true;
    }

    if (snapshot_envelope.is_object())
        emit_snapshot(snapshot_envelope);
    if (snapshot_envelope.is_object() && is_mapping_loading_snapshot(snapshot_envelope))
        schedule_pending_mapping_refresh(card_id);
    return true;
}
bool AISendWorkflowService::AutoMatch(const std::string& card_id)
{
    json snapshot_envelope;
    json error_envelope;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return false;

        Session& session = it->second;
        if (session.terminal)
            return true;

        if (session.in_progress) {
            error_envelope = build_error_envelope_locked(
                session,
                "SEND_ALREADY_RUNNING",
                "Auto match is disabled while AI send workflow is running.");
        } else {
            const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
            if (!current_device.valid) {
                error_envelope = build_error_envelope_locked(
                    session,
                    "DEVICE_NOT_AVAILABLE",
                    "Current printer device is not available.");
            } else if (!ensure_mapping_items_locked(session, true, false)) {
                error_envelope = build_error_envelope_locked(
                    session,
                    "MAPPING_DATA_NOT_AVAILABLE",
                    "Consumable mapping data is not available for the current scene.");
            } else {
                session.draft_selection_tokens.clear();
                session.mapping_applied_to_scene = false;
                sync_mapping_dirty_locked(session);

                std::string code;
                std::string message;
                if (!refresh_state_locked(session, code, message))
                    error_envelope = build_error_envelope_locked(session, code, message);
                else
                    snapshot_envelope = session.last_snapshot;
            }
        }

        if (error_envelope.is_object()) {
            session.waiting_user_action = true;
            error_envelope["finish_tool_call"] = false;
            error_envelope["data"]["finish_tool_call"] = false;
            update_last_snapshot_status_locked(session);
        }
    }

    if (error_envelope.is_object()) {
        emit_error(error_envelope);
        return true;
    }

    if (snapshot_envelope.is_object())
        emit_snapshot(snapshot_envelope);
    return true;
}
bool AISendWorkflowService::UpdateMapping(const std::string& card_id, const json& payload)
{
    json snapshot_envelope;
    json error_envelope;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return false;

        Session& session = it->second;
        if (session.terminal)
            return true;

        if (session.in_progress) {
            error_envelope = build_error_envelope_locked(
                session,
                "SEND_ALREADY_RUNNING",
                "Consumable mapping cannot be changed while AI send workflow is running.");
        } else {
            const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
            if (!current_device.valid) {
                error_envelope = build_error_envelope_locked(
                    session,
                    "DEVICE_NOT_AVAILABLE",
                    "Current printer device is not available.");
            } else if (!ensure_mapping_items_locked(session, false)) {
                error_envelope = build_error_envelope_locked(
                    session,
                    "MAPPING_DATA_NOT_AVAILABLE",
                    "Consumable mapping data is not available for the current scene.");
            } else {
                const json request_payload =
                    payload.contains("mapping") && payload["mapping"].is_object() ? payload["mapping"] : payload;

                int item_index = request_payload.value("item_index", -1);
                if (item_index < 0 && request_payload.contains("extruder_id"))
                    item_index = request_payload.value("extruder_id", 0) - 1;
                if (item_index < 0 && request_payload.contains("extruderId"))
                    item_index = request_payload.value("extruderId", 0) - 1;

                std::string selection_token = request_payload.value("selection_token", std::string());
                if (selection_token.empty())
                    selection_token = request_payload.value("selectionToken", std::string());

                const FilamentMappingService::Mode mode = FilamentMappingService::Mode::All;
                if (selection_token.empty()) {
                    std::string slot_label = request_payload.value("slot_label", std::string());
                    if (slot_label.empty())
                        slot_label = request_payload.value("slotLabel", std::string());
                    selection_token = FilamentMappingService::selection_token_for_slot_label(slot_label, current_device, mode);
                }

                if (item_index < 0) {
                    error_envelope = build_error_envelope_locked(
                        session,
                        "INVALID_MAPPING_ITEM",
                        "No valid mapping item was specified.",
                        {{"payload", request_payload}});
                } else if (!FilamentMappingService::is_valid_selection_token(selection_token, current_device, mode)) {
                    error_envelope = build_error_envelope_locked(
                        session,
                        "INVALID_MAPPING_SELECTION",
                        "No valid consumable selection was specified.",
                        {{"payload", request_payload}});
                } else if (!FilamentMappingService::apply_selection(
                               session.mapping_items,
                               item_index,
                               selection_token,
                               current_device,
                               mode)) {
                    error_envelope = build_error_envelope_locked(
                        session,
                        "APPLY_MAPPING_SELECTION_FAILED",
                        "Failed to apply the requested consumable mapping selection.",
                        {{"item_index", item_index}, {"selection_token", selection_token}});
                } else {
                    session.draft_selection_tokens.clear();
                    session.mapping_applied_to_scene = false;
                    sync_mapping_dirty_locked(session);

                    std::string code;
                    std::string message;
                    if (!refresh_state_locked(session, code, message))
                        error_envelope = build_error_envelope_locked(session, code, message);
                    else
                        snapshot_envelope = session.last_snapshot;
                }
            }
        }

        if (error_envelope.is_object()) {
            session.waiting_user_action = true;
            error_envelope["finish_tool_call"] = false;
            error_envelope["data"]["finish_tool_call"] = false;
            update_last_snapshot_status_locked(session);
        }
    }

    if (error_envelope.is_object()) {
        emit_error(error_envelope);
        return true;
    }

    if (snapshot_envelope.is_object())
        emit_snapshot(snapshot_envelope);
    return true;
}
bool AISendWorkflowService::ApplyMapping(const std::string& card_id)
{
    json snapshot_envelope;
    json error_envelope;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return false;

        Session& session = it->second;
        if (session.terminal)
            return true;

        if (session.in_progress) {
            error_envelope = build_error_envelope_locked(
                session,
                "SEND_ALREADY_RUNNING",
                "Consumable mapping cannot be applied while AI send workflow is running.");
        } else {
            const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
            if (!current_device.valid) {
                error_envelope = build_error_envelope_locked(
                    session,
                    "DEVICE_NOT_AVAILABLE",
                    "Current printer device is not available.");
            } else if (!ensure_mapping_items_locked(session, false)) {
                error_envelope = build_error_envelope_locked(
                    session,
                    "MAPPING_DATA_NOT_AVAILABLE",
                    "Consumable mapping data is not available for the current scene.");
            } else {
                const int plate_index = resolve_selected_plate_index_locked(session);
                const json current_plate_mapping_items = filter_mapping_items_to_plate(session.mapping_items, plate_index);
                if (FilamentMappingService::has_mixed_mapping_channels(current_plate_mapping_items)) {
                    error_envelope = build_error_envelope_locked(
                        session,
                        "MIXED_MAPPING_CHANNELS_NOT_SUPPORTED",
                        "CFS and external spool mappings cannot be mixed on the same plate.");
                } else {
                    const FilamentMappingService::Mode mode = FilamentMappingService::Mode::All;
                    if (!FilamentMappingService::apply_mapping_to_scene(current_plate_mapping_items, current_device, mode, plate_index)) {
                        error_envelope = build_error_envelope_locked(
                            session,
                            "APPLY_MAPPING_FAILED",
                            "Failed to apply the requested consumable mapping to the scene.");
                    } else {
                        session.draft_selection_tokens.clear();
                        m_last_applied_mapping_items = session.mapping_items;
                        m_last_applied_plate_index = plate_index;
                        sync_mapping_dirty_locked(session);

                        std::string code;
                        std::string message;
                        if (!refresh_state_locked(session, code, message))
                            error_envelope = build_error_envelope_locked(session, code, message);
                        else
                            snapshot_envelope = session.last_snapshot;
                    }
                }
            }
        }

        if (error_envelope.is_object()) {
            session.waiting_user_action = true;
            error_envelope["finish_tool_call"] = false;
            error_envelope["data"]["finish_tool_call"] = false;
            update_last_snapshot_status_locked(session);
        }
    }

    if (error_envelope.is_object()) {
        emit_error(error_envelope);
        return true;
    }

    if (snapshot_envelope.is_object())
        emit_snapshot(snapshot_envelope);
    return true;
}
bool AISendWorkflowService::ApplyProcessIntent(const std::string& card_id, const std::string& intent_key)
{
    json snapshot_envelope;
    json progress_envelope;
    json result_envelope;
    json error_envelope;
    AIProcessIntent intent = AIProcessIntent::Direct;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return false;

        Session& session = it->second;
        if (session.terminal)
            return true;

        if (session.in_progress) {
            error_envelope = {
                {"version", "1.0"},
                {"request_id", session.request_id},
                {"card_id", session.card_id},
                {"timestamp_ms", current_timestamp_ms()},
                {"code", "PROCESS_SWITCH_BUSY"},
                {"message", "Process preset cannot be changed while send workflow is running."},
                {"details", {{"intent", intent_key}}},
                {"data", {
                    {"code", "PROCESS_SWITCH_BUSY"},
                    {"message", "Process preset cannot be changed while send workflow is running."},
                    {"details", {{"intent", intent_key}}}
                }}
            };
        } else if (!parse_process_intent(intent_key, intent)) {
            error_envelope = {
                {"version", "1.0"},
                {"request_id", session.request_id},
                {"card_id", session.card_id},
                {"timestamp_ms", current_timestamp_ms()},
                {"code", "PROCESS_INTENT_INVALID"},
                {"message", "Unsupported process intent."},
                {"details", {{"intent", intent_key}}},
                {"data", {
                    {"code", "PROCESS_INTENT_INVALID"},
                    {"message", "Unsupported process intent."},
                    {"details", {{"intent", intent_key}}}
                }}
            };
        } else {
            session.selected_process_intent = to_string(intent);
            session.process_switch_in_progress = true;
            session.process_status = "running";
            session.process_status_text = "Applying process preset...";
            session.process_reslice_expected = false;
            session.last_snapshot = build_snapshot_envelope_locked(session);
            snapshot_envelope = session.last_snapshot;
            progress_envelope = {
                {"version", "1.0"},
                {"request_id", session.request_id},
                {"card_id", session.card_id},
                {"timestamp_ms", current_timestamp_ms()},
                {"progress", 5},
                {"message", "Applying process preset"},
                {"stage", "process_switch"},
                {"state", "running"},
                {"data", {
                    {"progress", 5},
                    {"message", "Applying process preset"},
                    {"stage", "process_switch"},
                    {"state", "running"},
                    {"intent", session.selected_process_intent}
                }}
            };
        }
    }

    if (error_envelope.is_object()) {
        emit_error(error_envelope);
        return true;
    }
    if (snapshot_envelope.is_object())
        emit_snapshot(snapshot_envelope);
    if (progress_envelope.is_object())
        emit_progress(progress_envelope);

    AIProcessSwitchService switch_service;
    const AIProcessApplyResult apply_result = switch_service.ApplyIntent(intent);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return false;

        Session& session = it->second;
        session.selected_process_intent = to_string(intent);
        session.process_switch_in_progress = false;
        session.process_reslice_expected = apply_result.reslice_expected;
        session.resolved_process_preset_name = apply_result.resolution.resolved_preset_name;
        session.process_summary_text = !apply_result.resolution.summary_text.empty()
            ? apply_result.resolution.summary_text
            : apply_result.message;
        session.process_status = apply_result.success ? (apply_result.changed ? "applied" : "ready") : "failed";
        session.process_status_text = apply_result.message;

        if (!apply_result.success) {
            session.last_snapshot = build_snapshot_envelope_locked(session);
            snapshot_envelope = session.last_snapshot;
            error_envelope = {
                {"version", "1.0"},
                {"request_id", session.request_id},
                {"card_id", session.card_id},
                {"timestamp_ms", current_timestamp_ms()},
                {"code", apply_result.code.empty() ? "PROCESS_SWITCH_FAILED" : apply_result.code},
                {"message", apply_result.message},
                {"details", {
                    {"intent", session.selected_process_intent},
                    {"resolution", {
                        {"resolved_preset_name", apply_result.resolution.resolved_preset_name},
                        {"strategy", apply_result.resolution.strategy},
                        {"fallback_reason", apply_result.resolution.fallback_reason}
                    }}
                }},
                {"data", {
                    {"code", apply_result.code.empty() ? "PROCESS_SWITCH_FAILED" : apply_result.code},
                    {"message", apply_result.message},
                    {"details", {
                        {"intent", session.selected_process_intent},
                        {"resolution", {
                            {"resolved_preset_name", apply_result.resolution.resolved_preset_name},
                            {"strategy", apply_result.resolution.strategy},
                            {"fallback_reason", apply_result.resolution.fallback_reason}
                        }}
                    }}
                }}
            };
        } else {
            std::string code;
            std::string message;
            if (!refresh_state_locked(session, code, message)) {
                session.process_status = "failed";
                session.process_status_text = message;
                session.last_snapshot = build_snapshot_envelope_locked(session);
                snapshot_envelope = session.last_snapshot;
                error_envelope = {
                    {"version", "1.0"},
                    {"request_id", session.request_id},
                    {"card_id", session.card_id},
                    {"timestamp_ms", current_timestamp_ms()},
                    {"code", code},
                    {"message", message},
                    {"details", {{"intent", session.selected_process_intent}}},
                    {"data", {
                        {"code", code},
                        {"message", message},
                        {"details", {{"intent", session.selected_process_intent}}}
                    }}
                };
            } else {
                snapshot_envelope = session.last_snapshot;
                result_envelope = {
                    {"version", "1.0"},
                    {"request_id", session.request_id},
                    {"card_id", session.card_id},
                    {"timestamp_ms", current_timestamp_ms()},
                    {"result_type", apply_result.changed ? "process_applied" : "process_kept"},
                    {"message", apply_result.message},
                    {"details", {
                        {"intent", session.selected_process_intent},
                        {"changed", apply_result.changed},
                        {"reslice_expected", apply_result.reslice_expected},
                        {"current_print_preset", session.current_print_preset_name},
                        {"resolved_preset_name", apply_result.resolution.resolved_preset_name},
                        {"strategy", apply_result.resolution.strategy},
                        {"fallback_reason", apply_result.resolution.fallback_reason},
                        {"bridge_result", apply_result.bridge_result}
                    }},
                    {"data", {
                        {"result_type", apply_result.changed ? "process_applied" : "process_kept"},
                        {"message", apply_result.message},
                        {"details", {
                            {"intent", session.selected_process_intent},
                            {"changed", apply_result.changed},
                            {"reslice_expected", apply_result.reslice_expected},
                            {"current_print_preset", session.current_print_preset_name},
                            {"resolved_preset_name", apply_result.resolution.resolved_preset_name},
                            {"strategy", apply_result.resolution.strategy},
                            {"fallback_reason", apply_result.resolution.fallback_reason},
                            {"bridge_result", apply_result.bridge_result}
                        }}
                    }}
                };
            }
        }
    }

    if (snapshot_envelope.is_object())
        emit_snapshot(snapshot_envelope);
    if (error_envelope.is_object()) {
        emit_error(error_envelope);
        return true;
    }
    if (result_envelope.is_object())
        emit_result(result_envelope);
    return true;
}

bool AISendWorkflowService::Cancel(const std::string& card_id)
{
    json result_envelope;
    std::shared_ptr<EasyPrintSender> sender;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return false;

        Session& session = it->second;
        if (session.terminal)
            return true;
        sender = session.sender;
        session.sender.reset();
        session.in_progress = false;
        session.waiting_user_action = false;
        result_envelope = build_result_envelope_locked(
            session,
            "canceled",
            "Send workflow canceled by user");
    }

    if (sender)
        sender->cancelUpload();
    emit_result(result_envelope);
    return true;
}

std::string AISendWorkflowService::FindCardIdByRequestId(const std::string& request_id)
{
    if (request_id.empty())
        return {};

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [card_id, session] : m_sessions) {
        if (session.request_id == request_id && !card_id.empty())
            return card_id;
    }
    return {};
}

std::vector<json> AISendWorkflowService::GetActiveSnapshots() const
{
    std::vector<json> snapshots;
    std::lock_guard<std::mutex> lock(m_mutex);
    snapshots.reserve(m_sessions.size());
    for (const auto& kv : m_sessions) {
        if (kv.second.last_snapshot.is_object())
            snapshots.push_back(kv.second.last_snapshot);
    }
    return snapshots;
}

void AISendWorkflowService::RefreshActiveSnapshots()
{
    std::vector<json> snapshots_to_emit;
    std::vector<std::string> cards_waiting_mapping;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        snapshots_to_emit.reserve(m_sessions.size());
        cards_waiting_mapping.reserve(m_sessions.size());

        for (auto& kv : m_sessions) {
            Session& session = kv.second;
            if (session.card_id.empty() || session.terminal)
                continue;

            std::string code;
            std::string message;
            if (!refresh_state_locked(session, code, message))
                continue;

            if (session.last_snapshot.is_object()) {
                snapshots_to_emit.push_back(session.last_snapshot);
                if (is_mapping_loading_snapshot(session.last_snapshot))
                    cards_waiting_mapping.push_back(session.card_id);
            }
        }
    }

    for (const auto& snapshot : snapshots_to_emit)
        emit_snapshot(snapshot);

    for (const auto& card_id : cards_waiting_mapping)
        schedule_pending_mapping_refresh(card_id);
}

void AISendWorkflowService::RefreshMappingForCurrentDevice(bool auto_match)
{
    std::vector<json> snapshots_to_emit;
    std::vector<std::string> cards_waiting_mapping;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        snapshots_to_emit.reserve(m_sessions.size());
        cards_waiting_mapping.reserve(m_sessions.size());

        for (auto& kv : m_sessions) {
            Session& session = kv.second;
            if (session.card_id.empty() || session.terminal || session.in_progress)
                continue;

            std::string code;
            std::string message;
            if (!refresh_state_locked(session, code, message))
                continue;

            if (!ensure_mapping_items_locked(session, auto_match))
                continue;

            if (auto_match)
                session.draft_selection_tokens.clear();
            session.mapping_applied_to_scene = false;
            sync_mapping_dirty_locked(session);

            session.revision += 1;
            session.last_snapshot = build_snapshot_envelope_locked(session);
            if (session.last_snapshot.is_object()) {
                snapshots_to_emit.push_back(session.last_snapshot);
                if (is_mapping_loading_snapshot(session.last_snapshot))
                    cards_waiting_mapping.push_back(session.card_id);
            }
        }
    }

    for (const auto& snapshot : snapshots_to_emit)
        emit_snapshot(snapshot);

    for (const auto& card_id : cards_waiting_mapping)
        schedule_pending_mapping_refresh(card_id);
}
AISendWorkflowService::OpenResult AISendWorkflowService::open_card_locked(const std::string& request_id, const json& args)
{
    const bool auto_match_on_open = args.value("auto_match_on_open", true);

    if (!request_id.empty()) {
        const std::string entry_mode = resolve_entry_mode(args);
        for (auto& [existing_card_id, existing_session] : m_sessions) {
            if (existing_session.request_id != request_id || existing_session.terminal)
                continue;
            if (resolve_entry_mode(existing_session.open_args) != entry_mode)
                continue;

            existing_session.open_args = args;
            invalidate_current_plate_mapping_preview_bases_locked(existing_session);

            if (auto_match_on_open && !existing_session.in_progress) {
                if (is_mapping_only_entry_mode(args)) {
                    rebuild_mapping_items_for_open_locked(existing_session, true);
                    sync_mapping_dirty_locked(existing_session);
                } else {
                    if (!existing_session.mapping_items.is_array() || existing_session.mapping_items.empty()) {
                        ensure_mapping_items_locked(existing_session, true);
                        existing_session.draft_selection_tokens.clear();
                    }
                    sync_mapping_dirty_locked(existing_session);
                }
            }

            std::string code;
            std::string message;
            if (!refresh_state_locked(existing_session, code, message))
                return {false, code, message, existing_card_id};

            return {true, {}, {}, existing_card_id};
        }
    }
    Session session;
    session.card_id = "ai-send-" + std::to_string(m_next_card_id++);
    session.request_id = request_id;
    session.open_args = args;
    session.waiting_user_action = true;
    session.in_progress = false;
    session.terminal = false;

    if (auto_match_on_open) {
        if (is_mapping_only_entry_mode(args)) {
            rebuild_mapping_items_for_open_locked(session, true);
            sync_mapping_dirty_locked(session);
        } else {
            bool inherited_applied_mapping = false;
            bool restored_current_scene_mapping = false;
            const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
            const FilamentMappingService::Mode mode = FilamentMappingService::Mode::All;
            const int desired_scope_plate_index = desired_mapping_scope_plate_index_locked(session);
            if (current_device.valid) {
                for (const auto& [existing_card_id, existing_session] : m_sessions) {
                    if (existing_session.terminal || existing_session.in_progress)
                        continue;
                    if (!existing_session.mapping_applied_to_scene || !existing_session.mapping_items.is_array() || existing_session.mapping_items.empty())
                        continue;
                    if (!FilamentMappingService::scene_matches_mapping(existing_session.mapping_items, current_device, mode))
                        continue;

                    session.mapping_items = desired_scope_plate_index == kAllSceneMappingItemsScopePlateIndex
                        ? existing_session.mapping_items
                        : filter_mapping_items_to_plate(existing_session.mapping_items, desired_scope_plate_index);
                    session.mapping_items_scope_plate_index = desired_scope_plate_index;
                    session.mapping_applied_to_scene = true;
                    session.mapping_dirty = false;
                    session.draft_selection_tokens.clear();
                    inherited_applied_mapping = true;
                    break;
                }
            }

            if (!inherited_applied_mapping && !m_sessions.empty()) {
                const DM::Device& current_device_for_scene = DM::DataCenter::Ins().get_current_device_data();
                auto* plater = wxGetApp().plater();
                ensure_original_source_snapshot_locked(session);
                const json all_source_mapping_items =
                    plater != nullptr ? plater->get_scene_filament_source_snapshot() : json::array();
                const json source_mapping_items = desired_scope_plate_index == kAllSceneMappingItemsScopePlateIndex
                    ? all_source_mapping_items
                    : filter_source_mapping_items_to_plate_locked(all_source_mapping_items, desired_scope_plate_index);
                json scene_mapping_items = FilamentMappingService::match_current_scene(source_mapping_items, current_device_for_scene, mode);
                const int scene_match_plate_index =
                    desired_scope_plate_index == kAllSceneMappingItemsScopePlateIndex ? -1 : desired_scope_plate_index;
                if (mapping_items_have_mapped_items(scene_mapping_items) &&
                    FilamentMappingService::scene_matches_mapping(scene_mapping_items, current_device_for_scene, mode, scene_match_plate_index)) {
                    session.mapping_items = std::move(scene_mapping_items);
                    session.mapping_items_scope_plate_index = desired_scope_plate_index;
                    session.mapping_applied_to_scene = true;
                    session.mapping_dirty = false;
                    session.draft_selection_tokens.clear();
                    restored_current_scene_mapping = true;
                }
            }

            if (!inherited_applied_mapping && !restored_current_scene_mapping) {
                ensure_mapping_items_locked(session, true);
                sync_mapping_dirty_locked(session);
            }
        }
    }

    invalidate_current_plate_mapping_preview_bases_locked(session);

    std::string code;
    std::string message;
    if (!refresh_state_locked(session, code, message)) {
        return {false, code, message, {}};
    }

    const std::string card_id = session.card_id;
    m_sessions[card_id] = std::move(session);
    return {true, {}, {}, card_id};
}

bool AISendWorkflowService::start_send_internal(const std::string& card_id, bool start_print)
{
    std::shared_ptr<EasyPrintSender> sender;
    std::string file_path;
    std::string request_id;
    json print_data;
    json progress_envelope;
    json error_envelope;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return false;

        Session& session = it->second;
        request_id = session.request_id;
        if (session.terminal)
            return true;
        if (session.in_progress) {
            error_envelope = build_error_envelope_locked(
                session,
                "SEND_ALREADY_RUNNING",
                "An AI send workflow is already running for this card.");
        } else {

            std::string code;
            std::string message;
            if (!refresh_state_locked(session, code, message)) {
                error_envelope = build_error_envelope_locked(session, code, message);
            } else {
                const auto resolved = resolve_gcode_file(session);
                file_path = resolved.first;
                if (file_path.empty()) {
                    error_envelope = build_error_envelope_locked(
                        session,
                        "GCODE_FILE_NOT_FOUND",
                        "Current plate does not have a sendable G-code file.");
                } else if (!can_send_locked(session, message)) {
                    error_envelope = build_error_envelope_locked(session, "SEND_NOT_AVAILABLE", message);
                } else {
                    session.sender = std::make_shared<EasyPrintSender>();
                    session.in_progress = true;
                    session.waiting_user_action = false;
                    session.terminal = false;
                    session.last_start_print = start_print;
                    bool forced_cloud_device = false;
                    std::string target_device_reason;
                    const DM::Device target_device = resolve_ai_send_target_device(&forced_cloud_device, &target_device_reason);
                    session.cloud_workflow_active = start_print && is_cloud_device_type(target_device.deviceType);
                    sender = session.sender;

                    print_data = build_print_data(session, resolved.second);
                    sender->setCloudPrintCallbacks({
                        [this, card_id](int progress, const std::string& stage, const std::string& message) {
                            on_cloud_print_progress(card_id, progress, stage, message);
                        },
                        [this, card_id](const json& result) {
                            on_cloud_print_success(card_id, result);
                        },
                        [this, card_id](const std::string& code, const std::string& message) {
                            on_cloud_print_error(card_id, code, message);
                        }
                    });
                    sender->setCloudClosedLoopEnabled(session.cloud_workflow_active && should_enable_cloud_closed_loop());

                    log_ai_send_stage(
                        "start_send_internal",
                        card_id,
                        request_id,
                        "start_print=" + std::string(start_print ? "true" : "false") +
                        ", force_cloud_macro=" + std::string(should_force_cloud_device_for_test() ? "true" : "false") +
                        ", forced_cloud_device=" + std::string(forced_cloud_device ? "true" : "false") +
                        ", target_device_reason=" + target_device_reason +
                        ", cloud_workflow_active=" + std::string(session.cloud_workflow_active ? "true" : "false") +
                        ", cloud_closed_loop_enabled=" + std::string((session.cloud_workflow_active && should_enable_cloud_closed_loop()) ? "true" : "false") +
                        ", target_device=" + describe_device(target_device) +
                        ", file_path=" + file_path +
                        ", print_data=" + safe_json_dump(print_data));

                    progress_envelope = build_progress_envelope_locked(
                        session,
                        5,
                        _u8L("Starting upload"),
                        "uploading",
                        "running");
                }
            }
        }
    }

    if (error_envelope.is_object()) {
        log_ai_send_stage(
            "start_send_internal_error",
            card_id,
            request_id,
            "error=" + safe_json_dump(error_envelope));
        emit_error(error_envelope);
        return true;
    }

    emit_progress(progress_envelope);
    sender->sendGcode(
        file_path,
        print_data,
        [this, card_id](std::string, float progress, double speed) {
            on_upload_progress(card_id, progress, speed);
        },
        [this, card_id](std::string, int status_code) {
            on_upload_status(card_id, status_code);
        },
        [this, card_id, start_print](std::string, std::string body) {
            on_upload_complete(card_id, start_print, body);
        },
        start_print);
    return true;
}
bool AISendWorkflowService::refresh_state_locked(Session& session, std::string& code, std::string& message)
{
    json bridge_result = Bridge::SlicerBridge::Instance().Execute(Bridge::ActionID::GET_SLICER_STATE, json::object());
    if (!bridge_result.value("success", false)) {
        code = "PROJECT_CONTEXT_FAILED";
        message = bridge_result.value("message", std::string("Failed to collect slicer state for AI send workflow."));
        return false;
    }

    session.last_state = bridge_result.value("state", json::object());
    if (session.selected_plate_index < 0)
        session.selected_plate_index = session.last_state.value("current_plate_index", -1);
    if (!refresh_process_context_locked(session, code, message))
        return false;

    ensure_mapping_items_locked(session, false);
    sync_mapping_dirty_locked(session);
    session.revision += 1;
    session.last_snapshot = build_snapshot_envelope_locked(session);
    return true;
}
bool AISendWorkflowService::refresh_process_context_locked(Session& session, std::string& code, std::string& message)
{
    const json state = session.last_state;
    if (!state.is_object()) {
        code = "PROCESS_CONTEXT_INVALID";
        message = "Failed to refresh process context.";
        return false;
    }

    session.current_print_preset_name = state.value("current_print_preset", std::string());
    if (session.resolved_process_preset_name.empty())
        session.resolved_process_preset_name = session.current_print_preset_name;

    if (session.selected_process_intent == "direct" || session.process_summary_text.empty()) {
        if (!session.current_print_preset_name.empty())
            session.process_summary_text = "Current process preset: " + session.current_print_preset_name;
        else
            session.process_summary_text = "Keep the current process preset.";
    }

    if (session.process_status.empty())
        session.process_status = "idle";
    if (!session.process_switch_in_progress && session.process_status_text.empty())
        session.process_status_text = "Process preset is ready.";

    return true;
}
bool AISendWorkflowService::can_send_locked(const Session& session, std::string& message) const
{
    const json state = session.last_state;
    const json device = state.value("current_device", json::object());
    if (!state.value("has_model", false)) {
        message = "No model is loaded in the current project.";
        return false;
    }
    if (!state.value("current_plate_can_print", false)) {
        message = "Current plate is not ready to send.";
        return false;
    }
    if (!device.value("has_bound_device", false)) {
        message = "No bound printer device found.";
        return false;
    }
    if (!device.value("online", false)) {
        message = "Current printer is offline.";
        return false;
    }
    if (!device.value("is_idle", false)) {
        message = "Current printer is busy and cannot accept a new job.";
        return false;
    }
    if (session.mapping_dirty) {
        message = "Please apply the pending consumable mapping changes first.";
        return false;
    }
    return true;
}
ImGuiFilamentPanel* AISendWorkflowService::get_filament_panel() const
{
    auto* plater = wxGetApp().plater();
    if (plater == nullptr)
        return nullptr;

    GLCanvas3D* canvas = plater->get_current_canvas3D(false);
    if (canvas == nullptr)
        canvas = plater->get_view3D_canvas3D();
    if (canvas == nullptr)
        return nullptr;

    return canvas->get_filament_panel();
}

int AISendWorkflowService::resolve_selected_plate_index_locked(const Session& session) const
{
    if (session.selected_plate_index >= 0)
        return session.selected_plate_index;

    if (session.last_state.is_object()) {
        const int state_plate_index = session.last_state.value("current_plate_index", -1);
        if (state_plate_index >= 0)
            return state_plate_index;
    }

    auto* plater = wxGetApp().plater();
    return plater != nullptr ? plater->get_partplate_list().get_curr_plate_index() : -1;
}

std::pair<std::string, std::string> AISendWorkflowService::resolve_gcode_file(const Session& session) const
{
    auto* plater = wxGetApp().plater();
    if (!plater)
        return {};

    std::string file_path;
    PartPlate* plate = nullptr;
    int plate_index = -1;
    if (plater->only_gcode_mode()) {
        file_path = plater->get_last_loaded_gcode().ToStdString();
    } else {
        PartPlateList& plate_list = plater->get_partplate_list();
        plate_index = resolve_selected_plate_index_locked(session);
        plate = plate_index >= 0 ? plate_list.get_plate(plate_index) : plate_list.get_curr_plate();
        if (plate != nullptr)
            plate_index = plate->get_index();
        if (plate)
            file_path = plate->get_tmp_gcode_path();
    }

    if (file_path.empty())
        return {};
    return {file_path, build_professional_mode_upload_name(plater, plate, plate_index, file_path)};
}

json AISendWorkflowService::build_print_data(const Session& session, const std::string& upload_name) const
{
    bool forced_cloud_device = false;
    std::string target_device_reason;
    const DM::Device current_device = resolve_ai_send_target_device(&forced_cloud_device, &target_device_reason);
    const std::string upload_device_key = !current_device.address.empty()
        ? current_device.address
        : (!current_device.mac.empty() ? current_device.mac : current_device.name);
    json data = {
        {"allPlate", false},
        {"upload_gcode_name", upload_name},
        {"color_match_info", json::array()},
        {"open_cfs", 0},
        {"print_calibration", 1},
        {"printer_name", current_device.name},
        {"device_address", current_device.address.empty() ? current_device.name : current_device.address},
        {"device_mac", current_device.mac},
        {"tb_id", current_device.tbId},
        {"device_type", current_device.deviceType},
        {"is_multi_color_device", current_device.isMultiColorDevice},
        {"upload_device_key", upload_device_key},
        {"force_cloud_for_test", forced_cloud_device},
        {"target_device_reason", target_device_reason}
    };

    try {
        const json effective_mapping_items = build_effective_mapping_items_for_send_locked(session);
        json color_match_info = FilamentMappingService::build_color_match_info(effective_mapping_items, current_device);
        backfill_cloud_device_color_match_info(color_match_info, current_device);
        data["color_match_info"] = std::move(color_match_info);
        data["open_cfs"] = FilamentMappingService::has_cfs_mapping(effective_mapping_items, current_device) ? 1 : 0;
    } catch (...) {
    }
    return data;
}

json AISendWorkflowService::build_send_info_locked(const Session& session) const
{
    std::string block_reason;
    bool can_confirm_send = can_send_locked(session, block_reason);

    if (session.in_progress)
        can_confirm_send = false;

    const int current_plate_index = resolve_selected_plate_index_locked(session);
    const auto display_info = resolve_current_plate_display_info();

    std::string plate_label = "Plate " + std::to_string(current_plate_index + 1);
    std::string print_time = display_info.first.empty() ? "--" : display_info.first;
    std::string filament_weight = display_info.second.empty() ? "--" : display_info.second;

    return {
        {"plate_index", current_plate_index},
        {"plate_label", plate_label},
        {"print_time", print_time},
        {"filament_weight", filament_weight},
        {"can_confirm_send", can_confirm_send},
        {"block_reason", can_confirm_send ? "" : block_reason}
    };
}
json AISendWorkflowService::build_mapping_items(const Session& session) const
{
    return session.mapping_items.is_array() ? session.mapping_items : json::array();
}

json AISendWorkflowService::build_effective_mapping_items_for_send_locked(const Session& session) const
{
    const int plate_index = resolve_selected_plate_index_locked(session);
    json mapping_items = filter_mapping_items_to_plate(build_mapping_items(session), plate_index);
    mapping_items = merge_original_source_into_effective_mapping_items(session, mapping_items);
    return mapping_items;
}
json AISendWorkflowService::build_mapping_option_groups() const
{
    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    if (!current_device.valid)
        return json::array();

    const FilamentMappingService::Mode mode = FilamentMappingService::Mode::All;
    return FilamentMappingService::build_mapping_option_groups(current_device, mode);
}

int AISendWorkflowService::desired_mapping_scope_plate_index_locked(const Session& session) const
{
    return is_mapping_only_entry_mode(session.open_args)
        ? resolve_selected_plate_index_locked(session)
        : kAllSceneMappingItemsScopePlateIndex;
}

json AISendWorkflowService::filter_source_mapping_items_to_plate_locked(
    const json& source_mapping_items,
    int preferred_plate_index) const
{
    return FilamentMappingService::filter_source_items_to_plate(source_mapping_items, preferred_plate_index);
}

bool AISendWorkflowService::rebuild_mapping_items_for_open_locked(Session& session, bool auto_match)
{
    ensure_original_source_snapshot_locked(session);

    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    if (!current_device.valid)
        return false;

    auto* plater = wxGetApp().plater();
    const int desired_scope_plate_index = desired_mapping_scope_plate_index_locked(session);
    const json all_source_mapping_items =
        plater != nullptr ? plater->get_scene_filament_source_snapshot() : json::array();
    const json source_mapping_items = desired_scope_plate_index == kAllSceneMappingItemsScopePlateIndex
        ? all_source_mapping_items
        : filter_source_mapping_items_to_plate_locked(all_source_mapping_items, desired_scope_plate_index);
    if (!source_mapping_items.is_array() || source_mapping_items.empty()) {
        session.mapping_items = json::array();
        session.mapping_items_scope_plate_index = desired_scope_plate_index;
        session.mapping_applied_to_scene = false;
        session.mapping_dirty = false;
        session.draft_selection_tokens.clear();
        return true;
    }

    const FilamentMappingService::Mode mode = FilamentMappingService::Mode::All;
    json scene_mapping_items = FilamentMappingService::match_current_scene(source_mapping_items, current_device, mode);
    const int scene_match_plate_index =
        desired_scope_plate_index == kAllSceneMappingItemsScopePlateIndex ? -1 : desired_scope_plate_index;
    if (mapping_items_have_mapped_items(scene_mapping_items) &&
        FilamentMappingService::scene_matches_mapping(scene_mapping_items, current_device, mode, scene_match_plate_index)) {
        session.mapping_items = std::move(scene_mapping_items);
        session.mapping_items_scope_plate_index = desired_scope_plate_index;
        session.mapping_applied_to_scene = true;
        session.mapping_dirty = false;
        session.draft_selection_tokens.clear();
        return true;
    }

    if (!auto_match) {
        session.mapping_items = FilamentMappingService::build_mapping_items(source_mapping_items, current_device, mode);
        session.mapping_items_scope_plate_index = desired_scope_plate_index;
        session.mapping_applied_to_scene = false;
        session.mapping_dirty = false;
        session.draft_selection_tokens.clear();
        return true;
    }

    const bool rebuilt = ensure_mapping_items_locked(session, true, false);
    if (rebuilt) {
        session.mapping_applied_to_scene = false;
        session.mapping_dirty = false;
        session.draft_selection_tokens.clear();
    }
    return rebuilt;
}

bool AISendWorkflowService::restore_last_applied_mapping_locked(Session& session)
{
    if (!m_last_applied_mapping_items.is_array() || m_last_applied_mapping_items.empty())
        return false;

    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    if (!current_device.valid)
        return false;

    int plate_index = resolve_selected_plate_index_locked(session);
    if (plate_index < 0) {
        if (auto* plater = wxGetApp().plater())
            plate_index = plater->get_partplate_list().get_curr_plate_index();
    }

    if (m_last_applied_plate_index >= 0 && plate_index >= 0 && m_last_applied_plate_index != plate_index)
        return false;

    const int desired_scope_plate_index = desired_mapping_scope_plate_index_locked(session);
    const json plate_mapping_items = filter_mapping_items_to_plate(m_last_applied_mapping_items, plate_index);
    if (!plate_mapping_items.is_array() || plate_mapping_items.empty() ||
        !mapping_items_have_mapped_items(plate_mapping_items)) {
        return false;
    }

    const FilamentMappingService::Mode mode = FilamentMappingService::Mode::All;
    const json diagnostics = FilamentMappingService::build_scene_match_diagnostics(
        plate_mapping_items,
        current_device,
        mode,
        plate_index);
    if (!diagnostics.value("matches", false) || diagnostics_have_ignored_color_mismatch(diagnostics))
        return false;

    session.mapping_items = desired_scope_plate_index == kAllSceneMappingItemsScopePlateIndex
        ? m_last_applied_mapping_items
        : plate_mapping_items;
    session.mapping_items_scope_plate_index = desired_scope_plate_index;
    session.selected_plate_index = plate_index;
    session.draft_selection_tokens.clear();
    log_ai_send_stage(
        "restore_last_applied_mapping",
        session.card_id,
        session.request_id,
        "plate_index=" + std::to_string(plate_index) +
        ", mapping_channel=" + FilamentMappingService::mapping_channel_to_string(
            FilamentMappingService::resolve_mapping_channel(plate_mapping_items)));
    return true;
}

bool AISendWorkflowService::ensure_mapping_items_locked(Session& session, bool auto_match, bool allow_restore_last_applied)
{
    ensure_original_source_snapshot_locked(session);

    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    if (!current_device.valid)
        return false;

    const int desired_scope_plate_index = desired_mapping_scope_plate_index_locked(session);
    const bool scope_changed = session.mapping_items_scope_plate_index != desired_scope_plate_index;

    if (allow_restore_last_applied && auto_match &&
        (!session.mapping_items.is_array() || session.mapping_items.empty() || scope_changed) &&
        restore_last_applied_mapping_locked(session)) {
        return true;
    }

    auto* plater = wxGetApp().plater();
    const json all_source_mapping_items =
        plater != nullptr ? plater->get_scene_filament_source_snapshot() : json::array();
    const json source_mapping_items = desired_scope_plate_index == kAllSceneMappingItemsScopePlateIndex
        ? all_source_mapping_items
        : filter_source_mapping_items_to_plate_locked(all_source_mapping_items, desired_scope_plate_index);
    if (!source_mapping_items.is_array() || source_mapping_items.empty()) {
        session.mapping_items = json::array();
        session.mapping_items_scope_plate_index = desired_scope_plate_index;
        session.mapping_applied_to_scene = false;
        return true;
    }

    const FilamentMappingService::Mode auto_mode = FilamentMappingService::resolve_auto_mapping_mode(current_device);
    const FilamentMappingService::Mode manual_mode = FilamentMappingService::Mode::All;
    const bool should_rebuild_with_auto_match = auto_match ||
        (desired_scope_plate_index != kAllSceneMappingItemsScopePlateIndex && scope_changed);

    if (should_rebuild_with_auto_match || !session.mapping_items.is_array() || session.mapping_items.empty() || scope_changed) {
        session.mapping_items = should_rebuild_with_auto_match
            ? FilamentMappingService::auto_match(source_mapping_items, current_device, auto_mode)
            : FilamentMappingService::build_mapping_items(source_mapping_items, current_device, manual_mode);
        session.mapping_items_scope_plate_index = desired_scope_plate_index;
    }

    return session.mapping_items.is_array();
}



void AISendWorkflowService::sync_mapping_dirty_locked(Session& session) const
{
    if (!session.mapping_items.is_array() || session.mapping_items.empty() ||
        !mapping_items_have_mapped_items(session.mapping_items)) {
        session.mapping_dirty = false;
        session.mapping_applied_to_scene = false;
        return;
    }

    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    if (!current_device.valid) {
        session.mapping_dirty = false;
        session.mapping_applied_to_scene = false;
        return;
    }

    const int plate_index = resolve_selected_plate_index_locked(session);
    const json plate_mapping_items = filter_mapping_items_to_plate(session.mapping_items, plate_index);
    if (!plate_mapping_items.is_array() || plate_mapping_items.empty() ||
        !mapping_items_have_mapped_items(plate_mapping_items)) {
        session.mapping_dirty = false;
        return;
    }

    const FilamentMappingService::Mode mode = FilamentMappingService::Mode::All;
    const json diagnostics = FilamentMappingService::build_scene_match_diagnostics(plate_mapping_items, current_device, mode, plate_index);
    session.mapping_dirty = !diagnostics.value("matches", false) || diagnostics_have_ignored_color_mismatch(diagnostics);
    if (session.mapping_dirty) {
        log_ai_send_stage(
            "mapping_dirty_sync",
            session.card_id,
            session.request_id,
            safe_json_dump(diagnostics));
    }
}
void AISendWorkflowService::ensure_original_source_snapshot_locked(Session& /*session*/)
{
    if (auto* plater = wxGetApp().plater())
        plater->capture_scene_filament_source_snapshot_if_needed();
}

void AISendWorkflowService::invalidate_current_plate_mapping_preview_bases_locked(const Session& session) const
{
    (void)session;
    auto* plater = wxGetApp().plater();
    if (plater == nullptr)
        return;

    PartPlate* plate = plater->get_partplate_list().get_curr_plate();
    if (plate == nullptr)
        return;

    plate->thumbnail_data.reset();
    plate->no_light_thumbnail_data.reset();
}


json AISendWorkflowService::merge_original_source_into_effective_mapping_items(
    const Session& session,
    const json& effective_items) const
{
    (void)session;
    auto* plater = wxGetApp().plater();
    const json source_mapping_items =
        plater != nullptr ? plater->get_scene_filament_source_snapshot() : json::array();
    if (!effective_items.is_array() || !source_mapping_items.is_array())
        return effective_items;

    std::unordered_map<int, json> source_by_index;
    for (const auto& source_item : source_mapping_items) {
        if (!source_item.is_object())
            continue;
        const int item_index = source_item.value("item_index", -1);
        if (item_index < 0)
            continue;
        source_by_index[item_index] = source_item;
    }

    json merged = effective_items;
    for (auto& item : merged) {
        if (!item.is_object())
            continue;
        const int item_index = item.value("item_index", -1);
        const auto source_it = source_by_index.find(item_index);
        if (source_it == source_by_index.end())
            continue;

        const json& source_item = source_it->second;
        if (source_item.contains("sourceColor"))
            item["sourceColor"] = source_item.value("sourceColor", std::string());
        if (source_item.contains("sourceFilamentPreset"))
            item["sourceFilamentPreset"] = source_item.value("sourceFilamentPreset", std::string());
        if (source_item.contains("extruderFilamentType"))
            item["extruderFilamentType"] = source_item.value("extruderFilamentType", std::string());
        if (source_item.contains("presetDisplay"))
            item["presetDisplay"] = source_item.value("presetDisplay", std::string());
        item["source_snapshot"] = true;
    }

    return merged;
}
json AISendWorkflowService::build_snapshot_envelope_locked(Session& session)
{
    const json state = session.last_state;
    const json current_plate = state.value("current_plate", json::object());
    const json device = state.value("current_device", json::object());
    const json plates = state.value("plates", json::array());
    const std::string entry_mode = resolve_entry_mode(session.open_args);
    const bool mapping_only = entry_mode == "mapping_only";
    const bool process_intent_only = entry_mode == "process_intent";

    std::string readiness_message;
    const bool can_send = can_send_locked(session, readiness_message);

    const int selected_plate_index = resolve_selected_plate_index_locked(session);
    json plate_selector = {
        {"selected_plate_index", selected_plate_index},
        {"available", json::array()}
    };
    if (plates.is_array()) {
        for (const auto& plate : plates) {
            if (!plate.is_object())
                continue;
            const int plate_index = plate.value("index", -1);
            plate_selector["available"].push_back({
                {"plate_index", plate_index},
                {"label", plate.value("name", std::string())},
                {"selected", plate_index == selected_plate_index},
                {"selectable", !session.in_progress}
            });
        }
    }

    const auto resolved = resolve_gcode_file(session);
    const auto display_info = resolve_current_plate_display_info();
    auto* plater = wxGetApp().plater();
    const json source_mapping_items = plater != nullptr ? plater->get_scene_filament_source_snapshot() : json::array();
    json print_data = build_print_data(session, resolved.second);
    const json send_info = build_send_info_locked(session);
    const bool show_send_info = !mapping_only && !process_intent_only;
    const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
    const FilamentMappingService::Mode mapping_mode = FilamentMappingService::Mode::All;
    const json base_mapping_items = build_mapping_items(session);
    const bool mapping_loading = is_mapping_data_loading(state, base_mapping_items);
    json mapping_items = filter_mapping_items_to_plate(base_mapping_items, selected_plate_index);
    mapping_items = merge_original_source_into_effective_mapping_items(session, mapping_items);
    const MixedFilamentManager effective_mixed_manager =
        build_effective_mixed_filament_manager(source_mapping_items, mapping_items);
    const std::string preview_image =
        resolve_current_plate_preview_image(mapping_items, &effective_mixed_manager);
    const json mixed_items = build_mixed_filament_display_items(
        source_mapping_items,
        mapping_items,
        effective_mixed_manager,
        selected_plate_index);
    json mapping_option_groups = build_mapping_option_groups();
    const auto mapping_channel = FilamentMappingService::resolve_mapping_channel(mapping_items);
    const bool mapping_channel_conflict = mapping_channel == FilamentMappingService::MappingChannel::Mixed;
    const std::string mapping_channel_name = FilamentMappingService::mapping_channel_to_string(mapping_channel);
    const std::string mapping_validation_message = mapping_channel_conflict
        ? "CFS and external spool mappings cannot be mixed on the same plate."
        : std::string();
    const std::size_t mapping_count = mapping_items.is_array() ? mapping_items.size() : 0;
    std::size_t mapped_count = 0;
    if (mapping_items.is_array()) {
        for (const auto& item : mapping_items) {
            if (item.is_object() && item.value("mapped", false))
                ++mapped_count;
        }
    }
    const bool mapping_required = !mapping_loading && mapping_count > 0;
    const bool mapping_complete = !mapping_loading && (!mapping_required || mapped_count == mapping_count);
    const bool can_auto_match = !mapping_loading && !session.in_progress && FilamentMappingService::device_has_available_materials(current_device, mapping_mode);
    bool mapping_color_sync_needed = false;
    if (!mapping_loading && mapping_items.is_array() && current_device.valid) {
        const json diagnostics = FilamentMappingService::build_scene_match_diagnostics(
            mapping_items,
            current_device,
            mapping_mode,
            selected_plate_index);
        mapping_color_sync_needed = diagnostics_have_ignored_color_mismatch(diagnostics);
    }
    const bool mapping_apply_needed = session.mapping_dirty || mapping_color_sync_needed;
    const bool can_apply_mapping =
        !mapping_loading &&
        !session.in_progress &&
        mapping_complete &&
        mapping_apply_needed &&
        !mapping_channel_conflict;

    std::string mapping_summary = "No consumable mapping is required";
    if (mapping_loading) {
        mapping_summary = build_mapping_loading_text();
    } else if (mapping_channel_conflict) {
        mapping_summary = mapping_validation_message;
    } else if (mapping_required) {
        mapping_summary = std::to_string(mapped_count) + "/" + std::to_string(mapping_count) + " mapping items ready";
        if (mapping_complete)
            mapping_summary += " (complete)";
    }
    if (!mapping_loading && mapping_apply_needed && !mapping_channel_conflict)
        mapping_summary += " - unapplied changes";

    std::string status_text = can_send ? into_u8(_L("Ready to send")) : readiness_message;
    if (mapping_loading)
        status_text = build_mapping_loading_text();
    else if (mapping_channel_conflict)
        status_text = mapping_validation_message;
    else if (mapping_only)
        status_text = build_mapping_only_status_text(state, mapping_required, mapping_complete);
    else if (process_intent_only)
        status_text = session.process_switch_in_progress
            ? session.process_status_text
            : build_process_intent_status_text(state);

    const bool can_start_send = !mapping_loading && !session.in_progress && !mapping_apply_needed && can_send;
    const bool can_start_slice =
        !mapping_loading &&
        !session.in_progress &&
        !session.process_switch_in_progress &&
        state.value("has_model", false) &&
        current_plate.value("has_printable_instances", false);

    json data = {
        {"entry_mode", entry_mode},
        {"status", session.status},
        {"status_text", session.in_progress ? session.status_text : status_text},
        {"revision", session.revision},
        {"device", {
            {"name", device.value("name", std::string())},
            {"address", device.value("address", std::string())},
            {"online", device.value("online", false)},
            {"device_state", device.value("device_state", -1)}
        }},
        {"plate_selector", plate_selector},
        {"plate", {
            {"plate_index", current_plate.value("index", -1)},
            {"label", current_plate.value("name", std::string())},
            {"has_objects", !current_plate.value("empty", true)},
            {"has_printable_instances", current_plate.value("has_printable_instances", false)},
            {"file_name", resolved.second},
            {"print_time", display_info.first},
            {"total_weight", display_info.second},
            {"preview_image", preview_image}
        }},
        {"show_send_info", show_send_info},
        {"send_info", send_info},
        {"mapping", {
            {"mode", FilamentMappingService::mode_to_string(mapping_mode)},
            {"loading", mapping_loading},
            {"required", mapping_required},
            {"complete", mapping_complete},
            {"editable", !session.in_progress && mapping_required},
            {"summary_text", mapping_summary},
            {"items", mapping_items},
            {"mixed_items", mixed_items},
            {"option_groups", mapping_option_groups},
            {"total_count", mapping_count},
            {"default_visible_count", kDefaultVisibleMappingCount},
            {"dirty", mapping_apply_needed},
            {"can_apply", can_apply_mapping},
            {"channel", mapping_channel_name},
            {"channel_conflict", mapping_channel_conflict},
            {"validation_message", mapping_validation_message},
            {"color_sync_needed", mapping_color_sync_needed}
        }},
        {"settings", {
            {"print_calibration", print_data.value("print_calibration", 1)},
            {"open_cfs", print_data.value("open_cfs", 0)},
            {"all_plate", false}
        }},
        {"process", {
            {"selected_intent", session.selected_process_intent},
            {"current_preset_name", session.current_print_preset_name},
            {"resolved_preset_name", session.resolved_process_preset_name},
            {"summary_text", session.process_summary_text},
            {"status", session.process_status},
            {"status_text", session.process_status_text},
            {"switching", session.process_switch_in_progress},
            {"reslice_expected", session.process_reslice_expected},
            {"intent_options", json::array({
                json{{"key", "direct"}, {"label", "Direct"}},
                json{{"key", "speed"}, {"label", "Speed"}},
                json{{"key", "appearance"}, {"label", "Appearance"}},
                json{{"key", "strength"}, {"label", "Strength"}}
            })}
        }},
        {"actions", {
            {"can_select_plate", !session.in_progress},
            {"can_update_mapping", !session.in_progress},
            {"can_auto_match", can_auto_match},
            {"can_apply_mapping", can_apply_mapping},
            {"can_apply_process_intent", !session.in_progress},
            {"can_start_slice", can_start_slice},
            {"can_send_only", can_start_send},
            {"can_start_print", can_start_send},
            {"can_cancel", session.in_progress || session.waiting_user_action},
            {"can_retry", !session.in_progress}
        }}
    };

    session.last_snapshot = {
        {"version", "1.0"},
        {"request_id", session.request_id},
        {"card_id", session.card_id},
        {"timestamp_ms", current_timestamp_ms()},
        {"data", data}
    };
    return session.last_snapshot;
}

bool AISendWorkflowService::is_mapping_loading_snapshot(const json& snapshot) const
{
    if (!snapshot.is_object())
        return false;

    const json data = snapshot.value("data", json::object());
    if (!data.is_object())
        return false;

    const json mapping = data.value("mapping", json::object());
    return mapping.is_object() && mapping.value("loading", false);
}

void AISendWorkflowService::schedule_pending_mapping_refresh(const std::string& card_id)
{
    std::uint64_t retry_token = 0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return;

        Session& session = it->second;
        if (!is_mapping_loading_snapshot(session.last_snapshot) || session.mapping_sync_retry_active)
            return;

        session.mapping_sync_retry_active = true;
        retry_token = ++session.mapping_sync_retry_token;
    }

    std::thread([this, card_id, retry_token]() {
        constexpr int retry_count = 12;
        const auto retry_delay = std::chrono::milliseconds(250);

        for (int attempt = 0; attempt < retry_count; ++attempt) {
            std::this_thread::sleep_for(retry_delay);

            json snapshot_envelope;
            bool should_emit = false;

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_sessions.find(card_id);
                if (it == m_sessions.end())
                    return;

                Session& session = it->second;
                if (session.mapping_sync_retry_token != retry_token)
                    return;

                if (session.terminal || session.in_progress) {
                    session.mapping_sync_retry_active = false;
                    return;
                }

                if (!is_mapping_loading_snapshot(session.last_snapshot)) {
                    session.mapping_sync_retry_active = false;
                    return;
                }

                std::string code;
                std::string message;
                if (!refresh_state_locked(session, code, message)) {
                    session.mapping_sync_retry_active = false;
                    return;
                }

                if (!is_mapping_loading_snapshot(session.last_snapshot)) {
                    snapshot_envelope = session.last_snapshot;
                    session.mapping_sync_retry_active = false;
                    should_emit = true;
                } else if (attempt + 1 >= retry_count) {
                    session.mapping_sync_retry_active = false;
                }
            }

            if (should_emit) {
                emit_snapshot(snapshot_envelope);
                return;
            }
        }
    }).detach();
}

json AISendWorkflowService::build_progress_envelope_locked(
    Session& session,
    int progress,
    const std::string& message,
    const std::string& stage,
    const std::string& state,
    double speed)
{
    if (!stage.empty())
        session.status = stage;
    session.status_text = message;
    session.progress = progress;
    update_last_snapshot_status_locked(session);

    return {
        {"version", "1.0"},
        {"request_id", session.request_id},
        {"card_id", session.card_id},
        {"timestamp_ms", current_timestamp_ms()},
        {"progress", progress},
        {"message", message},
        {"stage", stage},
        {"state", state},
        {"speed", speed},
        {"data", {
            {"progress", progress},
            {"message", message},
            {"stage", stage},
            {"state", state},
            {"speed", speed}
        }}
    };
}

json AISendWorkflowService::build_result_envelope_locked(
    Session& session,
    const std::string& result_type,
    const std::string& message,
    const json& details)
{
    session.in_progress = false;
    session.waiting_user_action = false;
    session.terminal = true;
    session.status = result_type;
    session.status_text = message;

    const json envelope = {
        {"version", "1.0"},
        {"request_id", session.request_id},
        {"card_id", session.card_id},
        {"timestamp_ms", current_timestamp_ms()},
        {"result_type", result_type},
        {"message", message},
        {"details", details},
        {"data", {
            {"result_type", result_type},
            {"message", message},
            {"details", details}
        }}
    };

    update_last_snapshot_status_locked(session);
    return envelope;
}

json AISendWorkflowService::build_error_envelope_locked(
    Session& session,
    const std::string& code,
    const std::string& message,
    const json& details)
{
    session.in_progress = false;
    session.waiting_user_action = false;
    session.terminal = false;
    session.status = "failed";
    session.status_text = message;

    const json envelope = {
        {"version", "1.0"},
        {"request_id", session.request_id},
        {"card_id", session.card_id},
        {"timestamp_ms", current_timestamp_ms()},
        {"code", code},
        {"message", message},
        {"details", details},
        {"data", {
            {"code", code},
            {"message", message},
            {"details", details}
        }}
    };

    update_last_snapshot_status_locked(session);
    return envelope;
}

void AISendWorkflowService::update_last_snapshot_status_locked(Session& session)
{
    if (!session.last_snapshot.is_object())
        return;

    session.revision += 1;
    session.last_snapshot["timestamp_ms"] = current_timestamp_ms();

    json& data = session.last_snapshot["data"];
    if (!data.is_object())
        return;

    data["status"] = session.status;
    data["status_text"] = session.status_text;
    data["revision"] = session.revision;

    json& process = data["process"];
    if (process.is_object()) {
        process["selected_intent"] = session.selected_process_intent;
        process["current_preset_name"] = session.current_print_preset_name;
        process["resolved_preset_name"] = session.resolved_process_preset_name;
        process["summary_text"] = session.process_summary_text;
        process["status"] = session.process_status;
        process["status_text"] = session.process_status_text;
        process["switching"] = session.process_switch_in_progress;
        process["reslice_expected"] = session.process_reslice_expected;
    }

    json& actions = data["actions"];
    if (actions.is_object()) {
        actions["can_select_plate"] = !session.in_progress;
        actions["can_update_mapping"] = !session.in_progress;
        const DM::Device& current_device = DM::DataCenter::Ins().get_current_device_data();
        const FilamentMappingService::Mode mapping_mode = FilamentMappingService::Mode::All;
        actions["can_auto_match"] = !session.in_progress && FilamentMappingService::device_has_available_materials(current_device, mapping_mode);
        actions["can_apply_process_intent"] = !session.in_progress;
        actions["can_start_slice"] =
            !session.in_progress &&
            !session.process_switch_in_progress &&
            session.last_state.value("has_model", false) &&
            session.last_state.value("current_plate", json::object()).value("has_printable_instances", false);
        actions["can_cancel"] = session.in_progress || session.waiting_user_action;
        actions["can_retry"] = !session.in_progress;
    }
}

void AISendWorkflowService::on_upload_progress(const std::string& card_id, float progress, double speed)
{
    json envelope;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return;
        if (it->second.terminal)
            return;

        envelope = build_progress_envelope_locked(
            it->second,
            static_cast<int>(std::round(progress)),
            _u8L("Uploading G-code"),
            "uploading",
            "running",
            speed);
    }

    emit_progress(envelope);
}

void AISendWorkflowService::on_upload_status(const std::string& card_id, int status_code)
{
    json envelope;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return;
        if (it->second.terminal)
            return;

        envelope = build_progress_envelope_locked(
            it->second,
            std::max(it->second.progress, 5),
            _u8L("Upload status updated"),
            "uploading",
            "running");
        envelope["status_code"] = status_code;
        envelope["data"]["status_code"] = status_code;
    }

    emit_progress(envelope);
}

void AISendWorkflowService::on_upload_complete(const std::string& card_id, bool start_print, const std::string& body)
{
    json envelope;
    std::string request_id;
    bool emit_as_error = false;
    bool emit_as_result = false;
    bool emit_as_progress = false;
    bool cloud_workflow_active = false;
    bool cloud_closed_loop_enabled = false;
    bool sender_present = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end())
            return;

        Session& session = it->second;
        request_id = session.request_id;
        if (session.terminal)
            return;
        cloud_workflow_active = session.cloud_workflow_active;
        cloud_closed_loop_enabled = session.cloud_workflow_active && should_enable_cloud_closed_loop();
        sender_present = static_cast<bool>(session.sender);

        if (!is_upload_successful(body)) {
            session.sender.reset();
            session.cloud_workflow_active = false;
            envelope = build_error_envelope_locked(
                session,
                "UPLOAD_FAILED",
                _u8L("G-code upload failed or did not return a success status."),
                {{"raw", body}});
            emit_as_error = true;
        } else if (start_print && session.cloud_workflow_active && should_enable_cloud_closed_loop()) {
            envelope = build_progress_envelope_locked(
                session,
                std::max(session.progress, 55),
                _u8L("Upload completed. Preparing cloud print task."),
                "cloud_prepare",
                "running");
            envelope["upload_result"] = body;
            envelope["data"]["upload_result"] = body;
            emit_as_progress = true;
        } else {
            session.sender.reset();
            session.cloud_workflow_active = false;
            envelope = build_result_envelope_locked(
                session,
                start_print ? "print_started" : "send_only_done",
                start_print ? into_u8(_L("Upload completed and print start was dispatched."))
                            : into_u8(_L("G-code upload completed successfully.")),
                {{"raw", body}});
            emit_as_result = true;
        }
    }

    log_ai_send_stage(
        "upload_complete",
        card_id,
        request_id,
        "start_print=" + std::string(start_print ? "true" : "false") +
        ", cloud_workflow_active=" + std::string(cloud_workflow_active ? "true" : "false") +
        ", cloud_closed_loop_enabled=" + std::string(cloud_closed_loop_enabled ? "true" : "false") +
        ", sender_present=" + std::string(sender_present ? "true" : "false") +
        ", emit_error=" + std::string(emit_as_error ? "true" : "false") +
        ", emit_result=" + std::string(emit_as_result ? "true" : "false") +
        ", emit_progress=" + std::string(emit_as_progress ? "true" : "false") +
        ", body=" + body);

    if (emit_as_error)
        emit_error(envelope);
    else if (emit_as_result)
        emit_result(envelope);
    else if (emit_as_progress)
        emit_progress(envelope);
}

void AISendWorkflowService::on_cloud_print_progress(const std::string& card_id,
                                                    int progress,
                                                    const std::string& stage,
                                                    const std::string& message)
{
    json envelope;
    std::string request_id;
    bool cloud_workflow_active = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end()) {
            log_ai_send_stage(
                "cloud_progress_ignored",
                card_id,
                std::string(),
                "reason=session_not_found, progress=" + std::to_string(progress) +
                ", stage=" + stage +
                ", message=" + message);
            return;
        }

        Session& session = it->second;
        request_id = session.request_id;
        if (session.terminal) {
            log_ai_send_stage(
                "cloud_progress_ignored",
                card_id,
                request_id,
                "reason=session_terminal, progress=" + std::to_string(progress) +
                ", stage=" + stage +
                ", message=" + message);
            return;
        }
        cloud_workflow_active = session.cloud_workflow_active;

        envelope = build_progress_envelope_locked(
            session,
            std::max(progress, session.progress),
            message,
            stage,
            "running");
    }

    log_ai_send_stage(
        "cloud_progress",
        card_id,
        request_id,
        "progress=" + std::to_string(progress) +
        ", cloud_workflow_active=" + std::string(cloud_workflow_active ? "true" : "false") +
        ", stage=" + stage +
        ", message=" + message);

    emit_progress(envelope);
}

void AISendWorkflowService::on_cloud_print_success(const std::string& card_id, const json& result)
{
    json envelope;
    std::string request_id;
    bool cloud_workflow_active = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end()) {
            log_ai_send_stage(
                "cloud_success_ignored",
                card_id,
                std::string(),
                "reason=session_not_found, result=" + safe_json_dump(result));
            return;
        }

        Session& session = it->second;
        request_id = session.request_id;
        if (session.terminal) {
            log_ai_send_stage(
                "cloud_success_ignored",
                card_id,
                request_id,
                "reason=session_terminal, result=" + safe_json_dump(result));
            return;
        }
        cloud_workflow_active = session.cloud_workflow_active;

        session.sender.reset();
        session.cloud_workflow_active = false;
        envelope = build_result_envelope_locked(
            session,
            "print_started",
            result.value("message", _u8L("Cloud print task created.")),
            result);
    }

    log_ai_send_stage(
        "cloud_success",
        card_id,
        request_id,
        "cloud_workflow_active_before_reset=" + std::string(cloud_workflow_active ? "true" : "false") +
        ", result=" + safe_json_dump(result));

    emit_result(envelope);
}

void AISendWorkflowService::on_cloud_print_error(const std::string& card_id,
                                                 const std::string& code,
                                                 const std::string& message)
{
    json envelope;
    std::string request_id;
    bool cloud_workflow_active = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_sessions.find(card_id);
        if (it == m_sessions.end()) {
            log_ai_send_stage(
                "cloud_error_ignored",
                card_id,
                std::string(),
                "reason=session_not_found, code=" + code + ", message=" + message);
            return;
        }

        Session& session = it->second;
        request_id = session.request_id;
        if (session.terminal) {
            log_ai_send_stage(
                "cloud_error_ignored",
                card_id,
                request_id,
                "reason=session_terminal, code=" + code + ", message=" + message);
            return;
        }
        cloud_workflow_active = session.cloud_workflow_active;

        session.sender.reset();
        session.cloud_workflow_active = false;
        envelope = build_error_envelope_locked(
            session,
            code,
            message,
            {{"stage", "cloud_closed_loop"}});
    }

    log_ai_send_stage(
        "cloud_error",
        card_id,
        request_id,
        "cloud_workflow_active_before_reset=" + std::string(cloud_workflow_active ? "true" : "false") +
        ", code=" + code + ", message=" + message);

    emit_error(envelope);
}

void AISendWorkflowService::emit_snapshot(const json& envelope) const
{
    if (m_snapshot_callback)
        m_snapshot_callback(envelope);
}

void AISendWorkflowService::emit_progress(const json& envelope) const
{
    if (m_progress_callback)
        m_progress_callback(envelope);
}

void AISendWorkflowService::emit_result(const json& envelope) const
{
    if (m_result_callback)
        m_result_callback(envelope);
}

void AISendWorkflowService::emit_error(const json& envelope) const
{
    if (m_error_callback)
        m_error_callback(envelope);
}

long long AISendWorkflowService::current_timestamp_ms()
{
    using namespace std::chrono;
    return static_cast<long long>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

bool AISendWorkflowService::is_upload_successful(const std::string& body)
{
    try {
        const json parsed = json::parse(body);
        if (parsed.contains("code") && parsed["code"].is_number_integer()) {
            const int code = parsed["code"].get<int>();
            if (code == 200)
                return true;
            if (code == 0 && parsed.value("msg", std::string()) == "ok")
                return true;
        }
        if (parsed.value("message", std::string()) == "OK")
            return true;
    } catch (...) {
    }
    return false;
}

} // namespace GUI
} // namespace Slic3r


