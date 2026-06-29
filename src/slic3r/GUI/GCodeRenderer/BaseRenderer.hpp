#ifndef slic3r_GUI_GCodeRenderer_BaseRenderer_hpp_
#define slic3r_GUI_GCodeRenderer_BaseRenderer_hpp_

#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/GCode/ThumbnailData.hpp"
#include "slic3r/GUI/3DScene.hpp"
#include "slic3r/GUI/IMSlider.hpp"
#include "slic3r/GUI/GLModel.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/HybridIndexBuffer.hpp"
#include "slic3r/GUI/HybridVertexBuffer.hpp"

#include <boost/iostreams/device/mapped_file.hpp>
#include <cstdint>
#include <float.h>
#include <set>
#include <unordered_set>


namespace Slic3r {

class Print;
class TriangleMesh;
class PresetBundle;

namespace GUI {

class PartPlateList;
class OpenGLManager;

static const float GCODE_VIEWER_SLIDER_SCALE = 0.6f;
static const float SLIDER_DEFAULT_RIGHT_MARGIN  = 10.0f;
static const float SLIDER_DEFAULT_BOTTOM_MARGIN = 10.0f;
static const float GCODE_REDUCE_HEIGHT = 150.0f;

static float round_to_bin(const float value)
{
    //    assert(value > 0);
    constexpr float const scale[5]     = {100.f, 1000.f, 10000.f, 100000.f, 1000000.f};
    constexpr float const invscale[5]  = {0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f};
    constexpr float const threshold[5] = {0.095f, 0.0095f, 0.00095f, 0.000095f, 0.0000095f};
    // Scaling factor, pointer to the tables above.
    int i = 0;
    // While the scaling factor is not yet large enough to get two integer digits after scaling and rounding:
    for (; value < threshold[i] && i < 4; ++i)
        ;
    return std::round(value * scale[i]) * invscale[i];
}

static unsigned char buffer_id(EMoveType type) 
{
    return static_cast<unsigned char>(type) - static_cast<unsigned char>(EMoveType::Retract);
}

static EMoveType buffer_type(unsigned char id) 
{
    return static_cast<EMoveType>(static_cast<unsigned char>(EMoveType::Retract) + id);
}

static bool role_been_filtered_in_lite_mode(ExtrusionRole role)
{
    return (role == ExtrusionRole::erInternalInfill ||
            role == ExtrusionRole::erSolidInfill ||
            role == ExtrusionRole::erInternalBridgeInfill);
}

enum EViewType : unsigned char 
{
    FeatureType = 0,
    Height,
    Width,
    Feedrate,
    FanSpeed,
    Temperature,
    VolumetricRate,
    Tool,
    ColorPrint,
    FilamentId,
    LayerTime,
    LayerTimeLog,
    Acceleration,
    Custom, // Creality:for appearance shortage
    Count
};

enum EOptionsColors : unsigned char
{
	Retractions,
    Unretractions,
    Seams,
    ToolChanges,
    ColorChanges,
    PausePrints,
    CustomGCodes
};

struct ETools
{
	std::vector<ColorRGBA> m_tool_colors;
    std::vector<bool> m_tool_visibles;
};


class BaseRenderer
{
    // helper to render shells
    struct Shells
    {
        GLVolumeCollection volumes;
        bool visible{ false };
        //BBS: always load shell when preview
        int print_id{ -1 };
        int print_modify_count { -1 };
        bool previewing{ false };
    };

#if ENABLE_GCODE_VIEWER_STATISTICS
    struct Statistics
    {
        // time
        int64_t results_time{ 0 };
        int64_t load_time{ 0 };
        int64_t load_vertices{ 0 };
        int64_t smooth_vertices{ 0 };
        int64_t load_indices{ 0 };
        int64_t refresh_time{ 0 };
        int64_t refresh_paths_time{ 0 };
        // opengl calls
        int64_t gl_multi_lines_calls_count{ 0 };
        int64_t gl_multi_triangles_calls_count{ 0 };
        int64_t gl_triangles_calls_count{ 0 };
        int64_t gl_instanced_models_calls_count{ 0 };
        int64_t gl_batched_models_calls_count{ 0 };
        // memory
        int64_t results_size{ 0 };
        int64_t total_vertices_gpu_size{ 0 };
        int64_t total_indices_gpu_size{ 0 };
        int64_t total_instances_gpu_size{ 0 };
        int64_t max_vbuffer_gpu_size{ 0 };
        int64_t max_ibuffer_gpu_size{ 0 };
        int64_t paths_size{ 0 };
        int64_t render_paths_size{ 0 };
        int64_t models_instances_size{ 0 };
        // other
        int64_t travel_segments_count{ 0 };
        int64_t wipe_segments_count{ 0 };
        int64_t extrude_segments_count{ 0 };
        int64_t instances_count{ 0 };
        int64_t batched_count{ 0 };
        int64_t vbuffers_count{ 0 };
        int64_t ibuffers_count{ 0 };

