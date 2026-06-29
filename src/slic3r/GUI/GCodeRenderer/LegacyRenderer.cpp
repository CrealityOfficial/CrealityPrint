#include "slic3r/GUI/GCodeRenderer/LegacyRenderer.hpp"

#include "libslic3r/ExtrusionEntity.hpp"

#include "slic3r/GUI/Camera.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/3DScene.hpp"
#include "slic3r/GUI/GUI_Preview.hpp"
#include "slic3r/Utils/TestHelper.hpp"

#include <GL/glew.h>

#define SHORT_TYPE_POSITION_SCALE (50.0)
#define SHORT_TYPE_POSITION_SCALE_INVERSE (1.0 / SHORT_TYPE_POSITION_SCALE)
#define SHORT_TYPE_NORMAL_SCALE (30000.0) // use 32767.0 may cause rendering issues

#define ENABLE_FILTERED_STRIDE_DIALOG (0)

#define MARKING_DIALOG
#ifndef MARKING_DIALOG
#define MARKING_DIALOG(msg) \
    if (std::string(PROJECT_VERSION_EXTRA) == std::string("Alpha")) { \
        MessageDialog dlg(nullptr, msg, _L("Preview"), wxYES); \
        dlg.ShowModal(); \
    }
#endif // !MARKING_DIALOG

namespace Slic3r{ 
namespace GUI{

static Vec2f calc_pt_in_screen(const Vec3d& pt, const Matrix4d& view_proj_mat, int window_width, int window_height)
{
    auto  tran = view_proj_mat;
    Vec4d temp_center(pt.x(), pt.y(), pt.z(), 1.0);
    Vec4d temp_ndc          = tran * temp_center;
    Vec3d screen_box_center = Vec3d(temp_ndc.x(), temp_ndc.y(), temp_ndc.z()) / temp_ndc.w();

    float x = 0.5f * (1 + screen_box_center(0)) * window_width;
    float y = 0.5f * (1 - screen_box_center(1)) * window_height;
    return Vec2f(x, y);
}

void LegacyRenderer::VBuffer::reset()
{
    // release gpu memory
    if (!vbos.empty()) {
        glsafe(::glDeleteBuffers(static_cast<GLsizei>(vbos.size()), static_cast<const GLuint*>(vbos.data())));
        vbos.clear();
    }
    sizes.clear();
    count = 0;
}

void LegacyRenderer::InstanceVBuffer::Ranges::reset()
{
    for (Range& range : ranges) {
        // release gpu memory
        if (range.vbo > 0)
            glsafe(::glDeleteBuffers(1, &range.vbo));
    }

    ranges.clear();
}

void LegacyRenderer::InstanceVBuffer::reset()
{
    s_ids.clear();
    s_ids.shrink_to_fit();
    buffer.clear();
    buffer.shrink_to_fit();
    offsets.clear();
    offsets.shrink_to_fit();
    render_ranges.reset();
}

void LegacyRenderer::IBuffer::reset()
{
    // release gpu memory
    if (ibo > 0) {
        glsafe(::glDeleteBuffers(1, &ibo));
        ibo = 0;
    }

    vbo   = 0;
    count = 0;
}

bool LegacyRenderer::Path::matches(const GCodeProcessorResult::MoveVertex& move) const
{
    auto matches_percent = [](float value1, float value2, float max_percent) { return std::abs(value2 - value1) / value1 <= max_percent; };

    switch (move.type) {
    case EMoveType::Tool_change:
    case EMoveType::Color_change:
    case EMoveType::Pause_Print:
    case EMoveType::Custom_GCode:
    case EMoveType::Retract:
    case EMoveType::Unretract:
    case EMoveType::Seam:
    case EMoveType::Extrude_Alter:
    case EMoveType::Extrude: {
        // use rounding to reduce the number of generated paths
        return type == move.type && extruder_id == move.extruder_id && cp_color_id == move.cp_color_id && role == move.extrusion_role &&
               move.position.z() <= sub_paths.front().first.position.z() && feedrate == move.feedrate && fan_speed == move.fan_speed &&
               height == round_to_bin(move.height) && width == round_to_bin(move.width) &&
               matches_percent(volumetric_rate, move.volumetric_rate(), 0.05f) && layer_time == move.layer_duration &&
               acceleration == move.acceleration;
    }
    case EMoveType::Travel: {
        return type == move.type && feedrate == move.feedrate && extruder_id == move.extruder_id && cp_color_id == move.cp_color_id;
    }
    default: {
        return false;
    }
    }
}

void LegacyRenderer::TBuffer::Model::reset() { instances.reset(); }

void LegacyRenderer::TBuffer::reset()
{
    vertices.reset();
    for (IBuffer& buffer : indices) {
        buffer.reset();
    }

    indices.clear();
    paths.clear();
    render_paths.clear();
    model.reset();
}

void LegacyRenderer::TBuffer::add_path(const GCodeProcessorResult::MoveVertex& move, unsigned int b_id, size_t i_id, size_t s_id)
{
    Path::Endpoint endpoint = {b_id, i_id, s_id, move.position};
    // use rounding to reduce the number of generated paths
    paths.push_back({move.type,
                     move.extrusion_role,
                     move.delta_extruder,
                     round_to_bin(move.height),
                     round_to_bin(move.width),
                     move.feedrate,
                     move.fan_speed,
                     move.temperature,
                     move.volumetric_rate(),
                     move.layer_duration,
                     move.acceleration,
                     move.extruder_id,
                     move.cp_color_id,
                     {{endpoint, endpoint}}});
}

unsigned int LegacyRenderer::TBuffer::max_vertices_per_segment() const
{
    switch (render_primitive_type) {
    case ERenderPrimitiveType::Line: {
        return 2;
    }
    case ERenderPrimitiveType::Triangle: {
        return 8;
    }
    default: {
        return 0;
    }
    }
}

size_t LegacyRenderer::TBuffer::max_vertices_per_segment_size_floats() const
{
    return vertices.vertex_size_floats() * static_cast<size_t>(max_vertices_per_segment());
}

size_t LegacyRenderer::TBuffer::max_vertices_per_segment_size_bytes() const { return max_vertices_per_segment_size_floats() * sizeof(float); }

unsigned int LegacyRenderer::TBuffer::indices_per_segment() const
{
    switch (render_primitive_type) {
    case ERenderPrimitiveType::Line: {
        return 2;
    }
    case ERenderPrimitiveType::Triangle: {
        return 30;
    } // 3 indices x 10 triangles
    default: {
        return 0;
    }
    }
}

size_t LegacyRenderer::TBuffer::indices_per_segment_size_bytes() const
{
    return static_cast<size_t>(indices_per_segment() * sizeof(IBufferType));
}

unsigned int LegacyRenderer::TBuffer::max_indices_per_segment() const
{
    switch (render_primitive_type) {
    case ERenderPrimitiveType::Line: {
        return 2;
    }
    case ERenderPrimitiveType::Triangle: {
        return 36;
    } // 3 indices x 12 triangles
    default: {
        return 0;
    }
    }
}

size_t LegacyRenderer::TBuffer::max_indices_per_segment_size_bytes() const {
	return max_indices_per_segment() * sizeof(IBufferType); 
}

bool LegacyRenderer::TBuffer::has_data() const
{
    switch (render_primitive_type) {
    case ERenderPrimitiveType::Line:
    case ERenderPrimitiveType::Triangle: {
        return !vertices.vbos.empty() && vertices.vbos.front() != 0 && !indices.empty() && indices.front().ibo != 0;
    }
    case ERenderPrimitiveType::InstancedModel: {
        return model.model.is_initialized() && !model.instances.buffer.empty();
    }
    case ERenderPrimitiveType::BatchedModel: {
        return !model.data.vertices.empty() && !model.data.indices.empty() && !vertices.vbos.empty() && vertices.vbos.front() != 0 &&
               !indices.empty() && indices.front().ibo != 0;
    }
    default: {
        return false;
    }
    }
}

LegacyRenderer::SequentialRangeCap::~SequentialRangeCap()
{
    if (ibo > 0)
        glsafe(::glDeleteBuffers(1, &ibo));
}

void LegacyRenderer::SequentialRangeCap::reset()
{
    if (ibo > 0)
        glsafe(::glDeleteBuffers(1, &ibo));

    buffer = nullptr;
    ibo    = 0;
    vbo    = 0;
    color  = {0.0f, 0.0f, 0.0f, 1.0f};
}



const std::vector<double>& LegacyRenderer::FilterLayerResult::rebuild_filter_layers_z_offset(
    const LegacyRenderer* viewer, const int dynamic_stride, const std::array<unsigned int, 2>& layers_z_range, const Layers& layers)
{
    double              filtered_z = 0.0;
    std::vector<size_t> filtered_layers;

    m_layers_z_offset.resize(layers.size(), 0.0);
    m_layers_z_offset.shrink_to_fit();

    for (size_t i = layers_z_range[0]; i <= layers_z_range[1]; i++) {
        if (viewer->should_be_filtered_of_layer(dynamic_stride, i)) {
            filtered_z += layers.get_z_at(i) - layers.get_z_at(i - 1);
            filtered_layers.push_back(i);
        }
    }

    double gap   = filtered_z / (layers_z_range[1] - layers_z_range[0] - filtered_layers.size()); // gap between retain layers
    double acc_z = layers.get_z_at(layers_z_range[0] - 1);

    // calculate the offset of each retain layers
    size_t xxd = 0;
    for (size_t i = layers_z_range[0]; i <= layers_z_range[1]; i++) {
#if 0
            auto it = std::find_if(filtered_layers.begin(), filtered_layers.end(), [i](size_t layer) { return layer == i; });
            
			if (it == filtered_layers.end()) {
                //layer i be retain, otherwise will be filtered
                double origin_z = layers.get_z_at(i);
                acc_z += origin_z - layers.get_z_at(i - 1);  
				
				double offset = acc_z - origin_z;
				
				acc_z += gap;
                m_layers_z_offset[i] = offset;
            }
#else
        bool be_filter = false;
        for (size_t j = xxd; j < filtered_layers.size(); j++) {
            const size_t& x = filtered_layers[j];
            if (x == i) {
                be_filter = true;
                xxd       = j + 1;
                break;
            } else if (x > i) {
                break;
            }
        }

        if (!be_filter) {
            double origin_z = layers.get_z_at(i);
            acc_z += origin_z - layers.get_z_at(i - 1);

            /*double offset = acc_z - origin_z;
            acc_z += gap;
            m_layers_z_offset[i] = offset;*/

            m_layers_z_offset[i] = acc_z - origin_z;
            acc_z += gap;
        }
#endif
    }

    // capture build parameters, it may include layers, but layers may to big
    m_dynamic_stride = dynamic_stride;
    m_layers_z_range = layers_z_range;
    m_valid          = true;

    return m_layers_z_offset;
}	


LegacyRenderer::LegacyRenderer() 
: BaseRenderer::BaseRenderer() 
{
}

LegacyRenderer::~LegacyRenderer()
{ 
	reset(); 
}

void LegacyRenderer::init(ConfigOptionMode mode, Slic3r::PresetBundle* preset_bundle, bool isgcode) 
{
    if (m_gl_data_initialized)
        return;

	BaseRenderer::init(mode, preset_bundle, isgcode);

    if (isgcode) {
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": enter, m_buffers.size=%1%") % m_buffers.size();
        // initializes opengl data of TBuffers
        for (size_t i = 0; i < m_buffers.size(); ++i) {
            TBuffer&  buffer = m_buffers[i];
            EMoveType type   = buffer_type(i);
            switch (type) {
            default: {
                break;
            }
            case EMoveType::Tool_change:
            case EMoveType::Color_change:
            case EMoveType::Pause_Print:
            case EMoveType::Custom_GCode:
            case EMoveType::Retract:
            case EMoveType::Unretract:
            case EMoveType::Seam: {
                //            if (wxGetApp().is_gl_version_greater_or_equal_to(3, 3)) {
                //                buffer.render_primitive_type = TBuffer::ERenderPrimitiveType::InstancedModel;
                //                buffer.shader = "gouraud_light_instanced";
                //                buffer.model.model.init_from(diamond(16));
                //                buffer.model.color = option_color(type);
                //                buffer.model.instances.format = InstanceVBuffer::EFormat::InstancedModel;
                //            }
                //            else {
                if (type == EMoveType::Seam)
                    buffer.visible = true;

                buffer.render_primitive_type = TBuffer::ERenderPrimitiveType::BatchedModel;
                buffer.vertices.format       = VBuffer::EFormat::PositionNormal3;
                buffer.shader                = "gouraud_light";

                buffer.model.data             = diamond(16);
                buffer.model.color            = option_color(type);
                buffer.model.instances.format = InstanceVBuffer::EFormat::BatchedModel;
                //            }
                break;
            }
            case EMoveType::Extrude_Alter:
            case EMoveType::Wipe:
            case EMoveType::Extrude: {
                buffer.render_primitive_type = TBuffer::ERenderPrimitiveType::Triangle;
                buffer.vertices.format       = VBuffer::EFormat::PositionNormal3;
                buffer.shader                = "gouraud_light";
                break;
            }
            case EMoveType::Travel: {
                buffer.render_primitive_type = TBuffer::ERenderPrimitiveType::Line;
                buffer.vertices.format       = VBuffer::EFormat::Position;
                buffer.shader                = "flat";
                break;
            }
            }

            set_toolpath_move_type_visible(EMoveType::Extrude, true);
        }
    }
    m_gl_data_initialized = true;
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": finished");
}

