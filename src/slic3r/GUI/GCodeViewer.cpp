#include "GCodeViewer.hpp"
#include "slic3r/GUI/GCodeRenderer/AdvancedRenderer.hpp"
#include "slic3r/GUI/GCodeRenderer/LegacyRenderer.hpp"
#include "slic3r/GUI/OpenGLManager.hpp"
#include "slic3r/GUI/GUI_App.hpp"

namespace Slic3r {
namespace GUI {

GCodeViewer::GCodeViewer()
{
}

GCodeViewer::~GCodeViewer() {
}

void GCodeViewer::on_change_color_mode(bool is_dark)
{
	const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->on_change_color_mode(is_dark);
    }
}

void GCodeViewer::set_scale(float scale)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->set_scale(scale);
    }
}

void GCodeViewer::init(ConfigOptionMode mode, PresetBundle* preset_bundle, bool isgcode)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->init(mode, preset_bundle, isgcode);
    }
}

void GCodeViewer::update_by_mode(ConfigOptionMode mode)
{
	const auto& p_renderer = get_renderer();
    if (p_renderer) {
		p_renderer->update_by_mode(mode);
    }
}

void GCodeViewer::load(const GCodeProcessorResult& gcode_result, const Print& print, const BuildVolume& build_volume,
                       const std::vector<BoundingBoxf3>& exclude_bounding_box, ConfigOptionMode mode, bool only_gcode)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->load(gcode_result, print, build_volume, exclude_bounding_box, mode, only_gcode);
    }
}

void GCodeViewer::refresh(const GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->refresh(gcode_result, str_tool_colors);
    }
}

void GCodeViewer::refresh_render_paths()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->refresh_render_paths();
    }
}

void GCodeViewer::update_shells_color_by_extruder(const DynamicPrintConfig* config)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->update_shells_color_by_extruder(config);
    }
}

void GCodeViewer::set_shell_transparency(float alpha)
{
	const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->set_shell_transparency(alpha);
    }
}

void GCodeViewer::reset()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->reset();
    }
}

void GCodeViewer::reset_shell()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->reset_shell();
    }
}

void GCodeViewer::load_shells(const Print& print, bool initialized, bool force_previewing)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->load_shells(print, initialized, force_previewing);
    }
}

void GCodeViewer::set_shells_on_preview(bool is_previewing)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->set_shells_on_preview(is_previewing);
    }
}

void GCodeViewer::render_all_plates_stats(const std::vector<const GCodeProcessorResult*>& gcode_result_list, bool show) const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->render_all_plates_stats(gcode_result_list, show);
    }
}

void GCodeViewer::render(int canvas_width, int canvas_height)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->render(canvas_width, canvas_height);
    }
}

// void GCodeViewer::_render_calibration_thumbnail_internal(ThumbnailData& thumbnail_data, const ThumbnailsParams& thumbnail_params,
//                                                          PartPlateList& partplate_list, OpenGLManager& opengl_manager)
// {
//     get_renderer()->_render_calibration_thumbnail_internal(thumbnail_data, thumbnail_params, partplate_list, opengl_manager);
// }

// void GCodeViewer::_render_calibration_thumbnail_framebuffer(ThumbnailData& thumbnail_data, unsigned int w, unsigned int h,
//                                                            const ThumbnailsParams& thumbnail_params,
//                                                            PartPlateList& partplate_list, OpenGLManager& opengl_manager)
// {
//     get_renderer()->_render_calibration_thumbnail_framebuffer(thumbnail_data, w, h, thumbnail_params, partplate_list, opengl_manager);
// }

 void GCodeViewer::render_calibration_thumbnail(ThumbnailData& thumbnail_data, unsigned int w, unsigned int h,
                                                const ThumbnailsParams& thumbnail_params, PartPlateList& partplate_list,
                                                OpenGLManager& opengl_manager)
 {
     const auto& p_renderer = get_renderer();
     if (p_renderer) {
         p_renderer->render_calibration_thumbnail(thumbnail_data, w, h, thumbnail_params, partplate_list, opengl_manager);
     }
 }

bool GCodeViewer::has_data() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->has_data();
    }
    return false;
}

bool GCodeViewer::can_export_toolpaths() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->can_export_toolpaths();
    }
    return false;
}

std::vector<int> GCodeViewer::get_plater_extruder()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_plater_extruder();
    }
    static std::vector<int> s_empty_list{};
    return s_empty_list;
}

bool GCodeViewer::is_gcode_result_valid() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->is_gcode_result_valid();
    }
    return false;
}

bool GCodeViewer::is_toolpath_outside() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->is_toolpath_outside();
    }
    return false;
}

const float GCodeViewer::get_max_print_height() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_max_print_height();
    }
    return 0.0f;
}

const BoundingBoxf3& GCodeViewer::get_paths_bounding_box() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_paths_bounding_box();
    }
    static BoundingBoxf3 s_empty;
    return s_empty;
}

const BoundingBoxf3& GCodeViewer::get_max_bounding_box() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_max_bounding_box();
    }
    static BoundingBoxf3 s_empty;
    return s_empty;
}

const BoundingBoxf3& GCodeViewer::get_shell_bounding_box() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_shell_bounding_box();
    }
    static BoundingBoxf3 s_empty;
    return s_empty;
}

BoundingBoxf3 GCodeViewer::get_paths_bounding_box_ex() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_paths_bounding_box_ex();
    }
    static BoundingBoxf3 s_empty;
    return s_empty;
}

bool GCodeViewer::has_printable_area() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->has_printable_area();
    }
    return false;
}

BoundingBoxf3& GCodeViewer::get_printable_area()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_printable_area();
    }
    static BoundingBoxf3 s_empty;
    return s_empty;
}

