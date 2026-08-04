#include "slic3r/GUI/GCodeRenderer/BaseRenderer.hpp"
#include "libslic3r/libslic3r.h"
#include "libslic3r/BuildVolume.hpp"
#include "libslic3r/ClipperUtils.hpp"
#include "libslic3r/Print.hpp"
#include "libslic3r/Geometry.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/ModelInstance.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/LocalesUtils.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/GCode/InterestRegion.hpp"
//BBS: add convex hull logic for toolpath check
#include "libslic3r/Geometry/ConvexHull.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/Utils/TestHelper.hpp"
#include "../Config/DispConfig.h"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/MainFrame.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/Camera.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/GUI_Utils.hpp"
#include "slic3r/GUI/GUI.hpp"
//#include "slic3r/GUI/GLCanvas3D.hpp"
//#include "slic3r/GUI/GLToolbar.hpp"
#include "slic3r/GUI/GUI_Preview.hpp"
//#include "libslic3r/Print.hpp"
#include "libslic3r/Layer.hpp"
#include "slic3r/GUI/Widgets/ProgressDialog.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "slic3r/GUI/MsgDialog.hpp"
#include <imgui/imgui_internal.h>

#include <glad/gl.h>
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/nowide/fstream.hpp>
#include <string>
#include <wx/progdlg.h>
#include <wx/numformatter.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "slic3r/GUI/GCodeRenderer/AdvancedRenderer.hpp"

namespace Slic3r {
namespace GUI {

    const std::vector<ColorRGBA> BaseRenderer::Extrusion_Role_Colors{{
        { 0.90f, 0.70f, 0.70f, 1.0f },   // erNone
        { 1.00f, 0.90f, 0.30f, 1.0f },   // erPerimeter
        { 1.00f, 0.49f, 0.22f, 1.0f },   // erExternalPerimeter
        { 0.12f, 0.12f, 1.00f, 1.0f },   // erOverhangPerimeter
        { 0.69f, 0.19f, 0.16f, 1.0f },   // erInternalInfill
        { 0.59f, 0.33f, 0.80f, 1.0f },   // erSolidInfill
        { 0.94f, 0.25f, 0.25f, 1.0f },   // erTopSolidInfill
        { 0.40f, 0.36f, 0.78f, 1.0f },   // erBottomSurface
        { 1.00f, 0.55f, 0.41f, 1.0f },   // erIroning
        { 0.30f, 0.40f, 0.63f, 1.0f },   // erBridgeInfill
        { 0.30f, 0.50f, 0.73f, 1.0f },   // erInternalBridgeInfill
        { 1.00f, 1.00f, 1.00f, 1.0f },   // erGapFill
        { 0.00f, 0.53f, 0.43f, 1.0f },   // erSkirt
        { 0.00f, 0.23f, 0.43f, 1.0f },   // erBrim
        { 0.00f, 1.00f, 0.00f, 1.0f },   // erSupportMaterial
        { 0.00f, 0.50f, 0.00f, 1.0f },   // erSupportMaterialInterface
        { 0.00f, 0.25f, 0.00f, 1.0f },   // erSupportTransition
        { 0.70f, 0.89f, 0.67f, 1.0f },   // erWipeTower
        { 1.00f, 0.49f, 0.22f, 1.0f },   // erSkinInfill
        { 0.37f, 0.82f, 0.58f, 1.0f },   // erCustom
        { 0.70f, 0.70f, 0.70f, 1.0f }    // erMixed
    } };

    const std::vector<ColorRGBA> BaseRenderer::Options_Colors{{
        { 0.803f, 0.135f, 0.839f, 1.0f },   // Retractions
        { 0.287f, 0.679f, 0.810f, 1.0f },   // Unretractions
        { 0.900f, 0.900f, 0.900f, 1.0f },   // Seams
        { 0.758f, 0.744f, 0.389f, 1.0f },   // ToolChanges
        { 0.856f, 0.582f, 0.546f, 1.0f },   // ColorChanges
        { 0.322f, 0.942f, 0.512f, 1.0f },   // PausePrints
        { 0.886f, 0.825f, 0.262f, 1.0f }    // CustomGCodes
    } };

    const std::vector<ColorRGBA> BaseRenderer::Travel_Colors{{
        { 0.219f, 0.282f, 0.609f, 1.0f }, // Move
        { 0.112f, 0.422f, 0.103f, 1.0f }, // Extrude
        { 0.505f, 0.064f, 0.028f, 1.0f }  // Retract
    } };

    const std::vector<ColorRGBA> BaseRenderer::Range_Colors{{
        decode_color_to_float_array("#0b2c7a"),  // bluish
        decode_color_to_float_array("#135985"),
        decode_color_to_float_array("#1c8891"),
        decode_color_to_float_array("#04d60f"),
        decode_color_to_float_array("#aaf200"),
        decode_color_to_float_array("#fcf903"),
        decode_color_to_float_array("#f5ce0a"),
        //decode_color_to_float_array("#e38820"),
        decode_color_to_float_array("#d16830"),
        decode_color_to_float_array("#c2523c"),
        decode_color_to_float_array("#942616")    // reddish
    } };

    const ColorRGBA  BaseRenderer::Wipe_Color    = ColorRGBA::YELLOW();
    const ColorRGBA  BaseRenderer::Flush_Color   = {1.0f, 0.6f, 0.8f, 1.0f}; // pink
    const ColorRGBA  BaseRenderer::Neutral_Color = ColorRGBA::DARK_GRAY();
    static bool s_preview_section_fold = false;

static std::string get_view_type_string(EViewType view_type)
{
    if (view_type == EViewType::FeatureType)
        return _u8L("Line Type");
    else if (view_type == EViewType::Custom)
        return _u8L("Custom");
    else if (view_type == EViewType::Height)
        return _u8L("Layer Height");
    else if (view_type == EViewType::Width)
        return _u8L("Line Width");
    else if (view_type == EViewType::Feedrate)
        return _u8L("Speed");
    else if (view_type == EViewType::FanSpeed)
        return _u8L("Fan Speed");
    else if (view_type == EViewType::Temperature)
        return _u8L("Temperature");
    else if (view_type == EViewType::VolumetricRate)
        return _u8L("Flow");
    else if (view_type == EViewType::Tool)
        return _u8L("Tool");
    else if (view_type == EViewType::ColorPrint)
        return _u8L("Filament");
    else if (view_type == EViewType::LayerTime)
        return _u8L("Layer Time");
    else if (view_type == EViewType::LayerTimeLog)
        return _u8L("Layer Time (log)");
    else if (view_type == EViewType::Acceleration)
        return _u8L("Acceleration");
    return "";
}

static std::pair<ColorRGBA, ColorRGBA> pick_interest_region_highlight_colors(const ColorRGBA& base_color)
{
    // Pick two highlight colors with good contrast against the base (filament) color.
    // They are also forced to be different from each other so that Trigger/Defect can be distinguished.
    static const std::array<ColorRGBA, 7> candidates = {
        ColorRGBA::YELLOW(),
        ColorRGBA::MAGENTA(),
        ColorRGBA::CYAN(),
        ColorRGBA::ORANGE(),
        ColorRGBA::GREEN(),
        ColorRGBA::RED(),
        ColorRGBA::BLUE()
    };

    auto dist2 = [](const ColorRGBA& a, const ColorRGBA& b) {
        const float dr = a.r() - b.r();
        const float dg = a.g() - b.g();
        const float db = a.b() - b.b();
        return dr * dr + dg * dg + db * db;
    };

    // Find best and second best by distance.
    ColorRGBA best1 = candidates.front();
    float best1_score = -1.0f;
    ColorRGBA best2 = candidates.front();
    float best2_score = -1.0f;

    for (const ColorRGBA& c : candidates) {
        const float score = dist2(base_color, c);
        if (score > best1_score) {
            best2 = best1;
            best2_score = best1_score;
            best1 = c;
            best1_score = score;
        } else if (score > best2_score) {
            best2 = c;
            best2_score = score;
        }
    }

    // Return (trigger_color, defect_color). Defect is more critical so use the farthest.
    return { best2, best1 };
}

static double print_time_for_display(const PrintEstimatedStatistics::Mode& mode)
{
    return mode.model_time_s();
}

#if 0
static unsigned char buffer_id(EMoveType type) {
    return static_cast<unsigned char>(type) - static_cast<unsigned char>(EMoveType::Retract);
}

static EMoveType buffer_type(unsigned char id) {
    return static_cast<EMoveType>(static_cast<unsigned char>(EMoveType::Retract) + id);
}

// Round to a bin with minimum two digits resolution.
// Equivalent to conversion to string with sprintf(buf, "%.2g", value) and conversion back to float, but faster.
static float round_to_bin(const float value)
{
//    assert(value > 0);
    constexpr float const scale    [5] = { 100.f,  1000.f,  10000.f,  100000.f,  1000000.f };
    constexpr float const invscale [5] = { 0.01f,  0.001f,  0.0001f,  0.00001f,  0.000001f };
    constexpr float const threshold[5] = { 0.095f, 0.0095f, 0.00095f, 0.000095f, 0.0000095f };
    // Scaling factor, pointer to the tables above.
    int                   i            = 0;
    // While the scaling factor is not yet large enough to get two integer digits after scaling and rounding:
    for (; value < threshold[i] && i < 4; ++ i) ;
    return std::round(value * scale[i]) * invscale[i];
}
#endif

// Find an index of a value in a sorted vector, which is in <z-eps, z+eps>.
// Returns -1 if there is no such member.
static int find_close_layer_idx(const std::vector<double> &zs, double &z, double eps)
{
    if (zs.empty()) return -1;
    auto it_h = std::lower_bound(zs.begin(), zs.end(), z);
    if (it_h == zs.end()) {
        auto it_l = it_h;
        --it_l;
        if (z - *it_l < eps) return int(zs.size() - 1);
    } else if (it_h == zs.begin()) {
        if (*it_h - z < eps) return 0;
    } else {
        auto it_l = it_h;
        --it_l;
        double dist_l = z - *it_l;
        double dist_h = *it_h - z;
        if (std::min(dist_l, dist_h) < eps) { return (dist_l < dist_h) ? int(it_l - zs.begin()) : int(it_h - zs.begin()); }
    }
    return -1;
}

static bool has_non_zero_volume(const std::map<size_t, double>& volumes, const double eps = 1e-9)
{
    return std::any_of(volumes.begin(), volumes.end(), [eps](const auto& item) { return item.second > eps; });
}

static void collect_active_extruders(const std::map<size_t, double>& volumes, std::unordered_set<size_t>& out, const double eps = 1e-9)
{
    for (const auto& [extruder_id, volume] : volumes) {
        if (volume > eps)
            out.insert(extruder_id);
    }
}

// Merge "Flushed" into "Model" only when the plate truly behaves as a single-nozzle print:
// one active extruder in statistics, flush exists, and no support / wipe tower material.
static bool should_merge_flush_into_model(const PrintEstimatedStatistics& stats)
{
    const bool has_flush = has_non_zero_volume(stats.flush_per_filament);
    if (!has_flush)
        return false;

    std::unordered_set<size_t> active_extruders;
    collect_active_extruders(stats.total_volumes_per_extruder, active_extruders);
    // Fallback for incomplete totals.
    if (active_extruders.empty()) {
        collect_active_extruders(stats.model_volumes_per_extruder, active_extruders);
        collect_active_extruders(stats.flush_per_filament, active_extruders);
        collect_active_extruders(stats.support_volumes_per_extruder, active_extruders);
        collect_active_extruders(stats.wipe_tower_volumes_per_extruder, active_extruders);
    }

    const bool has_support    = has_non_zero_volume(stats.support_volumes_per_extruder);
    const bool has_wipe_tower = has_non_zero_volume(stats.wipe_tower_volumes_per_extruder);
    return active_extruders.size() == 1 && !has_support && !has_wipe_tower;
}

ColorRGBA BaseRenderer::Extrusions::Range::get_color_at(float value) const
{
    // Input value scaled to the colors range
    const float step = step_size();
    float _min = min;
    if(log_scale) {
        value = std::log(value);
        _min = std::log(min);
    }
    const float global_t = (step != 0.0f) ? std::max(0.0f, value - _min) / step : 0.0f; // lower limit of 0.0f

    const size_t color_max_idx = Range_Colors.size() - 1;

    // Compute the two colors just below (low) and above (high) the input value
    const size_t color_low_idx = std::clamp<size_t>(static_cast<size_t>(global_t), 0, color_max_idx);
    const size_t color_high_idx = std::clamp<size_t>(color_low_idx + 1, 0, color_max_idx);

    // Interpolate between the low and high colors to find exactly which color the input value should get
    return lerp(Range_Colors[color_low_idx], Range_Colors[color_high_idx], global_t - static_cast<float>(color_low_idx));
}

float BaseRenderer::Extrusions::Range::step_size() const {
if (log_scale)
    {
        float min_range = min;
        if (min_range == 0)
            min_range = 0.001f;
        return (std::log(max / min_range) / (static_cast<float>(Range_Colors.size()) - 1.0f));
    } else
    return (max - min) / (static_cast<float>(Range_Colors.size()) - 1.0f);
}

float BaseRenderer::Extrusions::Range::get_value_at_step(int step) const {
    if (!log_scale)
        return min + static_cast<float>(step) * step_size();
    else
    return std::exp(std::log(min) + static_cast<float>(step) * step_size());
    
}

void BaseRenderer::Marker::init(std::string filename)
{
    if (filename.empty()) {
        m_model.init_from(stilized_arrow(16, 1.5f, 3.0f, 0.8f, 3.0f));
    } else {
        m_model.init_from_file(filename);
    }
    m_model.set_color({ 1.0f, 1.0f, 1.0f, 0.5f });
}

const Slic3r::BoundingBoxf3& BaseRenderer::Marker::get_bounding_box() const { 
    return m_model.get_bounding_box();
}

void BaseRenderer::Marker::set_world_position(const Vec3f& position)
{
    m_world_position = position;
    m_world_transform = (Geometry::assemble_transform((position + m_z_offset * Vec3f::UnitZ()).cast<double>()) * Geometry::assemble_transform(m_model.get_bounding_box().size().z() * Vec3d::UnitZ(), { M_PI, 0.0, 0.0 })).cast<float>();
}

void BaseRenderer::Marker::set_world_offset(const Vec3f& offset) { 
    m_world_offset = offset; 
}

void BaseRenderer::Marker::update_curr_move(const GCodeProcessorResult::MoveVertex move) {
    m_curr_move = move;
}

//BBS: GUI refactor: add canvas size from parameters
void BaseRenderer::Marker::render(int canvas_width, int canvas_height, const EViewType& view_type, bool showMark)
{
    if (!m_visible)
        return;

    if (showMark) {
    GLShaderProgram* shader = wxGetApp().get_shader("gouraud_light");
    if (shader == nullptr)
        return;

    glsafe(::glEnable(GL_BLEND));
    glsafe(::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    shader->start_using();
    shader->set_uniform("emission_factor", 0.0f);

    const Camera& camera = wxGetApp().plater()->get_camera();
    const Transform3d& view_matrix = camera.get_view_matrix();
    const Transform3d model_matrix = m_world_transform.cast<double>();
    shader->set_uniform("view_model_matrix", view_matrix * model_matrix);
    shader->set_uniform("projection_matrix", camera.get_projection_matrix());
    const Matrix3d view_normal_matrix = view_matrix.matrix().block(0, 0, 3, 3) * model_matrix.matrix().block(0, 0, 3, 3).inverse().transpose();
    shader->set_uniform("view_normal_matrix", view_normal_matrix);

    m_model.render();

    shader->stop_using();

    glsafe(::glDisable(GL_BLEND));
    }

    static float last_window_width = 0.0f;
    size_t text_line = 0;
    static size_t last_text_line = 0;
    const ImU32 text_name_clr = m_is_dark ? IM_COL32(255, 255, 255, 0.88 * 255) : IM_COL32(38, 46, 48, 255);
    const ImU32   text_value_clr    = m_is_dark ? IM_COL32(255, 255, 255, 255) : IM_COL32(51, 51, 51, 255);

    ImGuiWrapper& imgui = *wxGetApp().imgui();
    //BBS: GUI refactor: add canvas size from parameters

    const float scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    float left_widgets_width = (90 + 300) * scale; // thumbs + gcode viewer
    ImGuiWindow* win = ImGui::FindWindowByName("gcode_legend");
    if (win) {
        left_widgets_width = std::max(win->Pos.x + win->SizeFull.x, left_widgets_width);
    }
    const float right_widgets_width = (215 + 10) * scale; // slicer buttons
    const float remaining_space     = canvas_width - left_widgets_width - right_widgets_width;
       
    imgui.set_next_window_pos(left_widgets_width + remaining_space / 2.0, static_cast<float>(canvas_height), ImGuiCond_Always, 0.5f, 1.0f);
    imgui.push_toolbar_style(m_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0, 2.0 * m_scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0 * m_scale, 3.0 * m_scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,  m_is_dark ? 0.0f : 1.0f);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          m_is_dark ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(78.0f / 255.0f, 89.0f / 255.0f, 105.0f / 255.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, m_is_dark ? ImVec4(219.0f / 255.0f, 219.0f / 255.0f, 219.0f / 255.0f, 1.0f) :
                                                     ImVec4(78.0f / 255.0f, 89.0f / 255.0f, 105.0f / 255.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(225.0f / 255.0f, 228.0f / 255.0f, 233.0f / 255.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, m_is_dark ? ImVec4(34.0f / 255.0f, 34.0f / 255.0f, 34.0f / 255.0f, 0.6f) :
                                                     ImVec4(1.0f, 1.0f, 1.0f, 0.6f));

    const ImGuiStyle& st = ImGui::GetStyle();
    const float target_h = (ImGui::GetTextLineHeight() + st.WindowPadding.y * 1.4f) * 2.0f;
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, target_h), ImVec2(1e9f, target_h));
    imgui.begin(std::string("ExtruderPosition"),
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    // Use default font size (revert any scaling)
    ImGui::SetWindowFontScale(1.1f);

    std::vector<std::string> position_row_texts;
    position_row_texts.reserve(6);
    float       column_width = 0.0f;
    float       column_step  = 0.0f;
    const float window_padding = st.WindowPadding.x;
    const float item_spacing   = imgui.get_item_spacing().x;

    {
        //BBS: minus the plate offset when show tool position
        PartPlateList& partplate_list = wxGetApp().plater()->get_partplate_list();
        PartPlate*     plate          = partplate_list.get_curr_plate();
        const Vec3f    position       = m_world_position + m_world_offset;

        std::string x = ImGui::ColorMarkerStart + std::string("X: ") + ImGui::ColorMarkerEnd;
        std::string y = ImGui::ColorMarkerStart + std::string("Y: ") + ImGui::ColorMarkerEnd;
        std::string z = ImGui::ColorMarkerStart + std::string("Z: ") + ImGui::ColorMarkerEnd;
        std::string height = ImGui::ColorMarkerStart + _u8L("Height: ") + ImGui::ColorMarkerEnd;
        std::string width = ImGui::ColorMarkerStart + _u8L("Width: ") + ImGui::ColorMarkerEnd;
        std::string speed = ImGui::ColorMarkerStart + _u8L("Speed: ") + ImGui::ColorMarkerEnd;
        std::string flow = ImGui::ColorMarkerStart + _u8L("Flow: ") + ImGui::ColorMarkerEnd;
        std::string layer_time = ImGui::ColorMarkerStart + _u8L("Layer Time: ") + ImGui::ColorMarkerEnd;
        std::string fanspeed = ImGui::ColorMarkerStart + _u8L("Fan: ") + ImGui::ColorMarkerEnd;
        std::string temperature = ImGui::ColorMarkerStart + _u8L("Temperature: ") + ImGui::ColorMarkerEnd;
        std::string acceleration = ImGui::ColorMarkerStart + _u8L("Acceleration") + ": " + ImGui::ColorMarkerEnd;

        const float base_column_width = imgui.calc_text_size(std::string_view{"X: 000.000000  "}).x;
        char        buf[1024];

        sprintf(buf, "%s%.3f", x.c_str(), position.x() - plate->get_origin().x());
        position_row_texts.emplace_back(buf);
        sprintf(buf, "%s%.3f", y.c_str(), position.y() - plate->get_origin().y());
        position_row_texts.emplace_back(buf);
        sprintf(buf, "%s%.3f", z.c_str(), position.z());
        position_row_texts.emplace_back(buf);
        sprintf(buf, "%s%.0f", speed.c_str(), m_curr_move.feedrate);
        position_row_texts.emplace_back(buf);

        switch (view_type) {
        case EViewType::Height: {
            sprintf(buf, "%s%.2f", height.c_str(), m_curr_move.height);
            position_row_texts.emplace_back(buf);
            break;
        }
        case EViewType::Width: {
            sprintf(buf, "%s%.2f", width.c_str(), m_curr_move.width);
            position_row_texts.emplace_back(buf);
            break;
        }
        case EViewType::VolumetricRate: {
            sprintf(buf, "%s%.2f", flow.c_str(), m_curr_move.volumetric_rate());
            position_row_texts.emplace_back(buf);
            break;
        }
        case EViewType::FanSpeed: {
            sprintf(buf, "%s%.0f", fanspeed.c_str(), m_curr_move.fan_speed);
            position_row_texts.emplace_back(buf);
            break;
        }
        case EViewType::Temperature: {
            sprintf(buf, "%s%.0f", temperature.c_str(), m_curr_move.temperature);
            position_row_texts.emplace_back(buf);
            break;
        }
        case EViewType::LayerTime:
        case EViewType::LayerTimeLog: {
            sprintf(buf, "%s%.1f", layer_time.c_str(), m_curr_move.layer_duration);
            position_row_texts.emplace_back(buf);
            break;
        }
        case EViewType::Acceleration:
            sprintf(buf, "%s%.0f", acceleration.c_str(), m_curr_move.acceleration);
            position_row_texts.emplace_back(buf);
            break;
        default:
            break;
        }

        column_width = base_column_width;
        for (const std::string& text : position_row_texts)
            column_width = std::max(column_width, ImGui::CalcTextSize(text.c_str()).x);

        column_step = column_width + item_spacing;

        const float first_row_width = window_padding * 2.0f +
                                      static_cast<float>(position_row_texts.size()) * column_width +
                                      (static_cast<float>(position_row_texts.size()) - 1.0f) * item_spacing;
        ImGui::SetWindowSize(ImVec2(first_row_width, target_h));
    }


    {
        ImVec2 win_pos = ImGui::GetWindowPos();
        ImVec2 win_sz  = ImGui::GetWindowSize();
        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        const ImGuiStyle& style = ImGui::GetStyle();
        
        const ImVec4 sh_rgb = ImVec4(118.0f/255.0f, 142.0f/255.0f, 171.0f/255.0f, 1.0f);
       
        const float alphas[8] = { 0.050f, 0.040f, 0.032f, 0.026f, 0.020f, 0.014f, 0.010f, 0.006f };
        
        const float steps[8]  = { 2.00f, 1.60f, 1.20f, 0.90f, 0.70f, 0.50f, 0.35f, 0.20f };
        for (int i = 0; i < 8; ++i) {
            float spread = steps[i] * m_scale;
            float round  = style.WindowRounding + spread;

            float off_x = spread * 0.10f;
            float off_y = spread * 0.80f;
            ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(sh_rgb.x, sh_rgb.y, sh_rgb.z, alphas[i]));
            bg->AddRectFilled(ImVec2(win_pos.x - off_x, win_pos.y + off_y),
                              ImVec2(win_pos.x + win_sz.x + off_x, win_pos.y + win_sz.y + spread + off_y),
                              col, round);
        }
    }
    ImGui::AlignTextToFramePadding();

    {
        const ImGuiStyle& st        = ImGui::GetStyle();
        float             content_h = ImGui::GetWindowHeight() - st.WindowPadding.y * 2.0f;
        float             line_h    = ImGui::GetFrameHeight();
        float             y_offset  = std::max(0.0f, (content_h - line_h) * 0.5f);
        ImGui::SetCursorPosY(st.WindowPadding.y + y_offset);
    }

    if (true) {
        auto draw_value = [&](const std::string& text) {
            ImGui::PushItemWidth(column_width);
            imgui.text(text.c_str());
        };

        for (size_t idx = 0; idx < position_row_texts.size(); ++idx) {
            if (idx > 0)
                ImGui::SameLine(window_padding + static_cast<float>(idx) * column_step);
            draw_value(position_row_texts[idx]);
        }
        ImGui::NewLine();
        text_line = 1;
    }
    // else {
    //     sprintf(buf, "%s%.3f", x.c_str(), position.x() - plate->get_origin().x());
    //     imgui.text(buf);

    //     ImGui::SameLine();
    //     sprintf(buf, "%s%.3f", y.c_str(), position.y() - plate->get_origin().y());
    //     imgui.text(buf);

    //     ImGui::SameLine();
    //     sprintf(buf, "%s%.3f", z.c_str(), position.z());
    //     imgui.text(buf);

    //     text_line = 1;
    // }

    // force extra frame to automatically update window size
    float window_width = ImGui::GetWindowWidth();
    if (window_width != last_window_width || text_line != last_text_line) {
        last_window_width = window_width;
        last_text_line = text_line;
#if ENABLE_ENHANCED_IMGUI_SLIDER_FLOAT
        imgui.set_requires_extra_frame();
#else
        wxGetApp().plater()->get_current_canvas3D()->set_as_dirty();
        wxGetApp().plater()->get_current_canvas3D()->request_extra_frame();
#endif // ENABLE_ENHANCED_IMGUI_SLIDER_FLOAT
    }


    ImGui::SetWindowFontScale(1.0f);
    imgui.end();
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(4);
    imgui.pop_toolbar_style();
}

void BaseRenderer::GCodeWindow::load_gcode(const std::string& filename, const std::vector<size_t> &lines_ends)
{
    assert(! m_file.is_open());
    if (m_file.is_open())
        return;

    m_filename   = filename;
    m_lines_ends = lines_ends;

    m_selected_line_id = 0;
    m_last_lines_size = 0;

    try
    {
        m_file.open(boost::filesystem::path(m_filename));
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": mapping file " << m_filename;
    }
    catch (...)
    {
        BOOST_LOG_TRIVIAL(error) << "Unable to map file " << m_filename << ". Cannot show G-code window.";
        reset();
    }
}

void BaseRenderer::GCodeWindow::reset()
{
    stop_mapping_file();
    m_lines_ends.clear();
    m_lines_ends.shrink_to_fit();
    m_lines.clear();
    m_lines.shrink_to_fit();
    m_filename.clear();
    m_filename.shrink_to_fit();
}

void BaseRenderer::GCodeWindow::renderGcode(uint64_t curr_line_id, int canvas_width, int canvas_height, bool isReduceHeight)
{
    if (!wxGetApp().show_gcode_window() 
        || m_filename.empty() 
        || m_lines_ends.empty())
        return;

    if (!m_file.is_open() || m_file.data() == nullptr) {
        try {
            if (m_file.is_open())
                m_file.close();
            m_file.open(boost::filesystem::path(m_filename));
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": remapping file " << m_filename;
        }
        catch (...) {
            BOOST_LOG_TRIVIAL(error) << "Unable to map file " << m_filename << ". Cannot show G-code window.";
            return;
        }
    }
    if (m_file.data() == nullptr)
        return;

    if (curr_line_id == 0)
        curr_line_id = 1;

    auto pos = ImGui::GetCursorPos();
    auto siz = ImGui::GetWindowSize();

    float wnd_height ;
    float scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    auto screenPoxY = ImGui::GetCursorScreenPos().y;
    float canvas_h = wxGetApp().plater()->get_current_canvas3D()->get_canvas_size().get_height();
    if (isReduceHeight)
    {
        wnd_height = canvas_h - screenPoxY - GCODE_REDUCE_HEIGHT*scale;
    }
    else
    {
        wnd_height = canvas_h - screenPoxY;
    }

    const float zoom = 0.8f;
    //float wnd_height = siz.y - pos.y;
    const float text_height = ImGui::CalcTextSize("0").y* zoom;
    const ImGuiStyle& style = ImGui::GetStyle();
    uint64_t lines_count = static_cast<uint64_t>((wnd_height - 2.0f * style.WindowPadding.y + style.ItemSpacing.y) / (text_height + style.ItemSpacing.y));
    float sc = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    if (lines_count == 0) {
        ImVec2 size = DispConfig().getWindowSize(DispConfig::e_wt_gcode,sc);
        if (size.y < 0.0f) {
            ImVec2 cur_pos = ImGui::GetCursorScreenPos();
            wnd_height = canvas_height - cur_pos.y;

            lines_count = static_cast<uint64_t>((wnd_height - 2.0f * style.WindowPadding.y + style.ItemSpacing.y) / (text_height + style.ItemSpacing.y));
            if (lines_count == 0)
                return;
        }
    }

    // visible range
    const uint64_t half_lines_count = lines_count / 2;
    const uint64_t total_lines = static_cast<uint64_t>(m_lines_ends.size());
    uint64_t start_id = (curr_line_id > half_lines_count) ? curr_line_id - half_lines_count : 1;
    uint64_t end_id = start_id + lines_count - 1;
    if (end_id > total_lines) {
        end_id = total_lines;
        const uint64_t visible_lines = std::min<uint64_t>(lines_count, total_lines);
        start_id = end_id - visible_lines + 1;
    }
    if (start_id == 0 || start_id > end_id)
        return;

    // Orca: truncate long lines(>55 characters), add "..." at the end
    auto update_lines = [this](uint64_t start_id, uint64_t end_id) {
        std::vector<Line> ret;
        ret.reserve(end_id - start_id + 1);
        for (uint64_t id = start_id; id <= end_id; ++id) {
            // read line from file
            const size_t start = id == 1 ? 0 : m_lines_ends[id - 2];
            const size_t original_len = m_lines_ends[id - 1] - start;
            const size_t file_size = m_file.size();
            if (start >= file_size) {
                ret.push_back({ {}, {}, {} });
                continue;
            }
            const size_t len = std::min(original_len, file_size - start);

            // fix bug[10385] task[2520] : 超长，不截断，改为另起一行
            std::string  gline(m_file.data() + start, len);

            std::string command, parameters, comment;
            // extract comment
            std::vector<std::string> tokens;
            boost::split(tokens, gline, boost::is_any_of(";"), boost::token_compress_on);
            command = tokens.front();
            if (tokens.size() > 1)
                comment = ";" + tokens.back();

            // extract gcode command and parameters
            if (!command.empty()) {
                boost::split(tokens, command, boost::is_any_of(" "), boost::token_compress_on);
                command = tokens.front();
                if (tokens.size() > 1) {
                    for (size_t i = 1; i < tokens.size(); ++i) {
                        parameters += " " + tokens[i];
                    }
                }
            }
            ret.push_back({ command, parameters, comment });
        }
        return ret;
    };

    // updates list of lines to show, if needed
    if (m_selected_line_id != curr_line_id || m_last_lines_size != end_id - start_id + 1) {
        try
        {
            *const_cast<std::vector<Line>*>(&m_lines) = update_lines(start_id, end_id);
        }
        catch (...)
        {
            BOOST_LOG_TRIVIAL(error) << "Error while loading from file " << m_filename << ". Cannot show G-code window.";
            return;
        }
        *const_cast<uint64_t*>(&m_selected_line_id) = curr_line_id;
        *const_cast<size_t*>(&m_last_lines_size) = m_lines.size();
    }
    // line number's column width
    const float id_width = ImGui::CalcTextSize(std::to_string(end_id).c_str()).x;
    // center the text in the window by pushing down the first line
    const float f_lines_count = static_cast<float>(lines_count);
    ImGui::SetCursorPosY(0.5f * (wnd_height - f_lines_count * text_height - (f_lines_count - 1.0f) * style.ItemSpacing.y)+ pos.y);
    static const ImVec4 LINE_NUMBER_COLOR = ImGuiWrapper::COL_ORANGE_LIGHT;
    static const ImVec4 SELECTION_RECT_COLOR = ImGuiWrapper::COL_ORANGE_DARK;
    static const ImVec4 COMMAND_COLOR = { 0.8f, 0.8f, 0.0f, 1.0f };
    const ImVec4 PARAMETERS_COLOR = m_is_dark ? ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f } : ImVec4{ 0.0f, 0.0f, 0.0f, 1.0f };
    const ImVec4 COMMENT_COLOR = m_is_dark ? ImVec4{ 0.7f, 0.7f, 0.7f, 1.0f } : ImVec4{ 0.0f, 0.0f, 0.0f, 0.7f };

    ImGuiWrapper& imgui = *wxGetApp().imgui();
    for (uint64_t id = start_id; id <= end_id; ++id) {
        const Line& line = m_lines[id - start_id];

        // rect around the current selected line
        if (id == curr_line_id) {
            //BBS: GUI refactor: move to right
            const float pos_y = ImGui::GetCursorScreenPos().y;
            const float pos_x = ImGui::GetCursorScreenPos().x;
            const float half_ItemSpacing_y = 0.5f * style.ItemSpacing.y;
            const float half_ItemSpacing_x = 0.5f * style.ItemSpacing.x;
            ImGui::GetWindowDrawList()->AddRect({ pos_x, pos_y - half_ItemSpacing_y },
                { pos_x+ImGui::GetCurrentWindow()->Size.x - half_ItemSpacing_x- 2*style.WindowPadding.x ,
                pos_y + text_height + half_ItemSpacing_y },
                ImGui::GetColorU32(SELECTION_RECT_COLOR));
        }

        // render line number
        const std::string id_str = std::to_string(id);
        // spacer to right align text
        ImGui::Dummy({ id_width - ImGui::CalcTextSize(id_str.c_str()).x* zoom, text_height });
        ImGui::SameLine(0.0f, 0.0f);
        float id_start_x = ImGui::GetCursorPosX() + id_width; // 行号右边界

        ImGui::PushStyleColor(ImGuiCol_Text, LINE_NUMBER_COLOR);
        DispConfig().boldText(id_str, zoom);
        //imgui.text(id_str);
        ImGui::PopStyleColor();

        if (!line.command.empty() || !line.comment.empty())
            ImGui::SameLine();

        // render command
        if (!line.command.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, COMMAND_COLOR);
            DispConfig().boldText(line.command, zoom);
            ImGui::PopStyleColor();
        }

        // render parameters
        if (!line.parameters.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, PARAMETERS_COLOR);

            // 计算行号和 command 的宽度
            float id_width_actual  = ImGui::CalcTextSize(std::to_string(id).c_str()).x * zoom;
            float command_width    = ImGui::CalcTextSize(line.command.c_str()).x * zoom;
            float parameters_width = ImGui::CalcTextSize(line.parameters.c_str()).x * zoom;

            float total_width  = id_width_actual + command_width + parameters_width + 3 * style.ItemSpacing.x;
            float window_width = ImGui::GetContentRegionAvail().x;

            // fix bug[10385] task[2520] : 超长，不截断，改为另起一行
            if (total_width < window_width) {
                ImGui::SameLine(0.0f, 0.0f);
                DispConfig().boldText(line.parameters, zoom);
            } else {
                // 设置光标位置对齐行号
                std::string s_param = line.parameters;
                if (!s_param.empty() && s_param.front() == ' ') {
                    s_param.erase(0, 1); // 去掉第一个空格
                }
                ImGui::SetCursorPosX(id_start_x ); // 对齐行号右边界
                float wrap_x = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x ;
                DispConfig().boldTextWrapped(s_param, zoom, wrap_x);
            }

            ImGui::PopStyleColor();
        }