bool LegacyRenderer::load_toolpaths(const GCodeProcessorResult& gcode_result, const BuildVolume& build_volume, const std::vector<BoundingBoxf3>& exclude_bounding_box)
{
    MARKING_DIALOG("STARTING_#1");

    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " start memory info " << log_memory_info();
    system_memory_stats(__FUNCTION__);

	m_is_mem_optim = should_enable_memory_optimize(gcode_result);
    const bool enable_mem_compress = m_is_mem_optim;
    // max index buffer size, in bytes
    static const size_t IBUFFER_THRESHOLD_BYTES = 64 * 1024 * 1024;

    //BOOST_LOG_TRIVIAL(info) << __FUNCTION__<< boost::format(",build_volume center{%1%, %2%}, moves count %3%\n")%build_volume.bed_center().x() % build_volume.bed_center().y() %gcode_result.moves.size();
    auto log_memory_usage = [this](const std::string& label, const std::vector<MultiVertexBuffer>& vertices, const std::vector<MultiIndexBuffer>& indices) {
        int64_t vertices_size = 0;
        for (const MultiVertexBuffer& buffers : vertices) {
            for (const VertexBuffer& buffer : buffers) {
                vertices_size += SLIC3R_STDVEC_MEMSIZE(buffer, float);
            }
            //BOOST_LOG_TRIVIAL(info) << __FUNCTION__<< boost::format("vertices count %1%\n")%buffers.size();
        }
        int64_t indices_size = 0;
        for (const MultiIndexBuffer& buffers : indices) {
            for (const IndexBuffer& buffer : buffers) {
                indices_size += SLIC3R_STDVEC_MEMSIZE(buffer, IBufferType);
            }
            //BOOST_LOG_TRIVIAL(info) << __FUNCTION__<< boost::format("indices count %1%\n")%buffers.size();
        }
        log_memory_used(label, vertices_size + indices_size);
    };

    // format data into the buffers to be rendered as lines
    auto add_vertices_as_line = [this, enable_mem_compress](const GCodeProcessorResult::MoveVertex& prev, const GCodeProcessorResult::MoveVertex& curr, VertexBuffer& vertices) {
       
        auto add_vertex = [this, &vertices, enable_mem_compress](const Vec3f& position) {
            if (enable_mem_compress) {
                Vec3f en_position = encode_position(position);
                vertices.push_back(en_position.x());
                vertices.push_back(en_position.y());
                vertices.push_back(en_position.z());
            } else {
                
                vertices.push_back(position.x());
                vertices.push_back(position.y());
                vertices.push_back(position.z());
            }
        };

        // x component of the normal to the current segment (the normal is parallel to the XY plane)
        //BBS: Has modified a lot for this function to support arc move
        size_t loop_num = curr.is_arc_move_with_interpolation_points() ? curr.interpolation_points.size() : 0;
        for (size_t i = 0; i < loop_num + 1; i++) {
            const Vec3f &previous = (i == 0? prev.position : curr.interpolation_points[i-1]);
            const Vec3f &current = (i == loop_num? curr.position : curr.interpolation_points[i]);
            // add previous vertex
            add_vertex(previous);
            // add current vertex
            add_vertex(current);
        }
    };
    //BBS: modify a lot to support arc travel
    auto add_indices_as_line = [](const GCodeProcessorResult::MoveVertex& prev, const GCodeProcessorResult::MoveVertex& curr, TBuffer& buffer,
        size_t& vbuffer_size, unsigned int ibuffer_id, IndexBuffer& indices, size_t move_id) {

            if (buffer.paths.empty() || prev.type != curr.type || !buffer.paths.back().matches(curr)) {
                buffer.add_path(curr, ibuffer_id, indices.size(), move_id - 1);
                buffer.paths.back().sub_paths.front().first.position = prev.position;
            }

            Path& last_path = buffer.paths.back();
            size_t loop_num = curr.is_arc_move_with_interpolation_points() ? curr.interpolation_points.size() : 0;
            for (size_t i = 0; i < loop_num + 1; i++) {
                //BBS: add previous index
                indices.push_back(static_cast<IBufferType>(indices.size()));
                //BBS: add current index
                indices.push_back(static_cast<IBufferType>(indices.size()));
                vbuffer_size += buffer.max_vertices_per_segment();
            }
            last_path.sub_paths.back().last = { ibuffer_id, indices.size() - 1, move_id, curr.position };
    };

    // format data into the buffers to be rendered as solid.
    auto add_vertices_as_solid = [this, enable_mem_compress](const GCodeProcessorResult::MoveVertex& prev, const GCodeProcessorResult::MoveVertex& curr, TBuffer& buffer, unsigned int vbuffer_id, VertexBuffer& vertices, size_t move_id) {
        auto store_vertex = [this, enable_mem_compress](VertexBuffer& vertices, const Vec3f& position, const Vec3f& normal) {
			if (enable_mem_compress) {
                // append position
                Vec3f en_position = encode_position(position);
                vertices.push_back(en_position.x());
                vertices.push_back(en_position.y());
                vertices.push_back(en_position.z());
                // append normal
                vertices.push_back(normal.x() * SHORT_TYPE_NORMAL_SCALE);
                vertices.push_back(normal.y() * SHORT_TYPE_NORMAL_SCALE);
                vertices.push_back(normal.z() * SHORT_TYPE_NORMAL_SCALE);

			} else {
                // append position
                vertices.push_back(position.x());
                vertices.push_back(position.y());
                vertices.push_back(position.z());
                // append normal
                vertices.push_back(normal.x());
                vertices.push_back(normal.y());
                vertices.push_back(normal.z());

			}
            
        };

        if (buffer.paths.empty() || prev.type != curr.type || !buffer.paths.back().matches(curr)) {
            buffer.add_path(curr, vbuffer_id, vertices.size(), move_id - 1);
            buffer.paths.back().sub_paths.back().first.position = prev.position;
        }

        Path& last_path = buffer.paths.back();
        //BBS: Has modified a lot for this function to support arc move
        size_t loop_num = curr.is_arc_move_with_interpolation_points() ? curr.interpolation_points.size() : 0;
        for (size_t i = 0; i < loop_num + 1; i++) {
            const Vec3f &prev_position = (i == 0? prev.position : curr.interpolation_points[i-1]);
            const Vec3f &curr_position = (i == loop_num? curr.position : curr.interpolation_points[i]);

            Vec3f dir = (curr_position - prev_position).normalized();
            // In some cases (such as when the z-offset is set to -0.1 and previewing model slices with surface boundaries), 
            // the z-value of interpolated coordinates may be abnormal, and this situation should be corrected.
            if (curr.position.z() == prev.position.z())
                dir[2] = 0.0f;

            const Vec3f right = Vec3f(dir.y(), -dir.x(), 0.0f).normalized();
            const Vec3f left = -right;
            const Vec3f up = right.cross(dir).normalized();
            const Vec3f down = -up;
            const float half_width = 0.5f * last_path.width;
            //const float half_height = 0.5f * last_path.height;
            const float half_height = (enable_mem_compress && 0.5f * last_path.height < SHORT_TYPE_POSITION_SCALE_INVERSE) ?
                                          SHORT_TYPE_POSITION_SCALE_INVERSE * 1.50:
                                          0.5f * last_path.height;
            /*const Vec3f prev_pos = prev_position - half_height * up;
            const Vec3f curr_pos = curr_position - half_height * up;*/
            Vec3f prev_pos = prev_position - half_height * up;
			Vec3f curr_pos = curr_position - half_height * up;
            if (enable_mem_compress && curr.extrusion_role == ExtrusionRole::erIroning) {
                prev_pos += Vec3f(0.0f, 0.0f, SHORT_TYPE_POSITION_SCALE_INVERSE * 1.50);
                curr_pos += Vec3f(0.0f, 0.0f, SHORT_TYPE_POSITION_SCALE_INVERSE * 1.50);
            }
            const Vec3f d_up = half_height * up;
            const Vec3f d_down = -half_height * up;
            const Vec3f d_right = half_width * right;
            const Vec3f d_left = -half_width * right;

            if ((last_path.vertices_count() == 1 || vertices.empty()) && i == 0) {
                store_vertex(vertices, prev_pos + d_up, up);
                store_vertex(vertices, prev_pos + d_right, right);
                store_vertex(vertices, prev_pos + d_down, down);
                store_vertex(vertices, prev_pos + d_left, left);
            } else {
                store_vertex(vertices, prev_pos + d_right, right);
                store_vertex(vertices, prev_pos + d_left, left);
            }

            store_vertex(vertices, curr_pos + d_up, up);
            store_vertex(vertices, curr_pos + d_right, right);
            store_vertex(vertices, curr_pos + d_down, down);
            store_vertex(vertices, curr_pos + d_left, left);
        }

        last_path.sub_paths.back().last = { vbuffer_id, vertices.size(), move_id, curr.position };
    };
    auto add_indices_as_solid = [&](const GCodeProcessorResult::MoveVertex& prev, const GCodeProcessorResult::MoveVertex& curr, const GCodeProcessorResult::MoveVertex* next,
        TBuffer& buffer, size_t& vbuffer_size, unsigned int ibuffer_id, IndexBuffer& indices, size_t move_id) {
            static Vec3f prev_dir;
            static Vec3f prev_up;
            static float sq_prev_length;
            auto store_triangle = [](IndexBuffer& indices, IBufferType i1, IBufferType i2, IBufferType i3) {
                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            };
            auto append_dummy_cap = [store_triangle](IndexBuffer& indices, IBufferType id) {
                store_triangle(indices, id, id, id);
                store_triangle(indices, id, id, id);
            };
            auto convert_vertices_offset = [](size_t vbuffer_size, const std::array<int, 8>& v_offsets) {
                std::array<IBufferType, 8> ret = {
                    static_cast<IBufferType>(static_cast<int>(vbuffer_size) + v_offsets[0]),
                    static_cast<IBufferType>(static_cast<int>(vbuffer_size) + v_offsets[1]),
                    static_cast<IBufferType>(static_cast<int>(vbuffer_size) + v_offsets[2]),
                    static_cast<IBufferType>(static_cast<int>(vbuffer_size) + v_offsets[3]),
                    static_cast<IBufferType>(static_cast<int>(vbuffer_size) + v_offsets[4]),
                    static_cast<IBufferType>(static_cast<int>(vbuffer_size) + v_offsets[5]),
                    static_cast<IBufferType>(static_cast<int>(vbuffer_size) + v_offsets[6]),
                    static_cast<IBufferType>(static_cast<int>(vbuffer_size) + v_offsets[7])
                };
                return ret;
            };
            auto append_starting_cap_triangles = [&](IndexBuffer& indices, const std::array<IBufferType, 8>& v_offsets) {
                store_triangle(indices, v_offsets[0], v_offsets[2], v_offsets[1]);
                store_triangle(indices, v_offsets[0], v_offsets[3], v_offsets[2]);
            };
            auto append_stem_triangles = [&](IndexBuffer& indices, const std::array<IBufferType, 8>& v_offsets) {
                store_triangle(indices, v_offsets[0], v_offsets[1], v_offsets[4]);
                store_triangle(indices, v_offsets[1], v_offsets[5], v_offsets[4]);
                store_triangle(indices, v_offsets[1], v_offsets[2], v_offsets[5]);
                store_triangle(indices, v_offsets[2], v_offsets[6], v_offsets[5]);
                store_triangle(indices, v_offsets[2], v_offsets[3], v_offsets[6]);
                store_triangle(indices, v_offsets[3], v_offsets[7], v_offsets[6]);
                store_triangle(indices, v_offsets[3], v_offsets[0], v_offsets[7]);
                store_triangle(indices, v_offsets[0], v_offsets[4], v_offsets[7]);
            };
            auto append_ending_cap_triangles = [&](IndexBuffer& indices, const std::array<IBufferType, 8>& v_offsets) {
                store_triangle(indices, v_offsets[4], v_offsets[6], v_offsets[7]);
                store_triangle(indices, v_offsets[4], v_offsets[5], v_offsets[6]);
            };

            if (buffer.paths.empty() || prev.type != curr.type || !buffer.paths.back().matches(curr)) {
                buffer.add_path(curr, ibuffer_id, indices.size(), move_id - 1);
                buffer.paths.back().sub_paths.back().first.position = prev.position;
            }

            Path& last_path = buffer.paths.back();
            bool is_first_segment = (last_path.vertices_count() == 1);
            //BBS: has modified a lot for this function to support arc move
            std::array<IBufferType, 8> first_seg_v_offsets = convert_vertices_offset(vbuffer_size, { 0, 1, 2, 3, 4, 5, 6, 7 });
            std::array<IBufferType, 8> non_first_seg_v_offsets = convert_vertices_offset(vbuffer_size, { -4, 0, -2, 1, 2, 3, 4, 5 });

            size_t loop_num = curr.is_arc_move_with_interpolation_points() ? curr.interpolation_points.size() : 0;
            for (size_t i = 0; i < loop_num + 1; i++) {
                const Vec3f &prev_position = (i == 0? prev.position : curr.interpolation_points[i-1]);
                const Vec3f &curr_position = (i == loop_num? curr.position : curr.interpolation_points[i]);

                const Vec3f dir = (curr_position - prev_position).normalized();
                const Vec3f right = Vec3f(dir.y(), -dir.x(), 0.0f).normalized();
                const Vec3f up = right.cross(dir);
                const float sq_length = (curr_position - prev_position).squaredNorm();

                if ((is_first_segment || vbuffer_size == 0) && i == 0) {
                    if (is_first_segment && i == 0)
                        // starting cap triangles
                        append_starting_cap_triangles(indices, first_seg_v_offsets);
                    // dummy triangles outer corner cap
                    append_dummy_cap(indices, vbuffer_size);
                    // stem triangles
                    append_stem_triangles(indices, first_seg_v_offsets);

                    vbuffer_size += 8;
                } else {
                    float displacement = 0.0f;
                    float cos_dir = prev_dir.dot(dir);
                    if (cos_dir > -0.9998477f) {
                        // if the angle between adjacent segments is smaller than 179 degrees
                        const Vec3f med_dir = (prev_dir + dir).normalized();
                        const float half_width = 0.5f * last_path.width;
                        displacement = half_width * ::tan(::acos(std::clamp(dir.dot(med_dir), -1.0f, 1.0f)));
                    }

                    float sq_displacement = sqr(displacement);
                    bool can_displace = displacement > 0.0f && sq_displacement < sq_prev_length&& sq_displacement < sq_length;

                    bool is_right_turn = prev_up.dot(prev_dir.cross(dir)) <= 0.0f;
                    // whether the angle between adjacent segments is greater than 45 degrees
                    bool is_sharp = cos_dir < 0.7071068f;

                    bool right_displaced = false;
                    bool left_displaced = false;

                    if (!is_sharp && can_displace) {
                        if (is_right_turn)
                            left_displaced = true;
                        else
                            right_displaced = true;
                    }

                    // triangles outer corner cap
                    if (is_right_turn) {
                        if (left_displaced)
                            // dummy triangles
                            append_dummy_cap(indices, vbuffer_size);
                        else {
                            store_triangle(indices, vbuffer_size - 4, vbuffer_size + 1, vbuffer_size - 1);
                            store_triangle(indices, vbuffer_size + 1, vbuffer_size - 2, vbuffer_size - 1);
                        }
                    }
                    else {
                        if (right_displaced)
                            // dummy triangles
                            append_dummy_cap(indices, vbuffer_size);
                        else {
                            store_triangle(indices, vbuffer_size - 4, vbuffer_size - 3, vbuffer_size + 0);
                            store_triangle(indices, vbuffer_size - 3, vbuffer_size - 2, vbuffer_size + 0);
                        }
                    }
                    // stem triangles
                    non_first_seg_v_offsets = convert_vertices_offset(vbuffer_size, { -4, 0, -2, 1, 2, 3, 4, 5 });
                    append_stem_triangles(indices, non_first_seg_v_offsets);
                    vbuffer_size += 6;
                }
                prev_dir = dir;
                prev_up = up;
                sq_prev_length = sq_length;
            }

            if (next != nullptr && (curr.type != next->type || !last_path.matches(*next)))
                // ending cap triangles
                append_ending_cap_triangles(indices, (is_first_segment && !curr.is_arc_move_with_interpolation_points()) ? first_seg_v_offsets : non_first_seg_v_offsets);

            last_path.sub_paths.back().last = { ibuffer_id, indices.size() - 1, move_id, curr.position };
    };

    // format data into the buffers to be rendered as instanced model
    auto add_model_instance = [](const GCodeProcessorResult::MoveVertex& curr, InstanceBuffer& instances, InstanceIdBuffer& instances_ids, size_t move_id) {
        // append position
        instances.push_back(curr.position.x());
        instances.push_back(curr.position.y());
        instances.push_back(curr.position.z());
        // append width
        instances.push_back(curr.width);
        // append height
        instances.push_back(curr.height);

        // append id
        instances_ids.push_back(move_id);
    };

    // format data into the buffers to be rendered as batched model
    auto add_vertices_as_model_batch = [this, enable_mem_compress](const GCodeProcessorResult::MoveVertex& curr, const GLModel::Geometry& data, VertexBuffer& vertices, InstanceBuffer& instances, InstanceIdBuffer& instances_ids, size_t move_id) {
        const double width = static_cast<double>(1.5f * curr.width);
        const double height = static_cast<double>(1.5f * curr.height);

        const Transform3d trafo = Geometry::assemble_transform((curr.position - 0.5f * curr.height * Vec3f::UnitZ()).cast<double>(), Vec3d::Zero(), { width, width, height });
        const Eigen::Matrix<double, 3, 3, Eigen::DontAlign> normal_matrix = trafo.matrix().template block<3, 3>(0, 0).inverse().transpose();

        // append vertices
        const size_t vertices_count = data.vertices_count();
		
		if (!enable_mem_compress) {

			for (size_t i = 0; i < vertices_count; ++i) {
				// append position
				const Vec3d position = trafo * data.extract_position_3(i).cast<double>();
				vertices.push_back(float(position.x()));
				vertices.push_back(float(position.y()));
				vertices.push_back(float(position.z()));

				// append normal
				const Vec3d normal = normal_matrix * data.extract_normal_3(i).cast<double>();
				vertices.push_back(float(normal.x()));
				vertices.push_back(float(normal.y()));
				vertices.push_back(float(normal.z()));
			}
        
		} else {
        
			for (size_t i = 0; i < vertices_count; ++i) {
                // append position
                const Vec3d position = trafo * data.extract_position_3(i).cast<double>();
                const Vec3f en_position = encode_position(position.cast<float>());
                vertices.push_back(en_position.x());
                vertices.push_back(en_position.y());
                vertices.push_back(en_position.z());

                // append normal
                const Vec3d normal = (normal_matrix * data.extract_normal_3(i).cast<double>()).normalized();
                vertices.push_back(normal.x() * SHORT_TYPE_NORMAL_SCALE);
                vertices.push_back(normal.y() * SHORT_TYPE_NORMAL_SCALE);
                vertices.push_back(normal.z() * SHORT_TYPE_NORMAL_SCALE);
            }
		}

        // append instance position
        instances.push_back(curr.position.x());
        instances.push_back(curr.position.y());
        instances.push_back(curr.position.z());
        // append instance id
        instances_ids.push_back(move_id);
    };

    auto add_indices_as_model_batch = [](const GLModel::Geometry& data, IndexBuffer& indices, IBufferType base_index) {
        const size_t indices_count = data.indices_count();
        for (size_t i = 0; i < indices_count; ++i) {
            indices.push_back(static_cast<IBufferType>(data.extract_index(i) + base_index));
        }
    };

#if ENABLE_GCODE_VIEWER_STATISTICS
    auto start_time = std::chrono::high_resolution_clock::now();
    m_statistics.results_size = SLIC3R_STDVEC_MEMSIZE(gcode_result.moves, GCodeProcessorResult::MoveVertex);
    m_statistics.results_time = gcode_result.time;
#endif // ENABLE_GCODE_VIEWER_STATISTICS

    m_moves_count = gcode_result.moves.size();
    if (m_moves_count == 0)
        return false;

    m_extruders_count = gcode_result.extruders_count;

    // Import gcode file, preview using normal mode (no lite mode)
    bool is_lite_mode_cfg = (wxGetApp().app_config->get_bool("gcode_preview_lite_mode") && (!m_only_gcode_in_preview));

    // Auto enable lite mode on Linux if memory is tight or preview is huge.
#if defined(__linux__)
    {
        size_t avail_bytes = Slic3r::available_physical_memory();
        size_t total_bytes = Slic3r::total_physical_memory();
        const size_t moves_count_local = gcode_result.moves.size();
        const size_t avail_threshold_bytes = size_t(2) * size_t(1024) * size_t(1024) * size_t(1024); // 2 GB
        const bool low_available = (avail_bytes > 0 && avail_bytes < avail_threshold_bytes);
        const bool low_ratio     = (total_bytes > 0 && avail_bytes > 0 && (double)avail_bytes / (double)total_bytes < 0.125);
        const bool huge_preview  = (moves_count_local > 2000000);
        if (!m_only_gcode_in_preview && (low_available || low_ratio || huge_preview))
            is_lite_mode_cfg = true;
    }
#endif

    const bool is_lite_mode = m_is_lite_mode = is_lite_mode_cfg;

    unsigned int progress_count = 0;
    unsigned int progress_threshold = 1;
    if (m_moves_count >= 100) {
        progress_threshold = m_moves_count / 100;
    }
        
    //BBS: add only gcode mode
    //ProgressDialog *          progress_dialog    = m_only_gcode_in_preview ?
    //    new ProgressDialog(_L("Loading G-codes"), "...",
    //        100, wxGetApp().mainframe, wxPD_AUTO_HIDE | wxPD_APP_MODAL) : nullptr;

    // in only gcode mode, the UI update event would cause exception
    ProgressDialog *          progress_dialog = nullptr;

    wxBusyCursor busy;

    PartPlateList& partplate_list = wxGetApp().plater()->get_partplate_list();
    PartPlate* current_plate = partplate_list.get_curr_plate();

    m_contained_in_bed = (current_plate->get_slice_result()->toolpath_outside == false);
    m_paths_bounding_box = current_plate->get_gcode_path_bounding_box();

    // set approximate max bounding box (take in account also the tool marker)
    m_max_bounding_box = m_paths_bounding_box;
    m_max_bounding_box.merge(m_paths_bounding_box.max + marker.get_bounding_box().size().z() * Vec3d::UnitZ());

    // move the path bounding box calculation to partplate after slicing process complete
    /*
    //BBS: use convex_hull for toolpath outside check
    Points pts;

    // extract approximate paths bounding box from result
    //BBS: add only gcode mode
    for (const GCodeProcessorResult::MoveVertex& move : gcode_result.moves) {
        //if (wxGetApp().is_gcode_viewer()) {
        //if (m_only_gcode_in_preview) {
            // for the gcode viewer we need to take in account all moves to correctly size the printbed
        //    m_paths_bounding_box.merge(move.position.cast<double>());
        //}
        //else {
            if (move.type == EMoveType::Extrude && move.extrusion_role != erCustom && move.width != 0.0f && move.height != 0.0f) {
                m_paths_bounding_box.merge(move.position.cast<double>());
                //BBS: use convex_hull for toolpath outside check
                pts.emplace_back(Point(scale_(move.position.x()), scale_(move.position.y())));
            }
        //}
    }

    // BBS: also merge the point on arc to bounding box
    for (const GCodeProcessorResult::MoveVertex& move : gcode_result.moves) {
        // continue if not arc path
        if (!move.is_arc_move_with_interpolation_points())
            continue;

        //if (wxGetApp().is_gcode_viewer())
        //if (m_only_gcode_in_preview)
        //    for (int i = 0; i < move.interpolation_points.size(); i++)
        //        m_paths_bounding_box.merge(move.interpolation_points[i].cast<double>());
        //else {
            if (move.type == EMoveType::Extrude && move.width != 0.0f && move.height != 0.0f)
                for (int i = 0; i < move.interpolation_points.size(); i++) {
                    m_paths_bounding_box.merge(move.interpolation_points[i].cast<double>());
                    //BBS: use convex_hull for toolpath outside check
                    pts.emplace_back(Point(scale_(move.interpolation_points[i].x()), scale_(move.interpolation_points[i].y())));
                }
        //}
    }

    // set approximate max bounding box (take in account also the tool marker)
    m_max_bounding_box = m_paths_bounding_box;
    m_max_bounding_box.merge(m_paths_bounding_box.max + marker.get_bounding_box().size().z() * Vec3d::UnitZ());

    BOOST_LOG_TRIVIAL(info) << __FUNCTION__<< boost::format(",m_paths_bounding_box {%1%, %2%}-{%3%, %4%}\n")
        %m_paths_bounding_box.min.x() %m_paths_bounding_box.min.y() %m_paths_bounding_box.max.x() %m_paths_bounding_box.max.y();

    //if (wxGetApp().is_editor())
    {
        //BBS: use convex_hull for toolpath outside check
        m_contained_in_bed = build_volume.all_paths_inside(gcode_result, m_paths_bounding_box);
        if (m_contained_in_bed) {
            //PartPlateList& partplate_list = wxGetApp().plater()->get_partplate_list();
            //PartPlate* plate = partplate_list.get_curr_plate();
            //const std::vector<BoundingBoxf3>& exclude_bounding_box = plate->get_exclude_areas();
            if (exclude_bounding_box.size() > 0)
            {
                int index;
                Slic3r::Polygon convex_hull_2d = Slic3r::Geometry::convex_hull(std::move(pts));
                for (index = 0; index < exclude_bounding_box.size(); index ++)
                {
                    Slic3r::Polygon p = exclude_bounding_box[index].polygon(true);  // instance convex hull is scaled, so we need to scale here
                    if (intersection({ p }, { convex_hull_2d }).empty() == false)
                    {
                        m_contained_in_bed = false;
                        break;
                    }
                }
            }
        }
        (const_cast<GCodeProcessorResult&>(gcode_result)).toolpath_outside = !m_contained_in_bed;
    }
    */

    m_sequential_view.gcode_ids.clear();
    m_sid_to_moveid.clear();
    for (size_t i = 0; i < gcode_result.moves.size(); ++i) {
        const GCodeProcessorResult::MoveVertex& move = gcode_result.moves[i];
        if (move.type != EMoveType::Seam) {
            m_sequential_view.gcode_ids.push_back(move.gcode_id);
			m_sid_to_moveid.push_back(i);
        }
    }
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__<< boost::format(",m_contained_in_bed %1%\n")%m_contained_in_bed;

    std::vector<MultiVertexBuffer> vertices(m_buffers.size());
    std::vector<MultiIndexBuffer> indices(m_buffers.size());
    std::vector<InstanceBuffer> instances(m_buffers.size());
    std::vector<InstanceIdBuffer> instances_ids(m_buffers.size());
    std::vector<InstancesOffsets> instances_offsets(m_buffers.size());
    std::vector<float> options_zs;

    size_t seams_count = 0;
    std::vector<size_t> biased_seams_ids;

     BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " load indices start";
    // toolpaths data -> extract vertices from result
    for (size_t i = 0; i < m_moves_count; ++i) {
        const GCodeProcessorResult::MoveVertex& curr = gcode_result.moves[i];
        if (curr.type == EMoveType::Seam) {
            ++seams_count;
            biased_seams_ids.push_back(i - biased_seams_ids.size() - 1);
        }

        size_t move_id = i - seams_count;

        // skip first vertex
        if (i == 0)
            continue;

        if (is_lite_mode && !(curr.type == EMoveType::Extrude || curr.type == EMoveType::Seam || curr.type == EMoveType::Unretract || curr.type == EMoveType::Pause_Print))
            continue;

        const GCodeProcessorResult::MoveVertex& prev = gcode_result.moves[i - 1];

        // update progress dialog
        ++progress_count;
        if (progress_dialog != nullptr && progress_count % progress_threshold == 0) {
            progress_dialog->Update(int(100.0f * float(i) / (2.0f * float(m_moves_count))),
                _L("Generating geometry vertex data") + ": " + wxNumberFormatter::ToString(100.0 * double(i) / double(m_moves_count), 0, wxNumberFormatter::Style_None) + "%");
            progress_dialog->Fit();
            progress_count = 0;
        }

        const unsigned char id = buffer_id(curr.type);
        TBuffer& t_buffer = m_buffers[id];
        MultiVertexBuffer& v_multibuffer = vertices[id];
        InstanceBuffer& inst_buffer = instances[id];
        InstanceIdBuffer& inst_id_buffer = instances_ids[id];
        InstancesOffsets& inst_offsets = instances_offsets[id];

        /*if (i%1000 == 1) {
            BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(":i=%1%, buffer_id %2% render_type %3%, gcode_id %4%\n")
                %i %(int)id %(int)t_buffer.render_primitive_type %curr.gcode_id;
        }*/

        // ensure there is at least one vertex buffer
        if (v_multibuffer.empty()) {
            BOOST_LOG_TRIVIAL(warning) << "v_multibuffer.empty(),  v_multibuffer.push_back(VertexBuffer())";
            v_multibuffer.push_back(VertexBuffer(enable_mem_compress));        
        }


        // if adding the vertices for the current segment exceeds the threshold size of the current vertex buffer
        // add another vertex buffer
        // BBS: get the point number and then judge whether the remaining buffer is enough
        size_t points_num = curr.is_arc_move_with_interpolation_points() ? curr.interpolation_points.size() + 1 : 1;
        size_t vertices_size_to_add = (t_buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::BatchedModel) ? t_buffer.model.data.vertices_size_bytes() : points_num * t_buffer.max_vertices_per_segment_size_bytes();
        if (v_multibuffer.empty()) { // <--- 只在这里添加日志逻辑
            // --- 检测到致命错误：即将访问空 vector 的 back() ---
            std::ostringstream error_msg;
            // 获取尽可能多的相关上下文信息
            const GCodeProcessorResult::MoveVertex& prev = gcode_result.moves[i - 1]; // 确保 i > 0
            error_msg << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
                      << __FUNCTION__ << ": IMMINENT CRASH DETECTED!"<< ")\n"
                      << "Reason: v_multibuffer is unexpectedly EMPTY right before accessing back().size()!\n"
                      << "This contradicts earlier logic that should ensure it's not empty.\n"
                      << "Suspect memory corruption or subtle logic/optimization error.\n"
                      << "Allowing crash to proceed for reporting purposes after logging.\n"
                      << "---------------- Context Information -----------------\n"
                      << "Loop Index (i): " << i
                      << "\n"
                      // << "Move ID (move_id): " << move_id << "\n" // 如果 move_id 在此作用域可用
                      << "Buffer ID (id): " << (int) id << "\n"
                      << "Overall State: vertices.size()=" << vertices.size() << ", m_buffers.size()=" << m_buffers.size()
                      << "\n"
                      // << "Target TBuffer Info: render_primitive_type=" << static_cast<int>(t_buffer.render_primitive_type) << "\n" //
                      // 如果 t_buffer 可用
                      // << "Calculated vertices_size_to_add: " << vertices_size_to_add << "\n" // 如果 vertices_size_to_add 可用
                      << "--- Current Move (curr) ---\n"
                      << "  Type: " << static_cast<int>(curr.type) << " (" << buffer_id(curr.type) << ")\n"
                      << "  Position: (" << curr.position.x() << ", " << curr.position.y() << ", " << curr.position.z() << ")\n"
                      << "  Extruder ID: " << curr.extruder_id << ", G-code Line ID: " << curr.gcode_id << "\n"
                      << "--- Previous Move (prev) ---\n"
                      << "  Type: " << static_cast<int>(prev.type) << " (" << buffer_id(prev.type) << ")\n"
                      << "  Position: (" << prev.position.x() << ", " << prev.position.y() << ", " << prev.position.z() << ")\n"
                      << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";

            // 1. 使用日志库记录错误 (FATAL 级别)
            BOOST_LOG_TRIVIAL(error) << error_msg.str();

            // 2. 强制刷新日志库缓冲区 (关键步骤)
            BOOST_LOG_TRIVIAL(warning) << "Attempting to flush logs before expected crash...";
            try {
                boost::log::core::get()->flush();
                BOOST_LOG_TRIVIAL(warning) << "Log flush initiated successfully.";
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": FAILED TO INITIATE LOG FLUSH! Error: " << e.what();
            } catch (...) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": FAILED TO INITIATE LOG FLUSH! Unknown exception.";
            }

            // 3. 短暂延迟，给日志写入留出时间 (不阻塞后台日志线程)
            const int delay_ms = 1000;
            BOOST_LOG_TRIVIAL(info) << "Introducing " << delay_ms << "ms delay to aid log writing...";
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            BOOST_LOG_TRIVIAL(info) << "Delay finished. Proceeding to expected crash point...";
           
            BOOST_LOG_TRIVIAL(warning) << "Attempting to flush logs before expected crash...";
            try {
                boost::log::core::get()->flush();
                BOOST_LOG_TRIVIAL(warning) << "Log flush initiated successfully.";
            } catch (const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": FAILED TO INITIATE LOG FLUSH! Error: " << e.what();
            } catch (...) {
                BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << ": FAILED TO INITIATE LOG FLUSH! Unknown exception.";
            }

            // 4. 不做任何阻止，让代码自然执行到下一行导致崩溃
            BOOST_LOG_TRIVIAL(error) << ">>> Now executing the line expected to crash <<<";

        }   
        
        if (v_multibuffer.back().size() * sizeof(float) > t_buffer.vertices.max_size_bytes() - vertices_size_to_add) {
            v_multibuffer.push_back(VertexBuffer(enable_mem_compress));
            if (t_buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::Triangle) {
                Path& last_path = t_buffer.paths.back();
                if (prev.type == curr.type && last_path.matches(curr))
                    last_path.add_sub_path(prev, static_cast<unsigned int>(v_multibuffer.size()) - 1, 0, move_id - 1);
            }
        }

        VertexBuffer& v_buffer = v_multibuffer.back();

        switch (t_buffer.render_primitive_type)
        {
        case TBuffer::ERenderPrimitiveType::Line:     { add_vertices_as_line(prev, curr, v_buffer); break; }
        case TBuffer::ERenderPrimitiveType::Triangle: { add_vertices_as_solid(prev, curr, t_buffer, static_cast<unsigned int>(v_multibuffer.size()) - 1, v_buffer, move_id); break; }
        case TBuffer::ERenderPrimitiveType::InstancedModel:
        {
            add_model_instance(curr, inst_buffer, inst_id_buffer, move_id);
            inst_offsets.push_back(prev.position - curr.position);
#if ENABLE_GCODE_VIEWER_STATISTICS
            ++m_statistics.instances_count;
#endif // ENABLE_GCODE_VIEWER_STATISTICS
            break;
        }
        case TBuffer::ERenderPrimitiveType::BatchedModel:
        {
            add_vertices_as_model_batch(curr, t_buffer.model.data, v_buffer, inst_buffer, inst_id_buffer, move_id);
            inst_offsets.push_back(prev.position - curr.position);
#if ENABLE_GCODE_VIEWER_STATISTICS
            ++m_statistics.batched_count;
#endif // ENABLE_GCODE_VIEWER_STATISTICS
            break;
        }
        }

        // collect options zs for later use
        if (curr.type == EMoveType::Pause_Print || curr.type == EMoveType::Custom_GCode) {
            const float* const last_z = options_zs.empty() ? nullptr : &options_zs.back();
            if (last_z == nullptr || curr.position[2] < *last_z - EPSILON || *last_z + EPSILON < curr.position[2])
                options_zs.emplace_back(curr.position[2]);
        }
    }

    system_memory_stats("Gcode parsing completed");

    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " load indices end";

    /*for (size_t b = 0; b < vertices.size(); ++b) {
        MultiVertexBuffer& v_multibuffer = vertices[b];
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(":b=%1%, vertex buffer count %2%\n")
            %b %v_multibuffer.size();
    }*/
    auto extract_move_id = [&biased_seams_ids](size_t id) {
        size_t new_id = size_t(-1);
        auto it = std::lower_bound(biased_seams_ids.begin(), biased_seams_ids.end(), id);
        if (it == biased_seams_ids.end())
            new_id = id + biased_seams_ids.size();
        else {
            if (it == biased_seams_ids.begin() && *it < id)
                new_id = id;
            else if (it != biased_seams_ids.begin())
                new_id = id + std::distance(biased_seams_ids.begin(), it);
        }
        return (new_id == size_t(-1)) ? id : new_id;
    };
    //BBS: generate map from ssid to move id in advance to reduce computation
    m_ssid_to_moveid_map.clear();
    m_ssid_to_moveid_map.reserve( m_moves_count - biased_seams_ids.size());
    for (size_t i = 0; i < m_moves_count - biased_seams_ids.size(); i++)
        m_ssid_to_moveid_map.push_back(extract_move_id(i));

    // Build arc-interpolation-points prefix-sum for O(1) range queries in refresh_render_paths.
    // m_ssid_arc_extra_segments[i] = cumulative extra arc segments for all s_ids in [0, i].
    {
        const size_t ssid_count = m_ssid_to_moveid_map.size();
        m_ssid_arc_extra_segments.resize(ssid_count, 0u);
        unsigned int running_sum = 0;
        for (size_t i = 0; i < ssid_count; ++i) {
            const size_t move_id = m_ssid_to_moveid_map[i];
            if (move_id < gcode_result.moves.size()) {
                const auto& mv = gcode_result.moves[move_id];
                if (mv.is_arc_move())
                    running_sum += static_cast<unsigned int>(mv.interpolation_points.size());
            }
            m_ssid_arc_extra_segments[i] = running_sum;
        }
    }

	// dismiss, no more needed
    std::vector<size_t>().swap(biased_seams_ids);

    const float position_scale = enable_mem_compress ? SHORT_TYPE_POSITION_SCALE : 1.0;

    //BBS: smooth toolpaths corners for the given TBuffer using triangles
    auto smooth_triangle_toolpaths_corners = [&gcode_result, this, position_scale](const TBuffer& t_buffer, MultiVertexBuffer& v_multibuffer) {
        auto extract_position_at = [](const VertexBuffer& vertices, size_t offset) {
            return Vec3f(vertices[offset + 0], vertices[offset + 1], vertices[offset + 2]);
        };
        auto update_position_at = [](VertexBuffer& vertices, size_t offset, const Vec3f& position) {
            /*vertices[offset + 0] = position.x();
            vertices[offset + 1] = position.y();
            vertices[offset + 2] = position.z();*/
            vertices.store_data_at_index(offset + 0, position.x());
            vertices.store_data_at_index(offset + 1, position.y());
            vertices.store_data_at_index(offset + 2, position.z());
        };
        auto match_right_vertices_with_internal_point = [&](const Path::Sub_Path& prev_sub_path, const Path::Sub_Path& next_sub_path,
            size_t curr_s_id, bool is_internal_point, size_t interpolation_point_id, size_t vertex_size_floats, const Vec3f& displacement_vec) {
            if (&prev_sub_path == &next_sub_path || is_internal_point) { // previous and next segment are both contained into to the same vertex buffer
                VertexBuffer& vbuffer = v_multibuffer[prev_sub_path.first.b_id];
                // offset into the vertex buffer of the next segment 1st vertex
                size_t temp_offset = prev_sub_path.last.s_id - curr_s_id;
                for (size_t i = prev_sub_path.last.s_id; i > curr_s_id; i--) {
                    size_t move_id = m_ssid_to_moveid_map[i];
                    temp_offset += (gcode_result.moves[move_id].is_arc_move() ? gcode_result.moves[move_id].interpolation_points.size() : 0);
                }
                if (is_internal_point) {
                    size_t move_id = m_ssid_to_moveid_map[curr_s_id];
                    temp_offset += (gcode_result.moves[move_id].interpolation_points.size() - interpolation_point_id);
                }
                const size_t next_1st_offset = temp_offset * 6 * vertex_size_floats;
                // offset into the vertex buffer of the right vertex of the previous segment
                const size_t prev_right_offset = prev_sub_path.last.i_id - next_1st_offset - 3 * vertex_size_floats;
                // new position of the right vertices
                const Vec3f shared_vertex = extract_position_at(vbuffer, prev_right_offset) + displacement_vec;
                // update previous segment
                update_position_at(vbuffer, prev_right_offset, shared_vertex);
                // offset into the vertex buffer of the right vertex of the next segment
                const size_t next_right_offset = prev_sub_path.last.i_id - next_1st_offset;
                // update next segment
                update_position_at(vbuffer, next_right_offset, shared_vertex);
            }
            else { // previous and next segment are contained into different vertex buffers
                VertexBuffer& prev_vbuffer = v_multibuffer[prev_sub_path.first.b_id];
                VertexBuffer& next_vbuffer = v_multibuffer[next_sub_path.first.b_id];
                // offset into the previous vertex buffer of the right vertex of the previous segment
                const size_t prev_right_offset = prev_sub_path.last.i_id - 3 * vertex_size_floats;
                // new position of the right vertices
                const Vec3f shared_vertex = extract_position_at(prev_vbuffer, prev_right_offset) + displacement_vec;
                // update previous segment
                update_position_at(prev_vbuffer, prev_right_offset, shared_vertex);
                // offset into the next vertex buffer of the right vertex of the next segment
                const size_t next_right_offset = next_sub_path.first.i_id + 1 * vertex_size_floats;
                // update next segment
                update_position_at(next_vbuffer, next_right_offset, shared_vertex);
            }
        };
        //BBS: modify a lot of this function to support arc move
        auto match_left_vertices_with_internal_point = [&](const Path::Sub_Path& prev_sub_path, const Path::Sub_Path& next_sub_path,
            size_t curr_s_id, bool is_internal_point, size_t interpolation_point_id, size_t vertex_size_floats, const Vec3f& displacement_vec) {
            if (&prev_sub_path == &next_sub_path || is_internal_point) { // previous and next segment are both contained into to the same vertex buffer
                VertexBuffer& vbuffer = v_multibuffer[prev_sub_path.first.b_id];
                // offset into the vertex buffer of the next segment 1st vertex
                size_t temp_offset = prev_sub_path.last.s_id - curr_s_id;
                for (size_t i = prev_sub_path.last.s_id; i > curr_s_id; i--) {
                    size_t move_id = m_ssid_to_moveid_map[i];
                    temp_offset += (gcode_result.moves[move_id].is_arc_move() ? gcode_result.moves[move_id].interpolation_points.size() : 0);
                }
                if (is_internal_point) {
                    size_t move_id = m_ssid_to_moveid_map[curr_s_id];
                    temp_offset += (gcode_result.moves[move_id].interpolation_points.size() - interpolation_point_id);
                }
                const size_t next_1st_offset = temp_offset * 6 * vertex_size_floats;
                // offset into the vertex buffer of the left vertex of the previous segment
                const size_t prev_left_offset = prev_sub_path.last.i_id - next_1st_offset - 1 * vertex_size_floats;
                // new position of the left vertices
                const Vec3f shared_vertex = extract_position_at(vbuffer, prev_left_offset) + displacement_vec;
                // update previous segment
                update_position_at(vbuffer, prev_left_offset, shared_vertex);
                // offset into the vertex buffer of the left vertex of the next segment
                const size_t next_left_offset = prev_sub_path.last.i_id - next_1st_offset + 1 * vertex_size_floats;
                // update next segment
                update_position_at(vbuffer, next_left_offset, shared_vertex);
            }
            else { // previous and next segment are contained into different vertex buffers
                VertexBuffer& prev_vbuffer = v_multibuffer[prev_sub_path.first.b_id];
                VertexBuffer& next_vbuffer = v_multibuffer[next_sub_path.first.b_id];
                // offset into the previous vertex buffer of the left vertex of the previous segment
                const size_t prev_left_offset = prev_sub_path.last.i_id - 1 * vertex_size_floats;
                // new position of the left vertices
                const Vec3f shared_vertex = extract_position_at(prev_vbuffer, prev_left_offset) + displacement_vec;
                // update previous segment
                update_position_at(prev_vbuffer, prev_left_offset, shared_vertex);
                // offset into the next vertex buffer of the left vertex of the next segment
                const size_t next_left_offset = next_sub_path.first.i_id + 3 * vertex_size_floats;
                // update next segment
                update_position_at(next_vbuffer, next_left_offset, shared_vertex);
            }
        };

        size_t vertex_size_floats = t_buffer.vertices.vertex_size_floats();
        for (const Path& path : t_buffer.paths) {
            //BBS: the two segments of the path sharing the current vertex may belong
            //to two different vertex buffers
            size_t prev_sub_path_id = 0;
            size_t next_sub_path_id = 0;
            const size_t path_vertices_count = path.vertices_count();
            const float half_width = 0.5f * path.width;
            // BBS: modify a lot to support arc move which has internal points
            for (size_t j = 1; j < path_vertices_count; ++j) {
                size_t curr_s_id = path.sub_paths.front().first.s_id + j;
                size_t move_id = m_ssid_to_moveid_map[curr_s_id];
                int interpolation_points_num = gcode_result.moves[move_id].is_arc_move_with_interpolation_points()?
                                                    gcode_result.moves[move_id].interpolation_points.size() : 0;
                int loop_num = interpolation_points_num;
                //BBS: select the subpaths which contains the previous/next segments
                if (!path.sub_paths[prev_sub_path_id].contains(curr_s_id))
                    ++prev_sub_path_id;
                if (j == path_vertices_count - 1) {
                    if (!gcode_result.moves[move_id].is_arc_move_with_interpolation_points())
                        break;   // BBS: the last move has no internal point.
                    loop_num--;  //BBS: don't need to handle the endpoint of the last arc move of path
                    next_sub_path_id = prev_sub_path_id;
                } else {
                    if (!path.sub_paths[next_sub_path_id].contains(curr_s_id + 1))
                        ++next_sub_path_id;
                }
                const Path::Sub_Path& prev_sub_path = path.sub_paths[prev_sub_path_id];
                const Path::Sub_Path& next_sub_path = path.sub_paths[next_sub_path_id];

                // BBS: smooth triangle toolpaths corners including arc move which has internal interpolation point
                for (int k = 0; k <= loop_num; k++) {
                    const Vec3f& prev = k==0?
                                        gcode_result.moves[move_id - 1].position :
                                        gcode_result.moves[move_id].interpolation_points[k-1];
                    const Vec3f& curr = k==interpolation_points_num?
                                        gcode_result.moves[move_id].position :
                                        gcode_result.moves[move_id].interpolation_points[k];
                    const Vec3f& next = k < interpolation_points_num - 1?
                                        gcode_result.moves[move_id].interpolation_points[k+1]:
                                        (k == interpolation_points_num - 1? gcode_result.moves[move_id].position :
                                        (gcode_result.moves[move_id + 1].is_arc_move_with_interpolation_points()?
                                        gcode_result.moves[move_id + 1].interpolation_points[0] :
                                        gcode_result.moves[move_id + 1].position));

                    const Vec3f prev_dir = (curr - prev).normalized();
                    const Vec3f prev_right = Vec3f(prev_dir.y(), -prev_dir.x(), 0.0f).normalized();
                    const Vec3f prev_up = prev_right.cross(prev_dir);

                    const Vec3f next_dir = (next - curr).normalized();

                    const bool is_right_turn = prev_up.dot(prev_dir.cross(next_dir)) <= 0.0f;
                    const float cos_dir = prev_dir.dot(next_dir);
                    // whether the angle between adjacent segments is greater than 45 degrees
                    const bool is_sharp = cos_dir < 0.7071068f;

                    float displacement = 0.0f;
                    if (cos_dir > -0.9998477f) {
                        // if the angle between adjacent segments is smaller than 179 degrees
                        Vec3f med_dir = (prev_dir + next_dir).normalized();
                        displacement = half_width * ::tan(::acos(std::clamp(next_dir.dot(med_dir), -1.0f, 1.0f)));
                    }

                    const float sq_prev_length = (curr - prev).squaredNorm();
                    const float sq_next_length = (next - curr).squaredNorm();
                    const float sq_displacement = sqr(displacement);
                    const bool can_displace = displacement > 0.0f && sq_displacement < sq_prev_length&& sq_displacement < sq_next_length;
                    bool is_internal_point = interpolation_points_num > k;

                    if (can_displace) {
                        // displacement to apply to the vertices to match
                        Vec3f displacement_vec = displacement * prev_dir * position_scale;
                        // matches inner corner vertices
                        if (is_right_turn)
                            match_right_vertices_with_internal_point(prev_sub_path, next_sub_path, curr_s_id, is_internal_point, k, vertex_size_floats, -displacement_vec);
                        else
                            match_left_vertices_with_internal_point(prev_sub_path, next_sub_path, curr_s_id, is_internal_point, k, vertex_size_floats, -displacement_vec);

                        if (!is_sharp) {
                            //BBS: matches outer corner vertices
                            if (is_right_turn)
                                match_left_vertices_with_internal_point(prev_sub_path, next_sub_path, curr_s_id, is_internal_point, k, vertex_size_floats, displacement_vec);
                            else
                                match_right_vertices_with_internal_point(prev_sub_path, next_sub_path, curr_s_id, is_internal_point, k, vertex_size_floats, displacement_vec);
                        }
                    }
                }
            }
        }
    };

