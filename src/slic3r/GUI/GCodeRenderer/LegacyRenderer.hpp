#ifndef slic3r_GUI_GCodeRenderer_LegacyRenderer_hpp_
#define slic3r_GUI_GCodeRenderer_LegacyRenderer_hpp_

#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/GCode/ThumbnailData.hpp"

#include "slic3r/GUI/GCodeRenderer/BaseRenderer.hpp"
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

#define ENABLE_VECTOR_HYBRID_BACKEND 1

namespace Slic3r {
namespace GUI {

class LegacyRenderer : public BaseRenderer
{
    using IBufferType       = unsigned short;
    using VertexBuffer      = HybridVertexVector; // replace using VertexBuffer = std::vector<float>;
    using MultiVertexBuffer = std::vector<VertexBuffer>;
#if ENABLE_VECTOR_HYBRID_BACKEND
    using IndexBuffer = HybridIndexBuffer; // replace using IndexBuffer = std::vector<IBufferType>;
#else
    using IndexBuffer = std::vector<IBufferType>;
#endif
    using MultiIndexBuffer = std::vector<IndexBuffer>;
    using InstanceBuffer   = std::vector<float>;
    using InstanceIdBuffer = std::vector<size_t>;
    using InstancesOffsets = std::vector<Vec3f>;

public:
    // vbo buffer containing vertices data used to render a specific toolpath type
    struct VBuffer
    {
        enum class EFormat : unsigned char {
            // vertex format: 3 floats -> position.x|position.y|position.z
            Position,
            // vertex format: 4 floats -> position.x|position.y|position.z|normal.x
            PositionNormal1,
            // vertex format: 6 floats -> position.x|position.y|position.z|normal.x|normal.y|normal.z
            PositionNormal3
        };

        EFormat format{EFormat::Position};
        // vbos id
        std::vector<unsigned int> vbos;
        // sizes of the buffers, in bytes, used in export to obj
        std::vector<size_t> sizes;
        // count of vertices, updated after data are sent to gpu
        size_t count{0};

        size_t data_size_bytes() const { return count * vertex_size_bytes(); }
        // We set 65536 as max count of vertices inside a vertex buffer to allow
        // to use unsigned short in place of unsigned int for indices in the index buffer, to save memory
        size_t max_size_bytes() const { return 65536 * vertex_size_bytes(); }

        size_t vertex_size_floats() const { return position_size_floats() + normal_size_floats(); }
        size_t vertex_size_bytes() const { return vertex_size_floats() * sizeof(float); }

        size_t position_offset_floats() const { return 0; }
        size_t position_offset_bytes() const { return position_offset_floats() * sizeof(float); }

        size_t position_size_floats() const { return 3; }
        size_t position_size_bytes() const { return position_size_floats() * sizeof(float); }

        size_t normal_offset_floats() const
        {
            assert(format == EFormat::PositionNormal1 || format == EFormat::PositionNormal3);
            return position_size_floats();
        }
        size_t normal_offset_bytes() const { return normal_offset_floats() * sizeof(float); }

        size_t normal_size_floats() const
        {
            switch (format) {
            case EFormat::PositionNormal1: {
                return 1;
            }
            case EFormat::PositionNormal3: {
                return 3;
            }
            default: {
                return 0;
            }
            }
        }
        size_t normal_size_bytes() const { return normal_size_floats() * sizeof(float); }

        void reset();
    };

    // buffer containing instances data used to render a toolpaths using instanced or batched models
    // instance record format:
    // instanced models: 5 floats -> position.x|position.y|position.z|width|height (which are sent to the shader as -> vec3 (offset) + vec2
    // (scales) in GLModel::render_instanced()) batched models:   3 floats -> position.x|position.y|position.z
    struct InstanceVBuffer
    {
        // ranges used to render only subparts of the intances
        struct Ranges
        {
            struct Range
            {
                // offset in bytes of the 1st instance to render
                unsigned int offset;
                // count of instances to render
                unsigned int count;
                // vbo id
                unsigned int vbo{0};
                // Color to apply to the instances
                ColorRGBA color;
            };

            std::vector<Range> ranges;

            void reset();
        };

        enum class EFormat : unsigned char { InstancedModel, BatchedModel };

        EFormat format;

        // cpu-side buffer containing all instances data
        InstanceBuffer buffer;
        // indices of the moves for all instances
        std::vector<size_t> s_ids;
        // position offsets, used to show the correct value of the tool position
        InstancesOffsets offsets;
        Ranges           render_ranges;