        void reset_all() {
            reset_times();
            reset_opengl();
            reset_sizes();
            reset_others();
        }

        void reset_times() {
            results_time = 0;
            load_time = 0;
            load_vertices = 0;
            smooth_vertices = 0;
            load_indices = 0;
            refresh_time = 0;
            refresh_paths_time = 0;
        }

        void reset_opengl() {
            gl_multi_lines_calls_count = 0;
            gl_multi_triangles_calls_count = 0;
            gl_triangles_calls_count = 0;
            gl_instanced_models_calls_count = 0;
            gl_batched_models_calls_count = 0;
        }

        void reset_sizes() {
            results_size = 0;
            total_vertices_gpu_size = 0;
            total_indices_gpu_size = 0;
            total_instances_gpu_size = 0;
            max_vbuffer_gpu_size = 0;
            max_ibuffer_gpu_size = 0;
            paths_size = 0;
            render_paths_size = 0;
            models_instances_size = 0;
        }

        void reset_others() {
            travel_segments_count = 0;
            wipe_segments_count = 0;
            extrude_segments_count = 0;
            instances_count = 0;
            batched_count = 0;
            vbuffers_count = 0;
            ibuffers_count = 0;
        }
    };
#endif // ENABLE_GCODE_VIEWER_STATISTICS

public:
    // helper to render extrusion paths
    struct Extrusions
    {
        struct Range
        {
            enum class EType : unsigned char { 
				Linear, 
				Logarithmic 
			};
            float min;
            float max;
            unsigned int count;
            bool log_scale;

            Range() { reset(); }
            void update_from(const float value) {
                if (value != max && value != min)
                    ++count;
                min = std::min(min, value);
                max = std::max(max, value);
            }
            void reset(bool log = false) { min = FLT_MAX; max = -FLT_MAX; count = 0; log_scale = log; }

            float step_size() const;
            ColorRGBA get_color_at(float value) const;
            float get_value_at_step(int step) const;

        };

        struct Ranges
        {
            // Color mapping by layer height.
            Range height;
            // Color mapping by extrusion width.
            Range width;
            // Color mapping by feedrate.
            Range feedrate;
            // Color mapping by fan speed.
            Range fan_speed;
            // Color mapping by volumetric extrusion rate.
            Range volumetric_rate;
            // Color mapping by extrusion temperature.
            Range temperature;
            // Color mapping by layer time.
            Range layer_duration;
            Range layer_duration_log;
            
            // Color mapping by acceleration
            Range acceleration;
            void reset() {
                height.reset();
                width.reset();
                feedrate.reset();
                fan_speed.reset();
                volumetric_rate.reset();
                temperature.reset();
                layer_duration.reset();
                layer_duration_log.reset(true);
                acceleration.reset();
            }
        };

        unsigned int role_visibility_flags{ 0 };
        Ranges ranges;