#if ENABLE_GCODE_VIEWER_STATISTICS
    auto load_vertices_time = std::chrono::high_resolution_clock::now();
    m_statistics.load_vertices = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
#endif // ENABLE_GCODE_VIEWER_STATISTICS

    // smooth toolpaths corners for TBuffers using triangles
    for (size_t i = 0; i < m_buffers.size(); ++i) {
        const TBuffer& t_buffer = m_buffers[i];
        if (t_buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::Triangle) {
            smooth_triangle_toolpaths_corners(t_buffer, vertices[i]);
        }
    }
    system_memory_stats("smooth_triangle_toolpaths_corners end");

    for (MultiVertexBuffer& v_multibuffer : vertices) {
        for (VertexBuffer& v_buffer : v_multibuffer) {
            v_buffer.shrink_to_fit();
        }
    }

#define USE_PARALLEL 1

    // move the wipe toolpaths half height up to render them on proper position
    MultiVertexBuffer& wipe_vertices = vertices[buffer_id(EMoveType::Wipe)];
    for (VertexBuffer& v_buffer : wipe_vertices) {
#if USE_PARALLEL
        tbb::parallel_for(tbb::blocked_range<size_t>(0, v_buffer.size() / 3), [&v_buffer, position_scale](const tbb::blocked_range<size_t>& range) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                float v = v_buffer[i * 3 + 2] + 0.5f * GCodeProcessor::Wipe_Height * position_scale;
                v_buffer.store_data_at_index(i * 3 + 2, v);
            }
        });
