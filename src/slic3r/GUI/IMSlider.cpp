#include "IMSlider.hpp"
#include "libslic3r/GCode.hpp"
#include "GUI_App.hpp"
#include "NotificationManager.hpp"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>
#include "../Config/DispConfig.h"

namespace Slic3r {

namespace GUI {

// equal to 25 mm2
inline double min_delta_area() { return scale_(scale_(25)); }
// equal to 1 mm2
inline double miscalculation() { return scale_(scale_(1)); }

static const float  LEFT_MARGIN       = 13.0f + 100.0f;  // avoid thumbnail toolbar
static const float  HORIZONTAL_SLIDER_WINDOW_HEIGHT  = 64.0f;
static const float  VERTICAL_SLIDER_WINDOW_WIDTH     = 160.0f;
static const float  GROOVE_WIDTH      = 7.0f;
static const ImVec2 ONE_LAYER_MARGIN  = ImVec2(30.0f, 20.0f);
static const ImVec2 ONE_LAYER_BUTTON_SIZE  = ImVec2(17.0f, 17.0f);
static const ImVec2 ANIMATE_BUTTON_SIZE    = ImVec2(30.0f, 30.0f);

static const ImU32 BACKGROUND_COLOR_DARK  = IM_COL32(65, 65, 71, 255);
static const ImU32 BACKGROUND_COLOR_LIGHT = IM_COL32(255, 255, 255, 255);
static const ImU32 GROOVE_COLOR_DARK      = IM_COL32(45, 45, 49, 255);
static const ImU32 GROOVE_COLOR_LIGHT     = IM_COL32(206, 206, 206, 255);
static const ImU32 BRAND_COLOR            = IM_COL32(0, 150, 136, 255);

static const double animate_duration = 200;
static int m_tick_value = -1;
static ImVec4 m_tick_rect;

bool equivalent_areas(const double& bottom_area, const double& top_area)
{
    return fabs(bottom_area - top_area) <= miscalculation();
}

bool check_color_change(PrintObject *object, size_t frst_layer_id, size_t layers_cnt, bool check_overhangs, std::function<bool(Layer *)> break_condition)
{
    double prev_area = area(object->get_layer(frst_layer_id)->lslices);

    bool detected = false;
    for (size_t i = frst_layer_id + 1; i < layers_cnt; i++) {
        Layer *layer    = object->get_layer(i);
        double cur_area = area(layer->lslices);

        // check for overhangs
        if (check_overhangs && cur_area > prev_area && !equivalent_areas(prev_area, cur_area)) break;

        // Check percent of the area decrease.
        // This value have to be more than min_delta_area and more then 10%
        if ((prev_area - cur_area > min_delta_area()) && (cur_area / prev_area < 0.9)) {
            detected = true;
            if (break_condition(layer)) break;
        }

        prev_area = cur_area;
    }
    return detected;
}


static std::string gcode(Type type)
{
    Slic3r::DynamicPrintConfig config = wxGetApp().preset_bundle->full_config();
    switch (type) {
    //BBS
    case Template:    return config.opt_string("template_custom_gcode");
    case PausePrint:  return config.opt_string("machine_pause_gcode");

    default:          return "";
    }

    //const PrintConfig& config = GUI::wxGetApp().plater()->fff_print().config();
    //switch (type) {
    ////BBS
    ////case ColorChange: return config.color_change_gcode;
    //case PausePrint:  return config.machine_pause_gcode;
    //case Template:    return config.template_custom_gcode;
    //default:          return "";
    //}
}

static std::string short_and_splitted_time(const std::string &time)
{
    // Parse the dhms time format.
    int days    = 0;
    int hours   = 0;
    int minutes = 0;
    int seconds = 0;
    if (time.find('d') != std::string::npos)
        ::sscanf(time.c_str(), "%dd %dh %dm %ds", &days, &hours, &minutes, &seconds);
    else if (time.find('h') != std::string::npos)
        ::sscanf(time.c_str(), "%dh %dm %ds", &hours, &minutes, &seconds);
    else if (time.find('m') != std::string::npos)
        ::sscanf(time.c_str(), "%dm %ds", &minutes, &seconds);
    else if (time.find('s') != std::string::npos)
        ::sscanf(time.c_str(), "%ds", &seconds);

    // Format the dhm time.
    char buffer[64];
    if (days > 0)
        ::sprintf(buffer, "%dd%dh%dm", days, hours, minutes);
    else if (hours > 0) {
        if (hours < 10 && minutes < 10 && seconds < 10)
            ::sprintf(buffer, "%dh%dm%ds", hours, minutes, seconds);
        else if (hours > 10 && minutes > 10 && seconds > 10)
            ::sprintf(buffer, "%dh%dm%ds", hours, minutes, seconds);
        else if ((minutes < 10 && seconds > 10) || (minutes > 10 && seconds < 10))
            ::sprintf(buffer, "%dh%dm%ds", hours, minutes, seconds);
        else
            ::sprintf(buffer, "%dh%dm%ds", hours, minutes, seconds);
    } else if (minutes > 0) {
        if (minutes > 10 && seconds > 10)
            ::sprintf(buffer, "%dm%ds", minutes, seconds);
        else
            ::sprintf(buffer, "%dm%ds", minutes, seconds);
    } else
        ::sprintf(buffer, "%ds", seconds);
    return std::string(buffer);
}


IMSlider::IMSlider(int lowerValue, int higherValue, int minValue, int maxValue, long style)
{
    m_lower_value  = lowerValue;
    m_higher_value = higherValue;
    m_min_value    = minValue;
    m_max_value    = maxValue;
    m_style        = style == wxSL_HORIZONTAL || style == wxSL_VERTICAL ? style : wxSL_HORIZONTAL;
    // BBS set to none style by default
    m_extra_style = style == wxSL_VERTICAL ? 0 : 0;
    m_selection   = ssHigher;
    m_is_need_post_tick_changed_event = false;
    m_tick_change_event_type = Type::Unknown;

    m_ticks.set_extruder_colors(&m_extruder_colors);
}

bool IMSlider::init_texture(bool onelayer)
{
    bool result = true;
    if (onelayer)
    {
        result &= IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/one_layer_on.svg", 24, 24, m_one_layer_on_id);
        result &= IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/one_layer_on_hover.svg", 28, 28, m_one_layer_on_hover_id);
        result &= IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/one_layer_off.svg", 28, 28, m_one_layer_off_id);
        result &= IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/one_layer_off_hover.svg", 28, 28,
                                                m_one_layer_off_hover_id);
        result &= IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/one_layer_on_dark.svg", 24, 24, m_one_layer_on_dark_id);
        result &= IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/one_layer_on_hover_dark.svg", 28, 28,
                                                m_one_layer_on_hover_dark_id);
        result &= IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/one_layer_off_dark.svg", 28, 28, m_one_layer_off_dark_id);
        result &= IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/one_layer_off_hover_dark.svg", 28, 28,m_one_layer_off_hover_dark_id);
    }
    if (!is_horizontal()) {
        // BBS init image texture id
        result &= IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/im_gcode_pause.svg", 14, 14, m_pause_icon_id);
        result &= IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/im_gcode_custom.svg", 14, 14, m_custom_icon_id);
        result &= IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/im_slider_delete.svg", 14, 14, m_delete_icon_id);
    }
    return result;
}

int IMSlider::GetMinValue() const
{
    return m_min_value;
}

int IMSlider::GetMaxValue() const
{
    return m_max_value;
}

double IMSlider::GetMinValueD()
{
    return m_values.empty() ? 0. : m_values[m_min_value];
}

double IMSlider::GetMaxValueD()
{
    return m_values.empty() ? 0. : m_values[m_max_value];
}

int IMSlider::GetLowerValue() const
{
    return m_lower_value;
}

int IMSlider::GetHigherValue() const
{
    return m_higher_value;
}

int IMSlider::GetActiveValue() const
{
    return m_selection == ssLower ?
    m_lower_value : m_selection == ssHigher ?
                m_higher_value : -1;
}

double IMSlider::GetLowerValueD()
{
    return get_double_value(ssLower);
}

double IMSlider::GetHigherValueD()
{
    return get_double_value(ssHigher);
}

Slic3r::GUI::SelectedSlider IMSlider::GetSelection()
{
    return m_selection;
}

void IMSlider::SetLowerValue(const int lower_val)
{
    m_selection   = ssLower;
    m_lower_value = lower_val;
    correct_lower_value();
    set_as_dirty();
}

void IMSlider::SetHigherValue(const int higher_val)
{
    m_selection    = ssHigher;
    m_higher_value = higher_val;
    correct_higher_value();
    set_as_dirty();
}

void IMSlider::SetSelectionSpan(const int lower_val, const int higher_val)
{
    m_lower_value  = std::max(lower_val, m_min_value);
    m_higher_value = std::max(std::min(higher_val, m_max_value), m_lower_value);
    //if (m_lower_value < m_higher_value) m_is_one_layer = false;

    set_as_dirty();
}

void IMSlider::SetMaxValue(const int max_value)
{
    m_max_value = max_value;
    set_as_dirty();
}

void IMSlider::SetKoefForLabels(const double koef)
{
    m_label_koef = koef;
}

void IMSlider::SetSliderValues(const std::vector<double> &values)
{
    m_values = values;
}

void IMSlider::SetSliderAlternateValues(const std::vector<double>& values)
{
    m_alternate_values = values;
}

Info IMSlider::GetTicksValues() const
{
    Info    custom_gcode_per_print_z;
    std::vector<CustomGCode::Item>& values = custom_gcode_per_print_z.gcodes;

    const int val_size = m_values.size();
    if (!m_values.empty())
        for (const TickCode& tick : m_ticks.ticks) {
            if (tick.tick > val_size)
                break;
            values.emplace_back(CustomGCode::Item{ m_values[tick.tick], tick.type, tick.extruder, tick.color, tick.extra });
        }

    if (m_force_mode_apply)
        custom_gcode_per_print_z.mode = m_mode;

    return custom_gcode_per_print_z;
}