        // render comment
        if (!line.comment.empty()) {
            if (!line.command.empty())
                ImGui::SameLine(0.0f, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, COMMENT_COLOR);
            DispConfig().boldText(line.comment, zoom);
            ImGui::PopStyleColor();
        }
    }
}

void BaseRenderer::GCodeWindow::stop_mapping_file()
{
    //BBS: add log to trace the gcode file issue
    if (m_file.is_open()) {
        m_file.close();
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << ": finished mapping file " << m_filename;
    }
}

class GcodeHelper
{
    DispConfig config;ImGuiWrapper* imgui;
    const std::vector<unsigned char>& m_extruder_ids;
    const PrintEstimatedStatistics& m_print_statistics;
    const PrintEstimatedStatistics::Mode& time_mode;
    const std::vector<float>& m_filament_diameters;
    const std::vector<float>& m_filament_densities;
    const std::vector<ExtrusionRole>& m_roles;
    const std::vector<EMoveType>& options_items;
    BaseRenderer::Extrusions& m_extrusions;
    BaseRenderer& m_renderer;
    const PrintStatistics& ps;
    bool imperial_units;char buf[512];
    float unit_conver,koef, icon_size, m_scale, window_padding;
    ImTextureID m_fold_icon_texture;

public:
    enum EItemType
    {
        Rect,
        Circle,
        Hexagon,
        Line,
        None
    };
    enum ColumnData {
        Model = 1,
        Flushed = 2,
        WipeTower = 4,
        Support = 1 << 3,
    };

    GcodeHelper(
        float scal,int md, 
        std::vector<EMoveType>&em,
        const std::vector<ExtrusionRole>&ro,
        BaseRenderer& renderer,
        BaseRenderer::Extrusions& ex,
        const PrintEstimatedStatistics& st,
        const std::vector<unsigned char>&ids,
        const std::vector<float>& di, 
        const std::vector<float>&de,
        ImTextureID fold_icon_texture)
        :m_filament_diameters(di), m_filament_densities(de), m_scale(scal), m_fold_icon_texture(fold_icon_texture)
        ,m_print_statistics(st),time_mode(st.modes[md]),m_extruder_ids(ids)
        , m_roles(ro), m_renderer(renderer) , m_extrusions(ex), options_items(em)
        ,ps(wxGetApp().plater()->get_partplate_list().get_current_fff_print().print_statistics()){
             imgui = wxGetApp().imgui();
             imperial_units = wxGetApp().app_config->get("use_inches") == "1";
             config = DispConfig();
             window_padding = 10 * m_scale;
             icon_size = ImGui::GetTextLineHeight();
             unit_conver = imperial_units ? GizmoObjectManipulation::oz_to_g : 1;
             koef = imperial_units ? GizmoObjectManipulation::in_to_mm : 1000.0;
    }

    DispConfig::WindowConfig prepare(bool fold, float contentWidth) {
        std::string btn_name = fold ? ">>" : "<<";
        auto wsz = config.getWindowSize(DispConfig::e_wt_gcode, wxGetApp().plater()->get_current_canvas3D()->get_scale());

        GLCanvas3D* canvas = wxGetApp().plater()->get_current_canvas3D();

        ImVec2 pos = canvas->get_printer_objects_panel_pos();
        ImVec2 size = canvas->get_printer_objects_panel_size();

        ImVec2 bias = ImVec2(pos.x + 1.0f * m_scale, pos.y + size.y + 8);

        ImGui::SetNextWindowPos(bias, ImGuiCond_Always);

        float min_width = std::max(wsz.x, 280.0f * m_scale);
        if (fold || contentWidth < (wsz.x - 2 * window_padding))
        {
            ImGui::SetNextWindowSize(ImVec2(min_width, wsz.y));
        }
        else
        {         
            float target_width = std::max(contentWidth, 300.0f);
            const float max_window_width = std::max(min_width, ImGui::GetIO().DisplaySize.x - bias.x - 2.0f * m_scale);
            target_width = std::min(target_width, max_window_width);
            auto size = ImVec2(target_width, -1.0f);
            ImGui::SetNextWindowSize(size);
        }

        DispConfig::WindowConfig wcfg;
        wcfg.padding = { window_padding,window_padding };
        wcfg.bgalpha = 0.2;
        wcfg.txt = DispConfig::e_ct_white;
        return wcfg;
    }

    void initColorData() {

        // get used filament (meters and grams) from used volume in respect to the active extruder
        auto get_used_filament_from_volume = [&](double volume, int extruder_id) {
            double koef = imperial_units ? 1.0 / GizmoObjectManipulation::in_to_mm : 0.001;
            std::pair<double, double> ret = { koef * volume / (PI * sqr(0.5 * m_filament_diameters[extruder_id])),
                                                volume * m_filament_densities[extruder_id] * 0.001 };
            return ret;
        };

        for (size_t extruder_id : m_extruder_ids) {
            if (m_print_statistics.model_volumes_per_extruder.find(extruder_id) == m_print_statistics.model_volumes_per_extruder.end()) {
                model_used_filaments_m.push_back(0.0);
                model_used_filaments_g.push_back(0.0);
            }
            else {
                double volume = m_print_statistics.model_volumes_per_extruder.at(extruder_id);
                auto [model_used_filament_m, model_used_filament_g] = get_used_filament_from_volume(volume, extruder_id);
                model_used_filaments_m.push_back(model_used_filament_m);
                model_used_filaments_g.push_back(model_used_filament_g);
                total_model_used_filament_m += model_used_filament_m;
                total_model_used_filament_g += model_used_filament_g;
                displayed_columns |= ColumnData::Model;
            }
        }

        for (size_t extruder_id : m_extruder_ids) {
            if (m_print_statistics.wipe_tower_volumes_per_extruder.find(extruder_id) == m_print_statistics.wipe_tower_volumes_per_extruder.end()) {
                wipe_tower_used_filaments_m.push_back(0.0);
                wipe_tower_used_filaments_g.push_back(0.0);
            }
            else {
                double volume = m_print_statistics.wipe_tower_volumes_per_extruder.at(extruder_id);
                auto [wipe_tower_used_filament_m, wipe_tower_used_filament_g] = get_used_filament_from_volume(volume, extruder_id);
                wipe_tower_used_filaments_m.push_back(wipe_tower_used_filament_m);
                wipe_tower_used_filaments_g.push_back(wipe_tower_used_filament_g);
                total_wipe_tower_used_filament_m += wipe_tower_used_filament_m;
                total_wipe_tower_used_filament_g += wipe_tower_used_filament_g;
                displayed_columns |= ColumnData::WipeTower;
            }
        }

        for (size_t extruder_id : m_extruder_ids) {
            if (m_print_statistics.flush_per_filament.find(extruder_id) == m_print_statistics.flush_per_filament.end()) {
                flushed_filaments_m.push_back(0.0);
                flushed_filaments_g.push_back(0.0);
            }
            else {
                double volume = m_print_statistics.flush_per_filament.at(extruder_id);
                auto [flushed_filament_m, flushed_filament_g] = get_used_filament_from_volume(volume, extruder_id);
                flushed_filaments_m.push_back(flushed_filament_m);
                flushed_filaments_g.push_back(flushed_filament_g);
                total_flushed_filament_m += flushed_filament_m;
                total_flushed_filament_g += flushed_filament_g;
                displayed_columns |= ColumnData::Flushed;
            }
        }

        for (size_t extruder_id : m_extruder_ids) {
            if (m_print_statistics.support_volumes_per_extruder.find(extruder_id) == m_print_statistics.support_volumes_per_extruder.end()) {
                support_used_filaments_m.push_back(0.0);
                support_used_filaments_g.push_back(0.0);
            }
            else {
                double volume = m_print_statistics.support_volumes_per_extruder.at(extruder_id);
                auto [used_filament_m, used_filament_g] = get_used_filament_from_volume(volume, extruder_id);
                support_used_filaments_m.push_back(used_filament_m);
                support_used_filaments_g.push_back(used_filament_g);
                total_support_used_filament_m += used_filament_m;
                total_support_used_filament_g += used_filament_g;
                displayed_columns |= ColumnData::Support;
            }
        }
    }
    
    bool showTitle(bool fold) {

        float view_scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();

        float title_y = ImGui::GetCursorPosY();
        config.boldText(_u8L("G-code Preview"), 1.2);
        ImGui::SameLine();

        float arrow_size = 24.0f * view_scale;
        ImVec2 btn_size{arrow_size, arrow_size};
        float   btn_y_offset = fold ? 2.0f * view_scale : -2.0f * view_scale;

        float right       = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
        float btn_start_x = right - btn_size.x;
        float baseline_y  = title_y + (ImGui::GetTextLineHeight() - btn_size.y) * 0.5f + btn_y_offset;

        ImVec2 btn_pos_local = ImVec2(btn_start_x, baseline_y);
        ImGui::SetCursorPos(btn_pos_local);

        ImVec2 btn_pos = ImGui::GetCursorScreenPos();
        bool pressed = ImGui::InvisibleButton("##gcode_fold_btn", btn_size);
        const bool hovered = ImGui::IsItemHovered();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32 arrow_tint = wxGetApp().dark_mode() ? IM_COL32_WHITE : IM_COL32(65, 65, 65, 255);
        const float icon_draw_size = 22.0f * view_scale;
        const float icon_inset = (btn_size.x - icon_draw_size) * 0.5f;
        const ImVec2 icon_min(btn_pos.x + icon_inset, btn_pos.y + icon_inset);
        const ImVec2 icon_max(icon_min.x + icon_draw_size, icon_min.y + icon_draw_size);

        if (m_fold_icon_texture != nullptr) {
            const float uv_top = fold ? 1.0f : 0.0f;
            const float uv_bottom = fold ? 0.0f : 1.0f;
            dl->AddImage(m_fold_icon_texture, icon_min, icon_max,
                ImVec2(0.0f, uv_top), ImVec2(1.0f, uv_bottom), arrow_tint);
        }
        if (hovered) {
            const ImVec2 btn_max(btn_pos.x + btn_size.x, btn_pos.y + btn_size.y);
            dl->AddRect(btn_pos, btn_max, IM_COL32(21, 191, 89, 255),
                6.0f * view_scale, ImDrawFlags_RoundCornersAll, 1.0f * view_scale);
        }

        if (pressed) {
            fold = !fold;
            wxGetApp().imgui()->set_requires_extra_frame();
        }
        if (!fold)
        {
//             ImGui::Dummy(ImVec2(0, 5));
//             auto printer_name = Slic3r::GUI::wxGetApp().preset_bundle->printers.get_selected_preset_name();
//             ::sprintf(buf, "%s:%s", _u8L("Current Machine").c_str(), (char*)printer_name.c_str());
//             ImGui::Text(buf);

            ImGui::Dummy(ImVec2(0, 3));
            auto timestr = short_time(get_time_dhms(print_time_for_display(time_mode)));
            ::sprintf(buf, "%s:%s", _u8L("Printing Time").c_str(), (char*)timestr.c_str());
            ImGui::Text("%s", buf);

            ImGui::SameLine(160 * view_scale);
            ::sprintf(buf, imperial_units ? " %s:%.2f oz" : " %s:%.2f g", _u8L("Material Wt").c_str(), ps.total_weight / unit_conver);
            imgui->text(buf);

            ImGui::Dummy(ImVec2(0, 3));
            ::sprintf(buf, imperial_units ? "%s:%.2f in" : "%s:%.2f m", _u8L("Material Length").c_str(), ps.total_used_filament / koef);
            imgui->text(buf);

            ImGui::SameLine(160 * view_scale);
            ::sprintf(buf, "%s:%.2f", _u8L("Material Cost").c_str(), ps.total_cost);
            imgui->text(buf);
            ImGui::Dummy(ImVec2(0, 5));
            if (!s_preview_section_fold && !wxGetApp().easy_mode())
                ImGui::Separator();
        } else if (fold) {
            ImGui::Dummy(ImVec2(0, 4));
        }
        return fold;
    }