#else
        for (size_t i = 2; i < v_buffer.size(); i += 3) {
            //v_buffer[i] += 0.5f * GCodeProcessor::Wipe_Height;
            float v = v_buffer[i * 3 + 2] + 0.5f * GCodeProcessor::Wipe_Height;
            v_buffer.store_data_at_index(i * 3 + 2, v);
        }
#endif // USE_PARALLEL
    }

    // send vertices data to gpu, where needed
    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " send vertices data to gpu, where needed start";

    for (size_t i = 0; i < m_buffers.size(); ++i) {
        
        if (is_lite_mode && !(i == buffer_id(EMoveType::Extrude) || i == buffer_id(EMoveType::Seam) || i == buffer_id(EMoveType::Unretract) || i == buffer_id(EMoveType::Pause_Print)))
            continue;
        
        TBuffer& t_buffer = m_buffers[i];
        if (t_buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::InstancedModel) {
            const InstanceBuffer& inst_buffer = instances[i];
            if (!inst_buffer.empty()) {
                t_buffer.model.instances.buffer = inst_buffer;
                t_buffer.model.instances.s_ids = instances_ids[i];
                t_buffer.model.instances.offsets = instances_offsets[i];
            }
        }
        else {
            if (t_buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::BatchedModel) {
                const InstanceBuffer& inst_buffer = instances[i];
                if (!inst_buffer.empty()) {
                    t_buffer.model.instances.buffer = inst_buffer;
                    t_buffer.model.instances.s_ids = instances_ids[i];
                    t_buffer.model.instances.offsets = instances_offsets[i];
                }
            }
            const MultiVertexBuffer& v_multibuffer = vertices[i];
            for (const VertexBuffer& v_buffer : v_multibuffer) {
                const size_t size_elements = v_buffer.size();
                const size_t size_bytes     = size_elements * v_buffer.size_of_element_type();
                const size_t vertices_count = size_elements / (t_buffer.vertices.vertex_size_floats() * v_buffer.size_of_element_type());
                t_buffer.vertices.count += vertices_count;

#if ENABLE_GCODE_VIEWER_STATISTICS
                m_statistics.total_vertices_gpu_size += static_cast<int64_t>(size_bytes);
                m_statistics.max_vbuffer_gpu_size = std::max(m_statistics.max_vbuffer_gpu_size, static_cast<int64_t>(size_bytes));
                ++m_statistics.vbuffers_count;
#endif // ENABLE_GCODE_VIEWER_STATISTICS

                GLuint id = 0;
                glsafe(::glGenBuffers(1, &id));
                glsafe(::glBindBuffer(GL_ARRAY_BUFFER, id));
                glsafe(::glBufferData(GL_ARRAY_BUFFER, size_bytes, v_buffer.data(), GL_STATIC_DRAW));
                glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));

                t_buffer.vertices.vbos.push_back(static_cast<unsigned int>(id));
                t_buffer.vertices.sizes.push_back(size_bytes);
            }
        }
    }

    system_memory_stats("send vertices data to gpu end");

    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " send vertices data to gpu, where needed end";

    MARKING_DIALOG("Vertices_Peak_#2");

#if ENABLE_GCODE_VIEWER_STATISTICS
    auto smooth_vertices_time = std::chrono::high_resolution_clock::now();
    m_statistics.smooth_vertices = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - load_vertices_time).count();
#endif // ENABLE_GCODE_VIEWER_STATISTICS
    log_memory_usage("Loaded G-code generated vertex buffers ", vertices, indices);

    // dismiss vertices data, no more needed
    std::vector<MultiVertexBuffer>().swap(vertices);
    std::vector<InstanceBuffer>().swap(instances);
    std::vector<InstanceIdBuffer>().swap(instances_ids);
    std::vector<InstancesOffsets>().swap(instances_offsets);
    
    // toolpaths data -> extract indices from result
    // paths may have been filled while extracting vertices,
    // so reset them, they will be filled again while extracting indices
    for (TBuffer& buffer : m_buffers) {
        buffer.paths.clear();
    }

	MARKING_DIALOG("Vertices_Ending_#3");

    // variable used to keep track of the current vertex buffers index and size
    using CurrVertexBuffer = std::pair<unsigned int, size_t>;
    std::vector<CurrVertexBuffer> curr_vertex_buffers(m_buffers.size(), { 0, 0 });

    // variable used to keep track of the vertex buffers ids
    using VboIndexList = std::vector<unsigned int>;
    std::vector<VboIndexList> vbo_indices(m_buffers.size());


    seams_count = 0;

    for (size_t i = 0; i < m_moves_count; ++i) {
        const GCodeProcessorResult::MoveVertex& curr = gcode_result.moves[i];
        if (curr.type == EMoveType::Seam)
            ++seams_count;

        size_t move_id = i - seams_count;

        // skip first vertex
        if (i == 0)
            continue;

        if (is_lite_mode && !(curr.type == EMoveType::Extrude || curr.type == EMoveType::Seam || curr.type == EMoveType::Unretract || curr.type == EMoveType::Pause_Print))
            continue;

        const GCodeProcessorResult::MoveVertex& prev = gcode_result.moves[i - 1];
        const GCodeProcessorResult::MoveVertex* next = nullptr;
        if (i < m_moves_count - 1)
            next = &gcode_result.moves[i + 1];

        ++progress_count;
        if (progress_dialog != nullptr && progress_count % progress_threshold == 0) {
            progress_dialog->Update(int(100.0f * float(m_moves_count + i) / (2.0f * float(m_moves_count))),
                _L("Generating geometry index data") + ": " + wxNumberFormatter::ToString(100.0 * double(i) / double(m_moves_count), 0, wxNumberFormatter::Style_None) + "%");
            progress_dialog->Fit();
            progress_count = 0;
        }

        const unsigned char id = buffer_id(curr.type);
        TBuffer& t_buffer = m_buffers[id];
        MultiIndexBuffer& i_multibuffer = indices[id];
        CurrVertexBuffer& curr_vertex_buffer = curr_vertex_buffers[id];
        VboIndexList& vbo_index_list = vbo_indices[id];

        // ensure there is at least one index buffer
        if (i_multibuffer.empty()) {
#if ENABLE_VECTOR_HYBRID_BACKEND
            i_multibuffer.push_back(IndexBuffer(enable_mem_compress));
#else
            i_multibuffer.push_back(IndexBuffer());
#endif
            if (!t_buffer.vertices.vbos.empty())
                vbo_index_list.push_back(t_buffer.vertices.vbos[curr_vertex_buffer.first]);
        }

        // if adding the indices for the current segment exceeds the threshold size of the current index buffer
        // create another index buffer
        // BBS: get the point number and then judge whether the remaining buffer is enough
        size_t points_num = curr.is_arc_move_with_interpolation_points() ? curr.interpolation_points.size() + 1 : 1;
        size_t indiced_size_to_add = (t_buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::BatchedModel) ? t_buffer.model.data.indices_size_bytes() : points_num * t_buffer.max_indices_per_segment_size_bytes();
        if (i_multibuffer.back().size() * sizeof(IBufferType) >= IBUFFER_THRESHOLD_BYTES - indiced_size_to_add) {

#if ENABLE_VECTOR_HYBRID_BACKEND

            if (!i_multibuffer.empty()) {
                i_multibuffer.back().flush_memory_and_close();
            }
            i_multibuffer.push_back(IndexBuffer(enable_mem_compress));
#else
            i_multibuffer.push_back(IndexBuffer());
#endif
            vbo_index_list.push_back(t_buffer.vertices.vbos[curr_vertex_buffer.first]);
            //if (t_buffer.render_primitive_type != TBuffer::ERenderPrimitiveType::BatchedModel) {
            //    Path& last_path = t_buffer.paths.back();
            //    last_path.add_sub_path(prev, static_cast<unsigned int>(i_multibuffer.size()) - 1, 0, move_id - 1);
            //}
            if (t_buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::Triangle) {
                Path& last_path = t_buffer.paths.back();
                if (prev.type == curr.type && last_path.matches(curr))
                    last_path.add_sub_path(prev, static_cast<unsigned int>(i_multibuffer.size()) - 1, 0, move_id - 1);
            }
        }

        // if adding the vertices for the current segment exceeds the threshold size of the current vertex buffer
        // create another index buffer
        // BBS: support multi points in one MoveVertice, should multiply point number
        size_t vertices_size_to_add = (t_buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::BatchedModel) ? t_buffer.model.data.vertices_size_bytes() : points_num * t_buffer.max_vertices_per_segment_size_bytes();
        if (curr_vertex_buffer.second * t_buffer.vertices.vertex_size_bytes() > t_buffer.vertices.max_size_bytes() - vertices_size_to_add) {
#if ENABLE_VECTOR_HYBRID_BACKEND
            if (!i_multibuffer.empty()) {
                i_multibuffer.back().flush_memory_and_close();
            }
            i_multibuffer.push_back(IndexBuffer(enable_mem_compress));
#else
            i_multibuffer.push_back(IndexBuffer());
#endif

            ++curr_vertex_buffer.first;
            curr_vertex_buffer.second = 0;
            vbo_index_list.push_back(t_buffer.vertices.vbos[curr_vertex_buffer.first]);
            //if (t_buffer.render_primitive_type != TBuffer::ERenderPrimitiveType::BatchedModel) {
            //     Path& last_path = t_buffer.paths.back();
            //    last_path.add_sub_path(prev, static_cast<unsigned int>(i_multibuffer.size()) - 1, 0, move_id - 1);
            //}
            if (t_buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::Triangle) {
                Path& last_path = t_buffer.paths.back();
                if (prev.type == curr.type && last_path.matches(curr))
                    last_path.add_sub_path(prev, static_cast<unsigned int>(i_multibuffer.size()) - 1, 0, move_id - 1);
            }
        }

        IndexBuffer& i_buffer = i_multibuffer.back();

        switch (t_buffer.render_primitive_type)
        {
        case TBuffer::ERenderPrimitiveType::Line: {
            add_indices_as_line(prev, curr, t_buffer, curr_vertex_buffer.second, static_cast<unsigned int>(i_multibuffer.size()) - 1, i_buffer, move_id);
            break;
        }
        case TBuffer::ERenderPrimitiveType::Triangle: {
            add_indices_as_solid(prev, curr, next, t_buffer, curr_vertex_buffer.second, static_cast<unsigned int>(i_multibuffer.size()) - 1, i_buffer, move_id);
            break;
        }
        case TBuffer::ERenderPrimitiveType::BatchedModel: {
            add_indices_as_model_batch(t_buffer.model.data, i_buffer, curr_vertex_buffer.second);
            curr_vertex_buffer.second += t_buffer.model.data.vertices_count();
            break;
        }
        default: { break; }
        }
    }

    for (MultiIndexBuffer& i_multibuffer : indices) {
        for (IndexBuffer& i_buffer : i_multibuffer) {
            i_buffer.shrink_to_fit();
        }
    }

#if ENABLE_VECTOR_HYBRID_BACKEND
    std::vector<unsigned short> tmp_index_buffer;
#endif
    // toolpaths data -> send indices data to gpu
    for (size_t i = 0; i < m_buffers.size(); ++i) {

        if (is_lite_mode && !(i == buffer_id(EMoveType::Extrude) || i == buffer_id(EMoveType::Seam) || i == buffer_id(EMoveType::Unretract) || i == buffer_id(EMoveType::Pause_Print)))
            continue;

        TBuffer& t_buffer = m_buffers[i];
        if (t_buffer.render_primitive_type != TBuffer::ERenderPrimitiveType::InstancedModel) {
            MultiIndexBuffer& i_multibuffer = indices[i];
            for (IndexBuffer& i_buffer : i_multibuffer) {
                const size_t size_elements = i_buffer.size();
                const size_t size_bytes = size_elements * sizeof(IBufferType);

                // stores index buffer informations into TBuffer
                t_buffer.indices.push_back(IBuffer());
                IBuffer& ibuf = t_buffer.indices.back();
                ibuf.count = size_elements;
                ibuf.vbo = vbo_indices[i][t_buffer.indices.size() - 1];

#if ENABLE_GCODE_VIEWER_STATISTICS
                m_statistics.total_indices_gpu_size += static_cast<int64_t>(size_bytes);
                m_statistics.max_ibuffer_gpu_size = std::max(m_statistics.max_ibuffer_gpu_size, static_cast<int64_t>(size_bytes));
                ++m_statistics.ibuffers_count;
#endif // ENABLE_GCODE_VIEWER_STATISTICS

                glsafe(::glGenBuffers(1, &ibuf.ibo));
                glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf.ibo));

#if ENABLE_VECTOR_HYBRID_BACKEND
                if (enable_mem_compress) {
#if 0
                    glsafe(::glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_bytes, nullptr, GL_STATIC_DRAW));

                    Slic3r::ChunkCallback callback = [](const std::vector<unsigned short>& chunk, const int offset,
                                                               const int chunk_size) {
                        glsafe(::glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset * sizeof(unsigned short),
                                                chunk_size * sizeof(unsigned short),
                                          chunk.data()));
                    };
                    bool result = i_buffer.loop_load_chunks(callback);
                    if (result == false) {
						//
					}
#endif
					size_t loaded_size = i_buffer.load_all_data(tmp_index_buffer);
					if (loaded_size == size_elements) {
                        glsafe(::glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_bytes, tmp_index_buffer.data(), GL_STATIC_DRAW));
                    } else {
						// something error happen;
					}
					
				} else {
                    glsafe(::glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_bytes, i_buffer.data(), GL_STATIC_DRAW));
                }
#else
                
				glsafe(::glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_bytes, i_buffer.data(), GL_STATIC_DRAW));

#endif // ENABLE_VECTOR_HYBRID_BACKEND
                
				glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
            }
        }
    }

#if ENABLE_VECTOR_HYBRID_BACKEND
    std::vector<unsigned short>().swap(tmp_index_buffer);
#endif

    if (progress_dialog != nullptr) {
        progress_dialog->Update(100, "");
        progress_dialog->Fit();
    }

#if ENABLE_GCODE_VIEWER_STATISTICS
    for (const TBuffer& buffer : m_buffers) {
        m_statistics.paths_size += SLIC3R_STDVEC_MEMSIZE(buffer.paths, Path);
    }

    auto update_segments_count = [&](EMoveType type, int64_t& count) {
        unsigned int id = buffer_id(type);
        const MultiIndexBuffer& buffers = indices[id];
        int64_t indices_count = 0;
        for (const IndexBuffer& buffer : buffers) {
            indices_count += buffer.size();
        }
        const TBuffer& t_buffer = m_buffers[id];
        if (t_buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::Triangle)
            indices_count -= static_cast<int64_t>(12 * t_buffer.paths.size()); // remove the starting + ending caps = 4 triangles

        count += indices_count / t_buffer.indices_per_segment();
    };

    update_segments_count(EMoveType::Travel, m_statistics.travel_segments_count);
    update_segments_count(EMoveType::Wipe, m_statistics.wipe_segments_count);
    update_segments_count(EMoveType::Extrude, m_statistics.extrude_segments_count);

    m_statistics.load_indices = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - smooth_vertices_time).count();