        size_t data_size_bytes() const { return s_ids.size() * instance_size_bytes(); }

        size_t instance_size_floats() const
        {
            switch (format) {
            case EFormat::InstancedModel: {
                return 5;
            }
            case EFormat::BatchedModel: {
                return 3;
            }
            default: {
                return 0;
            }
            }
        }
        size_t instance_size_bytes() const { return instance_size_floats() * sizeof(float); }

        void reset();
    };

    // ibo buffer containing indices data (for lines/triangles) used to render a specific toolpath type
    struct IBuffer
    {
        // id of the associated vertex buffer
        unsigned int vbo{0};
        // ibo id
        unsigned int ibo{0};
        // count of indices, updated after data are sent to gpu
        size_t count{0};

        void reset();
    };

    // Used to identify different toolpath sub-types inside a IBuffer
    struct Path
    {
        struct Endpoint
        {
            // index of the buffer in the multibuffer vector
            // the buffer type may change:
            // it is the vertex buffer while extracting vertices data,
            // the index buffer while extracting indices data
            unsigned int b_id{0};
            // index into the buffer
            size_t i_id{0};
            // move id
            size_t s_id{0};
            Vec3f  position{Vec3f::Zero()};
        };

        struct Sub_Path
        {
            Endpoint first;
            Endpoint last;

            bool contains(size_t s_id) const { return first.s_id <= s_id && s_id <= last.s_id; }
        };

        EMoveType             type{EMoveType::Noop};
        ExtrusionRole         role{erNone};
        float                 delta_extruder{0.0f};
        float                 height{0.0f};
        float                 width{0.0f};
        float                 feedrate{0.0f};
        float                 fan_speed{0.0f};
        float                 temperature{0.0f};
        float                 volumetric_rate{0.0f};
        float                 layer_time{0.0f};
        float                 acceleration{0.0f};
        unsigned char         extruder_id{0};
        unsigned char         cp_color_id{0};
        std::vector<Sub_Path> sub_paths;

        bool   matches(const GCodeProcessorResult::MoveVertex& move) const;
        size_t vertices_count() const { return sub_paths.empty() ? 0 : sub_paths.back().last.s_id - sub_paths.front().first.s_id + 1; }
        bool   contains(size_t s_id) const
        {
            return sub_paths.empty() ? false : sub_paths.front().first.s_id <= s_id && s_id <= sub_paths.back().last.s_id;
        }
        int get_id_of_sub_path_containing(size_t s_id) const
        {
            if (sub_paths.empty())
                return -1;
            else {
                for (int i = 0; i < static_cast<int>(sub_paths.size()); ++i) {
                    if (sub_paths[i].contains(s_id))
                        return i;
                }
                return -1;
            }
        }
        void add_sub_path(const GCodeProcessorResult::MoveVertex& move, unsigned int b_id, size_t i_id, size_t s_id)
        {
            Endpoint endpoint = {b_id, i_id, s_id, move.position};
            sub_paths.push_back({endpoint, endpoint});
        }
    };

    // Used to batch the indices needed to render the paths
    struct RenderPath
    {
        // Index of the parent tbuffer
        unsigned char tbuffer_id;
        // Render path property
        ColorRGBA color;
        // Index of the buffer in TBuffer::indices
        unsigned int ibuffer_id;
        // Render path content
        // Index of the path in TBuffer::paths
        unsigned int              path_id;
        int                       layer_no; // layer number, from bottom to top
        std::vector<unsigned int> sizes;
        std::vector<size_t> offsets; // use size_t because we need an unsigned integer whose size matches pointer's size (used in the call
                                     // glMultiDrawElements())
        bool contains(size_t offset) const
        {
            for (size_t i = 0; i < offsets.size(); ++i) {
                if (offsets[i] <= offset && offset <= offsets[i] + static_cast<size_t>(sizes[i] * sizeof(IBufferType)))
                    return true;
            }
            return false;
        }
    };
    struct RenderPathPropertyLower
    {
        bool operator()(const RenderPath& l, const RenderPath& r) const
        {
            if (l.tbuffer_id < r.tbuffer_id)
                return true;
            if (l.color < r.color)
                return true;
            else if (l.color > r.color)
                return false;
            return l.ibuffer_id < r.ibuffer_id;
        }
    };
    struct RenderPathPropertyEqual
    {
        bool operator()(const RenderPath& l, const RenderPath& r) const
        {
            return l.tbuffer_id == r.tbuffer_id && l.ibuffer_id == r.ibuffer_id && l.color == r.color;
        }
    };

