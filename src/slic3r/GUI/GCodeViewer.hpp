#ifndef slic3r_GCodeViewer_hpp_
#define slic3r_GCodeViewer_hpp_


#include <memory>
#include "GCodeRenderer/BaseRenderer.hpp"
//#include "GCodeRenderer/LegacyRenderer.hpp"

namespace Slic3r {
namespace GUI {


class GCodeViewer
{
public:
    explicit GCodeViewer();
    virtual ~GCodeViewer();

    void on_change_color_mode(bool is_dark);
    void set_scale(float scale = 1.0);
    void init(ConfigOptionMode mode, Slic3r::PresetBundle* preset_bundle, bool isgcode = true);
    void update_by_mode(ConfigOptionMode mode);

    void load(const GCodeProcessorResult& gcode_result, const Print& print, const BuildVolume& build_volume,
              const std::vector<BoundingBoxf3>& exclude_bounding_box, ConfigOptionMode mode, bool only_gcode = false);
    void refresh(const GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors);
    void refresh_render_paths();
    void update_shells_color_by_extruder(const DynamicPrintConfig* config);
    void set_shell_transparency(float alpha = 0.15f);

    void reset();

    // 释放 gcode 文件的内存映射（释放文件锁，用于导出前替换占位符）
    void release_gcode_file_mapping();
    //BBS: always load shell at preview
    void reset_shell();
    void load_shells(const Print& print, bool initialized, bool force_previewing = false);
    void set_shells_on_preview(bool is_previewing);
    void render_all_plates_stats(const std::vector<const GCodeProcessorResult*>& gcode_result_list, bool show = true) const;
    void render(int canvas_width, int canvas_height);
    // void _render_calibration_thumbnail_internal(ThumbnailData& thumbnail_data, const ThumbnailsParams& thumbnail_params,
    //                                             PartPlateList& partplate_list, OpenGLManager& opengl_manager);
    // void _render_calibration_thumbnail_framebuffer(ThumbnailData& thumbnail_data, unsigned int w, unsigned int h,
    //                                               const ThumbnailsParams& thumbnail_params, PartPlateList& partplate_list,
    //                                               OpenGLManager& opengl_manager);
     void render_calibration_thumbnail(ThumbnailData& thumbnail_data, unsigned int w, unsigned int h,
                                       const ThumbnailsParams& thumbnail_params, PartPlateList& partplate_list,
                                       OpenGLManager& opengl_manager);

    bool has_data() const;
    bool can_export_toolpaths() const;
    std::vector<int> get_plater_extruder();
    bool is_gcode_result_valid() const;
    bool is_toolpath_outside() const;
    const float get_max_print_height() const;
    const BoundingBoxf3& get_paths_bounding_box() const;
    const BoundingBoxf3& get_max_bounding_box() const;
    const BoundingBoxf3& get_shell_bounding_box() const;
    BoundingBoxf3 get_paths_bounding_box_ex() const;
    bool has_printable_area() const;
    BoundingBoxf3& get_printable_area();

    std::vector<double> get_layers_zs() const;
    const std::array<unsigned int, 2>& get_layers_z_range() const;
    //const SequentialView& get_sequential_view() const;
    void update_sequential_view_current(unsigned int first, unsigned int last);

    IMSlider* get_moves_slider();
    IMSlider* get_layers_slider();
    IMSlider* get_cliper_slider();
    void enable_moves_slider(bool enable) const;
    void update_moves_slider(bool set_to_max = false);
    void update_layers_slider_mode();
    void update_marker_curr_move();

    bool is_contained_in_bed() const;
    bool is_only_gcode_in_preview() const;

    EViewType get_view_type() const;
    void set_view_type(EViewType type, bool reset_feature_type_visible = true);
    void reset_visible(EViewType type);
    bool is_toolpath_move_type_visible(EMoveType type) const;
    void set_toolpath_move_type_visible(EMoveType type, bool visible);
    unsigned int get_toolpath_role_visibility_flags() const;
    void set_toolpath_role_visibility_flags(unsigned int flags);
    unsigned int get_options_visibility_flags() const;
    void set_options_visibility_from_flags(unsigned int flags);
    void set_layers_z_range(const std::array<unsigned int, 2>& layers_z_range);

    bool is_legend_enabled() const;
    void enable_legend(bool enable);
    float get_legend_height();
    void export_toolpaths_to_obj(const char* filename) const;

    std::vector<CustomGCode::Item>& get_custom_gcode_per_print_z();
    size_t get_extruders_count();
    void set_fold(bool fold);

	const ConflictResultOpt& get_conflict_result() const;
    const ToolpathOutsideResultOpt& get_toolpath_outside_result() const;

	bool get_show_bed();
	bool get_loaded();

private:
    const std::shared_ptr<BaseRenderer>& get_renderer() const;

private:
    mutable std::shared_ptr<BaseRenderer> m_p_renderer{nullptr};
	mutable bool m_b_advanced_gcode_viewer_enabled{ false };
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GCodeViewer_hpp_