#endif // ENABLE_GCODE_VIEWER_STATISTICS

    log_memory_usage("Loaded G-code generated indices buffers ", vertices, indices);

	MARKING_DIALOG("Indices_Peak_#4");
    // dismiss indices data, no more needed
    std::vector<MultiIndexBuffer>().swap(indices);

	MARKING_DIALOG("END_#5");

    // layers zs / roles / extruder ids -> extract from result
    size_t last_travel_s_id = 0;
    seams_count = 0;
    std::vector<size_t> move_ids_mapper;
    //std::vector<const GCodeProcessorResult::MoveVertex*> vertices_mapper;
    system_memory_stats("move_ids_mapper.resize ");
    if(m_moves_count > 0)
        move_ids_mapper.resize(m_moves_count, -1);

    for (size_t i = 0; i < m_moves_count; ++i) {
        const GCodeProcessorResult::MoveVertex& move = gcode_result.moves[i];
        if (move.type == EMoveType::Seam)
            ++seams_count;
        //else
            //vertices_mapper.push_back(&move);

        size_t move_id = i - seams_count;
        if(move_ids_mapper[move_id] == -1)
            move_ids_mapper[move_id] = i;
        if (move.type == EMoveType::Extrude || move.type == EMoveType::Extrude_Alter) {
        //if (move.type == EMoveType::Extrude) {
            // layers zs
            const double* const last_z = m_layers.empty() ? nullptr : &m_layers.get_zs().back();
            const double z = static_cast<double>(move.position.z());
            if (last_z == nullptr || z < *last_z - EPSILON || *last_z + EPSILON < z)
                m_layers.append(z, { last_travel_s_id, move_id });
            else
                m_layers.get_endpoints().back().last = move_id;
            // extruder ids
            m_extruder_ids.emplace_back(move.extruder_id);
            // roles
            if (i > 0)
                m_roles.emplace_back(move.extrusion_role);
        }
        else if (move.type == EMoveType::Travel) {
            if (move_id - last_travel_s_id > 1 && !m_layers.empty())
                m_layers.get_endpoints().back().last = move_id;

            last_travel_s_id = move_id;
        }
    }

    // anoob , modify layer endpoint range
    // layer is composite of extrude, push continuous traval/retract/unretract/wipe before extrude to the same layer
    {
        std::vector<Layers::Endpoints>& endpoints = m_layers.get_endpoints();
        for (size_t i = 0; i < endpoints.size(); ++i)
        {
            Layers::Endpoints& endpoint = endpoints[i];
            if(i == 0)
                endpoint.first = 0;

            if(i == endpoints.size() - 1)
                endpoint.last = m_moves_count - 1 - seams_count;

            if(i >= 0 && i < endpoints.size() - 1)
            {
                Layers::Endpoints& next_endpoint = endpoints[i + 1];
                for(size_t j = endpoint.last; j > endpoint.first; --j)
                {
                    if(gcode_result.moves[move_ids_mapper[j]].type == EMoveType::Extrude)
                    {
                        endpoint.last = j;
                        next_endpoint.first = j + 1;
                        break;
                    }
                }
            }
        }
    }


    // roles -> remove duplicates
    sort_remove_duplicates(m_roles);
    m_roles.shrink_to_fit();

    // extruder ids -> remove duplicates
    sort_remove_duplicates(m_extruder_ids);
    m_extruder_ids.shrink_to_fit();

    std::vector<int> plater_extruder;
	for (auto mid : m_extruder_ids){
        int eid = mid;
        plater_extruder.push_back(++eid);
	}
    m_plater_extruder = plater_extruder;

    // replace layers for spiral vase mode
    if (!gcode_result.spiral_vase_layers.empty()) {
        m_layers.reset();
        for (const auto& layer : gcode_result.spiral_vase_layers) {
            m_layers.append(layer.first, { layer.second.first, layer.second.second });
        }
    }

    // set layers z range
    if (!m_layers.empty())
        m_layers_z_range = { 0, static_cast<unsigned int>(m_layers.size() - 1) };

    // change color of paths whose layer contains option points
    if (!options_zs.empty()) {
        TBuffer& extrude_buffer = m_buffers[buffer_id(EMoveType::Extrude)];

#if USE_PARALLEL
        tbb::parallel_for(tbb::blocked_range<size_t>(0, extrude_buffer.paths.size()), [&extrude_buffer, &options_zs](const tbb::blocked_range<size_t>& range) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                Path& path = extrude_buffer.paths[i];
                const float z = path.sub_paths.front().first.position.z();
                if (std::find_if(options_zs.begin(), options_zs.end(), [z](float f) { return f - EPSILON <= z && z <= f + EPSILON; }) !=
                    options_zs.end())
                    path.cp_color_id = 255 - path.cp_color_id;
            }
        });
#else
        for (Path& path : extrude_buffer.paths) {
            const float z = path.sub_paths.front().first.position.z();
            if (std::find_if(options_zs.begin(), options_zs.end(), [z](float f) { return f - EPSILON <= z && z <= f + EPSILON; }) != options_zs.end())
                path.cp_color_id = 255 - path.cp_color_id;
        }
#endif // USE_PARALLEL

    }

	// BBS
#if ENABLE_GCODE_VIEWER_STATISTICS
    m_statistics.load_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
#endif // ENABLE_GCODE_VIEWER_STATISTICS

    if (progress_dialog != nullptr)
        progress_dialog->Destroy();
    
	m_conflict_result = gcode_result.conflict_result;
    if (m_conflict_result) {
        m_conflict_result.value().layer = m_layers.get_l_at(m_conflict_result.value()._height);
    }

	// BBS: add logs
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": finished, m_buffers size %1%!") % m_buffers.size();
//    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " error";
    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " end memory info " << log_memory_info();
    boost::log::core::get()->flush();
    system_memory_stats(__FUNCTION__);

	return !m_layers.empty();
}



void LegacyRenderer::refresh(const GCodeProcessorResult& gcode_result, const std::vector<std::string>& str_tool_colors)
{
    BaseRenderer::refresh(gcode_result, str_tool_colors);
    log_memory_used("Refreshed G-code extrusion paths, ");
}

void LegacyRenderer::refresh_render_paths() 
{
	refresh_render_paths(false, false);
}

void LegacyRenderer::refresh_render_paths(bool keep_sequential_current_first, bool keep_sequential_current_last) const
{
    if (!m_bLoaded) {
        return;
    }

    if (!m_gcode_result) {
        return;
    }

#if ENABLE_AUE_CUSTOM_PREVIEW
    if (m_view_type == EViewType::Custom)
        update_custom_interest_regions();
#endif // ENABLE_AUE_CUSTOM_PREVIEW

#if ENABLE_GCODE_VIEWER_STATISTICS
    auto start_time = std::chrono::high_resolution_clock::now();
#endif // ENABLE_GCODE_VIEWER_STATISTICS

    BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << boost::format(": enter, m_buffers size %1%!") % m_buffers.size();
    auto extrusion_color = [this](const Path& path) {
        ColorRGBA color;
        switch (m_view_type) {
        case EViewType::FeatureType: {
            color = Extrusion_Role_Colors[static_cast<unsigned int>(path.role)];
            break;
        }
        case EViewType::Height: {
            color = m_extrusions.ranges.height.get_color_at(path.height);
            break;
        }
        case EViewType::Width: {
            color = m_extrusions.ranges.width.get_color_at(path.width);
            break;
        }
        case EViewType::Feedrate: {
            color = m_extrusions.ranges.feedrate.get_color_at(path.feedrate);
            break;
        }
        case EViewType::FanSpeed: {
            color = m_extrusions.ranges.fan_speed.get_color_at(path.fan_speed);
            break;
        }
        case EViewType::Temperature: {
            color = m_extrusions.ranges.temperature.get_color_at(path.temperature);
            break;
        }
        case EViewType::LayerTime: {
            color = m_extrusions.ranges.layer_duration.get_color_at(path.layer_time);
            break;
        }
        case EViewType::LayerTimeLog: {
            color = m_extrusions.ranges.layer_duration_log.get_color_at(path.layer_time);
            break;
        }
        case EViewType::VolumetricRate: {
            color = m_extrusions.ranges.volumetric_rate.get_color_at(path.volumetric_rate);
            break;
        }
        case EViewType::Acceleration: {
            color = m_extrusions.ranges.acceleration.get_color_at(path.acceleration);
            break;
        }
        case EViewType::Tool: {
            color = m_tools.m_tool_colors[path.extruder_id];
            break;
        }
        case EViewType::Custom: {
            color = m_tools.m_tool_colors[path.extruder_id];
            break;
        }
        case EViewType::ColorPrint: {
            if (path.cp_color_id >= static_cast<unsigned char>(m_tools.m_tool_colors.size()))
                color = ColorRGBA::GRAY();
            else {
                color = m_tools.m_tool_colors[path.cp_color_id];
                color = adjust_color_for_rendering(color);
            }
            break;
        }
        case EViewType::FilamentId: {
            float id   = float(path.extruder_id) / 256;
            float role = float(path.role) / 256;
            color      = {id, role, id, 1.0f};
            break;
        }
        default: {
            color = ColorRGBA::WHITE();
            break;
        }
        }

        return color;
    };

    auto travel_color = [](const Path& path) {
        return (path.delta_extruder < 0.0f) ? Travel_Colors[2] /* Retract */ :
                                              ((path.delta_extruder > 0.0f) ? Travel_Colors[1] /* Extrude */ : Travel_Colors[0] /* Move */);
    };

    auto is_in_layers_range = [this](const Path& path, size_t min_id, size_t max_id) {
        auto in_layers_range = [this, min_id, max_id](size_t id) {
            return m_layers.get_endpoints_at(min_id).first <= id && id <= m_layers.get_endpoints_at(max_id).last;
        };

        return in_layers_range(path.sub_paths.front().first.s_id) && in_layers_range(path.sub_paths.back().last.s_id);
    };

    // BBS
    auto is_extruder_in_layer_range = [this](const Path& path, size_t extruder_id) { return path.extruder_id == extruder_id; };

    auto is_travel_in_layers_range = [this](size_t path_id, size_t min_id, size_t max_id) {
        const TBuffer& buffer = m_buffers[buffer_id(EMoveType::Travel)];
        if (path_id >= buffer.paths.size())
            return false;

        Path path = buffer.paths[path_id];
        // size_t first = path_id;
        // size_t last = path_id;

        // check adjacent paths
        // while (first > 0 && path.sub_paths.front().first.position.isApprox(buffer.paths[first - 1].sub_paths.back().last.position)) {
        //     --first;
        //     path.sub_paths.front().first = buffer.paths[first].sub_paths.front().first;
        // }
        // while (last < buffer.paths.size() - 1 && path.sub_paths.back().last.position.isApprox(buffer.paths[last +
        // 1].sub_paths.front().first.position)) {
        //     ++last;
        //     path.sub_paths.back().last = buffer.paths[last].sub_paths.back().last;
        // }

        const size_t min_s_id = m_layers.get_endpoints_at(min_id).first;
        const size_t max_s_id = m_layers.get_endpoints_at(max_id).last;

        size_t ffs = path.sub_paths.front().first.s_id;
        size_t bls = path.sub_paths.back().last.s_id;
        return (min_s_id <= ffs && ffs <= max_s_id) || (min_s_id <= bls && bls <= max_s_id);
    };

#if ENABLE_GCODE_VIEWER_STATISTICS
    Statistics* statistics            = const_cast<Statistics*>(&m_statistics);
    statistics->render_paths_size     = 0;
    statistics->models_instances_size = 0;
#endif // ENABLE_GCODE_VIEWER_STATISTICS

    const bool top_layer_only     = true;
    const bool enable_preview_lod = GUI::wxGetApp().app_config->get_bool("enable_preview_lod") && m_gcode_result->should_enable_preview_lod;
    const_cast<bool&>(m_is_lod)   = enable_preview_lod;
    // BBS
    SequentialView::Endpoints global_endpoints    = {m_sequential_view.gcode_ids.size(), 0};
    SequentialView::Endpoints top_layer_endpoints = global_endpoints;
    SequentialView*           sequential_view     = const_cast<SequentialView*>(&m_sequential_view);
    if (top_layer_only || !keep_sequential_current_first)
        sequential_view->current.first = 0;
    // BBS
    if (!keep_sequential_current_last)
        sequential_view->current.last = m_sequential_view.gcode_ids.size();

    bool show_surface = show_gcode_surface();

    // first pass: collect visible paths and update sequential view data
    // Store layer index as signed int to preserve sentinel values (e.g. -1) and avoid narrowing later.
    std::vector<std::tuple<unsigned char, unsigned int, unsigned int, unsigned int, int>> paths;

    // Pre-compute s_id boundaries for the current layer range once.
    // This avoids repeated get_endpoints_at() calls (struct returned by value + bounds check)
    // inside the hot inner loop that may iterate over hundreds of thousands of paths.
    const size_t s_range_min = m_layers.get_endpoints_at(m_layers_z_range[0]).first;
    const size_t s_range_max = m_layers.get_endpoints_at(m_layers_z_range[1]).last;
    const size_t s_top_min   = m_layers.get_endpoints_at(m_layers_z_range[1]).first; // s_top_max == s_range_max

    std::set<int>& special_surface_layer = const_cast<std::set<int>&>(m_top_surface_layer);
    special_surface_layer.clear();

    for (size_t b = 0; b < m_buffers.size(); ++b) {
        TBuffer& buffer = const_cast<TBuffer&>(m_buffers[b]);
        // reset render paths
        buffer.render_paths.clear();

        if (!buffer.visible)
            continue;

        if (buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::InstancedModel ||
            buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::BatchedModel) {
            for (size_t id : buffer.model.instances.s_ids) {
                if (id < s_range_min || s_range_max < id)
                    continue;

                global_endpoints.first = std::min(global_endpoints.first, id);
                global_endpoints.last  = std::max(global_endpoints.last, id);

                if (top_layer_only) {
                    if (id < s_top_min || s_range_max < id)
                        continue;

                    top_layer_endpoints.first = std::min(top_layer_endpoints.first, id);
                    top_layer_endpoints.last  = std::max(top_layer_endpoints.last, id);
                }
            }
        } else {
            int ref_layer_idx = m_layers_z_range[0];

            for (size_t i = 0; i < buffer.paths.size(); ++i) {
                const Path&  path       = buffer.paths[i];
                const size_t path_front = path.sub_paths.front().first.s_id;
                const size_t path_back  = path.sub_paths.back().last.s_id;

                if (path.type == EMoveType::Travel) {
                    // Travel: accept if either endpoint falls within the layer range (path may span layers).
                    if (!((s_range_min <= path_front && path_front <= s_range_max) ||
                          (s_range_min <= path_back && path_back <= s_range_max)))
                        continue;
                } else if (path_front < s_range_min || path_back > s_range_max)
                    continue;

                if (path.type == EMoveType::Extrude && !is_visible(path))
                    continue;

                if (m_tools.m_tool_visibles.size() <= path.extruder_id)
                    continue;

                if (m_view_type == EViewType::ColorPrint && !m_tools.m_tool_visibles[path.extruder_id])
                    continue;

                if (show_surface && role_been_filtered_in_lite_mode(path.role))
                    continue;

                if (enable_preview_lod) {
                    Layers::Endpoints endpoints = m_layers.get_endpoints_at(ref_layer_idx);

                    int layer_idx = -1; // get path layer index
                    if (endpoints.first <= path_front && path_back <= endpoints.last) {
                        layer_idx = ref_layer_idx;
                    } else {
                        layer_idx     = get_layer_index(path);
                        ref_layer_idx = layer_idx;
                    }

                    /*if (layer_idx > -1 && stride > 0 && should_be_filtered_of_layer(stride, layer_idx))
                        continue;*/

                    if (path.role == ExtrusionRole::erTopSolidInfill || path.role == ExtrusionRole::erBrim) {
                        special_surface_layer.insert(ref_layer_idx);
                    }
                }

                // store valid path
                for (size_t j = 0; j < path.sub_paths.size(); ++j) {
                    paths.emplace_back(static_cast<unsigned char>(b), path.sub_paths[j].first.b_id, static_cast<unsigned int>(i),
                                       static_cast<unsigned int>(j), ref_layer_idx);
                }

                global_endpoints.first = std::min(global_endpoints.first, path_front);
                global_endpoints.last  = std::max(global_endpoints.last, path_back);

                if (top_layer_only) {
                    if (path.type == EMoveType::Travel) {
                        // Travel top: at least one endpoint in the top layer range.
                        if ((s_top_min <= path_front && path_front <= s_range_max) || (s_top_min <= path_back && path_back <= s_range_max)) {
                            top_layer_endpoints.first = std::min(top_layer_endpoints.first, path_front);
                            top_layer_endpoints.last  = std::max(top_layer_endpoints.last, path_back);
                        }
                    } else if (path_front >= s_top_min && path_back <= s_range_max) {
                        top_layer_endpoints.first = std::min(top_layer_endpoints.first, path_front);
                        top_layer_endpoints.last  = std::max(top_layer_endpoints.last, path_back);
                    }
                }
            }
        }
    }

    for (const int& x : m_gcode_result->wipe_tower_tool_changes_layers) {
        special_surface_layer.insert(x);
    }

    // update current sequential position
    sequential_view->current.first = !top_layer_only && keep_sequential_current_first ?
                                         std::clamp(sequential_view->current.first, global_endpoints.first, global_endpoints.last) :
                                         global_endpoints.first;
    if (global_endpoints.last == 0) {
        m_no_render_path              = true;
        sequential_view->current.last = global_endpoints.last;
    } else {
        m_no_render_path              = false;
        sequential_view->current.last = keep_sequential_current_last ?
                                            std::clamp(sequential_view->current.last, global_endpoints.first, global_endpoints.last) :
                                            global_endpoints.last;
    }

    // get the world position from the vertex buffer
    bool found = false;
    for (const TBuffer& buffer : m_buffers) {
        if (buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::InstancedModel ||
            buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::BatchedModel) {
            for (size_t i = 0; i < buffer.model.instances.s_ids.size(); ++i) {
                if (buffer.model.instances.s_ids[i] == m_sequential_view.current.last) {
                    size_t offset                         = i * buffer.model.instances.instance_size_floats();
                    sequential_view->current_position.x() = buffer.model.instances.buffer[offset + 0];
                    sequential_view->current_position.y() = buffer.model.instances.buffer[offset + 1];
                    sequential_view->current_position.z() = buffer.model.instances.buffer[offset + 2];
                    sequential_view->current_offset       = buffer.model.instances.offsets[i];
                    found                                 = true;
                    break;
                }
            }
        } else {
            // searches the path containing the current position
            for (const Path& path : buffer.paths) {
                if (path.contains(m_sequential_view.current.last)) {
                    const int sub_path_id = path.get_id_of_sub_path_containing(m_sequential_view.current.last);
                    if (sub_path_id != -1) {
#if 0
                        const Path::Sub_Path& sub_path = path.sub_paths[sub_path_id];
                        unsigned int offset = static_cast<unsigned int>(m_sequential_view.current.last - sub_path.first.s_id);
                        if (offset > 0) {
                            if (buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::Line) {
                                for (size_t i = sub_path.first.s_id + 1; i < m_sequential_view.current.last + 1; i++) {
                                    size_t move_id = m_ssid_to_moveid_map[i];
                                    const GCodeProcessorResult::MoveVertex& curr = m_gcode_result->moves[move_id];
                                    if (curr.is_arc_move()) {
                                        offset += curr.interpolation_points.size();
                                    }
                                }
                                offset = 2 * offset - 1;
                            }
                            else if (buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::Triangle) {
                                unsigned int indices_count = buffer.indices_per_segment();
                                // BBS: modify to support moves which has internal point
                                for (size_t i = sub_path.first.s_id + 1; i < m_sequential_view.current.last + 1; i++) {
                                    size_t move_id = m_ssid_to_moveid_map[i];
                                    const GCodeProcessorResult::MoveVertex& curr = m_gcode_result->moves[move_id];
                                    if (curr.is_arc_move()) {
                                        offset += curr.interpolation_points.size();
                                    }
                                }
                                offset = indices_count * (offset - 1) + (indices_count - 2);
                                if (sub_path_id == 0)
                                    offset += 6; // add 2 triangles for starting cap
                            }
                        }
                        offset += static_cast<unsigned int>(sub_path.first.i_id);

                        // gets the vertex index from the index buffer on gpu
                        const IBuffer& i_buffer = buffer.indices[sub_path.first.b_id];
                        unsigned int index = 0;
                        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i_buffer.ibo));
                        glsafe(::glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLintptr>(offset * sizeof(IBufferType)), static_cast<GLsizeiptr>(sizeof(IBufferType)), static_cast<void*>(&index)));
                        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

                        // gets the position from the vertices buffer on gpu
                        glsafe(::glBindBuffer(GL_ARRAY_BUFFER, i_buffer.vbo));
                        glsafe(::glGetBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(index * buffer.vertices.vertex_size_bytes()), static_cast<GLsizeiptr>(3 * sizeof(float)), static_cast<void*>(sequential_view->current_position.data())));
                        glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));

#else
                        size_t move_id                    = m_ssid_to_moveid_map[m_sequential_view.current.last];
                        Vec3f  position                   = m_gcode_result->moves[move_id].position;
                        sequential_view->current_position = position;