void IMSlider::SetTicksValues(const Info& custom_gcode_per_print_z)
{
    if (m_values.empty()) {
        m_ticks.mode = m_mode;
        return;
    }

    static bool last_spiral_vase_status = false;

    const bool was_empty = m_ticks.empty();

    m_ticks.ticks.clear();
    const std::vector<CustomGCode::Item>& heights = custom_gcode_per_print_z.gcodes;
    for (auto h : heights) {
        int tick = get_tick_from_value(h.print_z, true);
        if (tick >= 0) {
            //Creality: use the last tick
            TickCode tCode = {tick, h.type, h.extruder, h.color, h.extra};
            if (m_ticks.ticks.find(tCode) != m_ticks.ticks.end())
                m_ticks.ticks.erase(tCode); 
            m_ticks.ticks.emplace(tCode);
        }
    }

    if (!was_empty && m_ticks.empty())
        // Switch to the "Feature type"/"Tool" from the very beginning of a new object slicing after deleting of the old one
        ; // post_ticks_changed_event();

    if (m_ticks.has_tick_with_code(ToolChange) && !m_can_change_color) {
        if (!wxGetApp().plater()->only_gcode_mode() && !wxGetApp().plater()->using_exported_file()) {
            m_ticks.erase_all_ticks_with_code(ToolChange);
            post_ticks_changed_event();
        }
    }

    if (last_spiral_vase_status != m_is_spiral_vase) {
        last_spiral_vase_status = m_is_spiral_vase;
        if (!m_ticks.empty()) {
            m_ticks.ticks.clear();
            post_ticks_changed_event();
        }
    }

    // auto has_tick_execpt = [this](CustomGCode::Type type) {
    //    for (const TickCode& tick : m_ticks.ticks)
    //        if (tick.type != type) return true;

    //    return false;
    //};
    if ((!m_ticks.empty() /*&& has_tick_execpt(PausePrint)*/) && m_draw_mode == dmSequentialFffPrint) {
        for (auto it{m_ticks.ticks.begin()}, end{m_ticks.ticks.end()}; it != end;) {
            if (true /*it->type != PausePrint*/)
                it = m_ticks.ticks.erase(it);
            else
                ++it;
        }
        post_ticks_changed_event();
    }

    if (custom_gcode_per_print_z.mode && !custom_gcode_per_print_z.gcodes.empty())
        m_ticks.mode = custom_gcode_per_print_z.mode;

    set_as_dirty();
}

void IMSlider::SetLayersTimes(const std::vector<float> &layers_times, float total_time)
{
    m_layers_times.clear();
    if (layers_times.empty()) return;
    m_layers_times.resize(layers_times.size(), 0.0);
    m_layers_times[0] = layers_times[0];
    for (size_t i = 1; i < layers_times.size(); i++) m_layers_times[i] = m_layers_times[i - 1] + layers_times[i];

    // Erase duplicates values from m_values and save it to the m_layers_values
    // They will be used for show the correct estimated time for MM print, when "No sparce layer" is enabled
    if (m_is_wipe_tower && m_values.size() != m_layers_times.size()) {
        m_layers_values = m_values;
        sort(m_layers_values.begin(), m_layers_values.end());
        m_layers_values.erase(unique(m_layers_values.begin(), m_layers_values.end()), m_layers_values.end());

        // When whipe tower is used to the end of print, there is one layer which is not marked in layers_times
        // So, add this value from the total print time value
        if (m_layers_values.size() != m_layers_times.size())
            for (size_t i = m_layers_times.size(); i < m_layers_values.size(); i++) m_layers_times.push_back(total_time);
        set_as_dirty();
        set_as_dirty();
    }
}

void IMSlider::SetLayersTimes(const std::vector<double> &layers_times)
{
    m_is_wipe_tower = false;
    m_layers_times  = layers_times;
    for (size_t i = 1; i < m_layers_times.size(); i++) m_layers_times[i] += m_layers_times[i - 1];
}

void IMSlider::SetDrawMode(bool is_sequential_print)
{
    m_draw_mode = is_sequential_print   ? dmSequentialFffPrint  : 
                                          dmRegular; 
    m_can_change_color = m_can_change_color && !(m_draw_mode == dmSequentialFffPrint);
}

void IMSlider::SetModeAndOnlyExtruder(const bool is_one_extruder_printed_model, const int only_extruder, bool can_change_color)
{
    m_mode = !is_one_extruder_printed_model ? MultiExtruder : only_extruder < 0 ? SingleExtruder : MultiAsSingle;
    if (!m_ticks.mode || (m_ticks.empty() && m_ticks.mode != m_mode)) m_ticks.mode = m_mode;
    m_only_extruder = only_extruder;

    UseDefaultColors(m_mode == SingleExtruder);

    m_is_wipe_tower = m_mode != SingleExtruder;

    auto config = wxGetApp().preset_bundle->full_config();
    m_is_spiral_vase = config.option<ConfigOptionBool>("spiral_mode")->value;

    m_can_change_color = can_change_color && !m_is_spiral_vase;

    // close opened menu window after reslice
    m_show_menu = false;
    m_show_custom_gcode_window = false;
    m_show_go_to_layer_dialog = false;
}

void IMSlider::SetExtruderColors( const std::vector<std::string>& extruder_colors)
{
    m_extruder_colors = extruder_colors;
}

bool IMSlider::IsNewPrint()
{
    const Print &print = GUI::wxGetApp().plater()->fff_print();
    std::string  idxs;
    for (auto object : print.objects()) idxs += std::to_string(object->id().id) + "_";

    if (idxs == m_print_obj_idxs) return false;

    m_print_obj_idxs = idxs;
    return true;
}

void IMSlider::post_ticks_changed_event(Type type)
{
    m_tick_change_event_type = type;
    m_is_need_post_tick_changed_event = true;
}

void IMSlider::add_custom_gcode(std::string custom_gcode)
{
    if (m_selection == ssUndef) return;
    const int tick = m_selection == ssLower ? m_lower_value : m_higher_value;

    const auto it = m_ticks.ticks.find(TickCode{ tick });

    if (it != m_ticks.ticks.end()) {
        m_ticks.ticks.erase(it);
    }
    m_ticks.ticks.emplace(TickCode{ tick, Custom, std::max<int>(1, m_only_extruder), "", custom_gcode });

    post_ticks_changed_event(Custom);
}

void IMSlider::add_code_as_tick(Type type, int selected_extruder)
{
    if (m_selection == ssUndef) return;
    const int tick = m_selection == ssLower ? m_lower_value : m_higher_value;

    if (!check_ticks_changed_event(type)) {
        BOOST_LOG_TRIVIAL(trace) << "check ticks change event false";
        return;
    }

    if (type == ColorChange && gcode(ColorChange).empty()) GUI::wxGetApp().plater()->get_notification_manager()->push_notification(GUI::NotificationType::EmptyColorChangeCode);

    const int  extruder = selected_extruder > 0 ? selected_extruder : std::max<int>(1, m_only_extruder);
    const auto it       = m_ticks.ticks.find(TickCode{tick});

    if (it == m_ticks.ticks.end()) {
        // try to add tick
        if (!m_ticks.add_tick(tick, type, extruder, m_values[tick])) return;
    } else if (type == ToolChange || type == ColorChange) {
        // try to switch tick code to ToolChange or ColorChange accordingly
        if (!m_ticks.switch_code_for_tick(it, type, extruder)) return;
    } else
        return;

    post_ticks_changed_event(type);
}

void IMSlider::delete_tick(const TickCode& tick) {
    Type t = tick.type; // Avoid Use-After-Free issue, which resets the tick.type to 0
    m_ticks.ticks.erase(tick);
    post_ticks_changed_event(t);
}

bool IMSlider::check_ticks_changed_event(Type type)
{
    //BBL only support MultiExtruder
    if (m_ticks.mode == m_mode || (type != ColorChange && type != ToolChange) ||
        (m_ticks.mode == SingleExtruder && m_mode == MultiAsSingle) || // All ColorChanges will be applied for 1st extruder
        (m_ticks.mode == MultiExtruder && m_mode == MultiAsSingle))    // Just mark ColorChanges for all unused extruders
        return true;

    if ((m_ticks.mode == SingleExtruder && m_mode == MultiExtruder) || (m_ticks.mode == MultiExtruder && m_mode == SingleExtruder)) {
        if (!m_ticks.has_tick_with_code(ColorChange)) return true;
        /*
        wxString message = (m_ticks.mode == SingleExtruder ? _L("The last color change data was saved for a single extruder printing.") :
                                                             _L("The last color change data was saved for a multi extruder printing.")) +
                           "\n" + _L("Your current changes will delete all saved color changes.") + "\n\n\t" + _L("Are you sure you want to continue?");

        
        GUI::MessageDialog msg(this, message, _L("Notice"), wxYES_NO);
        if (msg.ShowModal() == wxID_YES) {
            m_ticks.erase_all_ticks_with_code(ColorChange);
            post_ticks_changed_event(ColorChange);
        }
        */
        return false;
    }

    return true;
}


// switch on/off one layer mode
bool IMSlider::switch_one_layer_mode()
{
    if (m_show_custom_gcode_window)
        return false;

    m_is_one_layer = !m_is_one_layer;
    if (!m_is_one_layer) {
        SetLowerValue(m_min_value);
        SetHigherValue(m_max_value);
    }
    m_selection == ssLower ? correct_lower_value() : correct_higher_value();
    if (m_selection == ssUndef) m_selection = ssHigher;
    set_as_dirty();
    return true;
}

void IMSlider::set_animating(bool animate) 
{
    if (m_is_animating == animate)
        return;

    m_is_animating = animate;
    
    if (m_is_animating) {
        m_last_time = std::chrono::high_resolution_clock::now();
        moves_tick(true);
    }
}

void IMSlider::moves_tick(bool manual)
{
    auto moves_slider_window = ImGui::FindWindowByName("moves_slider");
    if (moves_slider_window && m_animate_tick) {
        m_animate_tick(manual);
    }
}

void IMSlider::show_go_to_layer(bool show)
{
    m_show_go_to_layer_dialog = show;
}

void IMSlider::draw_background_and_groove(const ImRect& bg_rect, const ImRect& groove) {
    //const ImU32 bg_rect_col = m_is_dark ? BACKGROUND_COLOR_DARK : BACKGROUND_COLOR_LIGHT;
    auto groove_col = DispConfig().getColorImU32(DispConfig::e_ct_slider);

    // draw bg of slider
    //ImGui::RenderFrame(bg_rect.Min, bg_rect.Max, bg_rect_col, false, 0.5 * bg_rect.GetWidth());
    // draw groove
    ImGui::RenderFrame(groove.Min, groove.Max, groove_col, false, 0.5 * groove.GetWidth());
}