    bool showOption(bool& showmark, bool& showbed, bool& showcolor) {

        float view_scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();

        ImGui::Dummy(ImVec2(0, 5));
        if (s_preview_section_fold) {
            std::string folded_title = _u8L("Details");
            ImVec2 sz = ImGui::CalcTextSize(folded_title.c_str());
            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiWrapper::COL_CREALITY);
            ImVec4 transparent(0, 0, 0, 0);
            ImGui::PushStyleColor(ImGuiCol_Header, transparent);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, transparent);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, transparent);
            if (ImGui::Selectable(folded_title.c_str(), false, 0, sz)) {
                s_preview_section_fold = false;
                wxGetApp().imgui()->set_requires_extra_frame();
            }
            ImGui::PopStyleColor(3);
            ImRect txt_rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            ImGui::GetWindowDrawList()->AddLine(ImVec2(txt_rect.Min.x, txt_rect.Max.y - 1.0f),
                                                ImVec2(txt_rect.Max.x, txt_rect.Max.y - 1.0f),
                                                ImGui::GetColorU32(ImGuiCol_Text), 1.0f);
            ImGui::PopStyleColor();
            ImGui::Dummy(ImVec2(0, 5));
            return true;
        }

        std::string title       = _u8L("Show in Preview");
        std::string toggle_txt  = _u8L("Collapse");
        ImVec2       title_size = ImGui::CalcTextSize(title.c_str());
        ImVec2       toggle_sz  = ImGui::CalcTextSize(toggle_txt.c_str());
        float        line_y     = ImGui::GetCursorPosY();

        config.boldText(title, 1.2);
        ImGui::SameLine();
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - toggle_sz.x, line_y));
        ImVec4 transparent(0, 0, 0, 0);
        ImGui::PushStyleColor(ImGuiCol_Header, transparent);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, transparent);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, transparent);
        if (ImGui::Selectable(toggle_txt.c_str(), false, 0, toggle_sz)) {
            s_preview_section_fold = true;
            wxGetApp().imgui()->set_requires_extra_frame();
        }
        ImGui::PopStyleColor(3);
        ImRect txt_rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImGui::GetWindowDrawList()->AddLine(ImVec2(txt_rect.Min.x, txt_rect.Max.y - 1.0f), ImVec2(txt_rect.Max.x, txt_rect.Max.y - 1.0f),
                                            ImGui::GetColorU32(ImGuiCol_Text), 1.0f);
        DispConfig::ButtonConfig cfg;
        cfg.border = 1;

        ImGui::Dummy(ImVec2(0, 5));
        if (s_preview_section_fold) {
            return true;
        }

        ImGuiWindow* window            = ImGui::GetCurrentWindow();
        float        prev_font_scale   = window->FontWindowScale;
        float        enlarged_font     = prev_font_scale * 1.1f;
        ImVec2       enlarged_padding  = ImVec2(4.0f * view_scale, 3.0f * view_scale);
        ImGui::SetWindowFontScale(enlarged_font);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, enlarged_padding);
        showbed = config.checkBox(_u8L("Print Platform"), showbed, cfg);
        ImGui::SameLine();
        showmark = config.checkBox(_u8L("Nozzle"), showmark, cfg);
        ImGui::PopStyleVar(1);
        ImGui::SetWindowFontScale(prev_font_scale);

        // capsule toggle: Color Show (left) / G-code (right)
        ImGui::Dummy(ImVec2(0.0f, 4.0f * view_scale));
        ImVec2 pill_pos   = ImGui::GetCursorScreenPos();
        float  pill_h     = 30.0f * view_scale;
        float  pill_w     = 200.0f * view_scale;
        float  rounding   = pill_h * 0.5f;
        ImVec2 pill_size  = {pill_w, pill_h};
        ImVec2 left_size  = {pill_w * 0.5f, pill_h};
        ImVec2 right_size = left_size;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        bool   is_dark        = wxGetApp().dark_mode();
        ImVec4 inactive_bg    = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        ImVec4 inactive_border= is_dark ? ImVec4(0.30f, 0.33f, 0.36f, 1.0f) : ImVec4(0.82f, 0.86f, 0.90f, 1.0f);
        ImVec4 active_bg      = ImGuiWrapper::COL_CREALITY;
        ImVec4 active_text    = ImVec4(1, 1, 1, 1);
        ImVec4 inactive_text  = is_dark ? ImVec4(0.78f, 0.80f, 0.82f, 1.0f) : ImVec4(0.36f, 0.36f, 0.40f, 1.0f);

        // pill border
        draw_list->AddRectFilled(pill_pos, ImVec2(pill_pos.x + pill_w, pill_pos.y + pill_h),
                                 ImGui::ColorConvertFloat4ToU32(inactive_bg), rounding);
        draw_list->AddRect(pill_pos, ImVec2(pill_pos.x + pill_w, pill_pos.y + pill_h),
                           ImGui::ColorConvertFloat4ToU32(inactive_border), rounding, 0, 1.0f);

        // left (Color Show)
        bool left_active  = showcolor;
        ImVec2 left_min   = pill_pos;
        ImVec2 left_max   = ImVec2(pill_pos.x + left_size.x, pill_pos.y + left_size.y);
        if (left_active) {
            draw_list->AddRectFilled(left_min, left_max, ImGui::ColorConvertFloat4ToU32(active_bg), rounding,
                                     ImDrawFlags_RoundCornersLeft);
        }
        const float text_pad = 8.0f * view_scale;
        std::string left_full_txt = _u8L("Color Show");
        std::string left_draw_txt = left_full_txt;
        bool left_truncated = false;
        bool bold_pushed = wxGetApp().imgui()->push_bold_font();
        {
            const float max_text_w = left_size.x - 1.0f * text_pad;
            if (max_text_w > 0.0f) {
                left_draw_txt  = ImGuiWrapper::trunc(left_full_txt, max_text_w, "...");
                left_truncated = (left_draw_txt != left_full_txt);
            } else {
                left_draw_txt  = "...";
                left_truncated = true;
            }
            ImVec2 left_txt_sz  = ImGui::CalcTextSize(left_draw_txt.c_str());
            ImVec2 left_txt_pos = ImVec2(left_min.x + (left_size.x - left_txt_sz.x) * 0.5f,
                                         left_min.y + (left_size.y - left_txt_sz.y) * 0.5f);
            draw_list->PushClipRect(left_min, left_max, true);
            draw_list->AddText(left_txt_pos, ImGui::ColorConvertFloat4ToU32(left_active ? active_text : inactive_text), left_draw_txt.c_str());
            draw_list->PopClipRect();
        }
        if (bold_pushed)
            wxGetApp().imgui()->pop_bold_font();

        // right (G-code)
        bool right_active = !showcolor;
        ImVec2 right_min  = ImVec2(pill_pos.x + left_size.x, pill_pos.y);
        ImVec2 right_max  = ImVec2(pill_pos.x + left_size.x + right_size.x, pill_pos.y + right_size.y);
        if (right_active) {
            draw_list->AddRectFilled(right_min, right_max, ImGui::ColorConvertFloat4ToU32(active_bg), rounding,
                                     ImDrawFlags_RoundCornersRight);
        }
        std::string right_txt = _u8L("G-Code");
        ImVec2 right_txt_sz   = ImGui::CalcTextSize(right_txt.c_str());
        ImVec2 right_txt_pos  = ImVec2(right_min.x + (right_size.x - right_txt_sz.x) * 0.5f,
                                       right_min.y + (right_size.y - right_txt_sz.y) * 0.5f);
        bold_pushed = wxGetApp().imgui()->push_bold_font();
        draw_list->AddText(right_txt_pos, ImGui::ColorConvertFloat4ToU32(right_active ? active_text : inactive_text), right_txt.c_str());
        if (bold_pushed)
            wxGetApp().imgui()->pop_bold_font();

        ImGui::SetCursorScreenPos(left_min);
        ImGui::InvisibleButton("##color_show", left_size);
        if (ImGui::IsItemClicked() && !showcolor) {
            showcolor = true;
            wxGetApp().imgui()->set_requires_extra_frame();
        }
        if (ImGui::IsItemHovered() && left_truncated) {
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGuiWrapper::COL_WINDOW_BACKGROUND);
            ImGui::PushStyleColor(ImGuiCol_Border, {0, 0, 0, 0});
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
            ImGui::SetTooltip("%s", left_full_txt.c_str());
            ImGui::PopStyleColor(3);
        }
        ImGui::SetCursorScreenPos(right_min);
        ImGui::InvisibleButton("##gcode_toggle", right_size);
        if (ImGui::IsItemClicked() && showcolor) {
            showcolor = false;
            wxGetApp().imgui()->set_requires_extra_frame();
        }

        ImGui::SetCursorScreenPos(ImVec2(pill_pos.x, pill_pos.y + pill_h));

        return false;
    }

    void showColorHeader( EViewType m_view_type,float& contentWidth) {
        initColorData();
        ImGui::Dummy({ window_padding, window_padding });
        ImGui::SameLine();
        switch (m_view_type)
        {
        case EViewType::FeatureType:
        {
            char buffer[64];
            const bool single_color_flush = (m_extruder_ids.size() == 1) && (total_flushed_filament_m > 0.0 || total_flushed_filament_g > 0.0);
            bool       custom_role_merged = false;
            for (size_t i = 0; i < m_roles.size(); ++i) {
                ExtrusionRole role = m_roles[i];
                if (role < erCount) {
                    labels.push_back(_u8L(ExtrusionEntity::role_to_string(role)));
                    auto [time, percent] = role_time_and_percent(role);
                    auto [model_used_filament_m, model_used_filament_g] = used_filament_per_role(role);
                    if (single_color_flush && role == erCustom) {
                        time += time_mode.flush_time;
                        percent = (time_mode.time > 0.0f) ? time / time_mode.time : 0.0f;
                        model_used_filament_m += total_flushed_filament_m;
                        model_used_filament_g += total_flushed_filament_g;
                        custom_role_merged = true;
                    }
                    times.push_back((time > 0.0f) ? short_time(get_time_dhms(time)) : "");
                    if (percent == 0)
                        ::sprintf(buffer, "0%%");
                    else
                        percent > 0.001 ? ::sprintf(buffer, "%.1f%%", percent * 100) : ::sprintf(buffer, "<0.1%%");
                    percents.push_back(buffer);
                    ::sprintf(buffer, imperial_units ? "%.2f in" : "%.2f m", model_used_filament_m);
                    used_filaments_length.push_back(buffer);
                    ::sprintf(buffer, imperial_units ? "%.2f oz" : "%.2f g", model_used_filament_g);
                    used_filaments_weight.push_back(buffer);
                }
            }
            if (single_color_flush && !custom_role_merged) {
                labels.push_back(_u8L(ExtrusionEntity::role_to_string(erCustom)));
                times.push_back(short_time(get_time_dhms(time_mode.flush_time)));
                if (time_mode.time > 0.0f) {
                    float percent = time_mode.flush_time / time_mode.time;
                    percent > 0.001 ? ::sprintf(buffer, "%.1f%%", percent * 100) : ::sprintf(buffer, "<0.1%%");
                } else {
                    ::sprintf(buffer, "0%%");
                }
                percents.push_back(buffer);
                ::sprintf(buffer, imperial_units ? "%.2f in" : "%.2f m", total_flushed_filament_m);
                used_filaments_length.push_back(buffer);
                ::sprintf(buffer, imperial_units ? "%.2f oz" : "%.2f g", total_flushed_filament_g);
                used_filaments_weight.push_back(buffer);
            }

            auto [time, percent] = move_time_and_percent(EMoveType::Travel);
            travel_time = (time > 0.0f) ? short_time(get_time_dhms(time)) : "";
            if (percent == 0)
                ::sprintf(buffer, "0%%");
            else
                percent > 0.001 ? ::sprintf(buffer, "%.1f%%", percent * 100) : ::sprintf(buffer, "<0.1%%");
            travel_percent = buffer;

            // Collect flush data for display
            has_flush_data = (total_flushed_filament_m > 0.0 || total_flushed_filament_g > 0.0) && !single_color_flush;
            if (has_flush_data) {
                flush_label = _u8L("Flushed");
                {
                    float flush_time_val = std::max(0.0f, time_mode.flush_time);
                    flush_time_str = flush_time_val > 0.0f ? short_time(get_time_dhms(flush_time_val)) : "";
                    if (flush_time_val > 0.0f && time_mode.time > 0.0f) {
                        float flush_time_pct = flush_time_val / time_mode.time * 100.0f;
                        flush_time_pct > 0.001f ? ::sprintf(buffer, "%.1f%%", flush_time_pct) : ::sprintf(buffer, "<0.1%%");
                        flush_time_percent_str = buffer;
                    } else {
                        flush_time_percent_str = "";
                    }
                }
                // Calculate flush percentage based on filament volume
                if (ps.total_used_filament > 0.0) {
                    double flush_percent_val = total_flushed_filament_m / (ps.total_used_filament / koef) * 100.0;
                    flush_percent_val > 0.001 ? ::sprintf(buffer, "%.1f%%", flush_percent_val) : ::sprintf(buffer, "<0.1%%");
                } else {
                    ::sprintf(buffer, "0%%");
                }
                flush_percent_str = buffer;
                ::sprintf(buffer, imperial_units ? "%.2f in" : "%.2f m", total_flushed_filament_m);
                flush_length_str = buffer;
                ::sprintf(buffer, imperial_units ? "%.2f oz" : "%.2f g",
                    imperial_units ? total_flushed_filament_g / unit_conver : total_flushed_filament_g);
                flush_weight_str = buffer;
            }

            if (has_flush_data) {
                labels.push_back(flush_label);
                times.push_back(flush_time_str);
                percents.push_back(flush_time_percent_str.empty() ? flush_percent_str : flush_time_percent_str);
                used_filaments_length.push_back(flush_length_str);
                used_filaments_weight.push_back(flush_weight_str);
            }
            offsets = calculate_offsets({ 
                {_u8L("Line Type"), labels},
                {_u8L("Time"), times}, 
                {_u8L("Percent"), percents}, 
                {_u8L("Length"), used_filaments_length}, 
                {_u8L("Weight"), used_filaments_weight}, 
                {_u8L("Display"), 
                {""}} }, icon_size);
            {
                const ImGuiStyle& style = ImGui::GetStyle();
                const float display_gap = std::max(style.ItemSpacing.x, 6.0f * m_scale);
                offsets[5] = offsets[4] + max_width(used_filaments_weight, _u8L("Weight")) + display_gap;
            }
            append_headers({ 
                {_u8L("Line Type"), offsets[0]}, 
                {_u8L("Time"), offsets[1]}, 
                {_u8L("Percent"), offsets[2]}, 
                {_u8L("Length"), offsets[3]},
                {_u8L("Weight"), offsets[4]},
                //{_u8L("Display"), offsets[5]} 
                });

            {
                const ImGuiStyle& style = ImGui::GetStyle();
                const float checkbox_width = ImGui::GetFrameHeight();
                const float checkbox_right_margin = 6.0f * m_scale;

                float rightmost = 0.0f;
                rightmost = std::max(rightmost, offsets[0] + max_width(labels, _u8L("Line Type")));
                rightmost = std::max(rightmost, offsets[1] + max_width(times, _u8L("Time")));
                rightmost = std::max(rightmost, offsets[2] + max_width(percents, _u8L("Percent")));
                rightmost = std::max(rightmost, offsets[3] + max_width(used_filaments_length, _u8L("Length")));
                rightmost = std::max(rightmost, offsets[4] + max_width(used_filaments_weight, _u8L("Weight")));
                rightmost = std::max(rightmost, offsets[5] + checkbox_width + checkbox_right_margin);

                const float required_window_width = rightmost + style.ScrollbarSize + 0.5f * window_padding;
                if (required_window_width > contentWidth) {
                    contentWidth = required_window_width;
                    imgui->set_requires_extra_frame();
                }
            }
            break;
        }
        case EViewType::Height: { imgui->title(_u8L("Layer Height (mm)")); break; }
        case EViewType::Width: { imgui->title(_u8L("Line Width (mm)")); break; }
        case EViewType::Feedrate:
        {
            imgui->title(_u8L("Speed (mm/s)"));
            break;
        }

        case EViewType::FanSpeed: { imgui->title(_u8L("Fan Speed (%)")); break; }
        case EViewType::Temperature: { imgui->title(_u8L("Temperature (°C)")); break; }
        case EViewType::VolumetricRate: { imgui->title(_u8L("Volumetric flow rate (mm³/s)")); break; }
        case EViewType::LayerTime: { imgui->title(_u8L("Layer Time")); break; }
        case EViewType::LayerTimeLog: { imgui->title(_u8L("Layer Time (log)")); break; }
        case EViewType::Acceleration: {
            std::string str = _u8L("Acceleration") + std::string(" (mm/s²)");
            imgui->title(str); break; 
            }
#if ENABLE_AUE_CUSTOM_PREVIEW
        case EViewType::Custom:
        {
            std::vector<std::string> roi_labels;
            std::vector<std::string> roi_desc;
            roi_labels.reserve(m_extruder_ids.size() * 3);
            roi_desc.reserve(m_extruder_ids.size() * 3);

            const bool multi_extruder = (m_extruder_ids.size() > 1);
            for (unsigned char extruder_id : m_extruder_ids) {
                std::string prefix;
                if (multi_extruder)
                    prefix = _u8L("Extruder") + " " + std::to_string(extruder_id + 1) + " - ";

                roi_labels.push_back(prefix + _u8L("Filament color"));
                roi_desc.push_back(_u8L("Filament color (original toolpath color)"));

                roi_labels.push_back(prefix + _u8L("ROI Trigger"));
                roi_desc.push_back(_u8L("ROI Trigger (slow-speed extrusion, e.g. overhang slow-down)"));
                roi_labels.push_back(prefix + _u8L("ROI Defect"));
                roi_desc.push_back(_u8L("ROI Defect (after returning to normal speed, under-extrusion prone)"));
            }

            offsets = calculate_offsets({ { _u8L("Color"), roi_labels }, { _u8L("Description"), roi_desc } }, icon_size);
            append_headers({ { _u8L("Color"), offsets[0] }, { _u8L("Description"), offsets[1] } });

            {
                const ImGuiStyle& style = ImGui::GetStyle();
                const float reserved_right_width = 2.0f * window_padding + style.ScrollbarSize + style.ItemSpacing.x;
                const float desc_column_width = max_width(roi_desc, _u8L("Description"));
                const float required_window_width = offsets[1] + desc_column_width + reserved_right_width;
                if (required_window_width > contentWidth) {
                    contentWidth = required_window_width;
                    imgui->set_requires_extra_frame();
                }
            }
            break;
        }
#endif // ENABLE_AUE_CUSTOM_PREVIEW
        case EViewType::Tool:
        {
            std::vector<std::string> extruder_labels;
            std::vector<std::string> used_filaments;
            extruder_labels.reserve(m_extruder_ids.size());
            used_filaments.reserve(m_extruder_ids.size());

            char buffer[64];
            for (size_t i = 0; i < m_extruder_ids.size(); ++i) {
                const unsigned char extruder_id = m_extruder_ids[i];
                extruder_labels.push_back(_u8L("Extruder") + " " + std::to_string(extruder_id + 1));
                ::sprintf(buffer, imperial_units ? "%.2f in    %.2f g" : "%.2f m    %.2f g", model_used_filaments_m[i], model_used_filaments_g[i]);
                used_filaments.push_back(buffer);
            }

            offsets = calculate_offsets({ { _u8L("Filament"), extruder_labels }, { _u8L("Used filament"), used_filaments } }, icon_size);
            append_headers({ { _u8L("Filament"), offsets[0] }, { _u8L("Used filament"), offsets[1] } });

            {
                const ImGuiStyle& style = ImGui::GetStyle();
                const float reserved_right_width = 2.0f * window_padding + style.ScrollbarSize + style.ItemSpacing.x;
                const float used_column_width = max_width(used_filaments, _u8L("Used filament"));
                const float required_window_width = offsets[1] + used_column_width + reserved_right_width;
                if (required_window_width > contentWidth) {
                    contentWidth = required_window_width;
                    imgui->set_requires_extra_frame();
                }
            }
            break;
        }
        case EViewType::ColorPrint:
        {
            std::vector<std::string> total_filaments;
            char buffer[64];
            ::sprintf(buffer, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", ps.total_used_filament / /*1000*/koef, ps.total_weight / unit_conver);
            total_filaments.push_back(buffer);


            const bool merge_flush_into_model = should_merge_flush_into_model(m_print_statistics);
            const int color_print_displayed_columns = merge_flush_into_model ? (displayed_columns & ~ColumnData::Flushed) : displayed_columns;

            std::vector<std::pair<std::string, std::vector<std::string>>> title_columns;
            if (color_print_displayed_columns & ColumnData::Model) {
                //title_columns.push_back({ _u8L("Filament"), {""} });
                title_columns.push_back({ _u8L("Filament"), total_filaments });
                title_columns.push_back({ _u8L("Model"), total_filaments });
            }
            if (color_print_displayed_columns & ColumnData::Support) {
                title_columns.push_back({ _u8L("Support"), total_filaments });
            }
            if (color_print_displayed_columns & ColumnData::Flushed) {
                title_columns.push_back({ _u8L("Flushed"), total_filaments });
            }
            if (color_print_displayed_columns & ColumnData::WipeTower) {
                title_columns.push_back({ _u8L("Tower"), total_filaments });
            }
            if ((color_print_displayed_columns & ~ColumnData::Model) > 0) {
                title_columns.push_back({ _u8L("Total"), total_filaments });
            }

            //checkbox 占位
            title_columns.push_back({ _u8L(""), total_filaments });

            auto offsets_ = calculate_offsets(title_columns, icon_size);
            std::vector<std::pair<std::string, float>> title_offsets;
            for (int i = 0; i < offsets_.size(); i++) {
                title_offsets.push_back({ title_columns[i].first, offsets_[i] });
                color_print_offsets[title_columns[i].first] = offsets_[i];
            }
            append_headers(title_offsets);

            contentWidth = offsets_.empty() ? 0.0f : offsets_.back() + ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.x + window_padding + ImGui::GetStyle().ScrollbarSize;

            break;
        }
        default: { break; }
        }
    }

    void showColorTable(EViewType m_view_type
        , const std::vector<CustomGCode::Item>& m_custom_gcode_per_print_z
        , ETools &m_tools, bool preview_lite_mode
        , std::function<void()>featureFn, std::function<void()>fadeRateFn) {

        switch (m_view_type)
        {
        case Slic3r::GUI::EViewType::FeatureType: 
        {
            const bool is_lite_mode = preview_lite_mode;
            
            for (size_t i = 0; i < m_roles.size(); ++i) {
                ExtrusionRole role = m_roles[i];
                if (role >= erCount)
                    continue;
                bool enable = true;
                if (is_lite_mode) {
                    enable = !(role_been_filtered_in_lite_mode(role));
                }
                //const bool visible = is_visible(role);
                const bool visible = m_renderer.is_extrusion_role_visible(role);
                std::vector<std::pair<std::string, float>> columns_offsets;
                columns_offsets.push_back({ labels[i], offsets[0] });
                columns_offsets.push_back({ times[i], offsets[1] });
                columns_offsets.push_back({ percents[i], offsets[2] });
                columns_offsets.push_back({ used_filaments_length[i], offsets[3] });
                columns_offsets.push_back({ used_filaments_weight[i], offsets[4] });
                append_item(EItemType::Rect
                    , BaseRenderer::Extrusion_Role_Colors[static_cast<unsigned int>(role)]
                    , columns_offsets, enable, visible, [&]() {
                        /*m_extrusions.role_visibility_flags = visible ?
                            m_extrusions.role_visibility_flags & ~(1 << role) :
                            m_extrusions.role_visibility_flags | (1 << role);*/
                                m_renderer.set_extrusion_role_visible(role, !visible);
                        featureFn();
                    });
            }
            if ((m_extruder_ids.size() == 1) && (total_flushed_filament_m > 0.0 || total_flushed_filament_g > 0.0) &&
                std::find(m_roles.begin(), m_roles.end(), erCustom) == m_roles.end()) {
                //const bool visible = is_visible(erCustom);
                const bool visible = m_renderer.is_extrusion_role_visible(erCustom);
                const size_t idx = labels.size() - 1;
                std::vector<std::pair<std::string, float>> columns_offsets;
                columns_offsets.push_back({ labels[idx], offsets[0] });
                columns_offsets.push_back({ times[idx], offsets[1] });
                columns_offsets.push_back({ percents[idx], offsets[2] });
                columns_offsets.push_back({ used_filaments_length[idx], offsets[3] });
                columns_offsets.push_back({ used_filaments_weight[idx], offsets[4] });
                append_item(EItemType::Rect
                    , BaseRenderer::Extrusion_Role_Colors[static_cast<unsigned int>(erCustom)]
                    , columns_offsets, !is_lite_mode, visible, [&]() {
                       /* m_extrusions.role_visibility_flags = visible ?
                            m_extrusions.role_visibility_flags & ~(1 << erCustom) :
                            m_extrusions.role_visibility_flags | (1 << erCustom);*/
                                m_renderer.set_extrusion_role_visible(erCustom, !visible);
                        featureFn();
                    });
            }

            for (auto type : options_items) {
                if (type != EMoveType::Travel) {

                    //const bool visible = m_buffers[buffer_id(type)].visible;
                    const bool visible = m_renderer.is_toolpath_move_type_visible(type);
                    auto append_option_item_with_type = [&](const ColorRGBA& color, const std::string& label, bool checkbox = true) {
                        append_item(EItemType::Rect
                            , color, { { label , offsets[0] } }
                            , checkbox, visible, [&]() {
                                //m_buffers[buffer_id(type)].visible = !m_buffers[buffer_id(type)].visible;
								m_renderer.set_toolpath_move_type_visible(type, !visible);
                                featureFn();
                            });
                    };

                    if (type == EMoveType::Seam)
                        append_option_item_with_type(BaseRenderer::Options_Colors[EOptionsColors::Seams], _u8L("Seams"));
                    else if (type == EMoveType::Retract)
                        append_option_item_with_type(BaseRenderer::Options_Colors[EOptionsColors::Retractions], _u8L("Retract"),
                                                     !is_lite_mode);
                    else if (type == EMoveType::Unretract)
                        append_option_item_with_type(BaseRenderer::Options_Colors[EOptionsColors::Unretractions], _u8L("Unretract"));
                    else if (type == EMoveType::Tool_change)
                        append_option_item_with_type(BaseRenderer::Options_Colors[EOptionsColors::ToolChanges], _u8L("Filament Changes"),
                                                     !is_lite_mode);
                    else if (type == EMoveType::Wipe)
                        append_option_item_with_type(BaseRenderer::Wipe_Color, _u8L("Wipe"), !is_lite_mode);
                }
                else {
                    // Keep this before Travel because this list renders in reverse visual order.
                    if (has_flush_data) {
                        std::vector<std::pair<std::string, float>> flush_columns;
                        flush_columns.push_back({ flush_label, offsets[0] });
                        flush_columns.push_back({ flush_time_str, offsets[1] });
                        flush_columns.push_back({ flush_time_percent_str.empty() ? flush_percent_str : flush_time_percent_str, offsets[2] });
                        flush_columns.push_back({ flush_length_str, offsets[3] });
                        flush_columns.push_back({ flush_weight_str, offsets[4] });
                        //const bool flush_visible = is_visible(erWipeTower);
                        const bool flush_visible = m_renderer.is_extrusion_role_visible(erWipeTower);
                        append_item(EItemType::Rect, BaseRenderer::Flush_Color, flush_columns
                            , !is_lite_mode, flush_visible, [&]() {
                                /*m_extrusions.role_visibility_flags = flush_visible ?
                                    m_extrusions.role_visibility_flags & ~(1 << erWipeTower) :
                                    m_extrusions.role_visibility_flags | (1 << erWipeTower);*/
								m_renderer.set_extrusion_role_visible(erWipeTower, !flush_visible);
                                featureFn();
                            });
                    }

                    //const bool visible = m_buffers[buffer_id(EMoveType::Travel)].visible;
                    const bool visible = m_renderer.is_toolpath_move_type_visible(EMoveType::Travel);
                    std::vector<std::pair<std::string, float>> columns_offsets;
                    columns_offsets.push_back({ _u8L("Travel"), offsets[0] });
                    columns_offsets.push_back({ travel_time, offsets[1] });
                    columns_offsets.push_back({ travel_percent, offsets[2] });
                    append_item(EItemType::Rect
                        , BaseRenderer::Travel_Colors[0], columns_offsets
                        , !is_lite_mode, visible, [&]() {
                            //m_buffers[buffer_id(EMoveType::Travel)].visible = !m_buffers[buffer_id(EMoveType::Travel)].visible;
	                        m_renderer.set_toolpath_move_type_visible(EMoveType::Travel, !visible),
                            featureFn();
                        });
                }
            }
        }
            break;
        case Slic3r::GUI::EViewType::Height:
            append_range(m_extrusions.ranges.height, 2);
            break;
        case Slic3r::GUI::EViewType::Width:
            append_range(m_extrusions.ranges.width, 2);
            break;
        case Slic3r::GUI::EViewType::Feedrate: {
            append_range(m_extrusions.ranges.feedrate, 0);
            ImGui::Spacing();
            ImGui::Dummy({ window_padding, window_padding });
            ImGui::SameLine();
            offsets = calculate_offsets({ { _u8L("Options"), { _u8L("Travel")}}, { _u8L("Display"), {""}} }, icon_size);
            append_headers({ {_u8L("Options"), offsets[0] }, { _u8L("Display"), offsets[1]} });
            //const bool travel_visible = m_buffers[buffer_id(EMoveType::Travel)].visible;
            const bool travel_visible = m_renderer.is_toolpath_move_type_visible(EMoveType::Travel);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 3.0f));
            append_item(EItemType::None, BaseRenderer::Travel_Colors[0]
                , { {_u8L("travel"), offsets[0] } }, true, travel_visible, [&]() {
                //m_buffers[buffer_id(EMoveType::Travel)].visible = !m_buffers[buffer_id(EMoveType::Travel)].visible;
                m_renderer.set_toolpath_move_type_visible(EMoveType::Travel, !travel_visible);
                fadeRateFn();
                });
            ImGui::PopStyleVar(1);
        }
            break;
        case Slic3r::GUI::EViewType::FanSpeed:
            append_range(m_extrusions.ranges.fan_speed, 0);
            break;
        case Slic3r::GUI::EViewType::Temperature:
            append_range(m_extrusions.ranges.temperature, 0);
            break;
        case Slic3r::GUI::EViewType::VolumetricRate:
            append_range(m_extrusions.ranges.volumetric_rate, 2);
            break;
        case Slic3r::GUI::EViewType::Acceleration:
            append_range(m_extrusions.ranges.acceleration, 0);
            break;
#if ENABLE_AUE_CUSTOM_PREVIEW
        case Slic3r::GUI::EViewType::Custom: {
            const bool multi_extruder = (m_extruder_ids.size() > 1);
            for (unsigned char extruder_id : m_extruder_ids) {
                const ColorRGBA filament_color = (extruder_id < m_tools.m_tool_colors.size()) ? m_tools.m_tool_colors[extruder_id] : ColorRGBA::GRAY();
                const auto highlight_colors = pick_interest_region_highlight_colors(filament_color);
                const ColorRGBA& trigger_color = highlight_colors.first;
                const ColorRGBA& defect_color  = highlight_colors.second;

                std::string prefix;
                if (multi_extruder)
                    prefix = _u8L("Extruder") + " " + std::to_string(extruder_id + 1) + " - ";

                append_item(EItemType::Rect, filament_color,
                    { { prefix + _u8L("Filament color"), offsets[0] }, { _u8L("Filament color (original toolpath color)"), offsets[1] } });
                append_item(EItemType::Rect, trigger_color,
                    { { prefix + _u8L("ROI Trigger"), offsets[0] }, { _u8L("ROI Trigger (slow-speed extrusion, e.g. overhang slow-down)"), offsets[1] } });
                append_item(EItemType::Rect, defect_color,
                    { { prefix + _u8L("ROI Defect"), offsets[0] }, { _u8L("ROI Defect (after returning to normal speed, under-extrusion prone)"), offsets[1] } });
            }
        }
           break;
#endif // ENABLE_AUE_CUSTOM_PREVIEW
        case Slic3r::GUI::EViewType::Tool: {
            size_t i = 0;
            for (unsigned char extruder_id : m_extruder_ids) {
                ::sprintf(buf, imperial_units ? "%.2f in    %.2f g" : "%.2f m    %.2f g", model_used_filaments_m[i], model_used_filaments_g[i]);
                append_item(EItemType::Rect, m_tools.m_tool_colors[extruder_id], { { _u8L("Extruder") + " " + std::to_string(extruder_id + 1), offsets[0]}, {buf, offsets[1]} });
                i++;
            }
        }
           break;
        case Slic3r::GUI::EViewType::ColorPrint: {
            //BBS: replace model custom gcode with current plate custom gcode
            const std::vector<CustomGCode::Item>& custom_gcode_per_print_z = m_custom_gcode_per_print_z;
            const bool merge_flush_into_model = should_merge_flush_into_model(m_print_statistics);
            const int color_print_displayed_columns = merge_flush_into_model ? (displayed_columns & ~ColumnData::Flushed) : displayed_columns;
            size_t i = 0;
            for (auto extruder_idx : m_extruder_ids) {
                if (m_tools.m_tool_visibles.size() <= extruder_idx)
                    continue;

                const bool filament_visible = m_tools.m_tool_visibles[extruder_idx];
                if (i < model_used_filaments_m.size() && i < model_used_filaments_g.size()) {
                    std::vector<std::pair<std::string, float>> columns_offsets;
                    columns_offsets.push_back({ std::to_string(extruder_idx + 1), color_print_offsets[_u8L("Filament")] });

                    char buf[64];
                    float column_sum_m = 0.0f;
                    float column_sum_g = 0.0f;
                    if (color_print_displayed_columns & ColumnData::Model) {
                        const double model_filament_m = model_used_filaments_m[i] + (merge_flush_into_model ? flushed_filaments_m[i] : 0.0);
                        const double model_filament_g = model_used_filaments_g[i] + (merge_flush_into_model ? flushed_filaments_g[i] : 0.0);
                        if ((color_print_displayed_columns & ~ColumnData::Model) > 0)
                            ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", model_filament_m, model_filament_g / unit_conver);
                        else
                            ::sprintf(buf, imperial_units ? "%.2f in    %.2f oz" : "%.2f m    %.2f g", model_filament_m, model_filament_g / unit_conver);
                        columns_offsets.push_back({ buf, color_print_offsets[_u8L("Model")] });
                        column_sum_m += model_filament_m;
                        column_sum_g += model_filament_g;
                    }
                    if (color_print_displayed_columns & ColumnData::Support) {
                        ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", support_used_filaments_m[i], support_used_filaments_g[i] / unit_conver);
                        columns_offsets.push_back({ buf, color_print_offsets[_u8L("Support")] });
                        column_sum_m += support_used_filaments_m[i];
                        column_sum_g += support_used_filaments_g[i];
                    }
                    if (color_print_displayed_columns & ColumnData::Flushed) {
                        ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", flushed_filaments_m[i], flushed_filaments_g[i] / unit_conver);
                        columns_offsets.push_back({ buf, color_print_offsets[_u8L("Flushed")] });
                        column_sum_m += flushed_filaments_m[i];
                        column_sum_g += flushed_filaments_g[i];
                    }
                    if (color_print_displayed_columns & ColumnData::WipeTower) {
                        ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", wipe_tower_used_filaments_m[i], wipe_tower_used_filaments_g[i] / unit_conver);
                        columns_offsets.push_back({ buf, color_print_offsets[_u8L("Tower")] });
                        column_sum_m += wipe_tower_used_filaments_m[i];
                        column_sum_g += wipe_tower_used_filaments_g[i];
                    }
                    if ((color_print_displayed_columns & ~ColumnData::Model) > 0) {
                        ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", column_sum_m, column_sum_g / unit_conver);
                        columns_offsets.push_back({ buf, color_print_offsets[_u8L("Total")] });
                    }
						
                    append_color_print_item(EItemType::Rect, m_tools.m_tool_colors[extruder_idx]
                        , columns_offsets, true, filament_visible, [&]() {
                            m_tools.m_tool_visibles[extruder_idx] = !m_tools.m_tool_visibles[extruder_idx];
                            featureFn();
                        });
                }
                i++;
            }
            if (m_extruder_ids.size() > 1) {
                // Separator
                ImGuiWindow* window = ImGui::GetCurrentWindow();
                const ImRect separator(ImVec2(window->Pos.x + window_padding * 3, window->DC.CursorPos.y), ImVec2(window->Pos.x + window->Size.x - window_padding * 3, window->DC.CursorPos.y + 1.0f));
                ImGui::ItemSize(ImVec2(0.0f, 0.0f));
                const bool item_visible = ImGui::ItemAdd(separator, 0);
                window->DrawList->AddLine(separator.Min, ImVec2(separator.Max.x, separator.Min.y), ImGui::GetColorU32(ImGuiCol_Separator));

                std::vector<std::pair<std::string, float>> columns_offsets;
                columns_offsets.push_back({ _u8L("Total"), color_print_offsets[_u8L("Filament")] });
                if (color_print_displayed_columns & ColumnData::Model) {
                    const double model_filament_m = total_model_used_filament_m + (merge_flush_into_model ? total_flushed_filament_m : 0.0);
                    const double model_filament_g = total_model_used_filament_g + (merge_flush_into_model ? total_flushed_filament_g : 0.0);
                    if ((color_print_displayed_columns & ~ColumnData::Model) > 0)
                        ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", model_filament_m, model_filament_g / unit_conver);
                    else
                        ::sprintf(buf, imperial_units ? "%.2f in    %.2f oz" : "%.2f m    %.2f g", model_filament_m, model_filament_g / unit_conver);
                    columns_offsets.push_back({ buf, color_print_offsets[_u8L("Model")] });
                }
                if (color_print_displayed_columns & ColumnData::Support) {
                    ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", total_support_used_filament_m, total_support_used_filament_g / unit_conver);
                    columns_offsets.push_back({ buf, color_print_offsets[_u8L("Support")] });
                }
                if (color_print_displayed_columns & ColumnData::Flushed) {
                    ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", total_flushed_filament_m, total_flushed_filament_g / unit_conver);
                    columns_offsets.push_back({ buf, color_print_offsets[_u8L("Flushed")] });
                }
                if (color_print_displayed_columns & ColumnData::WipeTower) {
                    ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", total_wipe_tower_used_filament_m, total_wipe_tower_used_filament_g / unit_conver);
                    columns_offsets.push_back({ buf, color_print_offsets[_u8L("Tower")] });
                }
                if ((color_print_displayed_columns & ~ColumnData::Model) > 0) {
                    ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g",
                        total_model_used_filament_m + total_support_used_filament_m + (merge_flush_into_model ? 0.0 : total_flushed_filament_m) + total_wipe_tower_used_filament_m,
                        (total_model_used_filament_g + total_support_used_filament_g + (merge_flush_into_model ? 0.0 : total_flushed_filament_g) + total_wipe_tower_used_filament_g) / unit_conver);
                    columns_offsets.push_back({ buf, color_print_offsets[_u8L("Total")] });
                }
                append_item(EItemType::None, m_tools.m_tool_colors[0], columns_offsets);
            }

            //BBS display filament change times
            ImGui::Dummy({ window_padding, window_padding });
            ImGui::SameLine();
            imgui->text(_u8L("Filament change times") + ":");
            ImGui::SameLine();
            ::sprintf(buf, "%d", m_print_statistics.total_filamentchanges);
            imgui->text(buf);

            //BBS display cost
            ImGui::Dummy({ window_padding, window_padding });
            ImGui::SameLine();
            imgui->text(_u8L("Cost") + ":");
            ImGui::SameLine();
            ::sprintf(buf, "%.2f", ps.total_cost);
            imgui->text(buf);
        }
            break;
        case Slic3r::GUI::EViewType::LayerTime:
            append_range(m_extrusions.ranges.layer_duration, true);
            break;
        case Slic3r::GUI::EViewType::LayerTimeLog:
            append_range(m_extrusions.ranges.layer_duration_log, true);
            break;
        default:
            break;
        }
    }