#endif
                        sequential_view->current_offset = Vec3f::Zero();
                        found                           = true;
                        break;
                    }
                }
            }
        }

        if (found)
            break;
    }

    // second pass: filter paths by sequential data and collect them by color
    RenderPath* render_path = nullptr;
    for (const auto& [tbuffer_id, ibuffer_id, path_id, sub_path_id, layer_idx] : paths) {
        TBuffer&              buffer   = const_cast<TBuffer&>(m_buffers[tbuffer_id]);
        const Path&           path     = buffer.paths[path_id];
        const Path::Sub_Path& sub_path = path.sub_paths[sub_path_id];
        if (m_sequential_view.current.last < sub_path.first.s_id || sub_path.last.s_id < m_sequential_view.current.first)
            continue;

        // Creality:for appearance shortage
#if ENABLE_AUE_CUSTOM_PREVIEW
        if (path.type == EMoveType::Extrude && m_view_type == EViewType::Custom &&
            buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::Triangle) {
            const bool can_highlight = (!top_layer_only || m_sequential_view.current.last == global_endpoints.last ||
                                        is_in_layers_range(path, m_layers_z_range[1], m_layers_z_range[1]));
            // Keep the same per-layer filtering semantics as the original preview:
            // when other layers are neutralized, the ROI highlight must not "leak" through.
            if (can_highlight) {
                const ColorRGBA  base_color       = extrusion_color(path);
                const ColorRGBA  filament_color   = (path.extruder_id < m_tools.m_tool_colors.size()) ?
                                                        m_tools.m_tool_colors[path.extruder_id] :
                                                        ColorRGBA::GRAY();
                const auto       highlight_colors = pick_interest_region_highlight_colors(filament_color);
                const ColorRGBA& trigger_color    = highlight_colors.first;
                const ColorRGBA& defect_color     = highlight_colors.second;

                const size_t draw_min = std::max(m_sequential_view.current.first, sub_path.first.s_id);
                const size_t draw_max = std::min(m_sequential_view.current.last, sub_path.last.s_id);
                if (draw_max <= draw_min)
                    continue;

                auto interest_tag = [this](size_t end_ssid) -> unsigned char {
                    return (end_ssid < m_custom_interest_by_ssid.size()) ? m_custom_interest_by_ssid[end_ssid] : 0;
                };

                auto color_for_tag = [&](unsigned char tag) -> ColorRGBA {
                    // InterestRegion::SegmentTag::Trigger == 1, Defect == 2.
                    if (tag == 1)
                        return trigger_color;
                    if (tag == 2)
                        return defect_color;
                    return base_color;
                };

                auto append_chunk = [&](size_t chunk_min_s_id, size_t chunk_max_s_id, const ColorRGBA& chunk_color,
                                        bool skip_start_corner_cap) {
                    RenderPath key{tbuffer_id, chunk_color, static_cast<unsigned int>(ibuffer_id), path_id, layer_idx};
                    if (enable_preview_lod) {
                        if (render_path == nullptr || !RenderPathPropertyEqual_WithLayerNo()(*render_path, key)) {
                            buffer.render_paths.emplace_back(key);
                            render_path = const_cast<RenderPath*>(&buffer.render_paths.back());
                        }
                    } else {
                        if (render_path == nullptr || !RenderPathPropertyEqual()(*render_path, key)) {
                            buffer.render_paths.emplace_back(key);
                            render_path = const_cast<RenderPath*>(&buffer.render_paths.back());
                        }
                    }

                    unsigned int size_in_indices = 0;
                    // BBS: modify to support moves which has internal point
                    const size_t max_s_id       = chunk_max_s_id;
                    const size_t min_s_id       = chunk_min_s_id;
                    unsigned int segments_count = static_cast<unsigned int>(max_s_id - min_s_id);
#if 0
                    for (size_t i = min_s_id + 1; i < max_s_id + 1; ++i) {
                        const size_t move_id = m_ssid_to_moveid_map[i];
                        const GCodeProcessorResult::MoveVertex& curr = m_gcode_result->moves[move_id];
                        if (curr.is_arc_move())
                            segments_count += static_cast<unsigned int>(curr.interpolation_points.size());
                    }
#else
                    // O(1) prefix-sum query replaces the former O(N) inner loop over arc moves
                    if (max_s_id < m_ssid_arc_extra_segments.size() && min_s_id < m_ssid_arc_extra_segments.size())
                        segments_count += m_ssid_arc_extra_segments[max_s_id] - m_ssid_arc_extra_segments[min_s_id];
#endif
                    size_in_indices = buffer.indices_per_segment() * segments_count;

                    if (size_in_indices == 0)
                        return;

                    if (sub_path_id == 0 && chunk_min_s_id == sub_path.first.s_id)
                        size_in_indices += 6; // add 2 triangles for starting cap
                    if (sub_path_id == path.sub_paths.size() - 1 && chunk_max_s_id == path.sub_paths.back().last.s_id &&
                        path.sub_paths.back().last.s_id <= m_sequential_view.current.last)
                        size_in_indices += 6; // add 2 triangles for ending cap
                    if (skip_start_corner_cap && chunk_min_s_id > sub_path.first.s_id)
                        size_in_indices -= 6; // remove 2 triangles for corner cap

                    render_path->sizes.push_back(size_in_indices);

                    unsigned int delta_1st = static_cast<unsigned int>(chunk_min_s_id - sub_path.first.s_id);
                    delta_1st *= buffer.indices_per_segment();
                    if (delta_1st > 0) {
                        if (sub_path_id == 0)
                            delta_1st += 6; // skip 2 triangles for starting cap
                        if (skip_start_corner_cap)
                            delta_1st += 6; // skip 2 triangles for corner cap
                    }

                    render_path->offsets.push_back(static_cast<size_t>((sub_path.first.i_id + delta_1st) * sizeof(IBufferType)));
                };

                const bool    draw_is_trimmed_start = (draw_min > sub_path.first.s_id);
                size_t        block_first_end       = draw_min + 1;
                unsigned char block_tag             = interest_tag(block_first_end);
                for (size_t end_ssid = draw_min + 1; end_ssid <= draw_max; ++end_ssid) {
                    const unsigned char tag = interest_tag(end_ssid);
                    if (tag != block_tag) {
                        const size_t chunk_min   = block_first_end - 1;
                        const size_t chunk_max   = end_ssid - 1;
                        const bool   skip_corner = draw_is_trimmed_start && (chunk_min == draw_min);
                        append_chunk(chunk_min, chunk_max, color_for_tag(block_tag), skip_corner);
                        block_first_end = end_ssid;
                        block_tag       = tag;
                    }
                }

                {
                    const size_t chunk_min   = block_first_end - 1;
                    const size_t chunk_max   = draw_max;
                    const bool   skip_corner = draw_is_trimmed_start && (chunk_min == draw_min);
                    append_chunk(chunk_min, chunk_max, color_for_tag(block_tag), skip_corner);
                }

                continue;
            }
        }
#endif // ENABLE_AUE_CUSTOM_PREVIEW

        ColorRGBA color;
        switch (path.type) {
        case EMoveType::Tool_change:
        case EMoveType::Color_change:
        case EMoveType::Pause_Print:
        case EMoveType::Custom_GCode:
        case EMoveType::Retract:
        case EMoveType::Unretract:
        case EMoveType::Seam: {
            color = option_color(path.type);
            break;
        }
        case EMoveType::Extrude: {
            if (!top_layer_only || m_sequential_view.current.last == global_endpoints.last ||
                is_in_layers_range(path, m_layers_z_range[1], m_layers_z_range[1]))
                color = extrusion_color(path);
            else
                color = Neutral_Color;

            break;
        }
        case EMoveType::Travel: {
            if (!top_layer_only || m_sequential_view.current.last == global_endpoints.last ||
                is_travel_in_layers_range(path_id, m_layers_z_range[1], m_layers_z_range[1]))
#if ENABLE_AUE_CUSTOM_PREVIEW
                color = (m_view_type == EViewType::Feedrate || m_view_type == EViewType::Tool || m_view_type == EViewType::Custom) ?
                            extrusion_color(path) :
                            travel_color(path);
#else
                color = (m_view_type == EViewType::Feedrate || m_view_type == EViewType::Tool) ? extrusion_color(path) : travel_color(path);
#endif // ENABLE_AUE_CUSTOM_PREVIEW
            else
                color = Neutral_Color;

            break;
        }
        case EMoveType::Wipe: {
            color = Wipe_Color;
            break;
        }
        default: {
            color = {0.0f, 0.0f, 0.0f, 1.0f};
            break;
        }
        }

        RenderPath key{tbuffer_id, color, static_cast<unsigned int>(ibuffer_id), path_id, layer_idx};
        if (enable_preview_lod) {
            if (render_path == nullptr || !RenderPathPropertyEqual_WithLayerNo()(*render_path, key)) {
                buffer.render_paths.emplace_back(key);
                render_path = const_cast<RenderPath*>(&buffer.render_paths.back());
            }
        } else {
            if (render_path == nullptr || !RenderPathPropertyEqual()(*render_path, key)) {
                buffer.render_paths.emplace_back(key);
                render_path = const_cast<RenderPath*>(&buffer.render_paths.back());
            }
        }

        unsigned int delta_1st = 0;
        if (sub_path.first.s_id < m_sequential_view.current.first && m_sequential_view.current.first <= sub_path.last.s_id)
            delta_1st = static_cast<unsigned int>(m_sequential_view.current.first - sub_path.first.s_id);

        unsigned int size_in_indices = 0;
        switch (buffer.render_primitive_type) {
        case TBuffer::ERenderPrimitiveType::Line:
        case TBuffer::ERenderPrimitiveType::Triangle: {
            // BBS: modify to support moves which has internal point
            size_t       max_s_id       = std::min(m_sequential_view.current.last, sub_path.last.s_id);
            size_t       min_s_id       = std::max(m_sequential_view.current.first, sub_path.first.s_id);
            unsigned int segments_count = max_s_id - min_s_id;
#if 0
            for (size_t i = min_s_id + 1; i < max_s_id + 1; i++) {
                size_t move_id = m_ssid_to_moveid_map[i];
                const GCodeProcessorResult::MoveVertex& curr = m_gcode_result->moves[move_id];
                if (curr.is_arc_move()) {
                    segments_count += curr.interpolation_points.size();
                }
            }
#else
            // O(1) prefix-sum query replaces the former O(N) inner loop over arc moves
            if (max_s_id < m_ssid_arc_extra_segments.size() && min_s_id < m_ssid_arc_extra_segments.size())
                segments_count += m_ssid_arc_extra_segments[max_s_id] - m_ssid_arc_extra_segments[min_s_id];
#endif
            size_in_indices = buffer.indices_per_segment() * segments_count;
            break;
        }
        default: {
            break;
        }
        }

        if (size_in_indices == 0)
            continue;

        if (buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::Triangle) {
            if (sub_path_id == 0 && delta_1st == 0)
                size_in_indices += 6; // add 2 triangles for starting cap
            if (sub_path_id == path.sub_paths.size() - 1 && path.sub_paths.back().last.s_id <= m_sequential_view.current.last)
                size_in_indices += 6; // add 2 triangles for ending cap
            if (delta_1st > 0)
                size_in_indices -= 6; // remove 2 triangles for corner cap
        }

        render_path->sizes.push_back(size_in_indices);

        if (buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::Triangle) {
            delta_1st *= buffer.indices_per_segment();
            if (delta_1st > 0) {
                delta_1st += 6; // skip 2 triangles for corner cap
                if (sub_path_id == 0)
                    delta_1st += 6; // skip 2 triangles for starting cap
            }
        }

        render_path->offsets.push_back(static_cast<size_t>((sub_path.first.i_id + delta_1st) * sizeof(IBufferType)));

#if 0
        // check sizes and offsets against index buffer size on gpu
        GLint buffer_size;
        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->indices[render_path->ibuffer_id].ibo));
        glsafe(::glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size));
        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        if (render_path->offsets.back() + render_path->sizes.back() * sizeof(IBufferType) > buffer_size)
            BOOST_LOG_TRIVIAL(error) << "refresh_render_paths: Invalid render path data";
#endif
    }

    // Removes empty render paths and sort.
    for (size_t b = 0; b < m_buffers.size(); ++b) {
        TBuffer* buffer = const_cast<TBuffer*>(&m_buffers[b]);
        buffer->render_paths.erase(std::remove_if(buffer->render_paths.begin(), buffer->render_paths.end(),
                                                  [](const auto& path) { return path.sizes.empty() || path.offsets.empty(); }),
                                   buffer->render_paths.end());
    }

    // second pass: for buffers using instanced and batched models, update the instances render ranges
    for (size_t b = 0; b < m_buffers.size(); ++b) {
        TBuffer& buffer = const_cast<TBuffer&>(m_buffers[b]);
        if (buffer.render_primitive_type != TBuffer::ERenderPrimitiveType::InstancedModel &&
            buffer.render_primitive_type != TBuffer::ERenderPrimitiveType::BatchedModel)
            continue;

        buffer.model.instances.render_ranges.reset();

        if (!buffer.visible || buffer.model.instances.s_ids.empty())
            continue;

        buffer.model.instances.render_ranges.ranges.push_back({0, 0, 0, buffer.model.color});
        bool has_second_range = top_layer_only && m_sequential_view.current.last != m_sequential_view.global.last;
        if (has_second_range)
            buffer.model.instances.render_ranges.ranges.push_back({0, 0, 0, Neutral_Color});

        if (m_sequential_view.current.first <= buffer.model.instances.s_ids.back() &&
            buffer.model.instances.s_ids.front() <= m_sequential_view.current.last) {
            for (size_t id : buffer.model.instances.s_ids) {
                if (has_second_range) {
                    if (id < m_sequential_view.endpoints.first) {
                        ++buffer.model.instances.render_ranges.ranges.front().offset;
                        if (id <= m_sequential_view.current.first)
                            ++buffer.model.instances.render_ranges.ranges.back().offset;
                        else
                            ++buffer.model.instances.render_ranges.ranges.back().count;
                    } else if (id <= m_sequential_view.current.last)
                        ++buffer.model.instances.render_ranges.ranges.front().count;
                    else
                        break;
                } else {
                    if (id <= m_sequential_view.current.first)
                        ++buffer.model.instances.render_ranges.ranges.front().offset;
                    else if (id <= m_sequential_view.current.last)
                        ++buffer.model.instances.render_ranges.ranges.front().count;
                    else
                        break;
                }
            }
        }
    }

    // set sequential data to their final value
    sequential_view->endpoints     = top_layer_only ? top_layer_endpoints : global_endpoints;
    sequential_view->current.first = !top_layer_only && keep_sequential_current_first ?
                                         std::clamp(sequential_view->current.first, sequential_view->endpoints.first,
                                                    sequential_view->endpoints.last) :
                                         sequential_view->endpoints.first;
    sequential_view->global        = global_endpoints;

    // updates sequential range caps
    std::array<SequentialRangeCap, 2>* sequential_range_caps = const_cast<std::array<SequentialRangeCap, 2>*>(&m_sequential_range_caps);
    (*sequential_range_caps)[0].reset();
    (*sequential_range_caps)[1].reset();

    if (m_sequential_view.current.first != m_sequential_view.current.last) {
        for (const auto& [tbuffer_id, ibuffer_id, path_id, sub_path_id, layer_idx] : paths) {
            TBuffer& buffer = const_cast<TBuffer&>(m_buffers[tbuffer_id]);
            if (buffer.render_primitive_type != TBuffer::ERenderPrimitiveType::Triangle)
                continue;

            const Path&           path     = buffer.paths[path_id];
            const Path::Sub_Path& sub_path = path.sub_paths[sub_path_id];
            if (m_sequential_view.current.last <= sub_path.first.s_id || sub_path.last.s_id <= m_sequential_view.current.first)
                continue;

            // update cap for first endpoint of current range
            if (m_sequential_view.current.first > sub_path.first.s_id) {
                SequentialRangeCap& cap      = (*sequential_range_caps)[0];
                const IBuffer&      i_buffer = buffer.indices[ibuffer_id];
                cap.buffer                   = &buffer;
                cap.vbo                      = i_buffer.vbo;

                // calculate offset into the index buffer
                unsigned int offset = sub_path.first.i_id;
                offset += 6; // add 2 triangles for corner cap
                offset += static_cast<unsigned int>(m_sequential_view.current.first - sub_path.first.s_id) * buffer.indices_per_segment();
                if (sub_path_id == 0)
                    offset += 6; // add 2 triangles for starting cap

                // extract indices from index buffer
                std::array<IBufferType, 6> indices{0, 0, 0, 0, 0, 0};
                glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i_buffer.ibo));
                glsafe(::glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLintptr>((offset + 0) * sizeof(IBufferType)),
                                            static_cast<GLsizeiptr>(sizeof(IBufferType)), static_cast<void*>(&indices[0])));
                glsafe(::glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLintptr>((offset + 7) * sizeof(IBufferType)),
                                            static_cast<GLsizeiptr>(sizeof(IBufferType)), static_cast<void*>(&indices[1])));
                glsafe(::glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLintptr>((offset + 1) * sizeof(IBufferType)),
                                            static_cast<GLsizeiptr>(sizeof(IBufferType)), static_cast<void*>(&indices[2])));
                glsafe(::glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLintptr>((offset + 13) * sizeof(IBufferType)),
                                            static_cast<GLsizeiptr>(sizeof(IBufferType)), static_cast<void*>(&indices[4])));
                glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
                indices[3] = indices[0];
                indices[5] = indices[1];

                // send indices to gpu
                glsafe(::glGenBuffers(1, &cap.ibo));
                glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cap.ibo));
                glsafe(::glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(IBufferType), indices.data(), GL_STATIC_DRAW));
                glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

                // extract color from render path
                size_t offset_bytes = offset * sizeof(IBufferType);
                for (const RenderPath& render_path : buffer.render_paths) {
                    if (render_path.ibuffer_id == ibuffer_id) {
                        for (size_t j = 0; j < render_path.offsets.size(); ++j) {
                            if (render_path.contains(offset_bytes)) {
                                cap.color = render_path.color;
                                break;
                            }
                        }
                    }
                }
            }

            // update cap for last endpoint of current range
            if (m_sequential_view.current.last < sub_path.last.s_id) {
                SequentialRangeCap& cap      = (*sequential_range_caps)[1];
                const IBuffer&      i_buffer = buffer.indices[ibuffer_id];
                cap.buffer                   = &buffer;
                cap.vbo                      = i_buffer.vbo;

                // calculate offset into the index buffer
                unsigned int offset = sub_path.first.i_id;
                offset += 6; // add 2 triangles for corner cap
                offset += static_cast<unsigned int>(m_sequential_view.current.last - 1 - sub_path.first.s_id) *
                          buffer.indices_per_segment();
                if (sub_path_id == 0)
                    offset += 6; // add 2 triangles for starting cap

                // extract indices from index buffer
                std::array<IBufferType, 6> indices{0, 0, 0, 0, 0, 0};
                glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i_buffer.ibo));
                glsafe(::glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLintptr>((offset + 2) * sizeof(IBufferType)),
                                            static_cast<GLsizeiptr>(sizeof(IBufferType)), static_cast<void*>(&indices[0])));
                glsafe(::glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLintptr>((offset + 4) * sizeof(IBufferType)),
                                            static_cast<GLsizeiptr>(sizeof(IBufferType)), static_cast<void*>(&indices[1])));
                glsafe(::glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLintptr>((offset + 10) * sizeof(IBufferType)),
                                            static_cast<GLsizeiptr>(sizeof(IBufferType)), static_cast<void*>(&indices[2])));
                glsafe(::glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLintptr>((offset + 16) * sizeof(IBufferType)),
                                            static_cast<GLsizeiptr>(sizeof(IBufferType)), static_cast<void*>(&indices[5])));
                glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
                indices[3] = indices[0];
                indices[4] = indices[2];

                // send indices to gpu
                glsafe(::glGenBuffers(1, &cap.ibo));
                glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cap.ibo));
                glsafe(::glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(IBufferType), indices.data(), GL_STATIC_DRAW));
                glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

                // extract color from render path
                size_t offset_bytes = offset * sizeof(IBufferType);
                for (const RenderPath& render_path : buffer.render_paths) {
                    if (render_path.ibuffer_id == ibuffer_id) {
                        for (size_t j = 0; j < render_path.offsets.size(); ++j) {
                            if (render_path.contains(offset_bytes)) {
                                cap.color = render_path.color;
                                break;
                            }
                        }
                    }
                }
            }

            if ((*sequential_range_caps)[0].is_renderable() && (*sequential_range_caps)[1].is_renderable())
                break;
        }
    }

    const_cast<FilterLayerResult&>(m_filter_layer_result).reset();

    // BBS
    enable_moves_slider(!paths.empty());