bool IMSlider::horizontal_slider(const char* str_id, int* value, int v_min, int v_max, const ImVec2& size, float scale)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& context = *GImGui;
    const ImGuiID id = window->GetID(str_id);

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect draw_region(pos, pos + size);
    ImGui::ItemSize(draw_region);

    const float  handle_dummy_width  = 10.0f * m_scale;
    const float  text_right_dummy    = 70.0f * scale * m_scale;

    const float  handle_radius       = 12.0f * m_scale;
    const float  handle_border       = 2.0f * m_scale;

    const float  text_frame_rounding = 2.0f * scale * m_scale;
    const float  text_start_offset   = 8.0f * m_scale;
    const ImVec2 text_padding        = ImVec2(5.0f, 2.0f) * m_scale;
    const float  triangle_offsets[3] = {-3.5f * m_scale, 3.5f * m_scale, -6.06f * m_scale};

    const ImU32 white_bg = m_is_dark ? BACKGROUND_COLOR_DARK : BACKGROUND_COLOR_LIGHT;
    const ImU32 handle_clr = DispConfig().getColorImU32(DispConfig::e_ct_sliderSel);
    const ImU32 handle_border_clr = DispConfig().getColorImU32(DispConfig::e_ct_slider_hand);
    const ImU32 tip_bg = DispConfig().getColorImU32(DispConfig::e_ct_sliderTip);
    // calculate groove size
    const ImVec2 groove_start = ImVec2(pos.x + handle_dummy_width, pos.y + size.y - ONE_LAYER_MARGIN.y * m_scale - ONE_LAYER_BUTTON_SIZE.y * m_scale * 0.5f - GROOVE_WIDTH * m_scale * 0.5f);
            ImGui::SetCursorPos(ImVec2(size.x-ONE_LAYER_BUTTON_SIZE.x, 5));
    const ImVec2 groove_size = ImVec2(size.x - 2 * handle_dummy_width - text_right_dummy-ONE_LAYER_BUTTON_SIZE.x* m_scale, GROOVE_WIDTH * m_scale);
    const ImRect groove = ImRect(groove_start, groove_start + groove_size);
    const ImRect bg_rect = ImRect(groove.Min - ImVec2(6.0f, 6.0f) * m_scale, groove.Max + ImVec2(6.0f, 6.0f) * m_scale);
    const float mid_y = groove.GetCenter().y;

    // set mouse active region. active region.
    bool  hovered = ImGui::ItemHoverable(draw_region, id);
    if (hovered && context.IO.MouseDown[0]) {
        ImGui::SetActiveID(id, window);
        ImGui::SetFocusID(id, window);
        ImGui::FocusWindow(window);
    }

    // draw background
    draw_background_and_groove(bg_rect, groove);

    // set scrollable region
    const ImRect slideable_region = ImRect(bg_rect.Min + ImVec2(handle_radius, 0.0f), bg_rect.Max - ImVec2(handle_radius, 0.0f));

    // initialize the handle
    float  handle_pos = get_pos_from_value(v_min, v_max, *value, groove);
    ImRect handle = ImRect(handle_pos - handle_radius, mid_y - handle_radius, handle_pos + handle_radius, mid_y + handle_radius);

    // update handle position and value
    bool   value_changed = slider_behavior(id, slideable_region, (const ImS32) v_min, (const ImS32) v_max, (ImS32 *) value, &handle);
    ImVec2 handle_center = handle.GetCenter();

    // draw scroll line
    ImRect scroll_line = ImRect(groove.Min, ImVec2(handle_center.x, groove.Max.y));
    window->DrawList->AddRectFilled(scroll_line.Min, scroll_line.Max, handle_clr, 0.5f * GROOVE_WIDTH * m_scale);

    // draw handle
    window->DrawList->AddCircleFilled(handle_center, handle_radius, handle_border_clr);
    window->DrawList->AddCircleFilled(handle_center, handle_radius - handle_border, handle_clr);

    // draw label
    auto text_utf8 = into_u8(std::to_string(*value));
    ImVec2 text_content_size = ImGui::CalcTextSize(text_utf8.c_str());
    ImVec2 text_size = text_content_size + text_padding * 2;
    ImVec2 text_start = ImVec2(handle_center.x + handle_radius + text_start_offset, handle_center.y - 0.5 * text_size.y);
    ImRect text_rect(text_start, text_start + text_size);
    ImGui::RenderFrame(text_rect.Min, text_rect.Max, tip_bg, false, text_frame_rounding);
    ImVec2 pos_1 = ImVec2(text_rect.Min.x, text_rect.GetCenter().y + triangle_offsets[0]);
    ImVec2 pos_2 = ImVec2(text_rect.Min.x, text_rect.GetCenter().y + triangle_offsets[1]);
    ImVec2 pos_3 = ImVec2(text_rect.Min.x + triangle_offsets[2], text_rect.GetCenter().y);
    window->DrawList->AddTriangleFilled(pos_1, pos_2, pos_3, tip_bg);
    ImGui::RenderText(text_start + text_padding, std::to_string(*value).c_str());

    return value_changed;
}

void IMSlider::draw_colored_band(const ImRect& groove, const ImRect& slideable_region) {
    if (!m_ticks.has_tick_with_code(ToolChange))
        return;

    ImRect main_band = groove;

    auto draw_band = [this](const ImU32& clr, const ImRect& band_rc)
    {
        if (clr == (m_is_dark ? BACKGROUND_COLOR_DARK : BACKGROUND_COLOR_LIGHT)) {
            ImRect rc = band_rc;
            rc.Min += ImVec2(1, 1) * m_scale;
            rc.Max -= ImVec2(1, 1) * m_scale;
            ImGui::RenderFrame(band_rc.Min, band_rc.Max, m_is_dark ? GROOVE_COLOR_DARK : GROOVE_COLOR_LIGHT, false, band_rc.GetWidth() * 0.5);
            //cover round corner
            ImGui::RenderFrame(ImVec2(band_rc.Min.x, band_rc.Max.y - band_rc.GetWidth() * 0.5), band_rc.Max, m_is_dark ? GROOVE_COLOR_DARK : GROOVE_COLOR_LIGHT, false);

            ImGui::RenderFrame(rc.Min, rc.Max, clr, false, rc.GetWidth() * 0.5);
            //cover round corner
            ImGui::RenderFrame(ImVec2(rc.Min.x, rc.Max.y - rc.GetWidth() * 0.5), rc.Max, clr, false);
        }
        else {
            ImGui::RenderFrame(band_rc.Min, band_rc.Max, clr, false, band_rc.GetWidth() * 0.5);
            //cover round corner
            ImGui::RenderFrame(ImVec2(band_rc.Min.x, band_rc.Max.y - band_rc.GetWidth() * 0.5), band_rc.Max, clr, false);
        }
    };
    auto draw_main_band = [&main_band, this](const ImU32& clr) {
        if (clr == (m_is_dark ? BACKGROUND_COLOR_DARK : BACKGROUND_COLOR_LIGHT)) {
            ImRect rc = main_band;
            rc.Min += ImVec2(1, 1) * m_scale;
            rc.Max -= ImVec2(1, 1) * m_scale;
            ImGui::RenderFrame(main_band.Min, main_band.Max, m_is_dark ? GROOVE_COLOR_DARK : GROOVE_COLOR_LIGHT, false, main_band.GetWidth() * 0.5);
            ImGui::RenderFrame(rc.Min, rc.Max, clr, false, rc.GetWidth() * 0.5);
        }
        else {
            ImGui::RenderFrame(main_band.Min, main_band.Max, clr, false, main_band.GetWidth() * 0.5);
        }
    };
    //draw main colored band
    const int default_color_idx = m_mode == MultiAsSingle ? std::max<int>(m_only_extruder - 1, 0) : 0;
    ColorRGBA rgba = decode_color_to_float_array(m_extruder_colors[default_color_idx]);
    ImU32     band_clr          = ImGuiWrapper::to_ImU32(rgba);
    draw_main_band(band_clr);

    static float tick_pos;
    std::set<TickCode>::const_iterator tick_it = m_ticks.ticks.begin();
    while (tick_it != m_ticks.ticks.end())
    {
        //get position from tick
        tick_pos = get_pos_from_value(GetMinValue(), GetMaxValue(), tick_it->tick, slideable_region);
        //draw colored band
        if (tick_it->type == ToolChange) {
            if ((m_mode == SingleExtruder) || (m_mode == MultiAsSingle))
            {
                ImRect band_rect = ImRect(main_band.Min, ImVec2(main_band.Max.x, tick_pos));

                const std::string clr_str = m_mode == SingleExtruder ? tick_it->color : get_color_for_tool_change_tick(tick_it);

                if (!clr_str.empty()) {
                    ColorRGBA rgba = decode_color_to_float_array(clr_str);
                    ImU32     band_clr = ImGuiWrapper::to_ImU32(rgba);
                    if (tick_it->tick == 0)
                        draw_main_band(band_clr);
                    else
                        draw_band(band_clr, band_rect);
                }
            }
        }
        tick_it++;
    }
}

void IMSlider::draw_custom_label_block(const ImVec2 anchor, Type type)
{
    wxString label;
    switch (type)
    {
    case ColorChange:
        label = _L("Color");
        break;
    case PausePrint:
        label = _L("Pause");
        break;
    case ToolChange:
        label = _L("Color");
        break;
    case Template:
        label = _L("Template");
        break;
    case Custom:
        label = _L("Custom");
        break;
    case Unknown:
        break;
    default:
        break;
    }
    const ImVec2 text_size = ImGui::CalcTextSize(into_u8(label).c_str());
    const ImVec2 padding = ImVec2(4, 2) * m_scale;
    const ImU32  clr = IM_COL32(255, 111, 0, 255);
    const float  rounding = 2.0f * m_scale;
    ImVec2 block_pos = { anchor.x - text_size.x - padding.x * 2, anchor.y - text_size.y / 2 - padding.y };
    ImVec2 block_size = { text_size.x + padding.x * 2, text_size.y + padding.y * 2 };
    ImGui::RenderFrame(block_pos, block_pos + block_size, clr, false, rounding);
    ImGui::PushStyleColor(ImGuiCol_Text, { 1,1,1,1 });
    ImGui::RenderText(block_pos + padding, into_u8(label).c_str());
    ImGui::PopStyleColor();
}