        void reset_role_visibility_flags() {
            role_visibility_flags = 0;
            for (unsigned int i = 0; i < erCount; ++i) {
                role_visibility_flags |= 1 << i;
            }
        }

        void reset_ranges() { ranges.reset(); }
    };

    struct SequentialView
    {
        struct Endpoints
        {
            size_t first{ 0 };
            size_t last{ 0 };
        };

        bool skip_invisible_moves{ false };
        Endpoints endpoints;
        Endpoints current;
        Endpoints last_current;
        Endpoints global;
        Vec3f current_position{ Vec3f::Zero() };
        Vec3f current_offset{ Vec3f::Zero() };
        std::vector<unsigned int> gcode_ids;
    };

    class Marker
    {
        GLModel     m_model;
        Vec3f       m_world_position;
        Transform3f m_world_transform;
        // for seams, the position of the marker is on the last endpoint of the toolpath containing it
        // the offset is used to show the correct value of tool position in the "ToolPosition" window
        // see implementation of render() method
        Vec3f                            m_world_offset;
        float                            m_z_offset{0.5f};
        GCodeProcessorResult::MoveVertex m_curr_move;
        bool                             m_visible{true};
        bool                             m_is_dark = false;

    public:
        float m_scale = 1.0f;

        void init(std::string filename);

        const BoundingBoxf3& get_bounding_box() const;

        void set_world_position(const Vec3f& position);
        void set_world_offset(const Vec3f& offset);

        bool is_visible() const { return m_visible; }
        void set_visible(bool visible) { m_visible = visible; }

        void render(int canvas_width, int canvas_height, const EViewType& view_type, bool showMark);
        void on_change_color_mode(bool is_dark) { m_is_dark = is_dark; }

        void update_curr_move(const GCodeProcessorResult::MoveVertex move);
        const GCodeProcessorResult::MoveVertex& get_curr_move() { return m_curr_move; };
    };

    class GCodeWindow
    {
        struct Line
        {
            std::string command;
            std::string parameters;
            std::string comment;
        };
        bool                                 m_is_dark = false;
        uint64_t                             m_selected_line_id{0};
        size_t                               m_last_lines_size{0};
        std::string                          m_filename;
        boost::iostreams::mapped_file_source m_file;
        // map for accessing data in file by line number
        std::vector<size_t> m_lines_ends;
        // current visible lines
        std::vector<Line> m_lines;

    public:
        GCodeWindow() = default;
        ~GCodeWindow() { stop_mapping_file(); }
        void load_gcode(const std::string& filename, const std::vector<size_t>& lines_ends);
        void reset();
        void renderGcode(uint64_t curr_line_id, int canvas_width, int canvas_height, bool isReduceHeight = false);
        void on_change_color_mode(bool is_dark) { m_is_dark = is_dark; }
        void stop_mapping_file();
    };

    bool m_showBed{true}, m_showMark{true}, m_showColor{true}, m_bLoaded{true};

protected:	
    //BBS
    ConflictResultOpt m_conflict_result;
    ToolpathOutsideResultOpt m_toolpath_outside_result;
    
    std::vector<int> m_plater_extruder;
    bool m_gl_data_initialized{ false };
    unsigned int m_last_result_id{ 0 };
    float m_scale = 1.0;
    //BBS: save m_gcode_result as well
    const GCodeProcessorResult* m_gcode_result{ nullptr };
    //BBS: add only gcode mode
    bool m_only_gcode_in_preview {false};
    std::vector<size_t> m_ssid_to_moveid_map;
    

    
    // bounding box of toolpaths
    BoundingBoxf3 m_paths_bounding_box;
    // bounding box of toolpaths + marker tools
    BoundingBoxf3 m_max_bounding_box;
    //BBS: add shell bounding box
    BoundingBoxf3 m_shell_bounding_box;
    float m_max_print_height{ 0.0f };
    //creality add 
    BoundingBoxf3 m_printable_area_box;