#if ENABLE_GCODE_VIEWER_STATISTICS
    for (const TBuffer& buffer : m_buffers) {
        statistics->render_paths_size += SLIC3R_STDUNORDEREDSET_MEMSIZE(buffer.render_paths, RenderPath);
        for (const RenderPath& path : buffer.render_paths) {
            statistics->render_paths_size += SLIC3R_STDVEC_MEMSIZE(path.sizes, unsigned int);
            statistics->render_paths_size += SLIC3R_STDVEC_MEMSIZE(path.offsets, size_t);
        }
        statistics->models_instances_size += SLIC3R_STDVEC_MEMSIZE(buffer.model.instances.buffer, float);
        statistics->models_instances_size += SLIC3R_STDVEC_MEMSIZE(buffer.model.instances.s_ids, size_t);
        statistics->models_instances_size += SLIC3R_STDVEC_MEMSIZE(buffer.model.instances.render_ranges.ranges,
                                                                   InstanceVBuffer::Ranges::Range);
    }
    statistics->refresh_paths_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
#endif // ENABLE_GCODE_VIEWER_STATISTICS
}


void LegacyRenderer::reset()
{
    // BBS: should also reset the result id
    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": current result id %1% ") % m_last_result_id;
    
#if ENABLE_GCODE_VIEWER_STATISTICS
    m_statistics.reset_all();
#endif // ENABLE_GCODE_VIEWER_STATISTICS

	m_moves_count = 0;
    m_ssid_arc_extra_segments.clear();
    m_ssid_arc_extra_segments.shrink_to_fit();
    m_sid_to_moveid.clear();
    m_sid_to_moveid.shrink_to_fit();
    for (TBuffer& buffer : m_buffers) {
        buffer.reset();
    }
    m_layers.reset();

    m_filter_layer_result.reset();

	BaseRenderer::reset();
}

void LegacyRenderer::render(int canvas_width, int canvas_height)
{
#if ENABLE_GCODE_VIEWER_STATISTICS
    m_statistics.reset_opengl();
    m_statistics.total_instances_gpu_size = 0;
#endif // ENABLE_GCODE_VIEWER_STATISTICS

    // test helper
    bool is_capture_mode = false;
    if (Test::enable_test) {
        std::string mode = Test::Visitor().call_cmd("is_capture_mode", "{}");
        if (!mode.empty())
            is_capture_mode = nlohmann::json::parse(mode)["enable"].get<int>() != 0;
    }

    glsafe(::glEnable(GL_DEPTH_TEST));

    bool empty_roles = m_roles.empty();
    bool ban_shells_render = false;
    if (wxGetApp().preset_bundle->machine_is_belt() && empty_roles) // 路径没渲染完成时，cr30禁止显示半透明轮廓
        ban_shells_render = true;

    if (!ban_shells_render)
    {
        if (!m_roles.empty() && m_layers_slider && m_layers_slider->is_higher_at_max() && m_layers_slider->is_lower_at_min()) {
        } else {
            render_shells(canvas_width, canvas_height);
        }
    }

    if (empty_roles)
        return;

    if ((!m_no_render_path) && (!is_capture_mode)) {
        render_marker_sequential_view(canvas_width, canvas_height);
    }
    
    if (m_bLoaded)
        render_toolpaths();
    
    if (!is_capture_mode && m_legend_enabled) {
        render_legend(canvas_width, canvas_height);
	}

#if ENABLE_GCODE_VIEWER_STATISTICS
    render_statistics();
#endif // ENABLE_GCODE_VIEWER_STATISTICS

    //BBS render slider
    if (!is_capture_mode) {
        render_slider(canvas_width, canvas_height);
    }

#if ENABLE_FILTERED_STRIDE_DIALOG

    ImGui::SetNextWindowPos(ImVec2(canvas_width - 50, 100), ImGuiCond_Always, ImVec2(1, 0));
    ImGuiWrapper& imgui = *wxGetApp().imgui();
    imgui.begin(std::string("Adjust Stride factor"),
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    imgui.text("Stride factor:");
    ImGui::SameLine();
    // ImGui::SetNextItemWidth(100.0f);
    bool toogle = ImGui::SliderFloat("##Stride slider", &m_stride_factor, 1.0f, 20.0f, "%.3f", ImGuiSliderFlags_None);
    ImGui::Text("Dynamic Stride:%d", get_dynamic_stride(estimate_pixels_of_one_layer()));
    imgui.end();

#endif //ENABLE_FILTERED_STRIDE_DIALOG
}

void LegacyRenderer::render_toolpaths()
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

bool LegacyRenderer::can_export_toolpaths() const
{
    return has_data() && m_buffers[buffer_id(EMoveType::Extrude)].render_primitive_type == TBuffer::ERenderPrimitiveType::Triangle;
}

void LegacyRenderer::export_toolpaths_to_obj(const char* filename) const

{
#if 0
    if (filename == nullptr)
        return;

    if (!has_data())
        return;

    wxBusyCursor busy;

    // the data needed is contained into the Extrude TBuffer
    const TBuffer& t_buffer = m_buffers[buffer_id(EMoveType::Extrude)];
    if (!t_buffer.has_data())
        return;

    if (t_buffer.render_primitive_type != TBuffer::ERenderPrimitiveType::Triangle)
        return;

    // collect color information to generate materials
    std::vector<ColorRGBA> colors;
    for (const RenderPath& path : t_buffer.render_paths) {
        colors.push_back(path.color);
    }
    sort_remove_duplicates(colors);

    // save materials file
    boost::filesystem::path mat_filename(filename);
    mat_filename.replace_extension("mtl");

    CNumericLocalesSetter locales_setter;

    FILE* fp = boost::nowide::fopen(mat_filename.string().c_str(), "w");
    if (fp == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "BaseRenderer::export_toolpaths_to_obj: Couldn't open " << mat_filename.string().c_str() << " for writing";
        return;
    }

    fprintf(fp, "# G-Code Toolpaths Materials\n");
    fprintf(fp, "# Generated by %s-%s based on Slic3r\n", SLIC3R_APP_NAME, CREALITYPRINT_VERSION);

    unsigned int colors_count = 1;
    for (const ColorRGBA& color : colors) {
        fprintf(fp, "\nnewmtl material_%d\n", colors_count++);
        fprintf(fp, "Ka 1 1 1\n");
        fprintf(fp, "Kd %g %g %g\n", color.r(), color.g(), color.b());
        fprintf(fp, "Ks 0 0 0\n");
    }

    fclose(fp);

    // save geometry file
    fp = boost::nowide::fopen(filename, "w");
    if (fp == nullptr) {
        BOOST_LOG_TRIVIAL(error) << "BaseRenderer::export_toolpaths_to_obj: Couldn't open " << filename << " for writing";
        return;
    }

    fprintf(fp, "# G-Code Toolpaths\n");
    fprintf(fp, "# Generated by %s-%s based on Slic3r\n", SLIC3R_APP_NAME, CREALITYPRINT_VERSION);
    fprintf(fp, "\nmtllib ./%s\n", mat_filename.filename().string().c_str());

    const size_t floats_per_vertex = t_buffer.vertices.vertex_size_floats();

    std::vector<Vec3f> out_vertices;
    std::vector<Vec3f> out_normals;

    struct VerticesOffset
    {
        unsigned int vbo;
        size_t offset;
    };
    std::vector<VerticesOffset> vertices_offsets;
    vertices_offsets.push_back({ t_buffer.vertices.vbos.front(), 0 });

    // get vertices/normals data from vertex buffers on gpu
    for (size_t i = 0; i < t_buffer.vertices.vbos.size(); ++i) {
        const size_t floats_count = t_buffer.vertices.sizes[i] / sizeof(float);
        VertexBuffer vertices(floats_count);
        glsafe(::glBindBuffer(GL_ARRAY_BUFFER, t_buffer.vertices.vbos[i]));
        glsafe(::glGetBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(t_buffer.vertices.sizes[i]), static_cast<void*>(vertices.data())));
        glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));
        const size_t vertices_count = floats_count / floats_per_vertex;
        for (size_t j = 0; j < vertices_count; ++j) {
            const size_t base = j * floats_per_vertex;
            out_vertices.push_back({ vertices[base + 0], vertices[base + 1], vertices[base + 2] });
            out_normals.push_back({ vertices[base + 3], vertices[base + 4], vertices[base + 5] });
        }

        if (i < t_buffer.vertices.vbos.size() - 1)
            vertices_offsets.push_back({ t_buffer.vertices.vbos[i + 1], vertices_offsets.back().offset + vertices_count });
    }

    // save vertices to file
    fprintf(fp, "\n# vertices\n");
    for (const Vec3f& v : out_vertices) {
        fprintf(fp, "v %g %g %g\n", v.x(), v.y(), v.z());
    }

    // save normals to file
    fprintf(fp, "\n# normals\n");
    for (const Vec3f& n : out_normals) {
        fprintf(fp, "vn %g %g %g\n", n.x(), n.y(), n.z());
    }

    size_t i = 0;
    for (const ColorRGBA& color : colors) {
        // save material triangles to file
        fprintf(fp, "\nusemtl material_%zu\n", i + 1);
        fprintf(fp, "# triangles material %zu\n", i + 1);

        for (const RenderPath& render_path : t_buffer.render_paths) {
            if (render_path.color != color)
                continue;

            const IBuffer& ibuffer = t_buffer.indices[render_path.ibuffer_id];
            size_t vertices_offset = 0;
            for (size_t j = 0; j < vertices_offsets.size(); ++j) {
                const VerticesOffset& offset = vertices_offsets[j];
                if (offset.vbo == ibuffer.vbo) {
                    vertices_offset = offset.offset;
                    break;
                }
            }

            // get indices data from index buffer on gpu
            glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer.ibo));
            for (size_t j = 0; j < render_path.sizes.size(); ++j) {
                IndexBuffer indices(render_path.sizes[j]);
                glsafe(::glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLintptr>(render_path.offsets[j]),
                    static_cast<GLsizeiptr>(render_path.sizes[j] * sizeof(IBufferType)), static_cast<void*>(indices.data())));

                const size_t triangles_count = render_path.sizes[j] / 3;
                for (size_t k = 0; k < triangles_count; ++k) {
                    const size_t base = k * 3;
                    const size_t v1 = 1 + static_cast<size_t>(indices[base + 0]) + vertices_offset;
                    const size_t v2 = 1 + static_cast<size_t>(indices[base + 1]) + vertices_offset;
                    const size_t v3 = 1 + static_cast<size_t>(indices[base + 2]) + vertices_offset;
                    if (v1 != v2)
                        // do not export dummy triangles
                        fprintf(fp, "f %zu//%zu %zu//%zu %zu//%zu\n", v1, v1, v2, v2, v3, v3);
                }
            }
            glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        }
        ++i;
    }

    fclose(fp);
#endif
}


void LegacyRenderer::update_sequential_view_current(unsigned int first, unsigned int last) 
{
    auto is_visible = [this](unsigned int id) {
        for (const TBuffer& buffer : m_buffers) {
            if (buffer.visible) {
                for (const Path& path : buffer.paths) {
                    if (path.sub_paths.front().first.s_id <= id && id <= path.sub_paths.back().last.s_id)
                        return true;
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

void LegacyRenderer::set_layers_z_range(const std::array<unsigned int, 2>& layers_z_range) 
{
    bool keep_sequential_current_first = layers_z_range[0] >= m_layers_z_range[0];
    bool keep_sequential_current_last  = layers_z_range[1] <= m_layers_z_range[1];
    m_layers_z_range                   = layers_z_range;
    refresh_render_paths(keep_sequential_current_first, keep_sequential_current_last);
    update_moves_slider(true);
}

bool LegacyRenderer::is_toolpath_move_type_visible(EMoveType type) const
{
    size_t id = static_cast<size_t>(buffer_id(type));
    return (id < m_buffers.size()) ? m_buffers[id].visible : false;
}

void LegacyRenderer::set_toolpath_move_type_visible(EMoveType type, bool visible)
{
    size_t id = static_cast<size_t>(buffer_id(type));
    if (id < m_buffers.size())
        m_buffers[id].visible = visible;
}

unsigned int LegacyRenderer::get_options_visibility_flags() const
{
    auto set_flag = [](unsigned int flags, unsigned int flag, bool active) { return active ? (flags | (1 << flag)) : flags; };

    unsigned int flags = 0;
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::Travel), is_toolpath_move_type_visible(EMoveType::Travel));
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::Wipe), is_toolpath_move_type_visible(EMoveType::Wipe));
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::Retractions), is_toolpath_move_type_visible(EMoveType::Retract));
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::Unretractions),
                     is_toolpath_move_type_visible(EMoveType::Unretract));
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::Seams), is_toolpath_move_type_visible(EMoveType::Seam));
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::ToolChanges),
                     is_toolpath_move_type_visible(EMoveType::Tool_change));
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::ColorChanges),
                     is_toolpath_move_type_visible(EMoveType::Color_change));
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::PausePrints),
                     is_toolpath_move_type_visible(EMoveType::Pause_Print));
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::CustomGCodes),
                     is_toolpath_move_type_visible(EMoveType::Custom_GCode));
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::Shells), m_shells.visible);
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::ToolMarker), marker.is_visible());
    flags = set_flag(flags, static_cast<unsigned int>(Preview::OptionType::Legend), is_legend_enabled());
    return flags;
}

void LegacyRenderer::set_options_visibility_from_flags(unsigned int flags) 
{
    auto is_flag_set = [flags](unsigned int flag) { return (flags & (1 << flag)) != 0; };

    set_toolpath_move_type_visible(EMoveType::Travel, is_flag_set(static_cast<unsigned int>(Preview::OptionType::Travel)));
    set_toolpath_move_type_visible(EMoveType::Wipe, is_flag_set(static_cast<unsigned int>(Preview::OptionType::Wipe)));
    set_toolpath_move_type_visible(EMoveType::Retract, is_flag_set(static_cast<unsigned int>(Preview::OptionType::Retractions)));
    set_toolpath_move_type_visible(EMoveType::Unretract, is_flag_set(static_cast<unsigned int>(Preview::OptionType::Unretractions)));
    set_toolpath_move_type_visible(EMoveType::Seam, is_flag_set(static_cast<unsigned int>(Preview::OptionType::Seams)));
    set_toolpath_move_type_visible(EMoveType::Tool_change, is_flag_set(static_cast<unsigned int>(Preview::OptionType::ToolChanges)));
    set_toolpath_move_type_visible(EMoveType::Color_change, is_flag_set(static_cast<unsigned int>(Preview::OptionType::ColorChanges)));
    set_toolpath_move_type_visible(EMoveType::Pause_Print, is_flag_set(static_cast<unsigned int>(Preview::OptionType::PausePrints)));
    set_toolpath_move_type_visible(EMoveType::Custom_GCode, is_flag_set(static_cast<unsigned int>(Preview::OptionType::CustomGCodes)));
    m_shells.visible = is_flag_set(static_cast<unsigned int>(Preview::OptionType::Shells));
    marker.set_visible(is_flag_set(static_cast<unsigned int>(Preview::OptionType::ToolMarker)));
    enable_legend(is_flag_set(static_cast<unsigned int>(Preview::OptionType::Legend)));
}

void LegacyRenderer::update_marker_curr_move()
{
#if 0
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
#else
    if ((int) m_last_result_id == -1 || m_sequential_view.current.last >= m_sid_to_moveid.size())
        return;

    // Direct O(1) lookup using pre-built s_id -> move_id mapping
    // This replaces the original O(N) linear search through all moves
    const size_t move_id = m_sid_to_moveid[m_sequential_view.current.last];
    if (move_id < m_gcode_result->moves.size())
        marker.update_curr_move(m_gcode_result->moves[move_id]);
#endif
}

bool LegacyRenderer::is_extrusion_role_visible(ExtrusionRole role) const 
{
	return is_visible(role); 
}

void LegacyRenderer::set_extrusion_role_visible(ExtrusionRole role, bool is_visible) 
{
    if (is_visible) {
        m_extrusions.role_visibility_flags |= (1 << role);
    } else {
        m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags &= ~(1 << role);
    }
}

int LegacyRenderer::get_layer_index(const Path& path) const
{
    if (m_layers.empty())
        return -1;
    assert(m_layers.size() == m_layers.get_endpoints().size());
    const std::vector<Layers::Endpoints>& endpoints = m_layers.get_endpoints();
    size_t                                length    = endpoints.size();
    for (size_t i = 0; i < length; i++) {
        const auto& ep = endpoints.at(i);
        if (ep.first <= path.sub_paths.front().first.s_id && path.sub_paths.back().last.s_id <= ep.last)
            return i;
    }
    return -1;
}

bool LegacyRenderer::show_gcode_surface() const
{ 
	
	//BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << boost::format("verify m_gcode_result begin");
    //boost::log::core::get()->flush();

	if (m_gcode_result) {
        if (m_gcode_result->all_surface_with_shell == false) 
			return false;
	}	

	//BOOST_LOG_TRIVIAL(warning) << __FUNCTION__ << boost::format("verify m_gcode_result end");
    //boost::log::core::get()->flush();

	for (size_t i = 0; i < m_tools.m_tool_visibles.size(); i++) 
	{
        if (m_tools.m_tool_visibles.at(i) == false)
			return false;
	}

	bool full_range   = m_layers_slider->is_higher_at_max() && m_layers_slider->is_lower_at_min() && m_moves_slider->is_higher_at_max();
    bool show_surface = full_range && is_visible(ExtrusionRole::erExternalPerimeter);  //show outer wall

    if (show_surface) {
        bool has_top    = false;
        bool has_bottom = false;
        for (size_t i = 0; i < m_roles.size(); i++) {
            const ExtrusionRole& role = m_roles.at(i);
            if (role == ExtrusionRole::erTopSolidInfill) {
                has_top = true;
            } else if (role == ExtrusionRole::erBottomSurface) {
                has_bottom = true;
            }
        }
	
		// has top and show
        if (has_top) {
            show_surface = is_visible(ExtrusionRole::erTopSolidInfill);
		}

		// has bottom and show
		if (show_surface) {
            if (has_bottom) {
                show_surface = is_visible(ExtrusionRole::erBottomSurface);  
			}
		}
    }

	return show_surface;
}

double LegacyRenderer::estimate_pixels_of_one_layer() const
{ 
	auto                      camera        = GUI::wxGetApp().plater()->get_camera();
    auto                      zoom          = camera.get_zoom();
    Transform3d               vier_mat      = camera.get_view_matrix();
    Matrix4d                  vier_proj_mat = camera.get_projection_matrix().matrix() * vier_mat.matrix();
    const std::array<int, 4>& viewport      = camera.get_viewport();

    size_t       layers_num = m_layers.size();
    const double h          = m_paths_bounding_box.max.z();
    const double average_h  = h / layers_num;
    Vec3d target = camera.get_target();
	Vec3d up = camera.get_dir_up();
    Vec3d t = target + up.normalized() * average_h;

    auto  s_min = calc_pt_in_screen(target, vier_proj_mat, viewport[2], viewport[3]);
    auto  s_max = calc_pt_in_screen(t, vier_proj_mat, viewport[2], viewport[3]);
    double dist = (s_min - s_max).norm();

	return dist;
}

int LegacyRenderer::get_dynamic_stride(double layer_height_of_pixel) const
{
    if (layer_height_of_pixel <= 0.1) return 2;
    //int s = std::max((int) (m_stride_factor * layer_height_of_pixel), 3);
	int s = std::max((int)ceilf(m_stride_factor * layer_height_of_pixel), 3);
    if (s > 6) {
		return 0;
	}
	return s;
}