void IMSlider::draw_ticks(const ImRect& slideable_region) {
    if (m_ticks.empty())
        return;

    ImGuiContext &context       = *GImGui;

    ImVec2 tick_box      = ImVec2(52.0f, 16.0f) * m_scale;
    ImVec2 tick_offset   = ImVec2(22.0f, 14.0f) * m_scale;
    float  tick_width    = 1.0f * m_scale;
    ImVec2 icon_offset   = ImVec2(16.0f, 7.0f) * m_scale;
    ImVec2 icon_size     = ImVec2(14.0f, 14.0f) * m_scale;

    const ImU32 tick_clr = IM_COL32(144, 144, 144, 255);
    const ImU32 tick_hover_box_clr = m_is_dark ? IM_COL32(65, 65, 71, 255) : IM_COL32(219, 253, 231, 255);

    auto get_tick_pos = [this, slideable_region](int tick)
    {
        int v_min = GetMinValue();
        int v_max = GetMaxValue();
        return get_pos_from_value(v_min, v_max, tick, slideable_region);
    };

    std::set<TickCode>::const_iterator tick_it = m_ticks.ticks.begin();
    while (tick_it != m_ticks.ticks.end())
    {
        float tick_pos = get_tick_pos(tick_it->tick);

        //draw tick hover box when hovered
        ImRect tick_hover_box = ImRect(slideable_region.GetCenter().x - tick_box.x / 2, tick_pos - tick_box.y / 2, slideable_region.GetCenter().x + tick_box.x / 2,
                                       tick_pos + tick_box.y / 2);

        if (ImGui::IsMouseHoveringRect(tick_hover_box.Min, tick_hover_box.Max))
        {
            // render left tick box
            ImRect left_hover_box = ImRect(tick_hover_box.Min, { slideable_region.Min.x, tick_hover_box.Max.y });
            ImGui::RenderFrame(left_hover_box.Min, left_hover_box.Max, tick_hover_box_clr, false);
            // render right tick box
            ImRect right_hover_box = ImRect({ slideable_region.Max.x, tick_hover_box.Min.y }, tick_hover_box.Max);
            ImGui::RenderFrame(right_hover_box.Min, right_hover_box.Max, tick_hover_box_clr, false);

            show_tooltip(*tick_it);
            m_tick_value = tick_it->tick;
            m_tick_rect = ImVec4(tick_hover_box.Min.x, tick_hover_box.Min.y, tick_hover_box.Max.x, tick_hover_box.Max.y);
        }
        ++tick_it;
    }

    tick_it = m_ticks.ticks.begin();
    while (tick_it != m_ticks.ticks.end())
    {
        float tick_pos = get_tick_pos(tick_it->tick);

        //draw ticks
        ImRect tick_left  = ImRect(slideable_region.GetCenter().x - tick_offset.x, tick_pos - tick_width, slideable_region.GetCenter().x - tick_offset.y, tick_pos);
        ImRect tick_right = ImRect(slideable_region.GetCenter().x + tick_offset.y, tick_pos - tick_width, slideable_region.GetCenter().x + tick_offset.x, tick_pos);
        ImGui::RenderFrame(tick_left.Min, tick_left.Max, tick_clr, false);
        ImGui::RenderFrame(tick_right.Min, tick_right.Max, tick_clr, false);

        //draw pause icon
        if (tick_it->type == PausePrint) {
            ImTextureID pause_icon_id = m_pause_icon_id;
            ImVec2      icon_pos     = ImVec2(slideable_region.GetCenter().x + icon_offset.x, tick_pos - icon_offset.y);
            button_with_pos(pause_icon_id, icon_size, icon_pos);
        }
        if (tick_it->type == Custom || tick_it->type == Template) {
            ImTextureID custom_icon_id = m_custom_icon_id;
            ImVec2      icon_pos = ImVec2(slideable_region.GetCenter().x + icon_offset.x, tick_pos - icon_offset.y);
            button_with_pos(custom_icon_id, icon_size, icon_pos);
        }

        //draw label block
        ImVec2 label_block_anchor = ImVec2(slideable_region.GetCenter().x - tick_offset.y, tick_pos);
        draw_custom_label_block(label_block_anchor, tick_it->type);

        ++tick_it;
    }

    tick_it = GetSelection() == ssHigher ? m_ticks.ticks.find(TickCode{this->GetHigherValue()}) :
              GetSelection() == ssLower  ? m_ticks.ticks.find(TickCode{this->GetLowerValue()}) :
                                           m_ticks.ticks.end();
    if (tick_it != m_ticks.ticks.end()) {
        //draw label block again, to keep it in front
        ImVec2 label_block_anchor = ImVec2(slideable_region.GetCenter().x - tick_offset.y, get_tick_pos(tick_it->tick));
        draw_custom_label_block(label_block_anchor, tick_it->type);

        // draw delete icon
        ImVec2      icon_pos       = ImVec2(slideable_region.GetCenter().x + icon_offset.x, get_tick_pos(tick_it->tick) - icon_offset.y);
        button_with_pos(m_delete_icon_id, icon_size, icon_pos);
        if (ImGui::IsMouseHoveringRect(icon_pos, icon_pos + icon_size)) {
            if (context.IO.MouseClicked[0]) {
                // delete tick
                delete_tick(*tick_it);
            }
        }
    }
}

void IMSlider::show_tooltip(const std::string tooltip) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 6 * m_scale, 3 * m_scale });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, { 3 * m_scale });
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGuiWrapper::COL_WINDOW_BACKGROUND);
    ImGui::PushStyleColor(ImGuiCol_Border, { 0,0,0,0 });
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(tooltip.c_str());
    ImGui::EndTooltip();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

void IMSlider::show_tooltip(const TickCode& tick){
    // Use previous layer's complete time as current layer's tick time,
    // since ticks are added at the beginning of current layer
    std::string time_str = "";
    // TODO: support first layer
    if (tick.tick > 0) {
        time_str = get_label(tick.tick - 1, ltEstimatedTime);
    }
    if (!time_str.empty()) {
        time_str += "\n";
    }
    
    switch (tick.type)
    {
    case CustomGCode::ColorChange:
        break;
    case CustomGCode::PausePrint:
        show_tooltip(time_str + _u8L("Pause:") + " \"" + gcode(PausePrint) + "\"");
        break;
    case CustomGCode::ToolChange:
        show_tooltip(time_str + _u8L("Change Filament"));
        break;
    case CustomGCode::Template:
        show_tooltip(time_str + _u8L("Custom Template:") + " \"" + gcode(Template) + "\"");
        break;
    case CustomGCode::Custom:
        show_tooltip(time_str + _u8L("Custom G-code:") + " \"" + tick.extra + "\"");
        break;
    default:
        break;
    }
}

int IMSlider::get_tick_near_point(int v_min, int v_max, const ImVec2& pt, const ImRect& rect) {
    ImS32 v_range = (v_min < v_max ? v_max - v_min : v_min - v_max);
    
    const ImGuiAxis axis = is_horizontal() ? ImGuiAxis_X : ImGuiAxis_Y;
    const float region_usable_sz = (rect.Max[axis] - rect.Min[axis]);
    const float region_usable_pos_min = rect.Min[axis];
    
    const float abs_pos = pt[axis];
    
    float pos_ratio = (region_usable_sz > 0.0f) ? ImClamp((abs_pos - region_usable_pos_min) / region_usable_sz, 0.0f, 1.0f) : 0.0f;
    if (axis == ImGuiAxis_Y)
        pos_ratio = 1.0f - pos_ratio;
    
    return v_min + (ImS32)(v_range * pos_ratio + 0.5f);
}

void IMSlider::draw_tick_on_mouse_position(const ImRect& slideable_region) {
    int v_min = GetMinValue();
    int v_max = GetMaxValue();
    ImGuiContext& context = *GImGui;
    
    int tick = get_tick_near_point(v_min, v_max, context.IO.MousePos, slideable_region);
    
    //draw tick
//     ImVec2 tick_offset   = ImVec2(22.0f, 14.0f) * m_scale;
//     float  tick_width    = 1.0f * m_scale;
// 
//     const ImU32 tick_clr = IM_COL32(144, 144, 144, 255);
// 
//     float tick_pos = get_pos_from_value(v_min, v_max, tick, slideable_region);
//     ImRect tick_left  = ImRect(slideable_region.GetCenter().x - tick_offset.x, tick_pos - tick_width, slideable_region.GetCenter().x - tick_offset.y, tick_pos);
//     ImRect tick_right = ImRect(slideable_region.GetCenter().x + tick_offset.y, tick_pos - tick_width, slideable_region.GetCenter().x + tick_offset.x, tick_pos);
//     ImGui::RenderFrame(tick_left.Min, tick_left.Max, tick_clr, false);
//     ImGui::RenderFrame(tick_right.Min, tick_right.Max, tick_clr, false);
    
    // draw layer time
    std::string label = get_label(tick, ltEstimatedTime);
    show_tooltip(label);
}