private:

    mutable std::unordered_map<ExtrusionRole, std::pair<float, float>> cache;
        
        std::pair<float, float> role_time_and_percent(ExtrusionRole role) const {
            // 检查缓存
            auto cache_it = cache.find(role);
            if (cache_it != cache.end()) {
                return cache_it->second;
            }
            
            // 查找并计算
            auto it = std::find_if(
                time_mode.roles_times.begin(),
                time_mode.roles_times.end(),
                [role](const auto& item) { return item.first == role; }
            );
            
            std::pair<float, float> result;
            if (it == time_mode.roles_times.end()) {
                result = {0.0f, 0.0f};
            } else {
                const float time = it->second;
                const float percent = (time_mode.time > 0.0f) ? time / time_mode.time : 0.0f;
                result = {time, percent};
            }
            
            cache[role] = result;  // 缓存结果
            return result;
        }



    std::pair<float, float> used_filament_per_role(ExtrusionRole role) {
        auto it = m_print_statistics.used_filaments_per_role.find(role);
        if (it == m_print_statistics.used_filaments_per_role.end())
            return std::make_pair(0.0, 0.0);

        double koef = imperial_units ? GizmoObjectManipulation::in_to_mm / 1000.0 : 1.0;
        double unit_conver = imperial_units ? GizmoObjectManipulation::oz_to_g : 1;
        return std::make_pair(it->second.first / koef, it->second.second / unit_conver);
    };

    std::pair<float, float> move_time_and_percent(EMoveType move_type) {
        auto it = std::find_if(time_mode.moves_times.begin(), time_mode.moves_times.end(),
            [move_type](const std::pair<EMoveType, float>& item) {
                return move_type == item.first;
            });
        
        if (it == time_mode.moves_times.end()) {
            return {0.0f, 0.0f};
        }
        
        float time = it->second;
        float percent = (time_mode.time > 0.0f) ? time / static_cast<float>(time_mode.time) : 0.0f;
        return {time, percent};
    }


    float max_width(const std::vector<std::string>& items, const std::string& title, float extra_size = 0.0f) {
        float ret = ImGui::CalcTextSize(title.c_str()).x;
        for (const std::string& item : items) {
            ret = std::max(ret, extra_size + ImGui::CalcTextSize(item.c_str()).x);
        }
        return ret;
    };

    std::vector<float> calculate_offsets(const std::vector<std::pair<std::string, std::vector<std::string>>>& title_columns, float extra_size = 0.0f) {
        const ImGuiStyle& style = ImGui::GetStyle();
        std::vector<float> offsets;
        //offsets.push_back(max_width(title_columns[0].second, title_columns[0].first, extra_size) + 3.0f * style.ItemSpacing.x);
        offsets.push_back(max_width(title_columns[0].second, title_columns[0].first, extra_size) + style.ItemSpacing.x + icon_size);
        for (size_t i = 1; i < title_columns.size() - 1; i++)
            offsets.push_back(offsets.back() + max_width(title_columns[i].second, title_columns[i].first) + style.ItemSpacing.x);
        if (title_columns.back().first == _u8L("Percent")) {
            const auto preferred_offset = ImGui::GetWindowWidth() 
                - ImGui::CalcTextSize(_u8L("Percent").c_str()).x 
                - ImGui::GetFrameHeight() / 2 
                - 2 * window_padding 
                - ImGui::GetStyle().ScrollbarSize;
            if (preferred_offset > offsets.back()) {
                offsets.back() = preferred_offset;
                imgui->set_requires_extra_frame();
            }
        }

        float average_col_width = 400*m_scale / static_cast<float>(title_columns.size());
        std::vector<float> ret;
        ret.push_back(0);
        for (size_t i = 1; i < title_columns.size(); i++) {
            ret.push_back(std::max(offsets[i - 1], i * average_col_width));
        }
		
        return ret;
    };

    std::pair<double, double> get_used_filament_from_volume(double volume, int extruder_id) {
        double koef = imperial_units ? 1.0 / GizmoObjectManipulation::in_to_mm : 0.001;
        std::pair<double, double> ret = { koef * volume / (PI * sqr(0.5 * m_filament_diameters[extruder_id])),
                                          volume * m_filament_densities[extruder_id] * 0.001 };
        return ret;
    };

    void append_headers(const std::vector<std::pair<std::string, float>>& title_offsets) {
        for (size_t i = 0; i < title_offsets.size(); i++) {
            ImGui::SameLine(title_offsets[i].second);
            imgui->bold_text(title_offsets[i].first);
        }
        // Ensure right padding
        ImGui::SameLine();
        ImGui::Dummy({ window_padding, 1 });
        ImGui::Separator();
    };

    void append_item(EItemType type,const ColorRGBA& color,
        const std::vector<std::pair<std::string, float>>& columns_offsets,
        bool checkbox = true,bool visible = true,std::function<void()> callback = nullptr)
    {
        float row_start_y = ImGui::GetCursorPosY();
        auto draw_list = ImGui::GetWindowDrawList();
        // render icon
        ImVec2 pos = ImVec2(ImGui::GetCursorScreenPos().x + window_padding, ImGui::GetCursorScreenPos().y);
        switch (type) {
        default:
        case EItemType::Rect: {
            draw_list->AddRectFilled({ pos.x + 1.0f * m_scale, pos.y + 1.0f * m_scale }, { pos.x + icon_size - 1.0f * m_scale, pos.y + icon_size + 1.0f * m_scale },
                ImGuiWrapper::to_ImU32(color));
            break;
        }
        case EItemType::Circle: {
            ImVec2 center(0.5f * (pos.x + pos.x + icon_size), 0.5f * (pos.y + pos.y + icon_size + 5.0f));
            draw_list->AddCircleFilled(center, 0.5f * icon_size, ImGuiWrapper::to_ImU32(color), 16);
            break;
        }
        case EItemType::Hexagon: {
            ImVec2 center(0.5f * (pos.x + pos.x + icon_size), 0.5f * (pos.y + pos.y + icon_size + 5.0f));
            draw_list->AddNgonFilled(center, 0.5f * icon_size, ImGuiWrapper::to_ImU32(color), 6);
            break;
        }
        case EItemType::Line: {
            draw_list->AddLine({ pos.x + 1, pos.y + icon_size + 2 }, { pos.x + icon_size - 1, pos.y + 4 }, ImGuiWrapper::to_ImU32(color), 3.0f);
            break;
        case EItemType::None:
            break;
        }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20.0 * m_scale, 6.0 * m_scale));
        ImGui::Dummy({ 0.0, 0.0 });
        ImGui::SameLine();
        if (callback) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0 * m_scale, 0.0));
            float max_height = 0.f;
            for (auto column_offset : columns_offsets) {
                if (ImGui::CalcTextSize(column_offset.first.c_str()).y > max_height)
                    max_height = ImGui::CalcTextSize(column_offset.first.c_str()).y;
            }
            bool   b_menu_item = ImGui::BBLMenuItem(("##" + columns_offsets[0].first).c_str(), nullptr, false, true, max_height);
            ImRect row_rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            ImGui::PopStyleVar(1);
            if (b_menu_item)
                callback();
            if (checkbox) {
                const float checkbox_width = ImGui::GetFrameHeight();
                const float checkbox_right_margin = 6.0f * m_scale;
                float bias = ImGui::GetWindowContentRegionMax().x - checkbox_width - checkbox_right_margin;
                bias = std::max(0.0f, bias);
                ImGui::SameLine(bias);
                float  check_height  = ImGui::GetFrameHeight();
                float  target_y      = row_start_y + icon_size * 0.5f - check_height * 0.5f;
                target_y             = std::max(row_start_y, target_y);
                ImVec2 checkbox_pos  = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(checkbox_pos.x, target_y));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f * m_scale, 3.0f * m_scale));
                visible = config.checkBox("##" + columns_offsets[0].first, visible);
                ImGui::PopStyleVar(1);
                ImGui::SetCursorPos(checkbox_pos);
            }
        }

        float dummy_size = type == EItemType::None ? window_padding * 3 : ImGui::GetStyle().ItemSpacing.x + icon_size;
        ImGui::SameLine(dummy_size);
        imgui->text(columns_offsets[0].first);

        for (auto i = 1; i < columns_offsets.size(); i++) {
            ImGui::SameLine(columns_offsets[i].second);
            imgui->text(columns_offsets[i].first);
        }
        ImGui::PopStyleVar(1);
    };


	void append_color_print_item(EItemType                             type,
                     const ColorRGBA&                                  color,
                     const std::vector<std::pair<std::string, float>>& columns_offsets,
                     bool                                              checkbox = true,
                     bool                                              visible  = true,
                     std::function<void()>                             callback = nullptr)
    {
        float row_start_y = ImGui::GetCursorPosY();
        auto draw_list = ImGui::GetWindowDrawList();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20.0 * m_scale, 6.0 * m_scale));
        float dummy_size = type == EItemType::None ? window_padding * 3 : ImGui::GetStyle().ItemSpacing.x + icon_size;
        // render icon
        ImVec2 pos = ImVec2(ImGui::GetCursorScreenPos().x + dummy_size, ImGui::GetCursorScreenPos().y);
        switch (type) {
        default:
        case EItemType::Rect: {
            /*draw_list->AddRectFilled({pos.x + 1.0f * m_scale, pos.y + 1.0f * m_scale},
                                     {pos.x + icon_size - 1.0f * m_scale, pos.y + icon_size + 1.0f * m_scale},
                                     ImGuiWrapper::to_ImU32(color));*/

			bool is_dark = wxGetApp().dark_mode();			
			
			// frame background color
			ColorRGB x = is_dark ? ColorRGB((unsigned char) 43, (unsigned char) 43, (unsigned char) 45) :
                                   ColorRGB((unsigned char) 230, (unsigned char) 230, (unsigned char) 233);
			
			float k = abs(color.r() - x.r()) + abs(color.g() - x.g()) + abs(color.b() - x.b());
            if (k < 0.3) {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			} else {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
			}			

			ImGui::RenderFrame({pos.x - 1.0f * m_scale, pos.y - 1.0f * m_scale},
                               {pos.x + icon_size + 1.0f * m_scale, pos.y + icon_size + 1.0f * m_scale}, ImGuiWrapper::to_ImU32(color),
                               true,
                               3.0f);
			ImGui::PopStyleVar();
            break;
        }
        case EItemType::Circle: {
            ImVec2 center(0.5f * (pos.x + pos.x + icon_size), 0.5f * (pos.y + pos.y + icon_size + 5.0f));
            draw_list->AddCircleFilled(center, 0.5f * icon_size, ImGuiWrapper::to_ImU32(color), 16);
            break;
        }
        case EItemType::Hexagon: {
            ImVec2 center(0.5f * (pos.x + pos.x + icon_size), 0.5f * (pos.y + pos.y + icon_size + 5.0f));
            draw_list->AddNgonFilled(center, 0.5f * icon_size, ImGuiWrapper::to_ImU32(color), 6);
            break;
        }
        case EItemType::Line: {
            draw_list->AddLine({pos.x + 1, pos.y + icon_size + 2}, {pos.x + icon_size - 1, pos.y + 4}, ImGuiWrapper::to_ImU32(color), 3.0f);
            break;
        case EItemType::None: break;
        }
        }

        
        //ImGui::Dummy({0.0, 0.0});
        //ImGui::SameLine();
        if (callback) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0 * m_scale, 0.0));
            float max_height = 0.f;
            for (auto column_offset : columns_offsets) {
                if (ImGui::CalcTextSize(column_offset.first.c_str()).y > max_height)
                    max_height = ImGui::CalcTextSize(column_offset.first.c_str()).y;
            }
            bool   b_menu_item = ImGui::BBLMenuItem(("##" + columns_offsets[0].first).c_str(), nullptr, false, true, max_height);
            ImRect row_rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            ImGui::PopStyleVar(1);
            if (b_menu_item)
                callback();
            if (checkbox) {
                const float checkbox_width = ImGui::GetFrameHeight();
                const float checkbox_right_margin = 6.0f * m_scale;
                float bias = ImGui::GetWindowContentRegionMax().x - checkbox_width - checkbox_right_margin;
                bias = std::max(0.0f, bias);
                ImGui::SameLine(bias);
                float  check_height  = ImGui::GetFrameHeight();
                float  target_y      = row_start_y + icon_size * 0.5f - check_height * 0.5f;
                target_y             = std::max(row_start_y, target_y);
                ImVec2 checkbox_pos  = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(checkbox_pos.x, target_y));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f * m_scale, 3.0f * m_scale));
                visible = config.checkBox("##" + columns_offsets[0].first, visible);
                ImGui::PopStyleVar(1);
                ImGui::SetCursorPos(checkbox_pos);
            }
        }

        const auto& t = columns_offsets[0].first;
        ImVec2 text_size = ImGui::CalcTextSize(t.c_str());
        ImGui::SameLine(dummy_size + (icon_size - text_size.x) / 2.0f);

		float gray = color.r() * 0.299f + color.g() * 0.587f + color.b() * 0.114f;
        if (gray > 0.5f) {
            gray = 0.0f;
        } else {
            gray = 1.0f;
        }
        
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(gray, gray, gray, 1.0f));

        imgui->text(t);

		ImGui::PopStyleColor();

        for (auto i = 1; i < columns_offsets.size(); i++) {
            ImGui::SameLine(columns_offsets[i].second);
            imgui->text(columns_offsets[i].first);
        }
        ImGui::PopStyleVar(1);
    };

    void append_range_item(int i, float value, unsigned int decimals) {
        char buf[1024];
        ::sprintf(buf, "%.*f", decimals, value);
        append_item(EItemType::Rect, BaseRenderer::Range_Colors[i], {{buf, 0}});
    };

    void append_range(const BaseRenderer::Extrusions::Range& range, unsigned int decimals) {
        if (range.count == 1)
            // single item use case
            append_range_item(0, range.min, decimals);
        else if (range.count == 2) {
            append_range_item(static_cast<int>(BaseRenderer::Range_Colors.size()) - 1, range.max, decimals);
            append_range_item(0, range.min, decimals);
        }
        else {
            const float step_size = range.step_size();
            for (int i = static_cast<int>(BaseRenderer::Range_Colors.size()) - 1; i >= 0; --i) {
                append_range_item(i, range.get_value_at_step(i), decimals);
            }
        }
    };

    bool is_visible(ExtrusionRole role) const {
        return 
            role < erCount 
            && (m_extrusions.role_visibility_flags& (1 << role)) != 0;
    }

    ////////////////////////////////////
    std::vector<float> offsets;
    std::vector<std::string> labels;
    std::vector<std::string> times;
    std::string travel_time;
    std::vector<std::string> percents;
    std::vector<std::string> used_filaments_length;
    std::vector<std::string> used_filaments_weight;
    std::string travel_percent;
    // Flush row data for Line Type legend
    std::string flush_label;
    std::string flush_time_str;
    std::string flush_percent_str;
    std::string flush_length_str;
    std::string flush_weight_str;
    bool        has_flush_data = false;
    std::string flush_time_percent_str;
    std::vector<double> model_used_filaments_m;
    std::vector<double> model_used_filaments_g;
    double total_model_used_filament_m = 0, total_model_used_filament_g = 0;
    std::vector<double> flushed_filaments_m;
    std::vector<double> flushed_filaments_g;
    double total_flushed_filament_m = 0, total_flushed_filament_g = 0;
    std::vector<double> wipe_tower_used_filaments_m;
    std::vector<double> wipe_tower_used_filaments_g;
    double total_wipe_tower_used_filament_m = 0, total_wipe_tower_used_filament_g = 0;
    std::vector<double> support_used_filaments_m;
    std::vector<double> support_used_filaments_g;
    double total_support_used_filament_m = 0, total_support_used_filament_g = 0;
    int displayed_columns = 0;
    std::map<std::string, float> color_print_offsets;
};
 