bool LegacyRenderer::should_be_filtered_of_layer(int stride, int layer) const
{
    size_t layers_num = m_layers.size();
    
	if (layers_num == 0 || layer < m_layers_z_range[0] || layer > m_layers_z_range[1] || layer < 0 || layer > layers_num)
        return true;

	if (stride <= 0 || layer == m_layers_z_range[0] || layer == m_layers_z_range[1]) 
		return false;	
	
	if (m_top_surface_layer.find(layer) != m_top_surface_layer.end()) 
		return false;

	//const Camera& camera = wxGetApp().plater()->get_camera();

	
	//if (!camera.is_looking_downward()) {
        /*if ((layer - m_layers_z_range[0]) % stride == 0) {
            return true;
        }*/
 //   } else {
	//	// from top to bottom
        if ((m_layers_z_range[1] - layer) % stride == 0) {
            return true;
        }
	//}
	
	return false;
}

const std::vector<double>& LegacyRenderer::get_filtered_layers_z_offset(const int dynamic_stride)
{
	
	bool valid = m_filter_layer_result.check_valid(dynamic_stride, m_layers_z_range);
    if (valid){
		return m_filter_layer_result.get_layers_z_offset();
	}

	return m_filter_layer_result.rebuild_filter_layers_z_offset(this, dynamic_stride, m_layers_z_range, m_layers);
}

Vec3f LegacyRenderer::encode_position(const Vec3f& position)
{
	return (position - m_paths_bounding_box.min.cast<float>()) * SHORT_TYPE_POSITION_SCALE;
}

bool LegacyRenderer::should_enable_memory_optimize(const GCodeProcessorResult& gcode_result)
{
	
#if ENABLE_VECTOR_HYBRID_BACKEND
    bool enable_mem_compress = GUI::wxGetApp().app_config->get_bool("enable_preview_mem_optimize");
    if (enable_mem_compress) {
        // based on current conditions
        /*size_t avail            = Slic3r::available_physical_memory();
        size_t total            = Slic3r::total_physical_memory();
        size_t line_count       = m_moves_count - seams_count;
        size_t estmate_mem_size = (seams_count * 102 + line_count * 30) * sizeof(unsigned short);*/
        // enable_mem_compress     = (estmate_mem_size >= avail * 0.5 && total < 18 * 1024 * 1024 * 1024);
    }

    // Avoid overflow causing rendering issues
    {
        PartPlateList& partplate_list = wxGetApp().plater()->get_partplate_list();
        PartPlate*     current_plate  = partplate_list.get_curr_plate();

        
        BoundingBoxf3 paths_bounding_box = current_plate->get_gcode_path_bounding_box();

        //auto min3  = paths_bounding_box.min;
        //auto max3  = paths_bounding_box.max;
        auto size = paths_bounding_box.size();
        double max_size = std::max(size(0), std::max(size(1), size(2)));
		
        if (max_size > 600.0) {
            enable_mem_compress = false;
        }
    }
    return enable_mem_compress;
#endif

	return false; 
}

#define ENABLE_CALIBRATION_THUMBNAIL_OUTPUT 0
#if ENABLE_CALIBRATION_THUMBNAIL_OUTPUT
static void debug_calibration_output_thumbnail(const ThumbnailData& thumbnail_data)
{
    // debug export of generated image
    wxImage image(thumbnail_data.width, thumbnail_data.height);
    image.InitAlpha();

    for (unsigned int r = 0; r < thumbnail_data.height; ++r)
    {
        unsigned int rr = (thumbnail_data.height - 1 - r) * thumbnail_data.width;
        for (unsigned int c = 0; c < thumbnail_data.width; ++c)
        {
            unsigned char* px = (unsigned char*)thumbnail_data.pixels.data() + 4 * (rr + c);
            image.SetRGB((int)c, (int)r, px[0], px[1], px[2]);
            image.SetAlpha((int)c, (int)r, px[3]);
        }
    }

    image.SaveFile("D:/calibrate.png", wxBITMAP_TYPE_PNG);
}
#endif

void LegacyRenderer::_render_calibration_thumbnail_internal(ThumbnailData& thumbnail_data, const ThumbnailsParams& thumbnail_params, PartPlateList& partplate_list, OpenGLManager& opengl_manager)
{
    int plate_idx = thumbnail_params.plate_id;
    PartPlate* plate = partplate_list.get_plate(plate_idx);
    BoundingBoxf3 plate_box = plate->get_bounding_box(false);
    plate_box.min.z() = 0.0;
    plate_box.max.z() = 0.0;
    Vec3d center = plate_box.center();

#if 1
    Camera camera;
    camera.set_viewport(0, 0, thumbnail_data.width, thumbnail_data.height);
    camera.apply_viewport();
    camera.set_scene_box(plate_box);
    camera.set_type(Camera::EType::Ortho);
    camera.set_target(center);
    camera.select_view("top");
    camera.zoom_to_box(plate_box, 1.0f);
    camera.apply_projection(plate_box);

    auto render_as_triangles = [
#if ENABLE_GCODE_VIEWER_STATISTICS
        this
#endif // ENABLE_GCODE_VIEWER_STATISTICS
    ](TBuffer &buffer, std::vector<RenderPath>::iterator it_path, std::vector<RenderPath>::iterator it_end, GLShaderProgram& shader, int uniform_color) {
        for (auto it = it_path; it != it_end && it_path->ibuffer_id == it->ibuffer_id; ++it) {
            const RenderPath& path = *it;
            // Some OpenGL drivers crash on empty glMultiDrawElements, see GH #7415.
            assert(!path.sizes.empty());
            assert(!path.offsets.empty());
            shader.set_uniform(uniform_color, path.color);
            glsafe(::glMultiDrawElements(GL_TRIANGLES, (const GLsizei*)path.sizes.data(), GL_UNSIGNED_SHORT, (const void* const*)path.offsets.data(), (GLsizei)path.sizes.size()));
#if ENABLE_GCODE_VIEWER_STATISTICS
            ++m_statistics.gl_multi_triangles_calls_count;
#endif // ENABLE_GCODE_VIEWER_STATISTICS
        }
    };

    auto render_as_instanced_model = [
#if ENABLE_GCODE_VIEWER_STATISTICS
        this
#endif // ENABLE_GCODE_VIEWER_STATISTICS
    ](TBuffer& buffer, GLShaderProgram& shader) {
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
    auto render_as_batched_model = [](TBuffer& buffer, GLShaderProgram& shader, int position_id, int normal_id) {
#endif // ENABLE_GCODE_VIEWER_STATISTICS

        struct Range
        {
            unsigned int first;
            unsigned int last;
            bool intersects(const Range& other) const { return (other.last < first || other.first > last) ? false : true; }
        };
        Range buffer_range = { 0, 0 };
        size_t indices_per_instance = buffer.model.data.indices_count();

        for (size_t j = 0; j < buffer.indices.size(); ++j) {
            const IBuffer& i_buffer = buffer.indices[j];
            buffer_range.last = buffer_range.first + i_buffer.count / indices_per_instance;
            glsafe(::glBindBuffer(GL_ARRAY_BUFFER, i_buffer.vbo));
            if (position_id != -1) {
                glsafe(::glVertexAttribPointer(position_id, buffer.vertices.position_size_floats(), GL_FLOAT, GL_FALSE, buffer.vertices.vertex_size_bytes(), (const void*)buffer.vertices.position_offset_bytes()));
                glsafe(::glEnableVertexAttribArray(position_id));
            }
            bool has_normals = buffer.vertices.normal_size_floats() > 0;
            if (has_normals) {
                if (normal_id != -1) {
                    glsafe(::glVertexAttribPointer(normal_id, buffer.vertices.normal_size_floats(), GL_FLOAT, GL_FALSE, buffer.vertices.vertex_size_bytes(), (const void*)buffer.vertices.normal_offset_bytes()));
                    glsafe(::glEnableVertexAttribArray(normal_id));
                }
            }

            glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i_buffer.ibo));

            for (auto& range : buffer.model.instances.render_ranges.ranges) {
                Range range_range = { range.offset, range.offset + range.count };
                if (range_range.intersects(buffer_range)) {
                    shader.set_uniform("uniform_color", range.color);
                    unsigned int offset = (range_range.first > buffer_range.first) ? range_range.first - buffer_range.first : 0;
                    size_t offset_bytes = static_cast<size_t>(offset) * indices_per_instance * sizeof(IBufferType);
                    Range render_range = { std::max(range_range.first, buffer_range.first), std::min(range_range.last, buffer_range.last) };
                    size_t count = static_cast<size_t>(render_range.last - render_range.first) * indices_per_instance;
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

    unsigned char begin_id = buffer_id(EMoveType::Retract);
    unsigned char end_id = buffer_id(EMoveType::Count);

    BOOST_LOG_TRIVIAL(info) << boost::format("render_calibration_thumbnail: begin_id %1%, end_id %2%")%begin_id %end_id;
    for (unsigned char i = begin_id; i < end_id; ++i) {
        TBuffer& buffer = m_buffers[i];
        if (!buffer.visible || !buffer.has_data())
            continue;

        GLShaderProgram* shader = opengl_manager.get_shader("flat");
        if (shader != nullptr) {
            shader->start_using();

            shader->set_uniform("view_model_matrix", camera.get_view_matrix());
            shader->set_uniform("projection_matrix", camera.get_projection_matrix());
            int position_id = shader->get_attrib_location("v_position");
            int normal_id   = shader->get_attrib_location("v_normal");

            if (buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::InstancedModel) {
                //shader->set_uniform("emission_factor", 0.25f);
                render_as_instanced_model(buffer, *shader);
                //shader->set_uniform("emission_factor", 0.0f);
            }
            else if (buffer.render_primitive_type == TBuffer::ERenderPrimitiveType::BatchedModel) {
                //shader->set_uniform("emission_factor", 0.25f);
                render_as_batched_model(buffer, *shader, position_id, normal_id);
                //shader->set_uniform("emission_factor", 0.0f);
            }
            else {
                switch (buffer.render_primitive_type) {
                default: break;
                }
                int uniform_color = shader->get_uniform_location("uniform_color");
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
                        glsafe(::glVertexAttribPointer(position_id, buffer.vertices.position_size_floats(), GL_FLOAT, GL_FALSE, buffer.vertices.vertex_size_bytes(), (const void*)buffer.vertices.position_offset_bytes()));
                        glsafe(::glEnableVertexAttribArray(position_id));
                    }
                    bool has_normals = false;// buffer.vertices.normal_size_floats() > 0;
                    if (has_normals) {
                        if (normal_id != -1) {
                            glsafe(::glVertexAttribPointer(normal_id, buffer.vertices.normal_size_floats(), GL_FLOAT, GL_FALSE, buffer.vertices.vertex_size_bytes(), (const void*)buffer.vertices.normal_offset_bytes()));
                            glsafe(::glEnableVertexAttribArray(normal_id));
                        }
                    }

                    glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i_buffer.ibo));

                    // Render all elements with it_path->ibuffer_id == ibuffer_id, possible with varying colors.
                    switch (buffer.render_primitive_type)
                    {
                    case TBuffer::ERenderPrimitiveType::Triangle: {
                        render_as_triangles(buffer, it_path, buffer.render_paths.end(), *shader, uniform_color);
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
        else {
            BOOST_LOG_TRIVIAL(info) << boost::format("render_calibration_thumbnail: can not find shader");
        }
    }
#endif
    BOOST_LOG_TRIVIAL(info) << boost::format("render_calibration_thumbnail: exit");

}

void LegacyRenderer::_render_calibration_thumbnail_framebuffer(ThumbnailData& thumbnail_data, unsigned int w, unsigned int h, const ThumbnailsParams& thumbnail_params, PartPlateList& partplate_list, OpenGLManager& opengl_manager)
{
    BOOST_LOG_TRIVIAL(info) << boost::format("render_calibration_thumbnail prepare: width %1%, height %2%")%w %h;
    thumbnail_data.set(w, h);
    if (!thumbnail_data.is_valid())
        return;

    //TODO bool multisample = m_multisample_allowed;
    bool multisample = OpenGLManager::can_multisample();
    //if (!multisample)
    //    glsafe(::glEnable(GL_MULTISAMPLE));

    GLint max_samples;
    glsafe(::glGetIntegerv(GL_MAX_SAMPLES, &max_samples));
    GLsizei num_samples = max_samples / 2;

    GLuint render_fbo;
    glsafe(::glGenFramebuffers(1, &render_fbo));
    glsafe(::glBindFramebuffer(GL_FRAMEBUFFER, render_fbo));
    BOOST_LOG_TRIVIAL(info) << boost::format("render_calibration_thumbnail prepare: max_samples %1%, multisample %2%, render_fbo %3%")%max_samples %multisample %render_fbo;

    GLuint render_tex = 0;
    GLuint render_tex_buffer = 0;
    if (multisample) {
        // use renderbuffer instead of texture to avoid the need to use glTexImage2DMultisample which is available only since OpenGL 3.2
        glsafe(::glGenRenderbuffers(1, &render_tex_buffer));
        glsafe(::glBindRenderbuffer(GL_RENDERBUFFER, render_tex_buffer));
        glsafe(::glRenderbufferStorageMultisample(GL_RENDERBUFFER, num_samples, GL_RGBA8, w, h));
        glsafe(::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, render_tex_buffer));
    }
    else {
        glsafe(::glGenTextures(1, &render_tex));
        glsafe(::glBindTexture(GL_TEXTURE_2D, render_tex));
        glsafe(::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
        glsafe(::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        glsafe(::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        glsafe(::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_tex, 0));
    }

    GLuint render_depth;
    glsafe(::glGenRenderbuffers(1, &render_depth));
    glsafe(::glBindRenderbuffer(GL_RENDERBUFFER, render_depth));
    if (multisample)
        glsafe(::glRenderbufferStorageMultisample(GL_RENDERBUFFER, num_samples, GL_DEPTH_COMPONENT24, w, h));
    else
        glsafe(::glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h));

    glsafe(::glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_depth));

    GLenum drawBufs[] = { GL_COLOR_ATTACHMENT0 };
    glsafe(::glDrawBuffers(1, drawBufs));


    if (::glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        _render_calibration_thumbnail_internal(thumbnail_data, thumbnail_params, partplate_list, opengl_manager);

        if (multisample) {
            GLuint resolve_fbo;
            glsafe(::glGenFramebuffers(1, &resolve_fbo));
            glsafe(::glBindFramebuffer(GL_FRAMEBUFFER, resolve_fbo));

            GLuint resolve_tex;
            glsafe(::glGenTextures(1, &resolve_tex));
            glsafe(::glBindTexture(GL_TEXTURE_2D, resolve_tex));
            glsafe(::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
            glsafe(::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            glsafe(::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            glsafe(::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolve_tex, 0));

            glsafe(::glDrawBuffers(1, drawBufs));

            if (::glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
                glsafe(::glBindFramebuffer(GL_READ_FRAMEBUFFER, render_fbo));
                glsafe(::glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolve_fbo));
                glsafe(::glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR));

                glsafe(::glBindFramebuffer(GL_READ_FRAMEBUFFER, resolve_fbo));
                glsafe(::glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (void*)thumbnail_data.pixels.data()));
            }

            glsafe(::glDeleteTextures(1, &resolve_tex));
            glsafe(::glDeleteFramebuffers(1, &resolve_fbo));
        }
        else
            glsafe(::glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (void*)thumbnail_data.pixels.data()));
    }
#if ENABLE_CALIBRATION_THUMBNAIL_OUTPUT
     debug_calibration_output_thumbnail(thumbnail_data);
#endif

     glsafe(::glBindFramebuffer(GL_FRAMEBUFFER, 0));
     glsafe(::glDeleteRenderbuffers(1, &render_depth));
     if (render_tex_buffer != 0)
         glsafe(::glDeleteRenderbuffers(1, &render_tex_buffer));
     if (render_tex != 0)
         glsafe(::glDeleteTextures(1, &render_tex));
     glsafe(::glDeleteFramebuffers(1, &render_fbo));

    //if (!multisample)
    //    glsafe(::glDisable(GL_MULTISAMPLE));
    BOOST_LOG_TRIVIAL(info) << boost::format("render_calibration_thumbnail prepare: exit");
}

//BBS
void LegacyRenderer::render_calibration_thumbnail(ThumbnailData& thumbnail_data, unsigned int w, unsigned int h, const ThumbnailsParams& thumbnail_params, PartPlateList& partplate_list, OpenGLManager& opengl_manager)
{
    // reset values and refresh render
    int       last_view_type_sel = m_view_type_sel;
    EViewType last_view_type     = m_view_type;
    unsigned int last_role_visibility_flags = m_extrusions.role_visibility_flags;
    // set color scheme to FilamentId
    for (int i = 0; i < view_type_items.size(); i++) {
        if (view_type_items[i] == EViewType::FilamentId) {
            m_view_type_sel = i;
            break;
        }
    }
    set_view_type(EViewType::FilamentId, false);
    // set m_layers_z_range to 0, 1;
    // To be safe, we include both layers here although layer 1 seems enough
    // layer 0: custom extrusions such as flow calibration etc.
    // layer 1: the real first layer of object
    std::array<unsigned int, 2> tmp_layers_z_range = m_layers_z_range;
    m_layers_z_range = {0, 1};
    // BBS exclude feature types
    m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags & ~(1 << erSkirt);
    m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags & ~(1 << erCustom);
    // BBS include feature types
    m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags | (1 << erWipeTower);
    m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags | (1 << erPerimeter);
    m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags | (1 << erExternalPerimeter);
    m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags | (1 << erOverhangPerimeter);
    m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags | (1 << erSolidInfill);
    m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags | (1 << erTopSolidInfill);
    m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags | (1 << erInternalInfill);
    m_extrusions.role_visibility_flags = m_extrusions.role_visibility_flags | (1 << erBottomSurface);

    refresh_render_paths(false, false);

    _render_calibration_thumbnail_framebuffer(thumbnail_data, w, h, thumbnail_params, partplate_list, opengl_manager);

    // restore values and refresh render
    // reset m_layers_z_range and view type
    m_view_type_sel = last_view_type_sel;
    set_view_type(last_view_type, false);
    m_layers_z_range = tmp_layers_z_range;
    m_extrusions.role_visibility_flags = last_role_visibility_flags;
    refresh_render_paths(false, false);
}

void LegacyRenderer::log_memory_used(const std::string& label, int64_t additional) const
{
    if (Slic3r::get_logging_level() >= 0) {
        int64_t paths_size        = 0;
        int64_t render_paths_size = 0;
        for (const TBuffer& buffer : m_buffers) {
            paths_size += SLIC3R_STDVEC_MEMSIZE(buffer.paths, Path);
            render_paths_size += SLIC3R_STDUNORDEREDSET_MEMSIZE(buffer.render_paths, RenderPath);
            for (const RenderPath& path : buffer.render_paths) {
                render_paths_size += SLIC3R_STDVEC_MEMSIZE(path.sizes, unsigned int);
                render_paths_size += SLIC3R_STDVEC_MEMSIZE(path.offsets, size_t);
            }
        }
        int64_t layers_size = SLIC3R_STDVEC_MEMSIZE(m_layers.get_zs(), double);
        layers_size += SLIC3R_STDVEC_MEMSIZE(m_layers.get_endpoints(), Layers::Endpoints);
        BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " " << label << " "
                                 << boost::format("paths_size %1%, render_paths_size %2%,layers_size %3%, additional %4%\n") % paths_size %
                                        render_paths_size % layers_size % additional;
        BOOST_LOG_TRIVIAL(trace) << label << "(" << format_memsize_MB(additional + paths_size + render_paths_size + layers_size) << ");"
                                 << log_memory_info();
    }
}

void LegacyRenderer::on_visibility_changed()
{
    // update buffers' render paths
    refresh_render_paths();
    update_moves_slider();
    BaseRenderer::on_visibility_changed();
}


}//namespace Slic3r::GUI
}// namespace Slic3r::Slic3r