bool IMSlider::vertical_slider(const char* str_id, int* higher_value, int* lower_value
    , std::string& higher_label, std::string& lower_label
    ,int v_min, int v_max, const ImVec2& size
    , SelectedSlider& selection, bool one_layer_flag, float scale)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    GLCanvas3D::ECanvasType view_type = wxGetApp().plater()->get_current_canvas3D()->get_canvas_type();

    ImGuiContext& context = *GImGui;
    const ImGuiID id = window->GetID(str_id);
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect draw_region(pos, pos + size);
    ImGui::ItemSize(draw_region);

    const float  text_dummy_height   = 35.0f * scale * m_scale;
    const float  handle_radius       = 12.0f * m_scale;
    const float  handle_border       = 2.0f * m_scale;
    const float  line_width          = 1.0f * m_scale;
    const float  line_length         = 12.0f * m_scale;
    const float  one_handle_offset   = 26.0f * m_scale;
    const float  bar_width           = 28.0f * m_scale;

    const float  text_frame_rounding = 2.0f * scale * m_scale;
    const ImVec2 text_padding        = ImVec2(20.0f, 10.0f) * m_scale;
    const ImVec2 triangle_offsets[3] = {
        ImVec2(5.0f, 0.0f) * m_scale
        , ImVec2(0.0f, 15.0f) * m_scale
        , ImVec2(20.0f, 0.0f) * m_scale};
    ImVec2 text_content_size;
    ImVec2 text_size;

    auto config = DispConfig();

    // set mouse active region.
    ImRect active_region = draw_region;
    if (view_type == GLCanvas3D::CanvasPreview) {
        active_region.Min = draw_region.Max - config.getWindowSize(DispConfig::e_wt_slider, scale);
    }

    const ImU32 white_bg = m_is_dark ? BACKGROUND_COLOR_DARK : BACKGROUND_COLOR_LIGHT;
    const ImU32 handle_clr = config.getColorImU32(DispConfig::e_ct_sliderSel);
    const ImU32 handle_border_clr = config.getColorImU32(DispConfig::e_ct_slider_hand);
    const ImU32 tip_bg = config.getColorImU32(DispConfig::e_ct_sliderTip);
    // calculate slider groove size
    const ImVec2 groove_size  = ImVec2(GROOVE_WIDTH * m_scale, size.y - 2.0 * text_dummy_height);
    const ImVec2 groove_start = ImVec2(active_region.GetCenter().x - groove_size.x * 0.5f,
                                       pos.y + text_dummy_height);
    const ImRect groove = ImRect(groove_start, groove_start + groove_size);
    const ImRect bg_rect = ImRect(groove.Min - ImVec2(6.0f, 6.0f) * m_scale, groove.Max + ImVec2(6.0f, 6.0f) * m_scale);
    const float mid_x = groove.GetCenter().x;

    bool hovered = ImGui::ItemHoverable(active_region, id) && !ImGui::ItemHoverable(m_tick_rect, id);

    if (hovered) {
        ImGui::RenderFrame(active_region.Min, active_region.Max, IM_COL32(75, 75, 77, 125), false, 0.5 * bg_rect.GetWidth()); 
    }

    if (hovered && context.IO.MouseDown[0]) {
        ImGui::SetActiveID(id, window);
        ImGui::SetFocusID(id, window);
        ImGui::FocusWindow(window);
    }

    // draw background
    draw_background_and_groove(bg_rect, groove);

    
    // Processing interacting
    // set scrollable region
    const ImRect region = ImRect(bg_rect.Min + ImVec2(0.0f, handle_radius), bg_rect.Max - ImVec2(0.0f, handle_radius));
    const ImRect higher_slideable_region = ImRect(region.Min, region.Max - ImVec2(0, handle_radius));
    const ImRect lower_slideable_region = ImRect(region.Min + ImVec2(0, handle_radius), region.Max);
    const ImRect one_slideable_region = region;

    // initialize the handles.
    float higher_handle_pos = get_pos_from_value(v_min, v_max, *higher_value, higher_slideable_region);
    ImRect higher_handle = ImRect(mid_x - handle_radius, higher_handle_pos - handle_radius, mid_x + handle_radius, higher_handle_pos + handle_radius);

    float  lower_handle_pos = get_pos_from_value(v_min, v_max, *lower_value, lower_slideable_region);
    ImRect lower_handle = ImRect(mid_x - handle_radius, lower_handle_pos - handle_radius, mid_x + handle_radius, lower_handle_pos + handle_radius);

    ImRect one_handle = ImRect(higher_handle.Min - ImVec2(one_handle_offset, 0), higher_handle.Max - ImVec2(one_handle_offset, 0));

    bool value_changed = false;
    if (!one_layer_flag){
        // select higher handle by default
        static bool h_selected = (selection == ssHigher);
        if (ImGui::ItemHoverable(higher_handle, id) && context.IO.MouseClicked[0]) {
            selection = ssHigher; 
            h_selected = true;
        }
        if (ImGui::ItemHoverable(lower_handle, id) && context.IO.MouseClicked[0]) {
            selection = ssLower; 
            h_selected = false;
        }

        // update handle position and value
        if (h_selected)
        {
            value_changed = slider_behavior(id, higher_slideable_region, v_min, v_max,
                higher_value, &higher_handle, ImGuiSliderFlags_Vertical, 
                m_tick_value, m_tick_rect);
        }
        if (!h_selected) {
            value_changed = slider_behavior(id, lower_slideable_region, v_min, v_max,
                lower_value, &lower_handle, ImGuiSliderFlags_Vertical,
                m_tick_value, m_tick_rect);
        }

        ImVec2 higher_handle_center = higher_handle.GetCenter();
        ImVec2 lower_handle_center = lower_handle.GetCenter();
        if (higher_handle_center.y + handle_radius > lower_handle_center.y && h_selected)
        {
            lower_handle = higher_handle;
            lower_handle.TranslateY(handle_radius);
            lower_handle_center.y = higher_handle_center.y + handle_radius;
            *lower_value = *higher_value;
        }
        if (higher_handle_center.y + handle_radius > lower_handle_center.y && !h_selected)
        {
            higher_handle = lower_handle;
            higher_handle.TranslateY(-handle_radius);
            higher_handle_center.y = lower_handle_center.y - handle_radius;
            *higher_value = *lower_value;
        }

        // judge whether to open menu
        if (ImGui::ItemHoverable(h_selected ? higher_handle : lower_handle, id) && context.IO.MouseClicked[1]) {
            if (view_type == GLCanvas3D::CanvasPreview) {
                m_show_menu = true;
            }
        }
        if ((!ImGui::ItemHoverable(h_selected ? higher_handle : lower_handle, id) && context.IO.MouseClicked[1]) ||
            context.IO.MouseClicked[0])
            m_show_menu = false;

        // draw ticks
        draw_ticks(h_selected ? higher_slideable_region : lower_slideable_region);
        // draw colored band
        draw_colored_band(groove, h_selected ? higher_slideable_region : lower_slideable_region);

        if (!m_ticks.has_tick_with_code(ToolChange)) {
            // draw scroll line
            ImRect scroll_line = ImRect(ImVec2(groove.Min.x, higher_handle_center.y), ImVec2(groove.Max.x, lower_handle_center.y));
            window->DrawList->AddRectFilled(scroll_line.Min, scroll_line.Max, handle_clr);
        }

        // draw handles
        window->DrawList->AddCircleFilled(higher_handle_center, handle_radius, handle_border_clr);
        window->DrawList->AddCircleFilled(higher_handle_center, handle_radius - handle_border, handle_clr);
        window->DrawList->AddCircleFilled(lower_handle_center, handle_radius, handle_border_clr);
        window->DrawList->AddCircleFilled(lower_handle_center, handle_radius - handle_border, handle_clr);
        if (h_selected && view_type == GLCanvas3D::CanvasPreview) {
            window->DrawList->AddCircleFilled(higher_handle_center, handle_radius, handle_border_clr);
            window->DrawList->AddCircleFilled(higher_handle_center, handle_radius - handle_border, handle_clr);
            window->DrawList->AddLine(higher_handle_center + ImVec2(-0.5f * line_length, 0.0f), higher_handle_center + ImVec2(0.5f * line_length, 0.0f), white_bg, line_width);
            window->DrawList->AddLine(higher_handle_center + ImVec2(0.0f, -0.5f * line_length), higher_handle_center + ImVec2(0.0f, 0.5f * line_length), white_bg, line_width);
        }
        if (!h_selected && view_type == GLCanvas3D::CanvasPreview) {
            window->DrawList->AddLine(lower_handle_center + ImVec2(-0.5f * line_length, 0.0f), lower_handle_center + ImVec2(0.5f * line_length, 0.0f), white_bg, line_width);
            window->DrawList->AddLine(lower_handle_center + ImVec2(0.0f, -0.5f * line_length), lower_handle_center + ImVec2(0.0f, 0.5f * line_length), white_bg, line_width);
        }

        int tick = get_tick_near_point(m_min_value, m_max_value, ImGui::GetMousePos()
            , h_selected ? higher_slideable_region : lower_slideable_region);

        // draw higher label
        auto draw_higher_label = [&]() {
            auto text_utf8    = into_u8(higher_label);
            text_content_size = ImGui::CalcTextSize(text_utf8.c_str());
            text_size = text_content_size + text_padding * 2;
            ImVec2 text_start = ImVec2(higher_handle.Min.x - text_size.x - triangle_offsets[2].x, higher_handle_center.y - text_size.y);
            ImRect text_rect(text_start, text_start + text_size);

            ImGui::SetNextWindowPos(text_start);
            ImGui::SetNextWindowSize(text_size);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                     ImGuiWindowFlags_NoFocusOnAppearing;

            if (ImGui::Begin("##draw_higher_label", 0, flags)) {
             
                ImGui::RenderFrame(text_rect.Min, text_rect.Max, tip_bg, false, text_frame_rounding);
                ImVec2 pos_1 = text_rect.Max - triangle_offsets[0];
                ImVec2 pos_2 = pos_1 - triangle_offsets[1];
                ImVec2 pos_3 = pos_1 + triangle_offsets[2];
                window->DrawList->AddTriangleFilled(pos_1, pos_2, pos_3, tip_bg);
                
                ImGui::RenderText(text_start + text_padding, higher_label.c_str());

                ImGui::End(); 
            }
        };
        

        // draw lower label
        auto draw_lower_label = [&]() {
            auto text_utf8    = into_u8(lower_label);
            text_content_size = ImGui::CalcTextSize(text_utf8.c_str());
            text_size = text_content_size + text_padding * 2;
            auto text_start = ImVec2(lower_handle.Min.x - text_size.x - triangle_offsets[2].x, lower_handle_center.y);
            auto text_rect = ImRect(text_start, text_start + text_size);
#if 0
            ImVec4 bgc          = ImVec4(1.0, 1.0, 1.0, 1.0);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, bgc);
#endif // 0
            
            ImGui::SetNextWindowPos(text_start);
            ImGui::SetNextWindowSize(text_size); //+ triangle_offsets[2]

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                     ImGuiWindowFlags_NoFocusOnAppearing;
            if (ImGui::Begin("##draw_lower_label", 0, flags)) {

                ImGui::RenderFrame(text_rect.Min, text_rect.Max, tip_bg, false, text_frame_rounding);
                auto pos_1 = ImVec2(text_rect.Max.x, text_rect.Min.y) - triangle_offsets[0];
                auto pos_2 = pos_1 + triangle_offsets[1];
                auto pos_3 = pos_1 + triangle_offsets[2];
                window->DrawList->AddTriangleFilled(pos_1, pos_2, pos_3, tip_bg);
                
                ImGui::RenderText(text_start + text_padding, lower_label.c_str());

                ImGui::End(); 
            }
#if 0
            ImGui::PopStyleColor(1);
#endif
        };
        
        if (hovered && view_type == GLCanvas3D::CanvasView3D) {
            
            if (abs(tick - m_higher_value) < 2) 
            {
                draw_higher_label();
            } else if (abs(tick - m_lower_value) < 2) {
                draw_lower_label();
            }

        } else if (view_type == GLCanvas3D::CanvasPreview) {

            draw_higher_label();
            draw_lower_label();
        }
        

        // draw mouse position
        if (hovered&&!m_values.empty()) {
            draw_tick_on_mouse_position(h_selected ? higher_slideable_region : lower_slideable_region);
        }
    }
    else{
        // update handle position
        value_changed = slider_behavior(id, one_slideable_region, v_min, v_max,
            higher_value, &one_handle, ImGuiSliderFlags_Vertical,
            m_tick_value, m_tick_rect);

        ImVec2 handle_center = one_handle.GetCenter();

        // judge whether to open menu
        if (ImGui::ItemHoverable(one_handle, id) && context.IO.MouseClicked[1]) {
            if (view_type == GLCanvas3D::CanvasPreview) {
                m_show_menu = true;
            }
        }
        if ((!ImGui::ItemHoverable(one_handle, id) && context.IO.MouseClicked[1]) ||
            context.IO.MouseClicked[0])
            m_show_menu = false;
        
        ImVec2 bar_center = higher_handle.GetCenter();

        // draw ticks
        draw_ticks(one_slideable_region);
        // draw colored band
        draw_colored_band(groove, one_slideable_region);

        // draw handle
        window->DrawList->AddLine(ImVec2(mid_x - 0.5 * bar_width, handle_center.y)
            , ImVec2(mid_x + 0.5 * bar_width, handle_center.y), handle_clr, 2 * line_width);
        window->DrawList->AddCircleFilled(handle_center, handle_radius, handle_border_clr);
        window->DrawList->AddCircleFilled(handle_center, handle_radius - handle_border, handle_clr);
        window->DrawList->AddLine(handle_center + ImVec2(-0.5f * line_length, 0.0f)
            , handle_center + ImVec2(0.5f * line_length, 0.0f), white_bg, line_width);
        window->DrawList->AddLine(handle_center + ImVec2(0.0f, -0.5f * line_length)
            , handle_center + ImVec2(0.0f, 0.5f * line_length), white_bg, line_width);

        // draw label
        auto text_utf8 = into_u8(higher_label);
        text_content_size = ImGui::CalcTextSize(text_utf8.c_str());
        text_size = text_content_size + text_padding * 2;
        ImVec2 text_start = ImVec2(one_handle.Min.x - text_size.x, handle_center.y - 0.5 * text_size.y);
        ImRect text_rect = ImRect(text_start, text_start + text_size);
        ImGui::RenderFrame(text_rect.Min, text_rect.Max, tip_bg, false, text_frame_rounding);
        ImGui::RenderText(text_start + text_padding, higher_label.c_str());
        // draw mouse position
        if (hovered)
            draw_tick_on_mouse_position(one_slideable_region);
    }

    return value_changed;
}