std::vector<double> GCodeViewer::get_layers_zs() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_layers_zs();
    }
    static std::vector<double> s_empty_list{};
    return s_empty_list;
}

const std::array<unsigned int, 2>& GCodeViewer::get_layers_z_range() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_layers_z_range();
    }
    static std::array<unsigned int, 2> s_empty_array{};
    return s_empty_array;
}

//const SequentialView& GCodeViewer::get_sequential_view() const { return get_renderer()->get_sequential_view(); }

void GCodeViewer::update_sequential_view_current(unsigned int first, unsigned int last)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->update_sequential_view_current(first, last);
    }
}

IMSlider* GCodeViewer::get_moves_slider()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_moves_slider();
    }
    return nullptr;
}

IMSlider* GCodeViewer::get_layers_slider()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_layers_slider();
    }
    return nullptr;
}

IMSlider* GCodeViewer::get_cliper_slider()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_cliper_slider();
    }
    return nullptr;
}

void GCodeViewer::enable_moves_slider(bool enable) const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->enable_moves_slider(enable);
    }
}

void GCodeViewer::update_moves_slider(bool set_to_max)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->update_moves_slider(set_to_max);
    }
}

void GCodeViewer::update_layers_slider_mode()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->update_layers_slider_mode();
    }
}

void GCodeViewer::update_marker_curr_move()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->update_marker_curr_move();
    }
}

bool GCodeViewer::is_contained_in_bed() const
{
	const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->is_contained_in_bed();
    }
    return true;
}

bool GCodeViewer::is_only_gcode_in_preview() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->is_only_gcode_in_preview();
    }
    return false;
}

EViewType GCodeViewer::get_view_type() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_view_type();
    }
    return EViewType::Count;
}

void GCodeViewer::set_view_type(EViewType type, bool reset_feature_type_visible)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->set_view_type(static_cast<EViewType>(type), reset_feature_type_visible);
    }
}

void GCodeViewer::reset_visible(EViewType type)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->reset_visible(static_cast<EViewType>(type));
    }
}

bool GCodeViewer::is_toolpath_move_type_visible(EMoveType type) const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->is_toolpath_move_type_visible(type);
    }
    return 0;
}

void GCodeViewer::set_toolpath_move_type_visible(EMoveType type, bool visible)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->set_toolpath_move_type_visible(type, visible);
    }
}

void GCodeViewer::release_gcode_file_mapping()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->release_gcode_file_mapping();
    }
}

unsigned int GCodeViewer::get_toolpath_role_visibility_flags() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_toolpath_role_visibility_flags();
    }
    return 0;
}

void GCodeViewer::set_toolpath_role_visibility_flags(unsigned int flags)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->set_toolpath_role_visibility_flags(flags);
    }
}

unsigned int GCodeViewer::get_options_visibility_flags() const
{
	const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_options_visibility_flags();
    }
    return 0;
}

void GCodeViewer::set_options_visibility_from_flags(unsigned int flags)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->set_options_visibility_from_flags(flags);
    }
}

void GCodeViewer::set_layers_z_range(const std::array<unsigned int, 2>& layers_z_range)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->set_layers_z_range(layers_z_range);
    }
}

bool GCodeViewer::is_legend_enabled() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->is_legend_enabled();
    }
    return false;
}

void GCodeViewer::enable_legend(bool enable)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->enable_legend(enable);
    }
}

float GCodeViewer::get_legend_height()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_legend_height();
    }
    return 0.0f;
}

void GCodeViewer::export_toolpaths_to_obj(const char* filename) const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->export_toolpaths_to_obj(filename);
    }
}

std::vector<CustomGCode::Item>& GCodeViewer::get_custom_gcode_per_print_z()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_custom_gcode_per_print_z();
    }
    static std::vector<CustomGCode::Item> s_empty_list{};
    return s_empty_list;
}

size_t GCodeViewer::get_extruders_count()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_extruders_count();
    }
	return 0;
}

void GCodeViewer::set_fold(bool fold)
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        p_renderer->set_fold(fold);
    }
}

const ConflictResultOpt& GCodeViewer::get_conflict_result() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_conflict_result();
    }
    static ConflictResultOpt temp;
    return temp;
}

const ToolpathOutsideResultOpt& GCodeViewer::get_toolpath_outside_result() const
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->get_toolpath_outside_result();
    }
    static ToolpathOutsideResultOpt temp;
    return temp;
}

bool GCodeViewer::get_show_bed()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->m_showBed;
    }
    return true;
}

bool GCodeViewer::get_loaded()
{
    const auto& p_renderer = get_renderer();
    if (p_renderer) {
        return p_renderer->m_bLoaded;
    }
	return false;
}


const std::shared_ptr<BaseRenderer>& GCodeViewer::get_renderer() const
{
    OpenGLManager& p_ogl_manager = wxGetApp().get_opengl_manager();
    const bool  b_advanced_gcode_viewer_enabled = p_ogl_manager.is_advanced_gcode_viewer_enabled();
    const bool  b_dirty = m_b_advanced_gcode_viewer_enabled != b_advanced_gcode_viewer_enabled;
    if (!m_p_renderer || b_dirty) {
        if (p_ogl_manager.init_gl()) {
            const auto& gl_version = p_ogl_manager.get_gl_info().get_formated_gl_version();
            if (b_advanced_gcode_viewer_enabled && gl_version >= 31) {
                m_p_renderer = std::make_shared<AdvancedRenderer>();
            } else {
                m_p_renderer = std::make_shared<LegacyRenderer>();
            }
            if (m_p_renderer) {
                m_p_renderer->reset();
            }
            m_b_advanced_gcode_viewer_enabled = b_advanced_gcode_viewer_enabled;
        }
    }
    return m_p_renderer;
}

} // namespace GUI
} // namespace Slic3r