BaseRenderer::BaseRenderer()
{
    m_moves_slider  = new IMSlider(0, 0, 0, 100, wxSL_HORIZONTAL);
    m_layers_slider = new IMSlider(0, 0, 0, 100, wxSL_VERTICAL);
    m_cliper_slider = new IMSlider(0, 100, 0, 100, wxSL_VERTICAL);
    m_extrusions.reset_role_visibility_flags();
    m_moves_slider->set_animate_tick([this](bool manual) {

        const int cur_move = m_moves_slider->GetHigherValue();
        const int max_move = m_moves_slider->GetMaxValue();
        
        if (cur_move < max_move) 
        {
            const int next = cur_move + 1.0;
            m_moves_slider->SetHigherValue(next);
            m_moves_slider->set_as_dirty(true);
        
        } else {
        
            const int cur_layer = m_layers_slider->GetHigherValue();
            const int max_layer = m_layers_slider->GetMaxValue();
            if (cur_layer < max_layer) 
            {
                const int next_layer = cur_layer + 1.0;
                m_layers_slider->SetHigherValue(next_layer); 
                set_layers_z_range({static_cast<unsigned int>(m_layers_slider->GetLowerValue()),
                                                   static_cast<unsigned int>(m_layers_slider->GetHigherValue())});
                
                update_moves_slider(false);
                m_layers_slider->set_as_dirty(false);
                m_moves_slider->SetHigherValue(m_moves_slider->GetMinValue());
                m_moves_slider->set_as_dirty(true);

            } else {
                
                if (manual) {
                    //restart
                    const int first = m_layers_slider->GetMinValue();
                    m_layers_slider->SetHigherValue(first);
                    set_layers_z_range({static_cast<unsigned int>(m_layers_slider->GetLowerValue()),
                                        static_cast<unsigned int>(m_layers_slider->GetHigherValue())});

                    update_moves_slider(false);
                    m_layers_slider->set_as_dirty(false);
                    m_moves_slider->SetHigherValue(m_moves_slider->GetMinValue());
                    m_moves_slider->set_as_dirty(true);

                } else {
                    // stop
                    m_moves_slider->set_animating(false);
                }
            }
        }
        

        int layers = m_layers_slider->GetHigherValue() + 1.0;

    });
}

BaseRenderer::~BaseRenderer()
{
    reset();
    IMTexture::release_texture(m_fold_icon_svg_texture);
    if (m_moves_slider) {
        delete m_moves_slider;
        m_moves_slider = nullptr;
    }
    if (m_layers_slider) {
        delete m_layers_slider;
        m_layers_slider = nullptr;
    }
    if (m_cliper_slider) {
        delete m_cliper_slider;
        m_cliper_slider = nullptr;
    }
}

void BaseRenderer::init(ConfigOptionMode mode, PresetBundle* preset_bundle,bool isgcode)
{
    if (isgcode)
    {
        init_tool_maker(preset_bundle);

        // BBS initialzed view_type items
        m_user_mode = mode;
        update_by_mode(m_user_mode);
        m_moves_slider->init_texture(true);
    } else {
        m_cliper_slider->init_texture(false);
    }
}


void BaseRenderer::init_tool_maker(PresetBundle* preset_bundle) 
{
    // initializes tool marker
    std::string filename;
    if (preset_bundle != nullptr) {
        const Preset* curr = &preset_bundle->printers.get_selected_preset();
        if (curr->is_system)
            filename = PresetUtils::system_printer_hotend_model(*curr);
        else {
            auto* printer_model = curr->config.opt<ConfigOptionString>("printer_model");
            if (printer_model != nullptr && !printer_model->value.empty()) {
                filename = preset_bundle->get_hotend_model_for_printer_model(printer_model->value);
            }

            if (filename.empty()) {
                filename = preset_bundle->get_hotend_model_for_printer_model(PresetBundle::BBL_DEFAULT_PRINTER_MODEL);
            }
        }
    }

    marker.init(filename);

}

void BaseRenderer::on_change_color_mode(bool is_dark) {
    m_is_dark = is_dark;
    marker.on_change_color_mode(m_is_dark);
    gcode_window.on_change_color_mode(m_is_dark);
}

void BaseRenderer::set_scale(float scale)
{
    if (m_layers_slider)
        m_layers_slider->set_scale(scale * GCODE_VIEWER_SLIDER_SCALE);
    if (m_moves_slider)
        m_moves_slider->set_scale(scale * GCODE_VIEWER_SLIDER_SCALE);
    if (m_cliper_slider)
        m_cliper_slider->set_scale(scale * GCODE_VIEWER_SLIDER_SCALE);
    m_scale = scale;
    marker.m_scale = scale;
}

void BaseRenderer::update_by_mode(ConfigOptionMode mode)
{
    view_type_items.clear();
    view_type_items_str.clear();
    options_items.clear();

    // BBS initialzed view_type items
    view_type_items.push_back(EViewType::FeatureType);
    // Creality:for appearance shortage
#if ENABLE_AUE_CUSTOM_PREVIEW
    if (!m_only_gcode_in_preview)
        view_type_items.push_back(EViewType::Custom);
#endif // ENABLE_AUE_CUSTOM_PREVIEW
    view_type_items.push_back(EViewType::ColorPrint);
    view_type_items.push_back(EViewType::Feedrate);
    view_type_items.push_back(EViewType::Height);
    view_type_items.push_back(EViewType::Width);
    view_type_items.push_back(EViewType::VolumetricRate);
    view_type_items.push_back(EViewType::LayerTime);
    view_type_items.push_back(EViewType::LayerTimeLog);
    view_type_items.push_back(EViewType::FanSpeed);
    view_type_items.push_back(EViewType::Temperature);
    view_type_items.push_back(EViewType::Acceleration);
    //if (mode == ConfigOptionMode::comDevelop) {
    //    view_type_items.push_back(EViewType::Tool);
    //}

    for (int i = 0; i < view_type_items.size(); i++) {
        view_type_items_str.push_back(get_view_type_string(view_type_items[i]));
    }

    // Creality:for appearance shortage
    {
        auto it = std::find(view_type_items.begin(), view_type_items.end(), m_view_type);
        if (it != view_type_items.end())
            m_view_type_sel = static_cast<int>(std::distance(view_type_items.begin(), it));
        else {
            m_view_type_sel = 0;
            set_view_type(view_type_items.front());
        }
    }

    // BBS for first layer inspection
    view_type_items.push_back(EViewType::FilamentId);

    options_items.push_back(EMoveType::Travel);
    options_items.push_back(EMoveType::Retract);
    options_items.push_back(EMoveType::Unretract);
    options_items.push_back(EMoveType::Wipe);
    //if (mode == ConfigOptionMode::comDevelop) {
    //    options_items.push_back(EMoveType::Tool_change);
    //}
    //BBS: seam is not real move and extrusion, put at last line
    options_items.push_back(EMoveType::Seam);
}

std::vector<int> BaseRenderer::get_plater_extruder()
{
    return m_plater_extruder;
}

//BBS: always load shell at preview
void BaseRenderer::load(const GCodeProcessorResult& gcode_result, const Print& print, const BuildVolume& build_volume,
                const std::vector<BoundingBoxf3>& exclude_bounding_box, ConfigOptionMode mode, bool only_gcode)
{
    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " start";
    BOOST_LOG_TRIVIAL(error) << "[PERF_TIMING] GCODE_VIEWER_LOAD START";
    auto _perf_viewer_start = std::chrono::steady_clock::now();
    system_memory_stats(__FUNCTION__);
    m_is_belt = gcode_result.machine_is_belt;

    // avoid processing if called with the same gcode_result
    if (m_last_result_id == gcode_result.id) {
        //BBS: add logs
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": the same id %1%, return directly, result %2% ") % m_last_result_id % (&gcode_result);
        return;
    }

    ResGuard res([&]
        {
            m_bLoaded = false;
            if (!only_gcode)
            {
                wxGetApp().notification_manager()->set_slicing_progress_began();
                print.set_status(95, _u8L("Loading display data"));
                wxGetApp().process_msg_loop();
            }
        },
        [&]
        {
            if (!only_gcode)
            {
                wxGetApp().notification_manager()->set_slicing_progress_hidden();
                wxGetApp().process_msg_loop();
            }
            m_bLoaded = true;
        });

    //BBS: add logs
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": gcode result %1%, new id %2%, gcode file %3% ") % (&gcode_result) % m_last_result_id % gcode_result.filename;

    // release gpu memory, if used
    reset();

    //BBS: add mutex for protection of gcode result
    gcode_result.lock();
    //BBS: add safe check
    if (gcode_result.moves.size() == 0) {
        //result cleaned before slicing ,should return here
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << boost::format(": gcode result reset before, return directly!");
        gcode_result.unlock();
        return;
    }

	PartPlate* current_plate = wxGetApp().plater()->get_partplate_list().get_curr_plate();
    bool current_has_print_instances = current_plate->has_printable_instances();
    bool only_gcode_3mf = current_plate->is_slice_result_valid() && wxGetApp().model().objects.empty() && !current_has_print_instances;

    //BBS: move the id to the end of reset
    m_last_result_id = gcode_result.id;
    m_gcode_result = &gcode_result;
    m_only_gcode_in_preview = only_gcode || only_gcode_3mf;
    // Creality:for appearance shortage
    m_user_mode = mode;
    update_by_mode(mode);
    m_contentWidth = -1.0f;

    gcode_window.load_gcode(gcode_result.filename, gcode_result.lines_ends);

    //BBS: add only gcode mode
    //if (wxGetApp().is_gcode_viewer())
    if (m_only_gcode_in_preview)
        m_custom_gcode_per_print_z = gcode_result.custom_gcode_per_print_z;

    m_max_print_height = gcode_result.printable_height;
    PresetBundle* preset_bundle   = wxGetApp().preset_bundle;
    bool          machine_is_belt = preset_bundle->printers.get_edited_preset().config.opt_bool("machine_is_belt");
    if (machine_is_belt) {
        m_max_print_height = 0.0f;
    }

    bool rt = load_toolpaths(gcode_result, build_volume, exclude_bounding_box);

    //BBS: add mutex for protection of gcode result
    if (!rt) {
        gcode_result.unlock();
        return;
    }

    m_settings_ids = gcode_result.settings_ids;
    m_filament_diameters = gcode_result.filament_diameters;
    m_filament_densities = gcode_result.filament_densities;

    if (m_only_gcode_in_preview) {
        Pointfs printable_area;
        //BBS: add bed exclude area
        Pointfs bed_exclude_area = Pointfs();
        std::string texture;
        std::string model;

        if (!gcode_result.printable_area.empty()) {
            // bed shape detected in the gcode
            printable_area    = gcode_result.printable_area;
            const auto    bundle = wxGetApp().preset_bundle;
            const Preset* preset = nullptr;
            if (bundle != nullptr && !gcode_result.printer_model.empty()) {
                // Creality: find in all presets
                auto&         ppresets = bundle->printers.get_presets();
                for (const Preset& p : ppresets) {
                    if (p.name.find(gcode_result.printer_model) != std::string::npos) {
                        preset = &p;
                        break;
                    }
                }
            }

            //BBS: add bed exclude area
            if (!gcode_result.bed_exclude_area.empty())
                bed_exclude_area = gcode_result.bed_exclude_area;

            //Creality: update bed only by printer_model
            if (preset)
                wxGetApp().plater()->set_gcode_bed_shape(printable_area, bed_exclude_area, gcode_result.printable_height, *preset);
            else
                wxGetApp().plater()->set_bed_shape(printable_area, bed_exclude_area, gcode_result.printable_height, "", "", true);  
                
        }
    }

    m_print_statistics = gcode_result.print_statistics;

    if (m_time_estimate_mode != PrintEstimatedStatistics::ETimeMode::Normal) {
        const auto& selected_mode = m_print_statistics.modes[static_cast<size_t>(m_time_estimate_mode)];
        const auto& normal_mode = m_print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)];
        const double time = print_time_for_display(selected_mode);
        if (time == 0.0f ||
            short_time(get_time_dhms(time)) == short_time(get_time_dhms(print_time_for_display(normal_mode))))
            m_time_estimate_mode = PrintEstimatedStatistics::ETimeMode::Normal;
    }

    // set to color print by default if use multi extruders
    if (m_extruder_ids.size() > 1) {
        for (int i = 0; i < view_type_items.size(); i++) {
            if (view_type_items[i] == EViewType::ColorPrint) {
                m_view_type_sel = i;
                break;
            }
        }

        set_view_type(EViewType::ColorPrint);
    }

    m_fold = false;
    _on_set_fold(false);

    if (only_gcode_3mf) {
        wxGetApp().plater()->set_only_gcode(true);
        wxGetApp().plater()->check_sidebar_state_in_only_gcode_mode();
    }
    m_layers_slider->set_menu_enable(!(only_gcode || only_gcode_3mf));
    m_layers_slider->set_as_dirty();
    m_moves_slider->set_as_dirty();

    m_toolpath_outside_result = gcode_result.toolpath_outside_result;

    //BBS: add mutex for protection of gcode result
    gcode_result.unlock();
    {
        auto _vms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _perf_viewer_start).count();
        BOOST_LOG_TRIVIAL(error) << "[PERF_TIMING] GCODE_VIEWER_LOAD END elapsed=" << _vms << "ms";
        auto _tms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - wxGetApp().m_perf_slice_start_time).count();
        BOOST_LOG_TRIVIAL(error) << "[PERF_TIMING] TOTAL END elapsed=" << _tms << "ms";
    }
    
}

void BaseRenderer::refresh(const GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors)
{
    if (!m_bLoaded) {
        return;
    }

	if (m_gcode_result == nullptr) {
		return;
	}

	if (m_last_result_id != gcode_result.id || m_gcode_result != &gcode_result) {
		return;
	}

#if ENABLE_GCODE_VIEWER_STATISTICS
    auto start_time = std::chrono::high_resolution_clock::now();
#endif // ENABLE_GCODE_VIEWER_STATISTICS

    //BBS: add mutex for protection of gcode result
    gcode_result.lock();

    //BBS: add safe check
    if (gcode_result.moves.size() == 0) {
        //result cleaned before slicing ,should return here
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << boost::format(": gcode result reset before, return directly!");
        gcode_result.unlock();
        return;
    }
	
	const auto t_moves_count = gcode_result.moves.size();
    //BBS: add mutex for protection of gcode result
    if (t_moves_count == 0) {
        BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << boost::format(": gcode result m_moves_count is 0, return directly!");
        gcode_result.unlock();
        return;
    }

    wxBusyCursor busy;

    if ((m_view_type == EViewType::Tool
#if ENABLE_AUE_CUSTOM_PREVIEW
         || m_view_type == EViewType::Custom
#endif // ENABLE_AUE_CUSTOM_PREVIEW
        ) && !gcode_result.extruder_colors.empty()) {
        // update tool colors from config stored in the gcode
        decode_colors(gcode_result.extruder_colors, m_tools.m_tool_colors);
        m_tools.m_tool_visibles = std::vector<bool>(m_tools.m_tool_colors.size());
        for (auto item: m_tools.m_tool_visibles) item = true;
    }
    else {
        // update tool colors
        decode_colors(str_tool_colors, m_tools.m_tool_colors);
        m_tools.m_tool_visibles = std::vector<bool>(m_tools.m_tool_colors.size());
        for (auto item : m_tools.m_tool_visibles) item = true;
    }

    for (int i = 0; i < m_tools.m_tool_colors.size(); i++) {
        m_tools.m_tool_colors[i] = adjust_color_for_rendering(m_tools.m_tool_colors[i]);
    }
    ColorRGBA default_color;
    decode_color("#FF8000", default_color);
	// ensure there are enough colors defined
    while (m_tools.m_tool_colors.size() < std::max(size_t(1), gcode_result.extruders_count)) {
        m_tools.m_tool_colors.push_back(default_color);
        m_tools.m_tool_visibles.push_back(true);
    }

    // update ranges for coloring / legend
    m_extrusions.reset_ranges();
    for (size_t i = 0; i < t_moves_count; ++i) {
        // skip first vertex
        if (i == 0)
            continue;

        const GCodeProcessorResult::MoveVertex& curr = gcode_result.moves[i];

        switch (curr.type)
        {
        case EMoveType::Extrude:
        {
            m_extrusions.ranges.height.update_from(round_to_bin(curr.height));
            m_extrusions.ranges.width.update_from(round_to_bin(curr.width));
            m_extrusions.ranges.fan_speed.update_from(curr.fan_speed);
            m_extrusions.ranges.temperature.update_from(curr.temperature);
            if (curr.extrusion_role != erCustom || is_visible(erCustom))
                m_extrusions.ranges.volumetric_rate.update_from(round_to_bin(curr.volumetric_rate()));

            if (curr.layer_duration > 0.f) {
                m_extrusions.ranges.layer_duration.update_from(curr.layer_duration);
				m_extrusions.ranges.layer_duration_log.update_from(curr.layer_duration);
            }
            m_extrusions.ranges.acceleration.update_from(curr.acceleration);
            [[fallthrough]];
        }
        case EMoveType::Travel:
        {
            if (is_toolpath_move_type_visible(curr.type))
                m_extrusions.ranges.feedrate.update_from(curr.feedrate);

            break;
        }
        default: { break; }
        }
    }

#if ENABLE_GCODE_VIEWER_STATISTICS
    m_statistics.refresh_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
#endif // ENABLE_GCODE_VIEWER_STATISTICS

    //BBS: add mutex for protection of gcode result
    gcode_result.unlock();

    // update buffers' render paths
    refresh_render_paths();
}

//void BaseRenderer::refresh_render_paths()
//{
//    refresh_render_paths(false, false);
//}

void BaseRenderer::update_shells_color_by_extruder(const DynamicPrintConfig *config)
{
    if (config != nullptr)
        m_shells.volumes.update_colors_by_extruder(config, false);
}

void BaseRenderer::set_shell_transparency(float alpha) { m_shells.volumes.set_transparency(alpha); }

//BBS: always load shell at preview
void BaseRenderer::reset_shell()
{
    m_shells.volumes.clear();
    m_shells.print_id = -1;
    m_shell_bounding_box = BoundingBoxf3();
}

void BaseRenderer::reset()
{
    //BBS: should also reset the result id
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": current result id %1% ")%m_last_result_id;
    m_last_result_id = -1;
    //BBS: add only gcode mode
    m_only_gcode_in_preview = false;
	m_gcode_result = nullptr;
    m_is_lite_mode = false;
    
    m_ssid_to_moveid_map.clear();
    m_ssid_to_moveid_map.shrink_to_fit();
    
    m_paths_bounding_box = BoundingBoxf3();
    m_printable_area_box    = BoundingBoxf3();
    m_max_bounding_box = BoundingBoxf3();
    m_max_print_height = 0.0f;
    m_tools.m_tool_colors = std::vector<ColorRGBA>();
    m_tools.m_tool_visibles = std::vector<bool>();
    m_extruders_count = 0;
    m_extruder_ids = std::vector<unsigned char>();
    m_filament_diameters = std::vector<float>();
    m_filament_densities = std::vector<float>();
    m_extrusions.reset_ranges();
    //BBS: always load shell at preview
    //m_shells.volumes.clear();
    
    m_layers_z_range = { 0, 0 };
    m_roles = std::vector<ExtrusionRole>();
    m_print_statistics.reset();
    m_custom_gcode_per_print_z = std::vector<CustomGCode::Item>();
    gcode_window.reset();
#if ENABLE_GCODE_VIEWER_STATISTICS
    m_statistics.reset_all();
#endif // ENABLE_GCODE_VIEWER_STATISTICS
    m_contained_in_bed = true;
    m_top_surface_layer = std::set<int>();
    m_contentWidth = -1.0f;
    m_custom_interest_cache_result_id = 0;
    m_custom_interest_by_ssid.clear();
}

void BaseRenderer::release_gcode_file_mapping()
{
    gcode_window.stop_mapping_file();
}

#if 0
bool BaseRenderer::can_export_toolpaths() const
{
    return has_data() && m_buffers[buffer_id(EMoveType::Extrude)].render_primitive_type == TBuffer::ERenderPrimitiveType::Triangle;
}

void BaseRenderer::update_sequential_view_current(unsigned int first, unsigned int last)
{
    auto is_visible = [this](unsigned int id) {
        for (const TBuffer &buffer : m_buffers) {
            if (buffer.visible) {
                for (const Path &path : buffer.paths) {
                    if (path.sub_paths.front().first.s_id <= id && id <= path.sub_paths.back().last.s_id) return true;
                }
            }
        }
        return false;
    };

    const int first_diff = static_cast<int>(first) - static_cast<int>(m_sequential_view.last_current.first);
    const int last_diff  = static_cast<int>(last) - static_cast<int>(m_sequential_view.last_current.last);

    unsigned int new_first = first;
    unsigned int new_last  = last;

    if (m_sequential_view.skip_invisible_moves) {
        while (!is_visible(new_first)) {
            if (first_diff > 0)
                ++new_first;
            else
                --new_first;
        }

        while (!is_visible(new_last)) {
            if (last_diff > 0)
                ++new_last;
            else
                --new_last;
        }
    }

    m_sequential_view.current.first = new_first;
    m_sequential_view.current.last  = new_last;
    m_sequential_view.last_current  = m_sequential_view.current;

    refresh_render_paths(true, true);

    if (new_first != first || new_last != last) {
        update_moves_slider();
    }
}
#endif