bool IMSlider::render(int canvas_width, int canvas_height)
{
    auto config = DispConfig();
    bool result = false;
    ImGuiWrapper &imgui = *wxGetApp().imgui();
    /* style and colors */
    float scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    bool ret = false;
    DispConfig::WindowConfig cfg;
    cfg.bgalpha = 0;
    cfg.padding = ImVec2(0,0);
    if (is_horizontal()) {

        if (m_is_animating) 
        {
            auto cur = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration_cast<std::chrono::milliseconds>(cur - m_last_time).count();
            if (ms >= animate_duration) 
            {
                m_last_time = cur;
                moves_tick(false);
            }
        }

        config.prepareWindow(DispConfig::e_wt_slider_move, ImVec2(canvas_width / 2, canvas_height), scale);
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg,m_is_dark ? ImVec4(75.0 / 255.0, 75.0 / 255.0, 77.0 / 255.0, 1.0) : ImVec4(255,255,255,1));
        ImGui::PushStyleColor(ImGuiCol_Border, m_is_dark ? ImVec4(38.0 / 255.0, 38.0 / 255.0, 38.0 / 255.0, 1.0) : ImVec4(203/255, 203/255, 204/255,1.0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);

        int flag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                   ImGuiWindowFlags_NoFocusOnAppearing;
        
        if (ImGui::Begin("moves_slider", nullptr, flag)) {
            
            float win_h = ImGui::GetWindowHeight();
            float item_height = ImGui::GetFrameHeight();
            float slider_size = 290.0f * scale;
            float input_size = 50.0f * scale;
            const ImVec2 updown_button_size = ImVec2(15.0f * scale, item_height / 2.0f);
            float        total_size         = input_size + updown_button_size.x + 5.0f * scale;
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            
            // play button
            {
                DispConfig::TextureType tt     = m_is_animating ? DispConfig::e_tt_gcode_pause : DispConfig::e_tt_gcode_play;
                auto                    normtx = DispConfig().getTextureId(tt, false, false);
                if (ImGui::ImageButton3(normtx, normtx, ImVec2(ANIMATE_BUTTON_SIZE.x *scale , ANIMATE_BUTTON_SIZE.y * scale))) {
                    set_animating(!get_animating());
                }
                /*if (ImGui::IsItemHovered()) {
                    
                }*/
            }

            ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.81f, 0.81f, 0.81f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImGuiWrapper::COL_CREALITY);
            
            // slider
            ImGui::SameLine();
            ImGui::PushItemWidth(slider_size);
            ImGui::SetCursorPosY((win_h - item_height) / 2.0);

            int value = GetHigherValue();
            int min   = GetMinValue();
            int max   = GetMaxValue();
            if (ImGui::BBLSliderScalar("##steps_slider", ImGuiDataType_S32, &value, &min, &max)) {
                SetHigherValue(value);
            }
            
            // keyborad input
            {
                ImGui::SameLine();
                ImGui::PushItemWidth(input_size);
                ImGui::PushStyleColor(ImGuiCol_Text, m_is_dark ? ImVec4(255 / 255.0f, 255 / 255.f, 255 / 255.f, 1) : ImVec4(0/ 255.f, 0, 0, 1));
                //border
                {
                    ImVec2 win_pos = ImGui::GetWindowPos();
                    ImVec2 pos       = ImGui::GetCursorScreenPos();
                    
                    ImVec2 pos_min = ImVec2(pos.x, win_pos.y + 10.0f * scale);
                    ImVec2 pos_max = ImVec2(pos_min.x + total_size, pos_min.y + ImGui::GetWindowHeight() - 20.0f * scale);
                    draw_list->AddRect(pos_min, pos_max, IM_COL32(0, 255, 0, 255), 5.0f, ImDrawFlags_RoundCornersAll);
                }

                bool input = ImGui::BBLDragScalar("##steps_input", ImGuiDataType_S32, &value, 1, &min, &max);
                if (input) {
                    SetHigherValue(value);
                }
                ImGui::PopStyleColor(1);
                ImGui::PopStyleColor(5);

                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
                ImGui::PushStyleColor(ImGuiCol_Text, m_is_dark ? ImVec4(255 / 255.0f, 255 / 255.f, 255 / 255.f, 1) : ImVec4(0 / 255.f, 0, 0, 1));

                {
                    ImGui::SameLine(0,0);
                    ImGui::BeginGroup();

                    ImVec2 pos   = ImGui::GetCursorScreenPos();
                    bool   hover = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + updown_button_size.x, pos.y + updown_button_size.y));

                    if (hover)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImGuiWrapper::COL_CREALITY);

                    if (ImGui::ArrowButtonEX_CP("##up", ImGuiDir_Up, updown_button_size))
                    {
                        value++;
                        SetHigherValue(value);
                    }
                    if (hover)
                        ImGui::PopStyleColor();

                    pos   = ImGui::GetCursorScreenPos();
                    hover = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + updown_button_size.x, pos.y + updown_button_size.y));

                    if (hover)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImGuiWrapper::COL_CREALITY);

                    if (ImGui::ArrowButtonEX_CP("##down", ImGuiDir_Down, updown_button_size)) {
                        value--;
                        SetHigherValue(value);
                    }

                    if (hover)
                        ImGui::PopStyleColor();

                    ImGui::EndGroup();
                
                }
                ImGui::PopStyleColor(1);
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(3);
            }

            // one layer button
            ImGui::SameLine();
            ImGui::SetCursorPosY((win_h - (ONE_LAYER_BUTTON_SIZE.y * scale + ImGui::GetStyle().FramePadding.y * 2)) / 2.0);

            auto normtx = DispConfig().getTextureId(DispConfig::e_tt_layer, !is_one_layer());
            if (ImGui::ImageButton3(normtx, normtx, ImVec2(ONE_LAYER_BUTTON_SIZE.x * scale, ONE_LAYER_BUTTON_SIZE .y*scale))) {
                switch_one_layer_mode();
                ret = true;
            }
            if (ImGui::IsItemHovered()) {
                show_tooltip(_u8L("Click to toggle showing only the current layer").c_str());
            }
        }
        
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    } 
    else {
        //config.prepareWindow(DispConfig::e_wt_slider, ImVec2(canvas_width, 0), scale);

        GLCanvas3D::ECanvasType view_type       = wxGetApp().plater()->get_current_canvas3D()->get_canvas_type();
        DispConfig::WindowType tp = view_type == GLCanvas3D::CanvasView3D ? DispConfig::e_wt_slider : DispConfig::e_wt_slider_gcode;

        ImVec4 manipulate_rect = wxGetApp().plater()->get_current_canvas3D()->get_view_manipulate_rect();
        ImVec2 pos  = ImVec2(manipulate_rect.x, manipulate_rect.y);
        ImVec2 size = ImVec2(manipulate_rect.z, manipulate_rect.w);

        ImVec2 slider_pos = ImVec2(pos.x + size.x / 2, pos.y + size.y + 10 * scale);
        ImVec2 slider_size = config.getWindowSize(tp, scale);
        
        const float remain_h = canvas_height - slider_pos.y - 115 * scale;
        if (remain_h < slider_size.y) {
            slider_size.y = remain_h;
        }

        ImGui::SetNextWindowPos(slider_pos, ImGuiCond_Always, ImVec2(0.84, 0));
        ImGui::SetNextWindowSize(slider_size);

        config.processWindows("laysers_slider", [&]() { 
            render_menu();
            int higher_value = GetHigherValue();
            int lower_value = GetLowerValue();
            std::string higher_label = get_label(m_higher_value);
            std::string lower_label = get_label(m_lower_value);
            int temp_higher_value = higher_value;
            int temp_lower_value = lower_value;
            if (vertical_slider("laysers_slider", &higher_value, &lower_value, higher_label, lower_label, GetMinValue(), GetMaxValue(),
                                slider_size, m_selection, is_one_layer(), scale)) {
                if (temp_higher_value != higher_value)
                    SetHigherValue(higher_value);
                if (temp_lower_value != lower_value)
                    SetLowerValue(lower_value);
            }
        }, cfg);
    }
    return ret;
}