    //BBS save m_tools_color and m_tools_visible
    ETools m_tools;
    ConfigOptionMode m_user_mode;
    bool m_fold = {false};
    float m_contentWidth {-1.0f};

    std::array<unsigned int, 2> m_layers_z_range;
    std::vector<ExtrusionRole> m_roles;
    size_t m_extruders_count;
    std::vector<unsigned char> m_extruder_ids;
    std::vector<float> m_filament_diameters;
    std::vector<float> m_filament_densities;
    Extrusions m_extrusions;
    SequentialView m_sequential_view;
    GCodeWindow gcode_window;
    Marker marker;
    IMSlider*   m_moves_slider{nullptr};
    IMSlider* m_cliper_slider{nullptr};
    IMSlider* m_layers_slider{nullptr};
    Shells m_shells;
    /*BBS GUI refactor, store displayed items in color scheme combobox */
    std::vector<EViewType> view_type_items;
    std::vector<std::string> view_type_items_str;
    int       m_view_type_sel = 0;
    EViewType m_view_type{ EViewType::FeatureType };
    std::vector<EMoveType> options_items;

    // Creality:for appearance shortage
    // Cached "interest region" mask used by EViewType::Custom.
    // Indexed by ssid (end vertex index). Value = 0 for normal segments,
    // non-zero for segments belonging to an interest object.
    mutable unsigned int               m_custom_interest_cache_result_id{ 0 };
    mutable std::vector<unsigned char> m_custom_interest_by_ssid;

    bool m_legend_enabled{ true };
    float m_legend_height {0};
    PrintEstimatedStatistics m_print_statistics;
    PrintEstimatedStatistics::ETimeMode m_time_estimate_mode{ PrintEstimatedStatistics::ETimeMode::Normal };
#if ENABLE_GCODE_VIEWER_STATISTICS
    Statistics m_statistics;
#endif // ENABLE_GCODE_VIEWER_STATISTICS
    GCodeProcessorResult::SettingsIds m_settings_ids;
    //std::array<SequentialRangeCap, 2> m_sequential_range_caps;

    std::vector<CustomGCode::Item> m_custom_gcode_per_print_z;

    bool m_contained_in_bed{ true };
 
    mutable bool m_no_render_path { false };
    bool m_is_dark = false;
    bool m_is_lite_mode {false};
    bool m_is_belt {false};

	bool m_is_lod {false};
	float m_stride_factor {12.50f};
    int   m_filter_stride {0};
    std::set<int> m_top_surface_layer;

	bool m_is_mem_optim;

public:
    BaseRenderer();
    virtual ~BaseRenderer();

    void on_change_color_mode(bool is_dark);
    void set_scale(float scale = 1.0);
    virtual void init(ConfigOptionMode mode, Slic3r::PresetBundle* preset_bundle, bool isgcode);
    void update_by_mode(ConfigOptionMode mode);

    // extract rendering data from the given parameters
    //BBS: add only gcode mode
    void load(const GCodeProcessorResult& gcode_result, const Print& print, const BuildVolume& build_volume,
            const std::vector<BoundingBoxf3>& exclude_bounding_box, ConfigOptionMode mode, bool only_gcode = false);
    // recalculate ranges in dependence of what is visible and sets tool/print colors
    virtual void refresh(const GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors);
    virtual void refresh_render_paths() = 0;
    void update_shells_color_by_extruder(const DynamicPrintConfig* config);
    void set_shell_transparency(float alpha = 0.15f);

    virtual void reset();
	void release_gcode_file_mapping();
    //BBS: always load shell at preview
    void reset_shell();
    void load_shells(const Print& print, bool initialized, bool force_previewing = false);
    void set_shells_on_preview(bool is_previewing) { m_shells.previewing = is_previewing; }
    //BBS: add all plates filament statistics
    void render_all_plates_stats(const std::vector<const GCodeProcessorResult*>& gcode_result_list, bool show = true) const;
    //BBS: GUI refactor: add canvas width and height
    virtual void render(int canvas_width, int canvas_height);
    