Slic3r::GUI::IMSlider* BaseRenderer::get_moves_slider()
{
    return m_moves_slider;
}

Slic3r::GUI::IMSlider* BaseRenderer::get_layers_slider()
{
    return m_layers_slider;
}

Slic3r::GUI::IMSlider* BaseRenderer::get_cliper_slider()
{
    return m_cliper_slider;
}

void BaseRenderer::enable_moves_slider(bool enable) const
{
    bool render_as_disabled = !enable;
    if (m_moves_slider != nullptr && m_moves_slider->is_rendering_as_disabled() != render_as_disabled) {
        m_moves_slider->set_render_as_disabled(render_as_disabled);
        m_moves_slider->set_as_dirty();
    }
}

void BaseRenderer::update_moves_slider(bool set_to_max)
{
    const BaseRenderer::SequentialView &view = get_sequential_view();
    // this should not be needed, but it is here to try to prevent rambling crashes on Mac Asan
    if (view.endpoints.last < view.endpoints.first) return;

    std::vector<double> values(view.endpoints.last - view.endpoints.first + 1);
    std::vector<double> alternate_values(view.endpoints.last - view.endpoints.first + 1);
    unsigned int        count = 0;
    for (unsigned int i = view.endpoints.first; i <= view.endpoints.last; ++i) {
        values[count] = static_cast<double>(i + 1);
        if (view.gcode_ids[i] > 0) alternate_values[count] = static_cast<double>(view.gcode_ids[i]);
        ++count;
    }

    bool keep_min = m_moves_slider->GetActiveValue() == m_moves_slider->GetMinValue();

    m_moves_slider->SetSliderValues(values);
    m_moves_slider->SetSliderAlternateValues(alternate_values);
    m_moves_slider->SetMaxValue(view.endpoints.last - view.endpoints.first);
    m_moves_slider->SetSelectionSpan(view.current.first - view.endpoints.first, view.current.last - view.endpoints.first);
    if (set_to_max)
        m_moves_slider->SetHigherValue(keep_min ? m_moves_slider->GetMinValue() : m_moves_slider->GetMaxValue());
}

void BaseRenderer::update_layers_slider_mode()
{
    //    true  -> single-extruder printer profile OR
    //             multi-extruder printer profile , but whole model is printed by only one extruder
    //    false -> multi-extruder printer profile , and model is printed by several extruders
    bool one_extruder_printed_model = true;

    // extruder used for whole model for multi-extruder printer profile
    int only_extruder = -1;

    // BBS
    if (wxGetApp().filaments_cnt() > 1) {
        const ModelObjectPtrs &objects = wxGetApp().plater()->model().objects;

        // check if whole model uses just only one extruder
        if (!objects.empty()) {
            const int extruder = objects[0]->config.has("extruder") ? objects[0]->config.option("extruder")->getInt() : 0;

            auto is_one_extruder_printed_model = [objects, extruder]() {
                for (ModelObject *object : objects) {
                    if (object->config.has("extruder") && object->config.option("extruder")->getInt() != extruder) return false;

                    for (ModelVolume *volume : object->volumes)
                        if ((volume->config.has("extruder") && volume->config.option("extruder")->getInt() != extruder) || !volume->mmu_segmentation_facets.empty()) return false;

                    for (const auto &range : object->layer_config_ranges)
                        if (range.second.has("extruder") && range.second.option("extruder")->getInt() != extruder) return false;
                }
                return true;
            };

            if (is_one_extruder_printed_model())
                only_extruder = extruder;
            else
                one_extruder_printed_model = false;
        }
    }

    // TODO m_layers_slider->SetModeAndOnlyExtruder(one_extruder_printed_model, only_extruder);
}

void BaseRenderer::update_marker_curr_move() 
{
    if ((int)m_last_result_id != -1) {
        auto it = std::find_if(m_gcode_result->moves.begin(), m_gcode_result->moves.end(), [this](auto move) {
                if (m_sequential_view.current.last < m_sequential_view.gcode_ids.size() && m_sequential_view.current.last >= 0) {
                    return move.gcode_id == static_cast<uint64_t>(m_sequential_view.gcode_ids[m_sequential_view.current.last]);
                }
                return false;
            });
        if (it != m_gcode_result->moves.end())
            marker.update_curr_move(*it);
    }
}

void BaseRenderer::load_shells(const Print& print, bool initialized, bool force_previewing)
{
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": initialized=%1%, force_previewing=%2%")%initialized %force_previewing;
    if ((print.id().id == m_shells.print_id)&&(print.get_modified_count() == m_shells.print_modify_count)) {
        //BBS: update force previewing logic
        if (force_previewing)
            m_shells.previewing = force_previewing;
        //already loaded
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": already loaded, print=%1% print_id=%2%, print_modify_count=%3%, force_previewing %4%")%(&print) %m_shells.print_id %m_shells.print_modify_count %force_previewing;
        return;
    }

    //reset shell firstly
    reset_shell();

    //BBS: move behind of reset_shell, to clear previous shell for empty plate
    if (print.objects().empty()) {
        // no shells, return
        return;
    }
    // adds objects' volumes
    // BBS: fix the issue that object_idx is not assigned as index of Model.objects array
    int object_count = 0;
    const ModelObjectPtrs& model_objs = wxGetApp().model().objects;
	bool  enable_lod   = false;
    for (const PrintObject* obj : print.objects()) {
        const ModelObject* model_obj = obj->model_object();

        int object_idx = -1;
        for (int idx = 0; idx < model_objs.size(); idx++) {
            if (model_objs[idx]->id() == model_obj->id()) {
                object_idx = idx;
                break;
            }
        }

        // BBS: object may be deleted when this method is called when deleting an object
        if (object_idx == -1)
            continue;

        std::vector<int> instance_ids(model_obj->instances.size());
        //BBS: only add the printable instance
        int instance_index = 0;
        for (int i = 0; i < (int)model_obj->instances.size(); ++i) {
            //BBS: only add the printable instance
            if (model_obj->instances[i]->is_printable())
                instance_ids[instance_index++] = i;
        }
        instance_ids.resize(instance_index);

        size_t current_volumes_count = m_shells.volumes.volumes.size();
        m_shells.volumes.load_object(model_obj, object_idx, instance_ids, "object", initialized, enable_lod);

        // adjust shells' z if raft is present
        bool machine_is_belt = false;
        if (wxGetApp().preset_bundle) {
            machine_is_belt = wxGetApp().preset_bundle->machine_is_belt();
        }
        const SlicingParameters& slicing_parameters = obj->slicing_parameters();
        if (slicing_parameters.object_print_z_min != 0.0) {
            const Vec3d z_offset = slicing_parameters.object_print_z_min * (machine_is_belt ? Vec3d{0.0f, 0.0f, 0.0f} : Vec3d::UnitZ());
            for (size_t i = current_volumes_count; i < m_shells.volumes.volumes.size(); ++i) {
                GLVolume* v = m_shells.volumes.volumes[i];
                v->set_volume_offset(v->get_volume_offset() + z_offset);
            }
        }

        object_count++;
    }

    // Orca: disable wipe tower shell
    // if (wxGetApp().preset_bundle->printers.get_edited_preset().printer_technology() == ptFFF) {
        //     // BBS: adds wipe tower's volume
        //     std::vector<unsigned int> print_extruders = print.extruders(true);
        //     int extruders_count = print_extruders.size();

        //     const double max_z = print.objects()[0]->model_object()->get_model()->bounding_box().max(2);
        //     const PrintConfig& config = print.config();
        //     if (config.enable_prime_tower &&
            //         (print.enable_timelapse_print() || (extruders_count > 1 && (config.print_sequence == PrintSequence::ByLayer)))) {
            //         const float depth = print.wipe_tower_data(extruders_count).depth;
            //         const float brim_width = print.wipe_tower_data(extruders_count).brim_width;

            //         int plate_idx = print.get_plate_index();
            //         Vec3d plate_origin = print.get_plate_origin();
            //         double wipe_tower_x = config.wipe_tower_x.get_at(plate_idx) + plate_origin(0);
            //         double wipe_tower_y = config.wipe_tower_y.get_at(plate_idx) + plate_origin(1);
            //         m_shells.volumes.load_wipe_tower_preview(1000, wipe_tower_x, wipe_tower_y, config.prime_tower_width, depth, max_z, config.wipe_tower_rotation_angle,
                //             !print.is_step_done(psWipeTower), brim_width, initialized);
        //     }
    // }

    // remove modifiers
    while (true) {
        GLVolumePtrs::iterator it = std::find_if(m_shells.volumes.volumes.begin(), m_shells.volumes.volumes.end(), [](GLVolume* volume) { return volume->is_modifier; });
        if (it != m_shells.volumes.volumes.end()) {
            m_shells.volumes.release_volume(*it);
            delete (*it);
            m_shells.volumes.volumes.erase(it);
        }
        else
            break;
    }

    for (GLVolume* volume : m_shells.volumes.volumes) {
        volume->zoom_to_volumes = false;
        volume->color.a(0.5f);
        volume->force_native_color = true;
        volume->set_render_color();
        //BBS: add shell bounding box logic
        m_shell_bounding_box.merge(volume->transformed_bounding_box());
    }

    //BBS: always load shell when preview
    m_shells.print_id = print.id().id;
    m_shells.print_modify_count = print.get_modified_count();
    m_shells.previewing = true;
    BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << boost::format(": shell loaded, id change to %1%, modify_count %2%, object count %3%, glvolume count %4%")
        % m_shells.print_id % m_shells.print_modify_count % object_count %m_shells.volumes.volumes.size();
}

void BaseRenderer::on_visibility_changed() 
{
	wxGetApp().plater()->get_current_canvas3D()->set_as_dirty();
}

#if ENABLE_AUE_CUSTOM_PREVIEW

void BaseRenderer::update_custom_interest_regions() const
{
    if (m_gcode_result == nullptr) {
        m_custom_interest_cache_result_id = 0;
        m_custom_interest_by_ssid.clear();
        return;
    }

    if (m_custom_interest_cache_result_id == m_gcode_result->id &&
        m_custom_interest_by_ssid.size() == m_ssid_to_moveid_map.size())
        return;

    m_custom_interest_cache_result_id = m_gcode_result->id;
    m_custom_interest_by_ssid.clear();

    if (m_ssid_to_moveid_map.size() < 2)
        return;

    m_custom_interest_by_ssid.assign(m_ssid_to_moveid_map.size(), 0);

    // Scheme 2: ROI exists only when slicing from a 3mf and is cached into GCodeProcessorResult.
    // When opening a gcode file directly, do not compute ROI on the fly.
    if (m_only_gcode_in_preview || m_gcode_result->custom_interest_by_move_id.empty())
        return;

    for (size_t end_ssid = 0; end_ssid < m_ssid_to_moveid_map.size(); ++end_ssid) {
        const size_t move_id = m_ssid_to_moveid_map[end_ssid];
        if (move_id < m_gcode_result->custom_interest_by_move_id.size())
            m_custom_interest_by_ssid[end_ssid] = m_gcode_result->custom_interest_by_move_id[move_id];
    }
}

#endif // ENABLE_AUE_CUSTOM_PREVIEW

#if 0
void BaseRenderer::render_toolpaths()
{
    if (!m_bLoaded) {
        return;
    }

    const Camera& camera = wxGetApp().plater()->get_camera();
    const double zoom = camera.get_zoom();

    auto render_as_lines = [
#if ENABLE_GCODE_VIEWER_STATISTICS
        this
#endif // ENABLE_GCODE_VIEWER_STATISTICS
    ](std::vector<RenderPath>::iterator it_path, std::vector<RenderPath>::iterator it_end, GLShaderProgram& shader, int uniform_color) {
        for (auto it = it_path; it != it_end && it_path->ibuffer_id == it->ibuffer_id; ++it) {
            const RenderPath& path = *it;
            // Some OpenGL drivers crash on empty glMultiDrawElements, see GH #7415.
            assert(! path.sizes.empty());
            assert(! path.offsets.empty());
            shader.set_uniform(uniform_color, path.color);
            glsafe(::glMultiDrawElements(GL_LINES, (const GLsizei*)path.sizes.data(), GL_UNSIGNED_SHORT, (const void* const*)path.offsets.data(), (GLsizei)path.sizes.size()));
#if ENABLE_GCODE_VIEWER_STATISTICS
            ++m_statistics.gl_multi_lines_calls_count;
#endif // ENABLE_GCODE_VIEWER_STATISTICS
        }
    };

    auto render_as_triangles = [
#if ENABLE_GCODE_VIEWER_STATISTICS
        this
#endif // ENABLE_GCODE_VIEWER_STATISTICS
    ](std::vector<RenderPath>::iterator it_path, std::vector<RenderPath>::iterator it_end, GLShaderProgram& shader, int uniform_color) {
        for (auto it = it_path; it != it_end && it_path->ibuffer_id == it->ibuffer_id; ++it) {
            const RenderPath& path = *it;
            // Some OpenGL drivers crash on empty glMultiDrawElements, see GH #7415.
            assert(! path.sizes.empty());
            assert(! path.offsets.empty());
            shader.set_uniform(uniform_color, path.color);
            glsafe(::glMultiDrawElements(GL_TRIANGLES, (const GLsizei*)path.sizes.data(), GL_UNSIGNED_SHORT, (const void* const*)path.offsets.data(), (GLsizei)path.sizes.size()));
#if ENABLE_GCODE_VIEWER_STATISTICS
            ++m_statistics.gl_multi_triangles_calls_count;
#endif // ENABLE_GCODE_VIEWER_STATISTICS
        }
    };


	auto render_as_triangles_2 = [this, render_as_triangles](std::vector<RenderPath>::iterator it_path, std::vector<RenderPath>::iterator it_end, GLShaderProgram& shader, int uniform_color, const int dynamic_stride, const Camera& camera) {
		
		/*const std::vector<double>& z_offsets = get_filtered_layers_z_offset(dynamic_stride); 

		if (z_offsets.size() != m_layers.size()) {
            render_as_triangles(it_path, it_end, shader, uniform_color);
			return;
		}*/

		//int last_layer_no = -1;
        int last_filtered_layer = -1;
        bool last_filtered_layer_result = false;
	
        for (auto it = it_path; it != it_end && it_path->ibuffer_id == it->ibuffer_id; ++it) {
            
			const RenderPath& path = *it;
            // Some OpenGL drivers crash on empty glMultiDrawElements, see GH #7415.
            assert(!path.sizes.empty());
            assert(!path.offsets.empty());

			int current_layer = path.layer_no;
            /*if (should_be_filtered_of_layer(dynamic_stride, current_layer))
                continue;*/

			if (last_filtered_layer == current_layer) {
                if (last_filtered_layer_result)
                    continue;
            } else {
                last_filtered_layer        = current_layer;
                last_filtered_layer_result = should_be_filtered_of_layer(dynamic_stride, current_layer);
                if (last_filtered_layer_result)
                    continue;
			}
			
            shader.set_uniform(uniform_color, path.color);

			/*if (last_layer_no != current_layer && dynamic_stride > 2) {
                last_layer_no = current_layer;
                float z_offset = z_offsets[current_layer];

                const Transform3d model_matrix = Geometry::assemble_transform((z_offset * Vec3f::UnitZ()).cast<double>());
                shader.set_uniform("view_model_matrix", camera.get_view_matrix() * model_matrix);
			}*/
			
            glsafe(::glMultiDrawElements(GL_TRIANGLES, (const GLsizei*)path.sizes.data(), GL_UNSIGNED_SHORT,
                                         (const void* const*) path.offsets.data(), (GLsizei) path.sizes.size()));
#if ENABLE_GCODE_VIEWER_STATISTICS
            ++m_statistics.gl_multi_triangles_calls_count;
#endif // ENABLE_GCODE_VIEWER_STATISTICS
        }
    };

    auto render_as_instanced_model = [
#if ENABLE_GCODE_VIEWER_STATISTICS
        this
#endif // ENABLE_GCODE_VIEWER_STATISTICS
        ](TBuffer& buffer, GLShaderProgram & shader) {
        for (auto& range : buffer.model.instances.render_ranges.ranges) {
            if (range.vbo == 0 && range.count > 0) {
                glsafe(::glGenBuffers(1, &range.vbo));
                glsafe(::glBindBuffer(GL_ARRAY_BUFFER, range.vbo));
                glsafe(::glBufferData(GL_ARRAY_BUFFER, range.count * buffer.model.instances.instance_size_bytes(), (const void*)&buffer.model.instances.buffer[range.offset * buffer.model.instances.instance_size_floats()], GL_STATIC_DRAW));
                glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));
            }

            if (range.vbo > 0) {
                buffer.model.model.set_color(range.color);
                buffer.model.model.render_instanced(range.vbo, range.count);
#if ENABLE_GCODE_VIEWER_STATISTICS
                ++m_statistics.gl_instanced_models_calls_count;
                m_statistics.total_instances_gpu_size += static_cast<int64_t>(range.count * buffer.model.instances.instance_size_bytes());
#endif // ENABLE_GCODE_VIEWER_STATISTICS
            }
        }
    };

#if ENABLE_GCODE_VIEWER_STATISTICS
        auto render_as_batched_model = [this](TBuffer& buffer, GLShaderProgram& shader, int position_id, int normal_id) {
#else
        auto render_as_batched_model = [this](TBuffer& buffer, GLShaderProgram& shader, int position_id, int normal_id) {
#endif // ENABLE_GCODE_VIEWER_STATISTICS
        struct Range
        {
            unsigned int first;
            unsigned int last;
            bool intersects(const Range& other) const { return (other.last < first || other.first > last) ? false : true; }
        };
        Range buffer_range = { 0, 0 };
        const size_t indices_per_instance = buffer.model.data.indices_count();

        for (size_t j = 0; j < buffer.indices.size(); ++j) {
            const IBuffer& i_buffer = buffer.indices[j];
            buffer_range.last = buffer_range.first + i_buffer.count / indices_per_instance;
            glsafe(::glBindBuffer(GL_ARRAY_BUFFER, i_buffer.vbo));
            if (position_id != -1) {
                if (m_is_mem_optim) {
                    glsafe(::glVertexAttribPointer(position_id, buffer.vertices.position_size_floats(), GL_SHORT, GL_FALSE,
                                                   buffer.vertices.vertex_size_floats() * sizeof(short),
                                                   (const void*) (buffer.vertices.position_offset_floats() * sizeof(short))));
				} else {
                    glsafe(::glVertexAttribPointer(position_id, buffer.vertices.position_size_floats(), GL_FLOAT, GL_FALSE,
                     buffer.vertices.vertex_size_bytes(), (const void*)buffer.vertices.position_offset_bytes()));
				}
                
                
                glsafe(::glEnableVertexAttribArray(position_id));
            }
            const bool has_normals = buffer.vertices.normal_size_floats() > 0;
            if (has_normals) {
                if (normal_id != -1) {
                    if (m_is_mem_optim) {
                        glsafe(::glVertexAttribPointer(normal_id, buffer.vertices.normal_size_floats(), GL_SHORT, GL_FALSE,
                                                       buffer.vertices.vertex_size_floats() * sizeof(short),
                                                       (const void*) (buffer.vertices.normal_offset_floats() * sizeof(short))));
                    } else {
                        glsafe(::glVertexAttribPointer(normal_id, buffer.vertices.normal_size_floats(), GL_FLOAT, GL_FALSE,
                                                       buffer.vertices.vertex_size_bytes(),
                                                       (const void*) buffer.vertices.normal_offset_bytes()));
                    }
                    
                    glsafe(::glEnableVertexAttribArray(normal_id));
                }
            }

            glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i_buffer.ibo));

            for (auto& range : buffer.model.instances.render_ranges.ranges) {
                const Range range_range = { range.offset, range.offset + range.count };
                if (range_range.intersects(buffer_range)) {
                    shader.set_uniform("uniform_color", range.color);
                    const unsigned int offset = (range_range.first > buffer_range.first) ? range_range.first - buffer_range.first : 0;
                    const size_t offset_bytes = static_cast<size_t>(offset) * indices_per_instance * sizeof(IBufferType);
                    const Range render_range = { std::max(range_range.first, buffer_range.first), std::min(range_range.last, buffer_range.last) };
                    const size_t count = static_cast<size_t>(render_range.last - render_range.first) * indices_per_instance;
                    if (count > 0) {
                        glsafe(::glDrawElements(GL_TRIANGLES, (GLsizei)count, GL_UNSIGNED_SHORT, (const void*)offset_bytes));
#if ENABLE_GCODE_VIEWER_STATISTICS
                        ++m_statistics.gl_batched_models_calls_count;
#endif // ENABLE_GCODE_VIEWER_STATISTICS
                    }
                }
            }

            glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

            if (normal_id != -1)
                glsafe(::glDisableVertexAttribArray(normal_id));
            if (position_id != -1)
                glsafe(::glDisableVertexAttribArray(position_id));
            glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));

            buffer_range.first = buffer_range.last;
        }
    };

    auto line_width = [](double zoom) {
        return (zoom < 5.0) ? 1.0 : (1.0 + 5.0 * (zoom - 5.0) / (100.0 - 5.0));
    };

	double pixes  = estimate_pixels_of_one_layer();
    int    stride = get_dynamic_stride(pixes);

    const unsigned char begin_id = buffer_id(EMoveType::Retract);
    const unsigned char end_id   = buffer_id(EMoveType::Count);

    Transform3d transform = wxGetApp().plater()->get_current_canvas3D()->get_preview_extra_transform();
    for (unsigned char i = begin_id; i < end_id; ++i) {

        if (m_is_lite_mode && !(i == buffer_id(EMoveType::Extrude) || i == buffer_id(EMoveType::Seam) || i == buffer_id(EMoveType::Unretract) || i == buffer_id(EMoveType::Pause_Print)))
            continue;

        TBuffer& buffer = m_buffers[i];
        if (!buffer.visible || !buffer.has_data())
            continue;

        GLShaderProgram* shader = wxGetApp().get_shader(m_is_mem_optim ? ("gcode_" + buffer.shader).c_str() : buffer.shader.c_str());
        if (shader == nullptr)
            continue;

        shader->start_using();
        shader->set_uniform("view_model_matrix", camera.get_view_matrix() * transform);
        shader->set_uniform("projection_matrix", camera.get_projection_matrix());
        shader->set_uniform("view_normal_matrix", (Matrix3d)Matrix3d::Identity());
        if (m_is_mem_optim) {
            shader->set_uniform("position_origin", m_paths_bounding_box.min);
		}
	

        if (buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::InstancedModel) {
            shader->set_uniform("emission_factor", 0.25f);
            render_as_instanced_model(buffer, *shader);
            shader->set_uniform("emission_factor", 0.0f);
        }
        else if (buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::BatchedModel) {
            shader->set_uniform("emission_factor", 0.25f);
            const int position_id = shader->get_attrib_location("v_position");
            const int normal_id   = shader->get_attrib_location("v_normal");
            render_as_batched_model(buffer, *shader, position_id, normal_id);
            shader->set_uniform("emission_factor", 0.0f);
        }
        else {
            const int position_id = shader->get_attrib_location("v_position");
            const int normal_id   = shader->get_attrib_location("v_normal");
            const int uniform_color = shader->get_uniform_location("uniform_color");

            auto it_path = buffer.render_paths.begin();
            for (unsigned int ibuffer_id = 0; ibuffer_id < static_cast<unsigned int>(buffer.indices.size()); ++ibuffer_id) {
                const IBuffer& i_buffer = buffer.indices[ibuffer_id];
                // Skip all paths with ibuffer_id < ibuffer_id.
                for (; it_path != buffer.render_paths.end() && it_path->ibuffer_id < ibuffer_id; ++it_path);
                if (it_path == buffer.render_paths.end() || it_path->ibuffer_id > ibuffer_id)
                    // Not found. This shall not happen.
                    continue;

                glsafe(::glBindBuffer(GL_ARRAY_BUFFER, i_buffer.vbo));
                if (position_id != -1) {
                    if (m_is_mem_optim) {
                        glsafe(::glVertexAttribPointer(position_id, buffer.vertices.position_size_floats(), GL_SHORT, GL_FALSE,
                                                       buffer.vertices.vertex_size_floats() * sizeof(short),
                                                       (const void*) (buffer.vertices.position_offset_floats() * sizeof(short))));
                    } else {
						glsafe(::glVertexAttribPointer(position_id, buffer.vertices.position_size_floats(), GL_FLOAT, GL_FALSE, buffer.vertices.vertex_size_bytes(), (const void*)buffer.vertices.position_offset_bytes()));
                    }
                    
                    glsafe(::glEnableVertexAttribArray(position_id));
                }
                const bool has_normals = buffer.vertices.normal_size_floats() > 0;
                if (has_normals) {
                    if (normal_id != -1) {
                        if (m_is_mem_optim) {
                            glsafe(::glVertexAttribPointer(normal_id, buffer.vertices.normal_size_floats(), GL_SHORT, GL_FALSE,
                                                           buffer.vertices.vertex_size_floats() * sizeof(short),
                                                           (const void*) (buffer.vertices.normal_offset_floats() * sizeof(short))));
                        } else {
                            glsafe(::glVertexAttribPointer(normal_id, buffer.vertices.normal_size_floats(), GL_FLOAT, GL_FALSE,
                                                           buffer.vertices.vertex_size_bytes(),
                                                           (const void*) buffer.vertices.normal_offset_bytes()));
                        }
                        
                        glsafe(::glEnableVertexAttribArray(normal_id));
                    }
                }

                glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i_buffer.ibo));

                // Render all elements with it_path->ibuffer_id == ibuffer_id, possible with varying colors.
                switch (buffer.render_primitive_type)
                {
                case TBuffer::ERenderPrimitiveType::Line: {
                    glsafe(::glLineWidth(static_cast<GLfloat>(line_width(zoom))));
                    render_as_lines(it_path, buffer.render_paths.end(), *shader, uniform_color);
                    break;
                }
                case TBuffer::ERenderPrimitiveType::Triangle: {
                    if (!m_is_lod) {
                        render_as_triangles(it_path, buffer.render_paths.end(), *shader, uniform_color);
                    } else {
                        render_as_triangles_2(it_path, buffer.render_paths.end(), *shader, uniform_color, stride, camera);
                    }
                    break;
                }
                default: { break; }
                }

                glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

                if (normal_id != -1)
                    glsafe(::glDisableVertexAttribArray(normal_id));
                if (position_id != -1)
                    glsafe(::glDisableVertexAttribArray(position_id));
                glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));
            }
        }

        shader->stop_using();
    }