bool IMSlider::is_dirty()
{
    return m_dirty;
}

void IMSlider::set_as_dirty(bool dirty /*= true*/) {
    m_dirty = dirty;
}

bool IMSlider::is_need_post_tick_event()
{
    return m_is_need_post_tick_changed_event;
}

void IMSlider::reset_post_tick_event(bool val /*= false*/)
{
    m_is_need_post_tick_changed_event = val;
    m_tick_change_event_type = Type::Unknown;
}

void IMSlider::render_input_custom_gcode(std::string custom_gcode)
{
    if (m_show_custom_gcode_window)
        ImGui::OpenPopup((_u8L("Custom G-code")).c_str());

    ImGuiWrapper& imgui = *wxGetApp().imgui();
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    static bool set_focus = true;

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    imgui.push_menu_style(m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 10) * m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.f * m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 3) * m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 7) * m_scale);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, m_is_dark ? ImVec4(54 / 255.0f, 54 / 255.0f, 60 / 255.0f, 1.00f) : ImVec4(245 / 255.0f, 245 / 255.0f, 245 / 255.0f, 1.00f));
    ImGui::GetCurrentContext()->DimBgRatio = 1.0f;
    int windows_flag =
        ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::BeginPopupModal((_u8L("Custom G-code")).c_str(), NULL, windows_flag))
    {
        imgui.text(_u8L("Enter Custom G-code used on current layer:"));
        if (ImGui::IsMouseClicked(0)) {
            set_focus = false;
        }
        if (set_focus && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
            wxGetApp().plater()->get_current_canvas3D()->force_set_focus();
            ImGui::SetKeyboardFocusHere(0);
            strcpy(m_custom_gcode, custom_gcode.c_str());
        }
        const int text_height = 6;

        ImGui::InputTextMultiline("##text", m_custom_gcode, sizeof(m_custom_gcode), ImVec2(-1, ImGui::GetTextLineHeight() * text_height));

        ImGui::NewLine();
        ImGui::SameLine(ImGui::GetStyle().WindowPadding.x * 14);
        imgui.push_confirm_button_style();

        bool disable_button = false;
        if (strlen(m_custom_gcode) == 0)
            disable_button = true;
        if (disable_button) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            imgui.push_button_disable_style();
        }
        if (imgui.bbl_button(_L("OK"))) {
            add_custom_gcode(m_custom_gcode);
            m_show_custom_gcode_window = false;
            ImGui::CloseCurrentPopup();
            set_focus = true;
        }
        if (disable_button) {
            ImGui::PopItemFlag();
            imgui.pop_button_disable_style();
        }
        imgui.pop_confirm_button_style();

        ImGui::SameLine();
        imgui.push_cancel_button_style();
        if (imgui.bbl_button(_L("Cancel")) || ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
            m_show_custom_gcode_window = false;
            ImGui::CloseCurrentPopup();
            set_focus = true;
        }
        imgui.pop_cancel_button_style();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor();
    imgui.pop_menu_style();
}

void IMSlider::do_go_to_layer(size_t layer_number) {
    layer_number = clamp((int)layer_number, m_min_value, m_max_value);
    GetSelection() == ssLower ? SetLowerValue(layer_number) : SetHigherValue(layer_number);
}

void IMSlider::render_go_to_layer_dialog()
{
    if (m_show_go_to_layer_dialog)
        ImGui::OpenPopup((_u8L("Jump to Layer")).c_str());

    ImGuiWrapper& imgui = *wxGetApp().imgui();
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    static bool set_focus = true;

    imgui.push_menu_style(m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 10) * m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.f * m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 3) * m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 7) * m_scale);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, m_is_dark ? ImVec4(54 / 255.0f, 54 / 255.0f, 60 / 255.0f, 1.00f) : ImVec4(245 / 255.0f, 245 / 255.0f, 245 / 255.0f, 1.00f));
    ImGui::GetCurrentContext()->DimBgRatio = 1.0f;
    int windows_flag =
        ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::BeginPopupModal((_u8L("Jump to Layer")).c_str(), NULL, windows_flag))
    {
        imgui.text(_u8L("Please enter the layer number") + " (" + std::to_string(m_min_value + 1) + " - " + std::to_string(m_max_value + 1) + "):");
        if (ImGui::IsMouseClicked(0)) {
            set_focus = false;
        }
        if (set_focus && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
            wxGetApp().plater()->get_current_canvas3D()->force_set_focus();
            ImGui::SetKeyboardFocusHere(0);
        }
        ImGui::InputText("##input_layer_number", m_layer_number, sizeof(m_layer_number));

        ImGui::NewLine();
        ImGui::SameLine(GImGui->Style.WindowPadding.x * 8);
        imgui.push_confirm_button_style();
        bool disable_button = false;
        if (strlen(m_layer_number) == 0)
            disable_button = true;
        else {
            for (size_t i = 0; i < strlen(m_layer_number); i++)
                if (!isdigit(m_layer_number[i]))
                    disable_button = true;
            if (!disable_button && (m_min_value > atoi(m_layer_number) - 1 || atoi(m_layer_number) - 1 > m_max_value))
                disable_button = true;
        }
        if (disable_button) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            imgui.push_button_disable_style();
        }
        if (imgui.bbl_button(_L("OK")) || (!disable_button && ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_Enter)))) {
            do_go_to_layer(atoi(m_layer_number) - 1);
            m_show_go_to_layer_dialog = false;
            ImGui::CloseCurrentPopup();
            set_focus = true;
        }
        if (disable_button) {
            ImGui::PopItemFlag();
            imgui.pop_button_disable_style();
        }
        imgui.pop_confirm_button_style();

        ImGui::SameLine();
        imgui.push_cancel_button_style();
        if (imgui.bbl_button(_L("Cancel")) || ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
            m_show_go_to_layer_dialog = false;
            ImGui::CloseCurrentPopup();
            set_focus = true;
        }
        imgui.pop_cancel_button_style();

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor();
    imgui.pop_menu_style();
}

void IMSlider::render_menu() {
    if (!m_menu_enable)
        return;

    ImGuiWrapper::push_menu_style(m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_::ImGuiStyleVar_ChildRounding, 4.0f * m_scale);

    auto tick_it = GetSelection() == ssHigher ? m_ticks.ticks.find(TickCode{ GetHigherValue() }) :
        GetSelection() == ssLower ? m_ticks.ticks.find(TickCode{ GetLowerValue() }) :
        m_ticks.ticks.end();
    std::string custom_code;
    if (tick_it != m_ticks.ticks.end()) {
        if (tick_it->type == CustomGCode::Custom)
            custom_code = tick_it->extra;
        render_edit_menu(*tick_it);
    }
    else {
        render_add_menu();
    }

    ImGui::PopStyleVar(1);
    ImGuiWrapper::pop_menu_style();

    render_input_custom_gcode(custom_code);
    render_go_to_layer_dialog();
}

void IMSlider::render_add_menu()
{
    int extruder_num = m_extruder_colors.size();

    if (m_show_menu)
        ImGui::OpenPopup("slider_add_menu_popup");
    if (ImGui::BeginPopup("slider_add_menu_popup")) {
        bool menu_item_enable = m_draw_mode != dmSequentialFffPrint;
        bool hovered = false;
        {
            if (menu_item_with_icon(_u8L("Add Pause").c_str(), "", ImVec2(0, 0), 0, false, menu_item_enable, &hovered)) {
                add_code_as_tick(PausePrint);
            }
            if (hovered) { show_tooltip(_u8L("Insert a pause command at the beginning of this layer.")); }

            
            if (menu_item_with_icon(_u8L("Add Custom G-code").c_str(), "", ImVec2(0, 0), 0, false, menu_item_enable, &hovered)) {
                m_show_custom_gcode_window = true;
            }
            if (hovered) { show_tooltip(_u8L("Insert custom G-code at the beginning of this layer.")); }

            if (!gcode(Template).empty()) {
                if (menu_item_with_icon(_u8L("Add Custom Template").c_str(), "", ImVec2(0, 0), 0, false, menu_item_enable, &hovered)) {
                    add_code_as_tick(Template);
                }
                if (hovered) { show_tooltip(_u8L("Insert template custom G-code at the beginning of this layer.")); }
            }

            if (menu_item_with_icon(_u8L("Jump to Layer").c_str(), "")) {
                m_show_go_to_layer_dialog = true;
            }
        }

        //BBS render this menu item only when extruder_num > 1
        if (extruder_num > 1) {
            if (!m_can_change_color) {
                begin_menu(_u8L("Change Filament").c_str(), false);
            }
            else if (begin_menu(_u8L("Change Filament").c_str())) {
                for (int i = 0; i < extruder_num; i++) {
                    ColorRGBA rgba     = decode_color_to_float_array(m_extruder_colors[i]);
                    ImU32                icon_clr = ImGuiWrapper::to_ImU32(rgba);
                    if (rgba.a() == 0)
                        icon_clr = 0;
                    if (menu_item_with_icon((_u8L("Filament ") + std::to_string(i + 1)).c_str(), "", ImVec2(14, 14) * m_scale, icon_clr, false, true, &hovered)) add_code_as_tick(ToolChange, i + 1);
                    if (hovered) { show_tooltip(_u8L("Change filament at the beginning of this layer.")); }
                }
                end_menu();
            }
        }

        ImGui::EndPopup();
    }
}

void IMSlider::render_edit_menu(const TickCode& tick)
{
    if (m_show_menu)
        ImGui::OpenPopup("slider_edit_menu_popup");
    if (ImGui::BeginPopup("slider_edit_menu_popup")) {
        switch (tick.type)
        {
        case CustomGCode::PausePrint:
            if (menu_item_with_icon(_u8L("Delete Pause").c_str(), "")) {
                delete_tick(tick);
            }
            break;
        case CustomGCode::Template:
            if (!gcode(Template).empty()) {
                if (menu_item_with_icon(_u8L("Delete Custom Template").c_str(), "")) {
                    delete_tick(tick);
                }
            }
            break;
        case CustomGCode::Custom:
            if (menu_item_with_icon(_u8L("Edit Custom G-code").c_str(), "")) {
                m_show_custom_gcode_window = true;
            }
            if (menu_item_with_icon(_u8L("Delete Custom G-code").c_str(), "")) {
                delete_tick(tick);
            }
            break;
        case CustomGCode::ToolChange: {
            int extruder_num = m_extruder_colors.size();
            if (extruder_num > 1) {
                if (!m_can_change_color) {
                    begin_menu(_u8L("Change Filament").c_str(), false);
                }
                else if (begin_menu(_u8L("Change Filament").c_str())) {
                    for (int i = 0; i < extruder_num; i++) {
                        ColorRGBA rgba = decode_color_to_float_array(m_extruder_colors[i]);
                        ImU32     icon_clr = ImGuiWrapper::to_ImU32(rgba);
                        if (menu_item_with_icon((_u8L("Filament ") + std::to_string(i + 1)).c_str(), "", ImVec2(14, 14) * m_scale, icon_clr)) add_code_as_tick(ToolChange, i + 1);
                    }
                    end_menu();
                }
                if (menu_item_with_icon(_u8L("Delete Filament Change").c_str(), "")) {
                    delete_tick(tick);
                }
            }
            break;
        }
        case CustomGCode::ColorChange:
        case CustomGCode::Unknown:
        default:
            break;
        }
        ImGui::EndPopup();
    }
}