    bool has_data() const { return !m_roles.empty(); }
    virtual bool can_export_toolpaths() const = 0;
    std::vector<int> get_plater_extruder();
    bool is_gcode_result_valid() const {
        return m_gcode_result == nullptr ? false : true;
    }
    bool is_toolpath_outside() const { return m_gcode_result ? m_gcode_result->toolpath_outside : false; }
    const float                get_max_print_height() const { return m_max_print_height; }
    const BoundingBoxf3& get_paths_bounding_box() const { return m_paths_bounding_box; }
    const BoundingBoxf3& get_max_bounding_box() const { return m_max_bounding_box; }
    const BoundingBoxf3& get_shell_bounding_box() const { return m_shell_bounding_box; }

    BoundingBoxf3 get_paths_bounding_box_ex() const {
        BoundingBoxf3 paths_box_ex = m_paths_bounding_box;
        if (!m_gcode_result)
            return paths_box_ex;
        paths_box_ex.min.x() -= m_gcode_result->x_offset;
        paths_box_ex.min.y() -= m_gcode_result->y_offset;
        paths_box_ex.max.x() -= m_gcode_result->x_offset;
        paths_box_ex.max.y() -= m_gcode_result->y_offset;
        return paths_box_ex;
    }

    bool           has_printable_area() const { return m_gcode_result ? !m_gcode_result->printable_area.empty() : false; }
    BoundingBoxf3&        get_printable_area()
    {
        BoundingBoxf bboxf = get_extents(m_gcode_result->printable_area);
        m_printable_area_box = BoundingBoxf3{to_3d(bboxf.min, 0.), to_3d(bboxf.max, m_gcode_result->printable_height)};
        // Belt machines use swapped Y/Z in downstream checks (raw G-code bbox already in machine coords).
        if (m_is_belt) {
            std::swap(m_printable_area_box.min.y(), m_printable_area_box.min.z());
            std::swap(m_printable_area_box.max.y(), m_printable_area_box.max.z());
            m_printable_area_box.max.y() *= 1.42;
        }
        return m_printable_area_box;
    }

	virtual void render_calibration_thumbnail(ThumbnailData& thumbnail_data, unsigned int w, unsigned int h, const ThumbnailsParams& thumbnail_params, PartPlateList& partplate_list, OpenGLManager& opengl_manager) = 0;

    virtual std::vector<double> get_layers_zs() const = 0;

    const std::array<unsigned int,2> &get_layers_z_range() const { return m_layers_z_range; }

    const SequentialView& get_sequential_view() const { return m_sequential_view; }
    virtual void update_sequential_view_current(unsigned int first, unsigned int last) = 0;

    /* BBS IMSlider */
    IMSlider *get_moves_slider();
    IMSlider *get_layers_slider();
    IMSlider *get_cliper_slider();
    void enable_moves_slider(bool enable) const;
    virtual void update_moves_slider(bool set_to_max = false);
    void update_layers_slider_mode();
    virtual void update_marker_curr_move();

    bool is_contained_in_bed() const { return m_contained_in_bed; }
 
    //BBS: add only gcode mode
    bool is_only_gcode_in_preview() const { return m_only_gcode_in_preview; }

    EViewType get_view_type() const { return m_view_type; }
    void set_view_type(EViewType type, bool reset_feature_type_visible = true) {
        if (type == EViewType::Count)
            type = EViewType::FeatureType;
        m_view_type = (EViewType)type;
        if (reset_feature_type_visible && type == EViewType::ColorPrint) {
            reset_visible(EViewType::FeatureType);
        }
    }
    void reset_visible(EViewType type) {
        if (type == EViewType::FeatureType) {
            for (size_t i = 0; i < m_roles.size(); ++i) {
                ExtrusionRole role = m_roles[i];
                m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags | (1 << role);
            }
        } else if (type == EViewType::ColorPrint){
            for(auto item: m_tools.m_tool_visibles) item = true;
        }
    }

