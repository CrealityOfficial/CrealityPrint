#include "ResidentFilamentMappingLogic.hpp"

#include <algorithm>

namespace Slic3r {
namespace GUI {
namespace ResidentFilamentMapping {

UiMode resolve_ui_mode(const RuntimeSignals& signals)
{
    if (!signals.device_is_online)
        return UiMode::MultiColorOffline;

    if (!signals.device_supports_multi_color)
        return UiMode::SingleColorDevice;

    return UiMode::MultiColorOnline;
}

static SummaryViewModel build_summary(const RuntimeSignals& signals, UiMode mode)
{
    SummaryViewModel summary;

    switch (mode) {
    case UiMode::MultiColorOnline: {
        summary.tone = signals.device_materials_available ? SummaryTone::Good : SummaryTone::Warn;
        summary.subtitle = std::to_string(signals.scene_color_count) + " 个场景颜色";
        summary.pill_text = signals.device_materials_available ? "设备在线" : "待处理";
        summary.banner_text = signals.device_materials_available
            ? "设备在线，可逐项点击右侧映射目标进行调整。"
            : "设备在线，但当前未读取到可用料架，请检查设备耗材状态。";
        break;
    }
    case UiMode::SingleColorDevice:
        summary.tone = SummaryTone::Warn;
        summary.subtitle = "当前设备仅支持单色输出";
        summary.pill_text = "单色设备";
        summary.banner_text = (signals.scene_color_count <= 1)
            ? "当前场景为单色，可直接按统一耗材输出。"
            : "场景存在多种颜色，但当前设备只能使用 1 种耗材统一输出。";
        break;
    case UiMode::MultiColorOffline:
        summary.tone = SummaryTone::Offline;
        summary.subtitle = "设备离线，无法读取当前料架";
        summary.pill_text = "离线";
        summary.banner_text = signals.has_cached_mapping_result
            ? "保留上次映射结果供参考，当前不可修改；重新连接设备后可继续调整。"
            : "当前无法读取设备料架，也不提供新的推荐；重新连接设备后可继续调整。";
        break;
    }

    return summary;
}

UiModel build_ui_model(
    const RuntimeSignals& signals,
    const std::vector<RowInput>& row_inputs,
    const UnifiedOutputInput& unified_output)
{
    UiModel model;
    model.mode = resolve_ui_mode(signals);
    model.summary = build_summary(signals, model.mode);
    model.show_unified_output_card = (model.mode == UiMode::SingleColorDevice);
    model.unified_output = unified_output;
    model.rows.reserve(row_inputs.size());

    for (const RowInput& input : row_inputs) {
        RowViewModel row;
        row.item_index = input.item_index;
        row.scene_color = input.scene_color;
        row.scene_label = input.scene_label;
        row.scene_material_type = input.scene_material_type;

        switch (model.mode) {
        case UiMode::MultiColorOnline:
            row.presentation = RowPresentation::InteractiveSelector;
            row.selector_enabled = signals.device_materials_available;
            row.selector_show_chevron = signals.device_materials_available;
            row.selector_placeholder = !input.has_target;
            row.status_text = input.has_target ? "已映射" : "未映射";
            row.target_slot_label = input.has_target ? input.target_slot_label : std::string("待选择");
            row.target_material_type = input.has_target ? input.target_material_type : std::string("点击选择");
            row.target_material_color = input.has_target ? input.target_material_color : input.scene_color;
            break;

        case UiMode::SingleColorDevice:
            row.presentation = RowPresentation::UnifiedOutput;
            row.selector_enabled = false;
            row.selector_show_chevron = false;
            row.selector_placeholder = !unified_output.valid;
            row.status_text = unified_output.valid ? "将统一输出为当前耗材" : "未读取到统一输出耗材";
            row.target_slot_label = unified_output.valid ? unified_output.slot_label : std::string("--");
            row.target_material_type = unified_output.valid ? unified_output.material_type : std::string("未读取到设备耗材");
            row.target_material_color = unified_output.valid ? unified_output.material_color : ImVec4(0.46f, 0.50f, 0.56f, 1.f);
            break;

        case UiMode::MultiColorOffline:
            row.presentation = RowPresentation::DisabledSelector;
            row.selector_enabled = false;
            row.selector_show_chevron = false;
            row.selector_placeholder = !input.has_target;
            row.using_cached_target = input.has_target;
            row.status_text = input.has_target ? "上次映射结果" : "未读取到可用目标";
            row.target_slot_label = input.has_target ? input.target_slot_label : std::string("--");
            row.target_material_type = input.has_target ? input.target_material_type : std::string("离线不可选");
            row.target_material_color = input.has_target ? input.target_material_color : ImVec4(0.46f, 0.50f, 0.56f, 1.f);
            break;
        }

        model.rows.push_back(std::move(row));
    }

    return model;
}

} // namespace ResidentFilamentMapping
} // namespace GUI
} // namespace Slic3r