void IMSlider::on_change_color_mode(bool is_dark) {
    m_is_dark = is_dark;
}

void IMSlider::set_scale(float scale)
{
    if(m_scale != scale) m_scale = scale;
}

void IMSlider::UseDefaultColors(bool def_colors_on)
{
    m_ticks.set_default_colors(def_colors_on);
}

void IMSlider::on_mouse_wheel(wxMouseEvent& evt) {
    auto moves_slider_window = ImGui::FindWindowByName("moves_slider");
    auto layers_slider_window = ImGui::FindWindowByName("laysers_slider");
    float wheel = evt.GetWheelRotation() > 0 ? 1.0f : -1.0f;
    if (wheel == 0.0f)
        return;

#ifdef __WXOSX__
    if (wxGetKeyState(WXK_SHIFT)) {
        wheel *= -5;
    }
    else if (wxGetKeyState(WXK_RAW_CONTROL)) {
        wheel *= 5;
    } 
#else
    if (wxGetKeyState(WXK_COMMAND) || wxGetKeyState(WXK_SHIFT))
        wheel *= 5;
#endif
    if (is_horizontal()&& moves_slider_window) {
        if( evt.GetPosition().x > moves_slider_window->Pos.x &&
            evt.GetPosition().x < moves_slider_window->Pos.x + moves_slider_window->Size.x &&
            evt.GetPosition().y > moves_slider_window->Pos.y &&
            evt.GetPosition().y < moves_slider_window->Pos.y + moves_slider_window->Size.y){
            const int new_pos = GetHigherValue() + wheel;
            SetHigherValue(new_pos);
            set_as_dirty();
        }
    }
    else if(layers_slider_window){
        if (evt.GetPosition().x > layers_slider_window->Pos.x &&
            evt.GetPosition().x < layers_slider_window->Pos.x + layers_slider_window->Size.x &&
            evt.GetPosition().y > layers_slider_window->Pos.y &&
            evt.GetPosition().y < layers_slider_window->Pos.y + layers_slider_window->Size.y) {
            if (is_one_layer()) {
                const int new_pos = GetHigherValue() + wheel;
                SetHigherValue(new_pos);
            }
            else {
                const int new_pos = m_selection == ssLower ? GetLowerValue() + wheel : GetHigherValue() + wheel;
                m_selection == ssLower ? SetLowerValue(new_pos) : SetHigherValue(new_pos);
            }
            set_as_dirty();
        }
    }
}

void IMSlider::correct_lower_value()
{
    if (m_lower_value < m_min_value)
        m_lower_value = m_min_value;
    else if (m_lower_value > m_max_value)
        m_lower_value = m_max_value;

    if ((m_lower_value >= m_higher_value 
        && m_lower_value <= m_max_value) 
        || m_is_one_layer) 
        m_higher_value = m_lower_value;
}

void IMSlider::correct_higher_value()
{
    if (m_higher_value > m_max_value)
        m_higher_value = m_max_value;
    else if (m_higher_value < m_min_value)
        m_higher_value = m_min_value;

    if ((m_higher_value <= m_lower_value 
        && m_higher_value >= m_min_value) 
        || m_is_one_layer) 
        m_lower_value = m_higher_value;
}

bool IMSlider::is_wipe_tower_layer(int tick) const
{
    if (!m_is_wipe_tower || tick >= (int) m_values.size()) return false;
    if (tick == 0 || (tick == (int) m_values.size() - 1 && m_values[tick] > m_values[tick - 1])) return false;
    if ((m_values[tick - 1] == m_values[tick + 1] && m_values[tick] < m_values[tick + 1]) ||
        (tick > 0 && m_values[tick] < m_values[tick - 1])) // if there is just one wiping on the layer
        return true;

    return false;
}

std::string IMSlider::get_label(int tick, LabelType label_type)
{
    if (m_values.empty()&&m_alternate_values.size()==2)
    {
        char layer_height[64];
        ::sprintf(layer_height, "Z:%.2f", get_altarValue(tick));
        return std::string(layer_height);
    }

    const size_t value = tick;

    if (m_label_koef == 1.0 && m_values.empty()) {
        return std::to_string(value);
    }
    if (value >= m_values.size()) return "error";

    auto get_layer_number = [this](int value, LabelType label_type) {
        if (label_type == ltEstimatedTime && m_layers_times.empty()) return size_t(-1);
        double layer_print_z = m_values[is_wipe_tower_layer(value) ? std::max<int>(value - 1, 0) : value];
        auto   it            = std::lower_bound(m_layers_values.begin(), m_layers_values.end(), layer_print_z - epsilon());
        if (it == m_layers_values.end()) {
            it = std::lower_bound(m_values.begin(), m_values.end(), layer_print_z - epsilon());
            if (it == m_values.end()) return size_t(-1);
            return size_t(value);
        }
        return size_t(it - m_layers_values.begin());
    };

    if (m_draw_mode == dmSequentialGCodeView) {
        return std::to_string(tick);

    } else {
        if (label_type == ltEstimatedTime) {
            if (m_is_wipe_tower) {
                size_t layer_number = get_layer_number(value, label_type);
                return (layer_number == size_t(-1) || layer_number == m_layers_times.size()) ? "" : short_and_splitted_time(get_time_dhms(m_layers_times[layer_number]));
            }
            return value < m_layers_times.size() ? short_and_splitted_time(get_time_dhms(m_layers_times[value])) : "";
        }

        char layer_height[64];
        ::sprintf(layer_height, "%.2f", m_values.empty() ? m_label_koef * value : m_values[value]);
        if (label_type == ltHeight) 
            return std::string(layer_height);
        if (label_type == ltHeightWithLayer) {
            char   buffer[64];
            size_t layer_number;
            layer_number = m_draw_mode == dmSequentialFffPrint ? (m_values.empty() ? value : value + 1) : m_is_wipe_tower ? get_layer_number(value, label_type) + 1 : (m_values.empty() ? value : value + 1);
            ::sprintf(buffer, "%5s\n%5s", std::to_string(layer_number).c_str(), layer_height);
            return std::string(buffer);
        }
    }
    return "";
}

double IMSlider::get_altarValue(int v) {
    if (m_values.empty() && m_alternate_values.size() == 2)
    {
        auto maxv = m_alternate_values[1];
        auto minv = m_alternate_values[0];
        if (maxv > minv)
        {
            double ration = (maxv - minv) / (m_max_value - m_min_value);
            return v * ration + minv;
        }
    }
    return 0;
}

double IMSlider::get_double_value(const SelectedSlider &selection)
{
    if (m_lower_value < 0) 
        return 0.0;

    int v = selection == ssLower ? m_lower_value : m_higher_value;
    if (m_values.empty()&&m_alternate_values.size()==2)
    {
        return get_altarValue(v);
    }
    if (m_values.empty())
        return 0;
    if (m_values.size() <= size_t(m_higher_value)) {
        correct_higher_value();
        return m_values.back();
    }
    return m_values[v];
}

int IMSlider::get_tick_from_value(double value, bool force_lower_bound /* = false*/)
{
    std::vector<double>::iterator it;
    if (m_is_wipe_tower && !force_lower_bound)
        it = std::find_if(m_values.begin(), m_values.end(), [value](const double &val) { return fabs(value - val) <= epsilon(); });
    else
        it = std::lower_bound(m_values.begin(), m_values.end(), value - epsilon());

    if (it == m_values.end()) return -1;
    return int(it - m_values.begin());
}

float IMSlider::get_pos_from_value(int v_min, int v_max, int value, const ImRect& rect) {
    float pos_ratio = (v_max - v_min) != 0 ? ((float)(value - v_min) / (float)(v_max - v_min)) : 0.0f;
    float handle_pos;
    if (is_horizontal()) {
        handle_pos = rect.Min.x + (rect.Max.x - rect.Min.x) * pos_ratio;
    }
    else {
        pos_ratio = 1.0f - pos_ratio;
        handle_pos = rect.Min.y + (rect.Max.y - rect.Min.y) * pos_ratio;
    }
    return handle_pos;
}

std::string IMSlider::get_color_for_tool_change_tick(std::set<TickCode>::const_iterator it) const
{
    const int current_extruder = it->extruder == 0 ? std::max<int>(m_only_extruder, 1) : it->extruder;

    auto it_n = it;
    while (it_n != m_ticks.ticks.begin()) {
        --it_n;
        if (it_n->type == ColorChange && it_n->extruder == current_extruder)
            return it_n->color;
    }

    if ((current_extruder > 0 && (current_extruder - 1) < m_extruder_colors.size()))
    {
        return m_extruder_colors[current_extruder - 1]; // return a color for a specific extruder from the colors list
    }
    return "";
}

// Get active extruders for tick.
// Means one current extruder for not existing tick OR
// 2 extruders - for existing tick (extruder before ToolChange and extruder of current existing tick)
// Use those values to disable selection of active extruders
std::array<int, 2> IMSlider::get_active_extruders_for_tick(int tick) const
{
    int                default_initial_extruder = m_mode == MultiAsSingle ? std::max<int>(1, m_only_extruder) : 1;
    std::array<int, 2> extruders                = {default_initial_extruder, -1};
    if (m_ticks.empty()) return extruders;

    auto it = m_ticks.ticks.lower_bound(TickCode{tick});

    if (it != m_ticks.ticks.end() && it->tick == tick) // current tick exists
        extruders[1] = it->extruder;

    while (it != m_ticks.ticks.begin()) {
        --it;
        if (it->type == ToolChange) {
            extruders[0] = it->extruder;
            break;
        }
    }

    return extruders;
}

}

} // Slic3r