    virtual bool is_toolpath_move_type_visible(EMoveType type) const = 0;
    virtual void set_toolpath_move_type_visible(EMoveType type, bool visible) = 0;
    unsigned int get_toolpath_role_visibility_flags() const { return m_extrusions.role_visibility_flags; }
    void set_toolpath_role_visibility_flags(unsigned int flags) { m_extrusions.role_visibility_flags = flags; }
    virtual unsigned int get_options_visibility_flags() const = 0;
    virtual void set_options_visibility_from_flags(unsigned int flags) = 0;
    virtual void set_layers_z_range(const std::array<unsigned int, 2>& layers_z_range) = 0;

    bool is_legend_enabled() const { return m_legend_enabled; }
    void enable_legend(bool enable) { m_legend_enabled = enable; }
    float get_legend_height() { return m_legend_height; }

    virtual void export_toolpaths_to_obj(const char* filename) const = 0;

    std::vector<CustomGCode::Item>& get_custom_gcode_per_print_z() { return m_custom_gcode_per_print_z; }
    size_t get_extruders_count() { return m_extruders_count; }

    void set_fold(bool fold);

	const ConflictResultOpt& get_conflict_result() const { return m_conflict_result; };
    const ToolpathOutsideResultOpt& get_toolpath_outside_result() const { return m_toolpath_outside_result;} ;

	virtual bool is_extrusion_role_visible(ExtrusionRole role) const = 0;
    virtual void set_extrusion_role_visible(ExtrusionRole role, bool is_visible) = 0;
    /*virtual uint32_t get_extrusion_role_visibility_flags() const = 0;
    virtual void set_extrusion_role_visibility_flags(uint32_t flags) = 0;*/

	static const std::vector<ColorRGBA> Extrusion_Role_Colors;
    static const std::vector<ColorRGBA> Options_Colors;
    static const std::vector<ColorRGBA> Travel_Colors;
    static const std::vector<ColorRGBA> Range_Colors;
    static const ColorRGBA              Wipe_Color;
    static const ColorRGBA              Flush_Color;
    static const ColorRGBA              Neutral_Color;


protected:
    void init_tool_maker(PresetBundle* preset_bundle);

    virtual bool load_toolpaths(const GCodeProcessorResult& gcode_result, const BuildVolume& build_volume, const std::vector<BoundingBoxf3>& exclude_bounding_box) = 0;
    //BBS: always load shell at preview
    //void load_shells(const Print& print);

    virtual void on_visibility_changed();    


    // Creality:for appearance shortage
    void update_custom_interest_regions() const;
    virtual void render_toolpaths() = 0;
	void render_shells(int canvas_width, int canvas_height);

    //BBS: GUI refactor: add canvas size
    void render_legend(int canvas_width, int canvas_height);
    void render_slider(int canvas_width, int canvas_height);
    void render_marker_sequential_view(int canvas_width, int canvas_height);

#if ENABLE_GCODE_VIEWER_STATISTICS
    void render_statistics();
#endif // ENABLE_GCODE_VIEWER_STATISTICS
    bool is_visible(ExtrusionRole role) const {
        return role < erCount && (m_extrusions.role_visibility_flags & (1 << role)) != 0;
    }
    
    ColorRGBA option_color(EMoveType move_type) const;

    void _on_set_fold(bool fold);

	/*bool show_gcode_surface() const;
	
	double estimate_pixels_of_one_layer() const;
    int get_dynamic_stride(double pixels_of_layer_height) const;
	bool should_be_filtered_of_layer(int stride, int layer) const;
    int get_layer_index(const Path& path) const;

    const std::vector<double>& get_filtered_layers_z_offset(const int dynamic_stride);

	Vec3f encode_position(const Vec3f& position);
	bool should_enable_memory_optimize(const GCodeProcessorResult& gcode_result);*/

};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_BaseRenderer_hpp_