    struct RenderPathPropertyEqual_WithLayerNo
    {
        bool operator()(const RenderPath& l, const RenderPath& r) const
        {
            return l.tbuffer_id == r.tbuffer_id && l.ibuffer_id == r.ibuffer_id && l.color == r.color && l.layer_no == r.layer_no;
        }
    };

	struct TBuffer
    {
        enum class ERenderPrimitiveType : unsigned char { Line, Triangle, InstancedModel, BatchedModel };

        ERenderPrimitiveType render_primitive_type;

        // buffers for point, line and triangle primitive types
        VBuffer              vertices;
        std::vector<IBuffer> indices;

        struct Model
        {
            GLModel           model;
            ColorRGBA         color;
            InstanceVBuffer   instances;
            GLModel::Geometry data;

            void reset();
        };

        // contain the buffer for model primitive types
        Model model;

        std::string             shader;
        std::vector<Path>       paths;
        std::vector<RenderPath> render_paths;
        bool                    visible{false};

        void reset();
        void add_path(const GCodeProcessorResult::MoveVertex& move, unsigned int b_id, size_t i_id, size_t s_id);

        unsigned int max_vertices_per_segment() const;

        size_t       max_vertices_per_segment_size_floats() const;
        size_t       max_vertices_per_segment_size_bytes() const;
        unsigned int indices_per_segment() const;
        size_t       indices_per_segment_size_bytes() const;
        unsigned int max_indices_per_segment() const;
        size_t       max_indices_per_segment_size_bytes() const;

        bool has_data() const;
    };

    struct SequentialRangeCap
    {
        TBuffer*     buffer{nullptr};
        unsigned int ibo{0};
        unsigned int vbo{0};
        ColorRGBA    color;

        ~SequentialRangeCap();
        bool   is_renderable() const { return buffer != nullptr; }
        void   reset();
        size_t indices_count() const { return 6; }
    };

	class Layers
    {
    public:
        struct Endpoints
        {
            size_t first{0};
            size_t last{0};

            bool operator==(const Endpoints& other) const { return first == other.first && last == other.last; }
            bool operator!=(const Endpoints& other) const { return !operator==(other); }
        };

    private:
        std::vector<double>    m_zs;
        std::vector<Endpoints> m_endpoints;

    public:
        void append(double z, Endpoints endpoints)
        {
            m_zs.emplace_back(z);
            m_endpoints.emplace_back(endpoints);
        }

        void reset()
        {
            m_zs        = std::vector<double>();
            m_endpoints = std::vector<Endpoints>();
        }

        size_t                        size() const { return m_zs.size(); }
        bool                          empty() const { return m_zs.empty(); }
        const std::vector<double>&    get_zs() const { return m_zs; }
        const std::vector<Endpoints>& get_endpoints() const { return m_endpoints; }
        std::vector<Endpoints>&       get_endpoints() { return m_endpoints; }
        double                        get_z_at(unsigned int id) const { return (id < m_zs.size()) ? m_zs[id] : 0.0; }
        Endpoints get_endpoints_at(unsigned int id) const { return (id < m_endpoints.size()) ? m_endpoints[id] : Endpoints(); }
        int       get_l_at(float z) const
        {
            auto iter = std::upper_bound(m_zs.begin(), m_zs.end(), z);
            return std::distance(m_zs.begin(), iter);
        }

        bool operator!=(const Layers& other) const
        {
            if (m_zs != other.m_zs)
                return true;
            if (m_endpoints != other.m_endpoints)
                return true;
            return false;
        }
    };

	struct FilterLayerResult
    {
    public:
        void reset()
        {
            m_valid          = false;
            m_dynamic_stride = 0;
            m_layers_z_range.fill(0);
            m_layers_z_offset = std::vector<double>();
        }

        bool check_valid(const int dynamic_stride, const std::array<unsigned int, 2>& layers_z_range)
        {
            return m_valid && m_dynamic_stride == dynamic_stride && m_layers_z_range[0] == layers_z_range[0] &&
                   m_layers_z_range[1] == layers_z_range[1];
        };

        const std::vector<double>& get_layers_z_offset() { return m_layers_z_offset; }

        const std::vector<double>& rebuild_filter_layers_z_offset(const LegacyRenderer*              viewer,
                                                                  const int                          dynamic_stride,
                                                                  const std::array<unsigned int, 2>& layers_z_range,
                                                                  const Layers&                      layers);