#if ENABLE_GCODE_VIEWER_STATISTICS
    auto render_sequential_range_cap = [this, &camera]
#else
    auto render_sequential_range_cap = [this, &camera]
#endif // ENABLE_GCODE_VIEWER_STATISTICS
    (const SequentialRangeCap& cap) {
        const TBuffer* buffer = cap.buffer;
            GLShaderProgram* shader = wxGetApp().get_shader(m_is_mem_optim ? ("gcode_" + buffer->shader).c_str() :
                                                                         buffer->shader.c_str());
        if (shader == nullptr)
            return;

        shader->start_using();

        shader->set_uniform("view_model_matrix", camera.get_view_matrix());
        shader->set_uniform("projection_matrix", camera.get_projection_matrix());
        shader->set_uniform("view_normal_matrix", (Matrix3d)Matrix3d::Identity());

        const int position_id = shader->get_attrib_location("v_position");
        const int normal_id   = shader->get_attrib_location("v_normal");

        glsafe(::glBindBuffer(GL_ARRAY_BUFFER, cap.vbo));
        if (position_id != -1) {
            if (m_is_mem_optim) {
                glsafe(::glVertexAttribPointer(position_id, buffer->vertices.position_size_floats(), GL_SHORT, GL_FALSE,
                                               buffer->vertices.vertex_size_floats() * sizeof(short),
                                               (const void*) (buffer->vertices.position_offset_floats() * sizeof(short))));
            } else {
                glsafe(::glVertexAttribPointer(position_id, buffer->vertices.position_size_floats(), GL_FLOAT, GL_FALSE,
                                               buffer->vertices.vertex_size_bytes(),
                                               (const void*) buffer->vertices.position_offset_bytes()));
            }
            glsafe(::glEnableVertexAttribArray(position_id));
        }
        const bool has_normals = buffer->vertices.normal_size_floats() > 0;
        if (has_normals) {
            if (normal_id != -1) {
                if (m_is_mem_optim) {
                    glsafe(::glVertexAttribPointer(normal_id, buffer->vertices.normal_size_floats(), GL_SHORT, GL_FALSE,
                                                   buffer->vertices.vertex_size_floats() * sizeof(short),
                                                   (const void*) (buffer->vertices.normal_offset_floats() * sizeof(short))));

                } else {
                    glsafe(::glVertexAttribPointer(normal_id, buffer->vertices.normal_size_floats(), GL_FLOAT, GL_FALSE,
                                                   buffer->vertices.vertex_size_bytes(),
                                                   (const void*) buffer->vertices.normal_offset_bytes()));
                }
                glsafe(::glEnableVertexAttribArray(normal_id));
            }
        }

        shader->set_uniform("uniform_color", cap.color);

        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cap.ibo));
        glsafe(::glDrawElements(GL_TRIANGLES, (GLsizei)cap.indices_count(), GL_UNSIGNED_SHORT, nullptr));
        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

#if ENABLE_GCODE_VIEWER_STATISTICS
            ++m_statistics.gl_triangles_calls_count;
#endif // ENABLE_GCODE_VIEWER_STATISTICS

        if (normal_id != -1)
            glsafe(::glDisableVertexAttribArray(normal_id));
        if (position_id != -1)
            glsafe(::glDisableVertexAttribArray(position_id));

        glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));

        shader->stop_using();
    };

    for (unsigned int i = 0; i < 2; ++i) {
        if (m_sequential_range_caps[i].is_renderable())
            render_sequential_range_cap(m_sequential_range_caps[i]);
    }
}
#endif 

void BaseRenderer::render_shells(int canvas_width, int canvas_height)
{
    //BBS: add shell previewing logic
    if ((!m_shells.previewing && !m_shells.visible) || m_shells.volumes.empty())
        return;

    GLShaderProgram* shader = wxGetApp().get_shader("gouraud_preview");
    if (shader == nullptr)
        return;

    glsafe(::glDepthMask(GL_FALSE));

    shader->start_using();
    shader->set_uniform("emission_factor", 0.1f);
    Plater* plater = wxGetApp().plater();
    const Camera& camera = plater->get_camera();
    Model& model = plater->model();
    const CustomGCode::Info& info = model.plates_custom_gcodes[model.curr_plate_index];
    const Slic3r::DynamicPrintConfig* config = &wxGetApp().preset_bundle->project_config;
    if (config->has("filament_colour")) 
    {
        std::vector<std::string> filament_colors = (config->option<ConfigOptionStrings>("filament_colour"))->values;
        GLVolumeCollection::apply_custom_gcode(shader, info, filament_colors);
    }

    Transform3d transform = wxGetApp().plater()->get_current_canvas3D()->get_preview_extra_transform();
    m_shells.volumes.render(GUI::ERenderPipelineStage::Normal, GLVolumeCollection::ERenderType::Transparent, false,
                            camera.get_view_matrix() * transform, camera.get_projection_matrix());
    shader->set_uniform("emission_factor", 0.0f);
    shader->stop_using();

    glsafe(::glDepthMask(GL_TRUE));
}

void BaseRenderer::render_legend(int canvas_width, int canvas_height) 
 {
    ImVec4 tmp_color = ImVec4(158.0f / 255.0, 158.0f / 255.0, 158.0f / 255.0, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImGui::ColorConvertU32ToFloat4(IM_COL32_BLACK_TRANS));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, tmp_color);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, tmp_color);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, tmp_color);
    /*ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 3.0f);*/
    const float fold_icon_draw_size = 22.0f * m_scale;
    const unsigned fold_icon_raster_size =
        std::max(1u, static_cast<unsigned>(std::ceil(fold_icon_draw_size)));
    if (m_fold_icon_svg_texture == nullptr || m_fold_icon_svg_size != fold_icon_raster_size) {
        IMTexture::release_texture(m_fold_icon_svg_texture);
        m_fold_icon_svg_size = 0;
        if (IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/fold_dark_default.svg", fold_icon_raster_size,
                fold_icon_raster_size, m_fold_icon_svg_texture))
            m_fold_icon_svg_size = fold_icon_raster_size;
    }

    GcodeHelper helper(m_scale, (int) m_time_estimate_mode, options_items, m_roles, *this, m_extrusions, m_print_statistics, m_extruder_ids,
                       m_filament_diameters, m_filament_densities, m_fold_icon_svg_texture);

    auto wcfg = helper.prepare(m_fold, m_contentWidth);
    // wcfg.bgalpha = 5/100.f;
    DispConfig().processWindows(
        "gcode_legend",
        [&]() {
            {
                const ImGuiStyle& style     = ImGui::GetStyle();
                ImVec2            win_pos   = ImGui::GetWindowPos();
                ImVec2            win_sz    = ImGui::GetWindowSize();
                ImDrawList*       bg        = ImGui::GetBackgroundDrawList();
                const ImVec4      sh_rgb    = ImVec4(118.0f / 255.0f, 142.0f / 255.0f, 171.0f / 255.0f, 1.0f);
                const float       alphas[8] = {0.050f, 0.040f, 0.032f, 0.026f, 0.020f, 0.014f, 0.010f, 0.006f};
                const float       steps[8]  = {2.00f, 1.60f, 1.20f, 0.90f, 0.70f, 0.50f, 0.35f, 0.20f};
                for (int i = 0; i < 8; ++i) {
                    float spread = steps[i] * m_scale;
                    float round  = style.WindowRounding + spread;
                    float off_x  = spread * 0.10f;
                    float off_y  = spread * 0.80f;
                    ImU32 col    = ImGui::ColorConvertFloat4ToU32(ImVec4(sh_rgb.x, sh_rgb.y, sh_rgb.z, alphas[i]));
                    bg->AddRectFilled(ImVec2(win_pos.x - off_x, win_pos.y + off_y),
                                      ImVec2(win_pos.x + win_sz.x + off_x, win_pos.y + win_sz.y + spread + off_y), col, round);
                }
            }

            bool old = m_fold;
            m_fold   = helper.showTitle(old);
            if (m_fold != old) {
                _on_set_fold(m_fold);
            }
            if (m_fold)
                return;

            if (wxGetApp().easy_mode())
                return;

            bool  isReduceHeight      = false;
            auto* notificationManager = wxGetApp().plater()->get_notification_manager();
            if (notificationManager) {
                size_t notificationNum = notificationManager->get_warning_and_error_notification_count();
                if (notificationNum > 0) {
                    isReduceHeight = true;
                }
            }

            bool option_folded = helper.showOption(m_showMark, m_showBed, m_showColor);
            if (option_folded)
                return;
            if (m_showColor) {
                ImGui::Dummy(ImVec2(0, 5));
                int ret = DispConfig().combo(view_type_items_str, m_view_type_sel);
                if (ret >= 0) {
                    m_view_type_sel = ret;
                    set_view_type(view_type_items[m_view_type_sel]);
                    m_contentWidth = -1.0f;
                    reset_visible(view_type_items[m_view_type_sel]);
                    // refresh_render_paths(false, false);
                    refresh_render_paths();
                    update_moves_slider();
                    wxGetApp().plater()->get_current_canvas3D()->set_as_dirty();
                }

                {
                    if (!m_only_gcode_in_preview) {
                        const bool        is_lite_mode = m_is_lite_mode;
                        ImGuiContext&     g            = *GImGui;
                        ImGuiWindow*      window       = g.CurrentWindow;
                        const ImGuiStyle& style        = g.Style;

                        ImGui::SameLine();

                        auto label   = _u8L("Lite Mode");
                        auto labelsz = ImGui::CalcTextSize(label.c_str());

                        const float h = ImGui::GetTextLineHeight();
                        ImVec2      switch_size(2.0f * h, h);
                        ImVec2      grap_size(h, h);

                        // draw text
                        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - grap_size.x - switch_size.x - labelsz.x - 2);
                        ImVec2 p_start    = window->DC.CursorPos;
                        bool   hover_text = ImGui::IsMouseHoveringRect(p_start, ImVec2(p_start.x + labelsz.x, p_start.y + labelsz.y));
                        ImVec4 text_color = hover_text ? ImGuiWrapper::COL_CREALITY : ImGui::GetStyleColorVec4(ImGuiCol_Text);
                        ImGui::TextColored(text_color, "%s", label.c_str());

                        ImVec2 textPos = ImGui::GetItemRectMin();
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(textPos.x, textPos.y + labelsz.y),
                                                            ImVec2(textPos.x + labelsz.x, textPos.y + labelsz.y),
                                                            ImGui::ColorConvertFloat4ToU32(text_color));

                        if (ImGui::IsItemHovered()) {
                            // ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                            if (ImGui::IsMouseClicked(0)) {
                                wxGetApp().open_browser_with_warning_dialog(wxGetApp().app_config->get("language").find("zh_CN") == 0 ?
                                                                                "https://wiki.creality.com/zh/software/6-0/lite-mode" :
                                                                                "https://wiki.creality.com/en/software/6-0/lite-mode");
                            }
                        }

                        ImGui::SameLine();

                        // draw switch
                        ImVec2 cur_pos = window->DC.CursorPos;
                        ImVec2 p_min(cur_pos.x, cur_pos.y + g.Style.ItemSpacing.y);
                        ImVec2 p_max = ImVec2(p_min.x + switch_size.x, p_min.y + switch_size.y);

                        ImVec4 border_color = m_is_dark ? ImVec4(64.0f / 255.0f, 64.0f / 255.0f, 64.0f / 255.0f, 1.0f) :
                                                          ImVec4(218.0f / 255.0f, 219.0f / 255.0f, 223.0f / 255.0f, 1.0f);
                        ImGui::PushStyleColor(ImGuiCol_Border, border_color);
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, is_lite_mode ? 0.0f : 1.0f);

                        ImU32 sw_bg = is_lite_mode ? IM_COL32(8, 122, 51, 255) : IM_COL32_BLACK_TRANS;

                        ImGui::RenderFrame(p_min, p_max, sw_bg, true, h * 0.5f);

                        if (ImGui::IsMouseHoveringRect(p_start, p_max)) {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGuiWrapper::COL_WINDOW_BACKGROUND);
                            ImGui::PushStyleColor(ImGuiCol_Border, {0, 0, 0, 0});
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));

                            std::string tooltip = _u8L(
                                "In lite mode, only the essential toolpath data is displayed.\n"
                                "If you need to view internal parameters such as infill, please disable this mode and "
                                "re-slice the model.");
                            ImGuiIO& io = ImGui::GetIO();
                            BOOST_LOG_TRIVIAL(warning) << "GCodeViewer LiteMode tooltip, is_lite_mode=" << (is_lite_mode ? 1 : 0)
                                                       << ", only_gcode_in_preview=" << (m_only_gcode_in_preview ? 1 : 0)
                                                       << ", mouse_x=" << io.MousePos.x << ", mouse_y=" << io.MousePos.y;
                            boost::log::core::get()->flush();
                            ImGui::SetTooltip("%s", tooltip.c_str());
                            /*ImGui::SetTooltip(_u8L("In lite mode, only the essential toolpath data is displayed.\n"
                                                   "If you need to view internal parameters such as infill, please disable this mode and "
                                                   "re-slice the model.")
                                                  .c_str());*/
                            ImGui::PopStyleColor(3);
                        }

                        if (is_lite_mode) {
                            // on at right
                            ImVec2 p = ImVec2(p_min.x + 1.0 * grap_size.x, p_min.y);
                            ImGui::RenderFrame(p, ImVec2(p.x + grap_size.x, p.y + grap_size.y),
                                               ImGui::ColorConvertFloat4ToU32(ImGuiWrapper::COL_CREALITY), true, h * 0.5f);
                        } else {
                            // off at left
                            ImU32 fill_col = m_is_dark ? IM_COL32(3, 3, 3, 255) : IM_COL32(204, 204, 204, 255);
                            ImGui::RenderFrame(p_min, ImVec2(p_min.x + grap_size.x, p_min.y + grap_size.y), fill_col, true, h * 0.5f);
                        }

                        bool toggled = ImGui::InvisibleButton("##lite_mode_toggle", switch_size);

                        if (toggled) {
                            bool k = !is_lite_mode;
                            wxGetApp().app_config->set("gcode_preview_lite_mode", (k ? "true" : "false"));
                            wxGetApp().plater()->invalid_slice_result_need_reslice();
                        }

                        ImGui::PopStyleColor();
                        ImGui::PopStyleVar();
                    }
                }

                helper.showColorHeader(m_view_type, m_contentWidth);
                if (m_user_mode != wxGetApp().get_mode()) {
                    update_by_mode(wxGetApp().get_mode());
                    m_user_mode    = wxGetApp().get_mode();
                    m_contentWidth = -1.0f;
                }

                // try to show scrollbar in colortable region
                ImGui::SetCursorPosX(0);

                const ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                                     ImGuiWindowFlags_NoBackground;
                const ImGuiID child_id = ImGui::GetID("color_table");

                float pos_y    = ImGui::GetCursorScreenPos().y;
                float canvas_h = wxGetApp().plater()->get_current_canvas3D()->get_canvas_size().get_height();

                float scale        = wxGetApp().plater()->get_current_canvas3D()->get_scale();
                float reduceHeight = isReduceHeight ? GCODE_REDUCE_HEIGHT * scale : 20.0f;

                const bool child_is_visible = ImGui::BeginChild(child_id, ImVec2(-1.0f, canvas_h - pos_y - reduceHeight), false,
                                                                child_flags);

                if (m_bLoaded) {
                    helper.showColorTable(
                        m_view_type, m_custom_gcode_per_print_z, m_tools, m_is_lite_mode,
                        [this]() {
                            // refresh_render_paths(false, false);
                            /*refresh_render_paths();
                            update_moves_slider();
                            wxGetApp().plater()->get_current_canvas3D()->set_as_dirty();*/
                            on_visibility_changed();
                        },
                        [this]() {
                            refresh(*m_gcode_result, wxGetApp().plater()->get_extruder_colors_from_plater_config(m_gcode_result));
                            update_moves_slider();
                            wxGetApp().plater()->get_current_canvas3D()->set_as_dirty();
                        });
                }

#if 0
				//
				std::string version = std::string(PROJECT_VERSION_EXTRA);
                bool is_alpha = boost::algorithm::icontains(version, "alpha");
                bool is_beta = boost::algorithm::icontains(version, "beta");
                if (!m_is_lite_mode && (is_alpha || is_beta)) {
                    ImGui::Text("   Lite Mode Enable:");
                    ImGui::SameLine();
                    char* txt = show_gcode_surface() ? "YES" : "NO";
                    ImGui::Text(txt);
				}
#endif

#if 0
            {
				
				std::string version = std::string(PROJECT_VERSION_EXTRA);
                bool is_alpha = boost::algorithm::icontains(version, "alpha");
                bool is_beta = boost::algorithm::icontains(version, "beta");
                if ((is_alpha || is_beta)) {
                    ImGui::Text("   Advanced Gcode viewer:");
                    ImGui::SameLine();
                    const char* txt = dynamic_cast<AdvancedRenderer*>(this) != nullptr ? "YES" : "NO";
                    ImGui::Text("%s", txt);  
				}
			}
#endif

                ImGui::EndChild();

            }

            else if (!m_no_render_path) {
                uint64_t line = 0;
                if (!m_sequential_view.gcode_ids.empty()) {
                    const size_t idx = std::min<size_t>(static_cast<size_t>(m_sequential_view.current.last),
                                                        m_sequential_view.gcode_ids.size() - 1);
                    /*line             = m_sequential_view.gcode_ids[idx];*/
                    line = marker.get_curr_move().gcode_id;
                    if (line == 0) {
                        // Try to find the closest mapped G-code line id (some moves may not map to a line).
                        const size_t max_scan = 2048;
                        for (size_t j = idx; j > 0 && (idx - j) < max_scan && line == 0; --j)
                            line = m_sequential_view.gcode_ids[j];
                        for (size_t j = idx + 1; j < m_sequential_view.gcode_ids.size() && (j - idx) < max_scan && line == 0; ++j)
                            line = m_sequential_view.gcode_ids[j];
                    }
                }
                if (line == 0)
                    line = 1;
                gcode_window.renderGcode(line, canvas_width, canvas_height, isReduceHeight);
            }
        },
        wcfg);

    ImGui::PopStyleColor(4);
}

void BaseRenderer::render_slider(int canvas_width, int canvas_height) 
{
    if (m_moves_slider->render(canvas_width, canvas_height))
        m_layers_slider->switch_one_layer_mode();
    m_layers_slider->render(canvas_width, canvas_height);
}

void BaseRenderer::render_marker_sequential_view(int canvas_width, int canvas_height) 
{
    Transform3d transform = wxGetApp().plater()->get_current_canvas3D()->get_preview_extra_transform();
    auto pos = transform * m_sequential_view.current_position.cast<double>();
    marker.set_world_position(pos.cast<float>());
    marker.set_world_offset(m_sequential_view.current_offset);
    float sc = wxGetApp().plater()->get_current_canvas3D()->get_scale();
    int bottom_margin = 120 * GCODE_VIEWER_SLIDER_SCALE * sc;
    marker.render(canvas_width, canvas_height - bottom_margin, m_view_type, m_showMark);
}

void BaseRenderer::render_all_plates_stats(const std::vector<const GCodeProcessorResult*>& gcode_result_list, bool show /*= true*/) const {
#if AUTO_CONVERT_3MF
    return;
#endif

    if (!show)
        return;
    if(gcode_result_list.size() == 0)
        return;
    for (auto gcode_result : gcode_result_list) {
        if (gcode_result->moves.size() == 0)
            return;
    }
    ImGuiWrapper& imgui = *wxGetApp().imgui();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 10.0 * m_scale));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 1.0f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.00f, 0.68f, 0.26f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.00f, 0.68f, 0.26f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.42f, 0.42f, 0.42f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.93f, 0.93f, 0.93f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.93f, 0.93f, 0.93f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(340.f * m_scale * imgui.scaled(1.0f / 15.0f), 0));

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 0, ImVec2(0.5f, 0.5f));
    ImGui::Begin(_u8L("Statistics of All Plates").c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    std::vector<float> filament_diameters = gcode_result_list.front()->filament_diameters;
    std::vector<float> filament_densities = gcode_result_list.front()->filament_densities;
    std::vector<ColorRGBA> filament_colors;
    decode_colors(wxGetApp().plater()->get_extruder_colors_from_plater_config(gcode_result_list.back()), filament_colors);

    for (int i = 0; i < filament_colors.size(); i++) { 
        filament_colors[i] = adjust_color_for_rendering(filament_colors[i]);
    }

    bool imperial_units = wxGetApp().app_config->get("use_inches") == "1";
    float window_padding = 4.0f * m_scale;
    const float icon_size = ImGui::GetTextLineHeight() * 0.7;
    std::map<std::string, float> offsets;
    std::map<int, double> model_volume_of_extruders_all_plates; // map<extruder_idx, volume>
    std::map<int, double> flushed_volume_of_extruders_all_plates; // map<extruder_idx, flushed volume>
    std::map<int, double> wipe_tower_volume_of_extruders_all_plates; // map<extruder_idx, flushed volume>
    std::map<int, double> support_volume_of_extruders_all_plates; // map<extruder_idx, flushed volume>
    std::vector<double> model_used_filaments_m_all_plates;
    std::vector<double> model_used_filaments_g_all_plates;
    std::vector<double> flushed_filaments_m_all_plates;
    std::vector<double> flushed_filaments_g_all_plates;
    std::vector<double> wipe_tower_used_filaments_m_all_plates;
    std::vector<double> wipe_tower_used_filaments_g_all_plates;
    std::vector<double> support_used_filaments_m_all_plates;
    std::vector<double> support_used_filaments_g_all_plates;
    float total_time_all_plates = 0.0f;
    float total_cost_all_plates = 0.0f;
    bool show_detailed_statistics_page = false;
    std::map<int, float> plate_time; // map<plate_idx, time_s>
    struct ColumnData {
        enum {
            Model = 1,
            Flushed = 2,
            WipeTower = 4,
            Support = 1 << 3,
        };
    };
    int displayed_columns = 0;
    auto max_width = [](const std::vector<std::string>& items, const std::string& title, float extra_size = 0.0f) {
        float ret = ImGui::CalcTextSize(title.c_str()).x;
        for (const std::string& item : items) {
            ret = std::max(ret, extra_size + ImGui::CalcTextSize(item.c_str()).x);
        }
        return ret;
    };
    auto calculate_offsets = [max_width, window_padding](const std::vector<std::pair<std::string, std::vector<std::string>>>& title_columns, float extra_size = 0.0f) {
        const ImGuiStyle& style = ImGui::GetStyle();
        std::vector<float> offsets;
        offsets.push_back(max_width(title_columns[0].second, title_columns[0].first, extra_size) + 3.0f * style.ItemSpacing.x + style.WindowPadding.x);
        for (size_t i = 1; i < title_columns.size() - 1; i++)
            offsets.push_back(offsets.back() + max_width(title_columns[i].second, title_columns[i].first) + style.ItemSpacing.x);
        if (title_columns.back().first == _u8L("Display"))
            offsets.back() = ImGui::GetWindowWidth() - ImGui::CalcTextSize(_u8L("Display").c_str()).x - ImGui::GetFrameHeight() / 2 - 2 * window_padding;

        float average_col_width = ImGui::GetWindowWidth() / static_cast<float>(title_columns.size());
        std::vector<float> ret;
        ret.push_back(0);
        for (size_t i = 1; i < title_columns.size(); i++) {
            ret.push_back(std::max(offsets[i - 1], i * average_col_width));
        }

        return ret;
    };
    auto append_item = [icon_size, &imgui, imperial_units, &window_padding, &draw_list, this](bool draw_icon, const ColorRGBA& color, const std::vector<std::pair<std::string, float>>& columns_offsets)
    {
        if (draw_icon) {
            // render icon
            ImVec2 pos = ImVec2(ImGui::GetCursorScreenPos().x + window_padding * 3, ImGui::GetCursorScreenPos().y);

            draw_list->AddRectFilled({ pos.x + 1.0f * m_scale, pos.y + 3.0f * m_scale }, { pos.x + icon_size - 1.0f * m_scale, pos.y + icon_size + 1.0f * m_scale },
                ImGuiWrapper::to_ImU32(color));
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20.0 * m_scale, 6.0 * m_scale));

        // render selectable
        ImGui::Dummy({ 0.0, 0.0 });
        ImGui::SameLine();

        // render column item
        {
            const float dummy_size = draw_icon ? (ImGui::GetStyle().ItemSpacing.x + icon_size) : (window_padding * 3.0f);
            ImGui::SameLine(dummy_size);
            imgui.text(columns_offsets[0].first);

            for (size_t i = 1; i < columns_offsets.size(); i++) {
                ImGui::SameLine(columns_offsets[i].second);
                imgui.text(columns_offsets[i].first);
            }
        }

        ImGui::PopStyleVar(1);
    };
    auto append_headers = [&imgui](const std::vector<std::pair<std::string, float>>& title_offsets) {
        for (size_t i = 0; i < title_offsets.size(); i++) {
            ImGui::SameLine(title_offsets[i].second);
            imgui.bold_text(title_offsets[i].first);
        }
        ImGui::Separator();
    };
    auto get_used_filament_from_volume = [this, imperial_units, &filament_diameters, &filament_densities](double volume, int extruder_id) {
        double koef = imperial_units ? 1.0 / GizmoObjectManipulation::in_to_mm : 0.001;
        std::pair<double, double> ret = { koef * volume / (PI * sqr(0.5 * filament_diameters[extruder_id])),
                                            volume * filament_densities[extruder_id] * 0.001 };
        return ret;
    };

    ImGui::Dummy({ window_padding, window_padding });
    ImGui::SameLine();
    // title and item data
    {
        PartPlateList& plate_list = wxGetApp().plater()->get_partplate_list();
        PresetBundle *preset_bundle_for_summary = wxGetApp().preset_bundle;
        const size_t num_physical_for_summary = preset_bundle_for_summary ? preset_bundle_for_summary->filament_presets.size() : 0;
        for (auto plate : plate_list.get_nonempty_plate_list())
        {
            auto plate_print_statistics = plate->get_slice_result()->print_statistics;
            auto plate_extruders_raw   = plate->get_extruders(true);

            // Expand mixed filament IDs into their physical component IDs
            // before accumulation. The G-code already resolves mixed filaments
            // to physical extruders at slice time, so volumes are stored under
            // physical IDs in the statistics maps.
            std::vector<size_t> plate_extruders;
            for (size_t eid : plate_extruders_raw) {
                if (preset_bundle_for_summary && preset_bundle_for_summary->mixed_filaments.is_mixed(static_cast<unsigned int>(eid), num_physical_for_summary)) {
                    const MixedFilament* mf = preset_bundle_for_summary->mixed_filaments.mixed_filament_from_id(static_cast<unsigned int>(eid), num_physical_for_summary);
                    if (mf) {
                        plate_extruders.push_back(mf->component_a);
                        plate_extruders.push_back(mf->component_b);
                        for (char c : mf->gradient_component_ids)
                            if (c >= '1' && c <= '9') plate_extruders.push_back(static_cast<unsigned int>(c - '0'));
                    }
                } else {
                    plate_extruders.push_back(eid);
                }
            }
            std::sort(plate_extruders.begin(), plate_extruders.end());
            plate_extruders.erase(std::unique(plate_extruders.begin(), plate_extruders.end()), plate_extruders.end());

            const bool merge_plate_flush_into_model = should_merge_flush_into_model(plate_print_statistics);
            for (size_t extruder_id : plate_extruders) {
                extruder_id -= 1;
                double model_volume = 0.0;
                if (plate_print_statistics.model_volumes_per_extruder.find(extruder_id) != plate_print_statistics.model_volumes_per_extruder.end())
                    model_volume = plate_print_statistics.model_volumes_per_extruder.at(extruder_id);

                double flushed_volume = 0.0;
                if (plate_print_statistics.flush_per_filament.find(extruder_id) != plate_print_statistics.flush_per_filament.end())
                    flushed_volume = plate_print_statistics.flush_per_filament.at(extruder_id);

                model_volume_of_extruders_all_plates[extruder_id] += model_volume + (merge_plate_flush_into_model ? flushed_volume : 0.0);
                flushed_volume_of_extruders_all_plates[extruder_id] += merge_plate_flush_into_model ? 0.0 : flushed_volume;
                if (plate_print_statistics.wipe_tower_volumes_per_extruder.find(extruder_id) == plate_print_statistics.wipe_tower_volumes_per_extruder.end())
                    wipe_tower_volume_of_extruders_all_plates[extruder_id] += 0;
                else {
                    double wipe_tower_volume = plate_print_statistics.wipe_tower_volumes_per_extruder.at(extruder_id);
                    wipe_tower_volume_of_extruders_all_plates[extruder_id] += wipe_tower_volume;
                }
                if (plate_print_statistics.support_volumes_per_extruder.find(extruder_id) == plate_print_statistics.support_volumes_per_extruder.end())
                    support_volume_of_extruders_all_plates[extruder_id] += 0;
                else {
                    double support_volume = plate_print_statistics.support_volumes_per_extruder.at(extruder_id);
                    support_volume_of_extruders_all_plates[extruder_id] += support_volume;
                }
            }
            const PrintEstimatedStatistics::Mode& plate_time_mode = plate_print_statistics.modes[static_cast<size_t>(m_time_estimate_mode)];
            const double plate_display_time = print_time_for_display(plate_time_mode);
            plate_time[plate->get_index()] = plate_display_time;
            total_time_all_plates += plate_display_time;
            
            Print     *print;
            plate->get_print((PrintBase **) &print, nullptr, nullptr);
            total_cost_all_plates += print->print_statistics().total_cost;
        }
       
        for (auto it = model_volume_of_extruders_all_plates.begin(); it != model_volume_of_extruders_all_plates.end(); it++) {
            auto [model_used_filament_m, model_used_filament_g] = get_used_filament_from_volume(it->second, it->first);
            if (model_used_filament_m != 0.0 || model_used_filament_g != 0.0)
                displayed_columns |= ColumnData::Model;
            model_used_filaments_m_all_plates.push_back(model_used_filament_m);
            model_used_filaments_g_all_plates.push_back(model_used_filament_g);
        }
        for (auto it = flushed_volume_of_extruders_all_plates.begin(); it != flushed_volume_of_extruders_all_plates.end(); it++) {
            auto [flushed_filament_m, flushed_filament_g] = get_used_filament_from_volume(it->second, it->first);
            if (flushed_filament_m != 0.0 || flushed_filament_g != 0.0)
                displayed_columns |= ColumnData::Flushed;
            flushed_filaments_m_all_plates.push_back(flushed_filament_m);
            flushed_filaments_g_all_plates.push_back(flushed_filament_g);
        }
        for (auto it = wipe_tower_volume_of_extruders_all_plates.begin(); it != wipe_tower_volume_of_extruders_all_plates.end(); it++) {
            auto [wipe_tower_filament_m, wipe_tower_filament_g] = get_used_filament_from_volume(it->second, it->first);
            if (wipe_tower_filament_m != 0.0 || wipe_tower_filament_g != 0.0)
                displayed_columns |= ColumnData::WipeTower;
            wipe_tower_used_filaments_m_all_plates.push_back(wipe_tower_filament_m);
            wipe_tower_used_filaments_g_all_plates.push_back(wipe_tower_filament_g);
        }
        for (auto it = support_volume_of_extruders_all_plates.begin(); it != support_volume_of_extruders_all_plates.end(); it++) {
            auto [support_filament_m, support_filament_g] = get_used_filament_from_volume(it->second, it->first);
            if (support_filament_m != 0.0 || support_filament_g != 0.0)
                displayed_columns |= ColumnData::Support;
            support_used_filaments_m_all_plates.push_back(support_filament_m);
            support_used_filaments_g_all_plates.push_back(support_filament_g);
        }

        char buff[64];
        double longest_str = 0.0;
        for (auto i : model_used_filaments_g_all_plates) {
            if (i > longest_str)
                longest_str = i;
        }
        ::sprintf(buff, "%.2f", longest_str);

        std::vector<std::pair<std::string, std::vector<std::string>>> title_columns;
        if (displayed_columns & ColumnData::Model) {
            title_columns.push_back({ _u8L("Filament"), {""} });
            title_columns.push_back({ _u8L("Model"), {buff} });
        }
        if (displayed_columns & ColumnData::Support) {
            title_columns.push_back({ _u8L("Support"), {buff} });
        }
        if (displayed_columns & ColumnData::Flushed) {
            title_columns.push_back({ _u8L("Flushed"), {buff} });
        }
        if (displayed_columns & ColumnData::WipeTower) {
            title_columns.push_back({ _u8L("Tower"), {buff} });
        }
        if ((displayed_columns & ~ColumnData::Model) > 0) {
            title_columns.push_back({ _u8L("Total"), {buff} });
        }
        auto offsets_ = calculate_offsets(title_columns, icon_size);
        std::vector<std::pair<std::string, float>> title_offsets;
        for (int i = 0; i < offsets_.size(); i++) {
            title_offsets.push_back({ title_columns[i].first, offsets_[i] });
            offsets[title_columns[i].first] = offsets_[i];
        }
        append_headers(title_offsets);
    }

    // item
    {
        size_t i = 0;
        for (auto it = model_volume_of_extruders_all_plates.begin(); it != model_volume_of_extruders_all_plates.end(); it++) {
            if (i < model_used_filaments_m_all_plates.size() && i < model_used_filaments_g_all_plates.size()) {
                std::vector<std::pair<std::string, float>> columns_offsets;
                columns_offsets.push_back({ std::to_string(it->first + 1), offsets[_u8L("Filament")]});

                char buf[64];
                double unit_conver = imperial_units ? GizmoObjectManipulation::oz_to_g : 1.0;

                float column_sum_m = 0.0f;
                float column_sum_g = 0.0f;
                if (displayed_columns & ColumnData::Model) {
                    if ((displayed_columns & ~ColumnData::Model) > 0)
                        ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", model_used_filaments_m_all_plates[i], model_used_filaments_g_all_plates[i] / unit_conver);
                    else
                        ::sprintf(buf, imperial_units ? "%.2f in    %.2f oz" : "%.2f m    %.2f g", model_used_filaments_m_all_plates[i], model_used_filaments_g_all_plates[i] / unit_conver);
                    columns_offsets.push_back({ buf, offsets[_u8L("Model")] });
                    column_sum_m += model_used_filaments_m_all_plates[i];
                    column_sum_g += model_used_filaments_g_all_plates[i];
                }
                if (displayed_columns & ColumnData::Support) {
                    ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", support_used_filaments_m_all_plates[i], support_used_filaments_g_all_plates[i] / unit_conver);
                    columns_offsets.push_back({ buf, offsets[_u8L("Support")] });
                    column_sum_m += support_used_filaments_m_all_plates[i];
                    column_sum_g += support_used_filaments_g_all_plates[i];
                }
                if (displayed_columns & ColumnData::Flushed) {
                    ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", flushed_filaments_m_all_plates[i], flushed_filaments_g_all_plates[i] / unit_conver);
                    columns_offsets.push_back({ buf, offsets[_u8L("Flushed")] });
                    column_sum_m += flushed_filaments_m_all_plates[i];
                    column_sum_g += flushed_filaments_g_all_plates[i];
                }
                if (displayed_columns & ColumnData::WipeTower) {
                    ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", wipe_tower_used_filaments_m_all_plates[i], wipe_tower_used_filaments_g_all_plates[i] / unit_conver);
                    columns_offsets.push_back({ buf, offsets[_u8L("Tower")] });
                    column_sum_m += wipe_tower_used_filaments_m_all_plates[i];
                    column_sum_g += wipe_tower_used_filaments_g_all_plates[i];
                }
                if ((displayed_columns & ~ColumnData::Model) > 0) {
                    ::sprintf(buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", column_sum_m, column_sum_g / unit_conver);
                    columns_offsets.push_back({ buf, offsets[_u8L("Total")] });
                }

                append_item(true, filament_colors[it->first], columns_offsets);
            }
            i++;
        }

        if (!model_used_filaments_m_all_plates.empty()) {
            const double unit_conver = imperial_units ? GizmoObjectManipulation::oz_to_g : 1.0;

            double total_model_used_filament_m = 0.0;
            double total_model_used_filament_g = 0.0;
            double total_support_used_filament_m = 0.0;
            double total_support_used_filament_g = 0.0;
            double total_flushed_filament_m = 0.0;
            double total_flushed_filament_g = 0.0;
            double total_wipe_tower_used_filament_m = 0.0;
            double total_wipe_tower_used_filament_g = 0.0;

            for (double v : model_used_filaments_m_all_plates) total_model_used_filament_m += v;
            for (double v : model_used_filaments_g_all_plates) total_model_used_filament_g += v;
            for (double v : support_used_filaments_m_all_plates) total_support_used_filament_m += v;
            for (double v : support_used_filaments_g_all_plates) total_support_used_filament_g += v;
            for (double v : flushed_filaments_m_all_plates) total_flushed_filament_m += v;
            for (double v : flushed_filaments_g_all_plates) total_flushed_filament_g += v;
            for (double v : wipe_tower_used_filaments_m_all_plates) total_wipe_tower_used_filament_m += v;
            for (double v : wipe_tower_used_filaments_g_all_plates) total_wipe_tower_used_filament_g += v;

            ImGui::Separator();

            std::vector<std::pair<std::string, float>> columns_offsets;
            columns_offsets.push_back({ _u8L("Total"), offsets[_u8L("Filament")] });

            char total_buf[64];
            if (displayed_columns & ColumnData::Model) {
                if ((displayed_columns & ~ColumnData::Model) > 0)
                    ::sprintf(total_buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", total_model_used_filament_m, total_model_used_filament_g / unit_conver);
                else
                    ::sprintf(total_buf, imperial_units ? "%.2f in    %.2f oz" : "%.2f m    %.2f g", total_model_used_filament_m, total_model_used_filament_g / unit_conver);
                columns_offsets.push_back({ total_buf, offsets[_u8L("Model")] });
            }
            if (displayed_columns & ColumnData::Support) {
                ::sprintf(total_buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", total_support_used_filament_m, total_support_used_filament_g / unit_conver);
                columns_offsets.push_back({ total_buf, offsets[_u8L("Support")] });
            }
            if (displayed_columns & ColumnData::Flushed) {
                ::sprintf(total_buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", total_flushed_filament_m, total_flushed_filament_g / unit_conver);
                columns_offsets.push_back({ total_buf, offsets[_u8L("Flushed")] });
            }
            if (displayed_columns & ColumnData::WipeTower) {
                ::sprintf(total_buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g", total_wipe_tower_used_filament_m, total_wipe_tower_used_filament_g / unit_conver);
                columns_offsets.push_back({ total_buf, offsets[_u8L("Tower")] });
            }
            if ((displayed_columns & ~ColumnData::Model) > 0) {
                ::sprintf(total_buf, imperial_units ? "%.2f in\n%.2f oz" : "%.2f m\n%.2f g",
                    total_model_used_filament_m + total_support_used_filament_m + total_flushed_filament_m + total_wipe_tower_used_filament_m,
                    (total_model_used_filament_g + total_support_used_filament_g + total_flushed_filament_g + total_wipe_tower_used_filament_g) / unit_conver);
                columns_offsets.push_back({ total_buf, offsets[_u8L("Total")] });
            }

            append_item(false, { 0.0f, 0.0f, 0.0f, 0.0f }, columns_offsets);
        }

        ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize() * 0.1));
        ImGui::Dummy({ window_padding, window_padding });
        ImGui::SameLine();
        imgui.text(_u8L("Total cost") + ":");
        ImGui::SameLine();
        char buf[64];
        ::sprintf(buf, "%.2f", total_cost_all_plates);
        imgui.text(buf);

        if (!plate_time.empty()) {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetFontSize() * 0.1));
            ImGui::Dummy({ window_padding, window_padding });
            ImGui::SameLine();
            imgui.title(_u8L("Time Estimation"));

            std::vector<std::pair<std::string, std::string>> rows;
            rows.reserve(plate_time.size() + 1);
            for (const auto& it : plate_time) {
                rows.push_back({ _u8L("Plate") + " " + std::to_string(it.first + 1), short_time(get_time_dhms(it.second)) });
            }
            if (plate_time.size() > 1) {
                rows.push_back({ _u8L("Total"), short_time(get_time_dhms(total_time_all_plates)) });
            }

            float x_value = window_padding * 3.0f;
            const std::string value_column_keys[] = { _u8L("Model"), _u8L("Support"), _u8L("Flushed"), _u8L("Tower"), _u8L("Total") };
            for (const auto& key : value_column_keys) {
                auto it = offsets.find(key);
                if (it != offsets.end()) {
                    x_value = it->second;
                    break;
                }
            }

            const size_t total_row_idx = plate_time.size();
            for (size_t i = 0; i < rows.size(); i++) {
                if (plate_time.size() > 1 && i == total_row_idx) {
                    ImGui::Separator();
                }

                std::vector<std::pair<std::string, float>> columns_offsets;
                columns_offsets.reserve(2);
                columns_offsets.push_back({ rows[i].first, 0.0f });
                columns_offsets.push_back({ rows[i].second, x_value });
                append_item(false, { 0.0f, 0.0f, 0.0f, 0.0f }, columns_offsets);
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(3);
    return;
}

//BBS: GUI refactor: add canvas width and height
void BaseRenderer::render(int canvas_width, int canvas_height)
{
}

#if ENABLE_GCODE_VIEWER_STATISTICS
void BaseRenderer::render_statistics()
{
    static const float offset = 275.0f;

    ImGuiWrapper& imgui = *wxGetApp().imgui();

    auto add_time = [&imgui](const std::string& label, int64_t time) {
        imgui.text_colored(ImGuiWrapper::COL_ORANGE_LIGHT, label);
        ImGui::SameLine(offset);
        imgui.text(std::to_string(time) + " ms (" + get_time_dhms(static_cast<float>(time) * 0.001f) + ")");
    };

    auto add_memory = [&imgui](const std::string& label, int64_t memory) {
        auto format_string = [memory](const std::string& units, float value) {
            return std::to_string(memory) + " bytes (" +
                   Slic3r::float_to_string_decimal_point(float(memory) * value, 3)
                    + " " + units + ")";
        };

        static const float kb = 1024.0f;
        static const float inv_kb = 1.0f / kb;
        static const float mb = 1024.0f * kb;
        static const float inv_mb = 1.0f / mb;
        static const float gb = 1024.0f * mb;
        static const float inv_gb = 1.0f / gb;
        imgui.text_colored(ImGuiWrapper::COL_ORANGE_LIGHT, label);
        ImGui::SameLine(offset);
        if (static_cast<float>(memory) < mb)
            imgui.text(format_string("KB", inv_kb));
        else if (static_cast<float>(memory) < gb)
            imgui.text(format_string("MB", inv_mb));
        else
            imgui.text(format_string("GB", inv_gb));
    };

    auto add_counter = [&imgui](const std::string& label, int64_t counter) {
        imgui.text_colored(ImGuiWrapper::COL_ORANGE_LIGHT, label);
        ImGui::SameLine(offset);
        imgui.text(std::to_string(counter));
    };

    imgui.set_next_window_pos(0.5f * wxGetApp().plater()->get_current_canvas3D()->get_canvas_size().get_width(), 0.0f, ImGuiCond_Once, 0.5f, 0.0f);
    ImGui::SetNextWindowSizeConstraints({ 300.0f, 100.0f }, { 600.0f, 900.0f });
    imgui.begin(std::string("GCodeViewer Statistics"), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

    if (ImGui::CollapsingHeader("Time")) {
        add_time(std::string("GCodeProcessor:"), m_statistics.results_time);

        ImGui::Separator();
        add_time(std::string("Load:"), m_statistics.load_time);
        add_time(std::string("  Load vertices:"), m_statistics.load_vertices);
        add_time(std::string("  Smooth vertices:"), m_statistics.smooth_vertices);
        add_time(std::string("  Load indices:"), m_statistics.load_indices);
        add_time(std::string("Refresh:"), m_statistics.refresh_time);
        add_time(std::string("Refresh paths:"), m_statistics.refresh_paths_time);
    }

    if (ImGui::CollapsingHeader("OpenGL calls")) {
        add_counter(std::string("Multi GL_LINES:"), m_statistics.gl_multi_lines_calls_count);
        add_counter(std::string("Multi GL_TRIANGLES:"), m_statistics.gl_multi_triangles_calls_count);
        add_counter(std::string("GL_TRIANGLES:"), m_statistics.gl_triangles_calls_count);
        ImGui::Separator();
        add_counter(std::string("Instanced models:"), m_statistics.gl_instanced_models_calls_count);
        add_counter(std::string("Batched models:"), m_statistics.gl_batched_models_calls_count);
    }

    if (ImGui::CollapsingHeader("CPU memory")) {
        add_memory(std::string("GCodeProcessor results:"), m_statistics.results_size);

        ImGui::Separator();
        add_memory(std::string("Paths:"), m_statistics.paths_size);
        add_memory(std::string("Render paths:"), m_statistics.render_paths_size);
        add_memory(std::string("Models instances:"), m_statistics.models_instances_size);
    }

    if (ImGui::CollapsingHeader("GPU memory")) {
        add_memory(std::string("Vertices:"), m_statistics.total_vertices_gpu_size);
        add_memory(std::string("Indices:"), m_statistics.total_indices_gpu_size);
        add_memory(std::string("Instances:"), m_statistics.total_instances_gpu_size);
        ImGui::Separator();
        add_memory(std::string("Max VBuffer:"), m_statistics.max_vbuffer_gpu_size);
        add_memory(std::string("Max IBuffer:"), m_statistics.max_ibuffer_gpu_size);
    }

    if (ImGui::CollapsingHeader("Other")) {
        add_counter(std::string("Travel segments count:"), m_statistics.travel_segments_count);
        add_counter(std::string("Wipe segments count:"), m_statistics.wipe_segments_count);
        add_counter(std::string("Extrude segments count:"), m_statistics.extrude_segments_count);
        add_counter(std::string("Instances count:"), m_statistics.instances_count);
        add_counter(std::string("Batched count:"), m_statistics.batched_count);
        ImGui::Separator();
        add_counter(std::string("VBuffers count:"), m_statistics.vbuffers_count);
        add_counter(std::string("IBuffers count:"), m_statistics.ibuffers_count);
    }

    imgui.end();
}
#endif // ENABLE_GCODE_VIEWER_STATISTICS

ColorRGBA BaseRenderer::option_color(EMoveType move_type) const
{
    switch (move_type)
    {
    case EMoveType::Tool_change:  { return Options_Colors[static_cast<unsigned int>(EOptionsColors::ToolChanges)]; }
    case EMoveType::Color_change: { return Options_Colors[static_cast<unsigned int>(EOptionsColors::ColorChanges)]; }
    case EMoveType::Pause_Print:  { return Options_Colors[static_cast<unsigned int>(EOptionsColors::PausePrints)]; }
    case EMoveType::Custom_GCode: { return Options_Colors[static_cast<unsigned int>(EOptionsColors::CustomGCodes)]; }
    case EMoveType::Retract:      { return Options_Colors[static_cast<unsigned int>(EOptionsColors::Retractions)]; }
    case EMoveType::Unretract:    { return Options_Colors[static_cast<unsigned int>(EOptionsColors::Unretractions)]; }
    case EMoveType::Seam:         { return Options_Colors[static_cast<unsigned int>(EOptionsColors::Seams)]; }
    default:                      { return { 0.0f, 0.0f, 0.0f, 1.0f }; }
    }
}

void BaseRenderer::_on_set_fold(bool fold_value) 
{
    wxGetApp().plater()->get_current_canvas3D()->set_left_panel_fold(GLCanvas3D::CanvasPreview, fold_value);
}

void BaseRenderer::set_fold(bool fold)
{ 
    m_fold = fold;
    _on_set_fold(fold);
}

} // namespace GUI
} // namespace Slic3r