    private:
        // capture parameters
        int                         m_dynamic_stride{0};
        std::array<unsigned int, 2> m_layers_z_range = {0, 0};

        bool                m_valid{false};
        std::vector<double> m_layers_z_offset;
    };

public:
    LegacyRenderer();
    virtual ~LegacyRenderer();

	void init(ConfigOptionMode mode, Slic3r::PresetBundle* preset_bundle, bool isgcode) override;
    // recalculate ranges in dependence of what is visible and sets tool/print colors
    void refresh(const GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors) override;
    void refresh_render_paths() override;
    void reset() override;
    //BBS: GUI refactor: add canvas width and height
    void render(int canvas_width, int canvas_height) override;
    void render_toolpaths() override;
    //BBS
    void _render_calibration_thumbnail_internal(ThumbnailData& thumbnail_data, const ThumbnailsParams& thumbnail_params, PartPlateList& partplate_list, OpenGLManager& opengl_manager);
     void _render_calibration_thumbnail_framebuffer(ThumbnailData& thumbnail_data, unsigned int w, unsigned int h, const ThumbnailsParams& thumbnail_params, PartPlateList& partplate_list, OpenGLManager& opengl_manager);
    void render_calibration_thumbnail(ThumbnailData& thumbnail_data, unsigned int w, unsigned int h, const ThumbnailsParams& thumbnail_params, PartPlateList& partplate_list, OpenGLManager& opengl_manager) override;
    bool can_export_toolpaths() const override;
	void export_toolpaths_to_obj(const char* filename) const override;
    std::vector<int> get_plater_extruder();
    std::vector<double> get_layers_zs() const override { return m_layers.get_zs(); }
    void update_sequential_view_current(unsigned int first, unsigned int last) override;
    void set_layers_z_range(const std::array<unsigned int, 2>& layers_z_range) override;
    
    unsigned int get_options_visibility_flags() const override;
    void set_options_visibility_from_flags(unsigned int flags) override;
    
    bool is_toolpath_move_type_visible(EMoveType type) const override;
    void set_toolpath_move_type_visible(EMoveType type, bool visible) override;

	void update_marker_curr_move() override;
    /*bool is_move_type_visible(EMoveType type) const override;
    void set_move_type_visible(EMoveType type, bool visible) override;
	uint32_t get_extrusion_role_visibility_flags() const override;
    void set_extrusion_role_visibility_flags(uint32_t flags) override;*/
    bool is_extrusion_role_visible(ExtrusionRole role) const override;
    void set_extrusion_role_visible(ExtrusionRole role, bool is_visible) override;

private:

	bool load_toolpaths(const GCodeProcessorResult& gcode_result, const BuildVolume& build_volume, const std::vector<BoundingBoxf3>& exclude_bounding_box) override;
    
	bool is_visible(ExtrusionRole role) const { return BaseRenderer::is_visible(role); };
    bool is_visible(const Path& path) const { return is_visible(path.role); }
    
	void on_visibility_changed() override;

    void refresh_render_paths(bool keep_sequential_current_first, bool keep_sequential_current_last) const;

    bool show_gcode_surface() const;
	
	double estimate_pixels_of_one_layer() const;
    int get_dynamic_stride(double pixels_of_layer_height) const;
	bool should_be_filtered_of_layer(int stride, int layer) const;
    int get_layer_index(const Path& path) const;

    const std::vector<double>& get_filtered_layers_z_offset(const int dynamic_stride);

	Vec3f encode_position(const Vec3f& position);
	bool should_enable_memory_optimize(const GCodeProcessorResult& gcode_result);

	void log_memory_used(const std::string& label, int64_t additional = 0) const;


private:
	size_t m_moves_count{ 0 };
    std::vector<TBuffer> m_buffers{static_cast<size_t>(EMoveType::Extrude_Alter)};
    Layers m_layers;
    std::array<SequentialRangeCap, 2> m_sequential_range_caps;

	FilterLayerResult m_filter_layer_result;

	// Prefix-sum array for arc interpolation points.
    // m_ssid_arc_extra_segments[i] = total extra arc interpolation points for all s_ids in [0, i].
    // Allows O(1) range-sum queries: sum for [lo+1, hi] = arr[hi] - arr[lo].
    // Replaces the inner O(N) loop that iterates over ssid ranges in refresh_render_paths.
    std::vector<unsigned int> m_ssid_arc_extra_segments;

    std::vector<size_t> m_sid_to_moveid;
};


} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GUI_GCodeRenderer_LegacyRenderer_hpp_