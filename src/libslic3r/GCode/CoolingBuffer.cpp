#include "../GCode.hpp"
#include "CoolingBuffer.hpp"
#include "libslic3r/Geometry/ArcWelder.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/log/trivial.hpp>
#include <iostream>
#include <float.h>
#include <limits>
#include <numeric>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

#if 0
    #define DEBUG
    #define _DEBUG
    #undef NDEBUG
#endif

#include <assert.h>
#include <fast_float/fast_float.h>

namespace Slic3r {
#define MAX_ADDITIONAL_LAYES 4

const constexpr float SEGMENT_SPLIT_EPSILON = 10. * GCodeFormatter::XYZ_EPSILON;
static float s_last_nonzero_z = 0.f;
static bool  s_force_z_on_next_xy_move = false;
static bool  s_forced_first_layer_z = false;
static float s_forced_z_value = 0.f;

namespace {
struct OverhangFanRegion
{
    const CoolingLine* start_marker = nullptr;
    const CoolingLine* end_marker   = nullptr;
    double length                   = 0.;
    double gap_before               = std::numeric_limits<double>::infinity();
    double width                    = 0.;
    size_t merged_into              = size_t(-1);
};

struct OverhangFanEvent
{
    enum class Type { Start, End, Move, Boundary };

    size_t line_start = 0;
    Type type         = Type::Move;
    const CoolingLine* marker = nullptr;
    double length     = 0.;
    double width      = 0.;
};

static double overhang_fan_min_region_length(double width)
{
    return std::max(3.0, 6.0 * width);
}

static double overhang_fan_gap_merge_length(double width)
{
    return std::max(2.0, 4.0 * width);
}

static std::unordered_set<const CoolingLine*> collect_disabled_overhang_fan_markers(const std::vector<const CoolingLine*>& lines)
{
    bool   has_overhang_fan_markers = false;
    size_t first_marker_line        = std::numeric_limits<size_t>::max();
    size_t last_marker_line         = 0;
    for (const CoolingLine* line : lines) {
        if (line->type & (CoolingLine::TYPE_OVERHANG_FAN_START | CoolingLine::TYPE_OVERHANG_FAN_END)) {
            has_overhang_fan_markers = true;
            first_marker_line = std::min(first_marker_line, line->line_start);
            last_marker_line  = std::max(last_marker_line, line->line_start);
        }
    }
    if (!has_overhang_fan_markers)
        return {};

    auto is_in_marker_span = [first_marker_line, last_marker_line](size_t line_start) {
        return first_marker_line <= line_start && line_start <= last_marker_line;
    };

    std::vector<OverhangFanEvent> events;
    events.reserve(lines.size());

    for (const CoolingLine* line : lines) {
        const double width = std::max(0.f, line->overhang_fan_width);
        if (line->type & CoolingLine::TYPE_OVERHANG_FAN_START) {
            events.push_back({line->line_start, OverhangFanEvent::Type::Start, line, 0., width});
        } else if (line->type & CoolingLine::TYPE_OVERHANG_FAN_END) {
            events.push_back({line->line_start, OverhangFanEvent::Type::End, line, 0., width});
        } else if (is_in_marker_span(line->line_start) &&
                   (line->type & (CoolingLine::TYPE_SET_TOOL | CoolingLine::TYPE_FORCE_RESUME_FAN |
                                  CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_START | CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_END))) {
            events.push_back({line->line_start, OverhangFanEvent::Type::Boundary, nullptr, 0., 0.});
        }

        if (!line->move_segments.empty()) {
            for (const GCodeMoveSegment& segment : line->move_segments) {
                if (is_in_marker_span(segment.line_start))
                    events.push_back({segment.line_start, OverhangFanEvent::Type::Move, nullptr, segment.length, 0.});
            }
        } else if (line->length() > 0.f && is_in_marker_span(line->line_start)) {
            events.push_back({line->line_start, OverhangFanEvent::Type::Move, nullptr, line->length(), 0.});
        }
    }

    std::sort(events.begin(), events.end(), [](const OverhangFanEvent& lhs, const OverhangFanEvent& rhs) {
        if (lhs.line_start != rhs.line_start)
            return lhs.line_start < rhs.line_start;
        return int(lhs.type) < int(rhs.type);
    });

    std::unordered_set<const CoolingLine*> disabled;
    std::vector<OverhangFanRegion> regions;

    const size_t no_region = size_t(-1);
    size_t current_region  = no_region;
    size_t previous_region = no_region;
    double gap_length      = 0.;

    for (const OverhangFanEvent& event : events) {
        if (event.type == OverhangFanEvent::Type::Start) {
            if (current_region == no_region) {
                regions.emplace_back();
                current_region = regions.size() - 1;
                OverhangFanRegion& region = regions.back();
                region.start_marker = event.marker;
                region.gap_before   = previous_region == no_region ? std::numeric_limits<double>::infinity() : gap_length;
                region.width        = event.width;
                gap_length          = 0.;
            } else if (event.marker != nullptr) {
                disabled.insert(event.marker);
            }
            continue;
        }

        if (event.type == OverhangFanEvent::Type::End) {
            if (current_region != no_region) {
                OverhangFanRegion& region = regions[current_region];
                region.end_marker = event.marker;
                region.width      = std::max(region.width, event.width);
                previous_region   = current_region;
                current_region    = no_region;
                gap_length        = 0.;
            } else if (event.marker != nullptr) {
                disabled.insert(event.marker);
            }
            continue;
        }

        if (event.type == OverhangFanEvent::Type::Boundary && current_region == no_region) {
            previous_region = no_region;
            gap_length      = 0.;
            continue;
        }

        if (event.type == OverhangFanEvent::Type::Move) {
            if (current_region != no_region)
                regions[current_region].length += event.length;
            else if (previous_region != no_region)
                gap_length += event.length;
        }
    }

    size_t root_region = no_region;
    for (size_t i = 0; i < regions.size(); ++i) {
        OverhangFanRegion& region = regions[i];
        if (root_region != no_region && regions[root_region].end_marker != nullptr && region.start_marker != nullptr && region.end_marker != nullptr) {
            const double merge_width = std::max(regions[root_region].width, region.width);
            if (region.gap_before <= overhang_fan_gap_merge_length(merge_width)) {
                disabled.insert(regions[root_region].end_marker);
                disabled.insert(region.start_marker);
                regions[root_region].length += region.gap_before + region.length;
                regions[root_region].width = merge_width;
                regions[root_region].end_marker = region.end_marker;
                region.merged_into = root_region;
                continue;
            }
        }
        root_region = i;
    }

    for (const OverhangFanRegion& region : regions) {
        if (region.merged_into != no_region || region.start_marker == nullptr || region.end_marker == nullptr)
            continue;
        if (region.length < overhang_fan_min_region_length(region.width)) {
            disabled.insert(region.start_marker);
            disabled.insert(region.end_marker);
        }
    }

    return disabled;
}
} // namespace

// Strip explicit Z0 tokens to avoid resetting Z when previous Z is valid.
static std::string sanitize_z0_tokens(const std::string& line)
{
    if (line.find('Z') == std::string::npos)
        return line;
    std::string out;
    out.reserve(line.size());
    const char* p = line.c_str();
    const char* end = p + line.size();
    while (p < end) {
        if (*p == 'Z') {
            const char* z_start = p;
            ++p;
            const char* num_start = p;
            // optional sign
            if (p < end && (*p == '+' || *p == '-'))
                ++p;
            while (p < end && (std::isdigit(*p) || *p == '.'))
                ++p;
            std::string num(num_start, p - num_start);
            double z_val = 0.0;
            fast_float::from_chars(num.data(), num.data() + num.size(), z_val);
            if (std::abs(z_val) < GCodeFormatter::XYZ_EPSILON && s_last_nonzero_z > GCodeFormatter::XYZ_EPSILON) {
                // skip this Z token
                continue;
            } else {
                out.push_back('Z');
                out.append(num);
                continue;
            }
        }
        out.push_back(*p);
        ++p;
    }
    return out;
}

static std::string sanitize_buffer_z(const std::string& gcode_buffer)
{
    std::string buffer = gcode_buffer;
    if (s_force_z_on_next_xy_move) {
        if (s_forced_z_value > GCodeFormatter::XYZ_EPSILON) {
            GCodeG1Formatter z_formatter;
            z_formatter.emit_axis('Z', s_forced_z_value, GCodeFormatter::XYZF_EXPORT_DIGITS);
            std::string z_line = z_formatter.string();
            size_t insert_pos = 0;
            while (insert_pos < buffer.size()) {
                size_t line_end = buffer.find('\n', insert_pos);
                if (line_end == std::string::npos)
                    line_end = buffer.size();
                size_t p = insert_pos;
                while (p < line_end && (buffer[p] == ' ' || buffer[p] == '\t' || buffer[p] == '\r'))
                    ++p;
                if (p == line_end || buffer[p] == ';') {
                    insert_pos = (line_end < buffer.size()) ? line_end + 1 : line_end;
                    continue;
                }
                break;
            }
            size_t line_end = buffer.find('\n', insert_pos);
            if (line_end == std::string::npos)
                line_end = buffer.size();
            size_t semicolon = buffer.find(';', insert_pos);
            if (semicolon != std::string::npos && semicolon > line_end)
                semicolon = std::string::npos;
            size_t zpos = buffer.find('Z', insert_pos);
            const bool first_line_has_z = (zpos != std::string::npos && zpos < line_end && (semicolon == std::string::npos || zpos < semicolon));
            if (!first_line_has_z)
                buffer.insert(insert_pos, z_line);
        }
        s_force_z_on_next_xy_move = false;
        s_forced_z_value = 0.f;
    }
    // Fast path: nothing to clean if there is no Z token at all.
    const bool has_any_z = (buffer.find('Z') != std::string::npos);
    if (!has_any_z)
        return buffer;
    std::string result;
    result.reserve(buffer.size());
    size_t start = 0;
    double last_z = (s_last_nonzero_z > GCodeFormatter::XYZ_EPSILON) ? s_last_nonzero_z : 0.0;
    while (start < buffer.size()) {
        size_t end = buffer.find('\n', start);
        if (end == std::string::npos)
            end = buffer.size();
        std::string line = sanitize_z0_tokens(buffer.substr(start, end - start));
        // track last non-zero Z from the sanitized line
        auto posZ = line.find('Z');
        if (posZ != std::string::npos) {
            const char* zstr = line.c_str() + posZ + 1;
            double zval = 0.0;
            fast_float::from_chars(zstr, line.c_str() + line.size(), zval);
            if (zval > GCodeFormatter::XYZ_EPSILON)
                last_z = zval;
        }
        result.append(line);
        if (end < buffer.size())
            result.push_back('\n');
        start = end + 1;
    }
    if (last_z > GCodeFormatter::XYZ_EPSILON)
        s_last_nonzero_z = float(last_z);
    return result;
}

GCodeEditor::GCodeEditor(GCode &gcodegen) : m_config(gcodegen.config()), m_toolchange_prefix(gcodegen.writer().toolchange_prefix()), m_current_extruder(0)
{
    this->reset(gcodegen.writer().get_position());

    s_force_z_on_next_xy_move = false;
    s_forced_first_layer_z = false;
    s_forced_z_value = 0.f;

    const std::vector<Extruder> &extruders = gcodegen.writer().extruders();
    m_extruder_ids.reserve(extruders.size());
    for (const Extruder &ex : extruders) {
        m_num_extruders = std::max(ex.id() + 1, m_num_extruders);
        m_extruder_ids.emplace_back(ex.id());
    }
}

void GCodeEditor::reset(const Vec3d &position)
{
    // BBS: add I and J axis to store center of arc
    m_current_pos.assign(7, 0.f);
    m_current_pos[AxisIdx::X] = float(position.x());
    m_current_pos[AxisIdx::Y] = float(position.y());
    // Initialize Z with provided position; if zero (unset), fall back to initial layer height to avoid emitting Z0.
    double init_z = position.z();
    if (std::abs(init_z) < GCodeFormatter::XYZ_EPSILON) {
        // fallback: try initial_layer_print_height, otherwise layer_height
        init_z = m_config.initial_layer_print_height.value;
        if (std::abs(init_z) < GCodeFormatter::XYZ_EPSILON)
            init_z = 0.2; // minimal sane default to avoid Z0 output
    }
    m_current_pos[AxisIdx::Z] = float(init_z);
    s_last_nonzero_z = float(init_z);
    m_current_pos[AxisIdx::E] = 0.f;
    m_current_pos[AxisIdx::F] = float(m_config.travel_speed.value);
    m_fan_speed = -1;
    m_additional_fan_speed = -1;
    m_current_fan_speed = -1;
    m_additional_fan_count = 0;
}

static void record_wall_lines(bool& flag, int& line_idx, PerExtruderAdjustments* adjustment, const std::pair<int, int>& node_pos)
{
    if (flag && line_idx < adjustment->lines.size()) {
        CoolingLine& ptr        = adjustment->lines[line_idx];
        ptr.outwall_smooth_mark = true;
        ptr.object_id           = node_pos.first;
        ptr.cooling_node_id     = node_pos.second;
        flag                    = false;
    }
}

static void mark_node_pos(bool&                   flag,
                          int&                    line_idx,
                          std::pair<int, int>&    node_pos,
                          const std::vector<int>& object_label,
                          int                     cooling_node_id,
                          int                     object_id,
                          PerExtruderAdjustments* adjustment)
{
    for (size_t object_idx = 0; object_idx < object_label.size(); ++object_idx) {
        if (object_label[object_idx] == object_id) {
            if (cooling_node_id == -1)
                break;
            line_idx        = adjustment->lines.size();
            flag            = true;
            node_pos.first  = object_idx;
            node_pos.second = cooling_node_id;
            break;
        }
    }
}

std::string emit_feedrate(const float feedrate)
{
    GCodeG1Formatter feedrate_formatter;
    feedrate_formatter.emit_f(std::floor(60. * feedrate + 0.5));

    return feedrate_formatter.string();
}

static std::string emit_extrusion(const Vec4f& position_start, const Vec4f& position_end, const bool use_relative_e_distances)
{
    constexpr int AxisZ = 2;
    GCodeG1Formatter extrusion_formatter;
    float start_z = position_start[AxisZ];
    if (std::abs(start_z) < GCodeFormatter::XYZ_EPSILON && s_last_nonzero_z > GCodeFormatter::XYZ_EPSILON)
        start_z = s_last_nonzero_z;
    for (int axis_idx = 0; axis_idx < 3; ++axis_idx) {
        float end_val = position_end[axis_idx];
        if (axis_idx == AxisZ) {
            if (std::abs(end_val) < GCodeFormatter::XYZ_EPSILON && start_z > GCodeFormatter::XYZ_EPSILON)
                end_val = start_z;
            if (end_val > GCodeFormatter::XYZ_EPSILON)
                s_last_nonzero_z = end_val;
        }
        float cmp_start = (axis_idx == AxisZ) ? start_z : position_start[axis_idx];
        if (cmp_start != end_val) {
            extrusion_formatter.emit_axis(char('X' + axis_idx), end_val, GCodeFormatter::XYZF_EXPORT_DIGITS);
        }
    }

    if (position_start[3] != position_end[3]) {
        extrusion_formatter.emit_axis('E', use_relative_e_distances ? (position_end[3] - position_start[3]) : position_end[3],
                                      GCodeFormatter::E_EXPORT_DIGITS);
    }

    return extrusion_formatter.string();
}

static std::string emit_arc(
    const Vec4f& position_start, const Vec4f& position_end, const Vec2f& ij_params, const bool is_ccw, const bool use_relative_e_distances)
{
    constexpr int AxisZ = 2;
    GCodeG2G3Formatter arc_formatter(is_ccw);
    float start_z = position_start[AxisZ];
    if (std::abs(start_z) < GCodeFormatter::XYZ_EPSILON && s_last_nonzero_z > GCodeFormatter::XYZ_EPSILON)
        start_z = s_last_nonzero_z;

    for (int axis_idx = 0; axis_idx < 3; ++axis_idx) {
        float end_val = position_end[axis_idx];
        if (axis_idx == AxisZ) {
            if (std::abs(end_val) < GCodeFormatter::XYZ_EPSILON && start_z > GCodeFormatter::XYZ_EPSILON)
                end_val = start_z;
            if (end_val > GCodeFormatter::XYZ_EPSILON)
                s_last_nonzero_z = end_val;
        }
        float cmp_start = (axis_idx == AxisZ) ? start_z : position_start[axis_idx];
        if (cmp_start != end_val) {
            arc_formatter.emit_axis(char('X' + axis_idx), end_val, GCodeFormatter::XYZF_EXPORT_DIGITS);
        }
    }

    arc_formatter.emit_axis('I', ij_params.x(), GCodeFormatter::XYZF_EXPORT_DIGITS);
    arc_formatter.emit_axis('J', ij_params.y(), GCodeFormatter::XYZF_EXPORT_DIGITS);

    if (position_start[3] != position_end[3]) {
        arc_formatter.emit_axis('E', use_relative_e_distances ? (position_end[3] - position_start[3]) : position_end[3],
                                GCodeFormatter::E_EXPORT_DIGITS);
    }

    return arc_formatter.string();
}

static std::pair<std::string, std::string> split_linear_segment(const GCodeMoveSegment& segment,
                                                                const float             split_at_length,
                                                                const bool              use_relative_e_distances)
{
    assert(segment.length > 0.f && !segment.is_arc());

    const float t               = std::clamp(split_at_length / segment.length, 0.f, 1.f);
    const Vec4f position_middle = Slic3r::lerp(segment.position_start, segment.position_end, t);

    return {emit_extrusion(segment.position_start, position_middle, use_relative_e_distances),
            emit_extrusion(position_middle, segment.position_end, use_relative_e_distances)};
}

static std::pair<std::string, std::string> split_arc_segment(const GCodeMoveSegment& segment,
                                                             const float             split_at_length,
                                                             const bool              use_relative_e_distances)
{
    assert(segment.length > 0.f && segment.is_arc() && (segment.type & CoolingLine::TYPE_G2G3_IJ));

    const Vec2f position_start = segment.position_start.head<2>();
    const Vec2f position_end   = segment.position_end.head<2>();
    const Vec2f arc_center     = position_start + segment.ij_params;

    const Vec2f position_middle = Geometry::ArcWelder::arc_point_at_distance(position_start, position_end, arc_center, segment.is_ccw(),
                                                                             split_at_length);
    const float arc_length_val  = Geometry::ArcWelder::arc_length(position_start, position_end, arc_center, segment.is_ccw());
    const float t               = std::clamp(split_at_length / arc_length_val, 0.f, 1.f);

    const Vec4f position_middle_4d = Vec4f(position_middle.x(), position_middle.y(),
                                           Slic3r::lerp(segment.position_start.z(), segment.position_end.z(), t),
                                           Slic3r::lerp(segment.position_start.w(), segment.position_end.w(), t));

    const std::string first_part_gcode = emit_arc(segment.position_start, position_middle_4d, segment.ij_params, segment.is_ccw(),
                                                  use_relative_e_distances);

    const Vec2f       second_part_ij    = arc_center - position_middle;
    const std::string second_part_gcode = emit_arc(position_middle_4d, segment.position_end, second_part_ij, segment.is_ccw(),
                                                   use_relative_e_distances);

    return {first_part_gcode, second_part_gcode};
}






// Calculate a new feedrate when slowing down by time_stretch for segments faster than min_feedrate.
// Used by non-proportional slow down.
float new_feedrate_to_reach_time_stretch(
    std::vector<PerExtruderAdjustments*>::const_iterator it_begin, std::vector<PerExtruderAdjustments*>::const_iterator it_end, 
    float min_feedrate, float time_stretch, const AdjustableFeatureType additional_slowdown_features,size_t max_iter = 20)
{
	float new_feedrate = min_feedrate;
    for (size_t iter = 0; iter < max_iter; ++ iter) {
        double nomin = 0;
        double denom = time_stretch;
        for (auto it = it_begin; it != it_end; ++ it) {
			assert((*it)->slow_down_min_speed < min_feedrate + EPSILON);
			for (size_t i = 0; i < (*it)->n_lines_adjustable; ++i) {
				const CoolingLine &line = (*it)->lines[i];
                if (line.adjustable(additional_slowdown_features) && line.feedrate > min_feedrate) {
                    nomin += (double) line.adjustable_time * (double) line.feedrate;
                    denom += (double) line.adjustable_time;
                }
            }
        }
        assert(denom > 0);
        if (denom < 0)
            return min_feedrate;
        new_feedrate = (float)(nomin / denom);
        assert(new_feedrate > min_feedrate - EPSILON);
        if (new_feedrate < min_feedrate + EPSILON)
            goto finished;
        for (auto it = it_begin; it != it_end; ++ it)
			for (size_t i = 0; i < (*it)->n_lines_adjustable; ++i) {
				const CoolingLine &line = (*it)->lines[i];
                if (line.adjustable(additional_slowdown_features) && line.feedrate > min_feedrate && line.feedrate < new_feedrate)
                    // Some of the line segments taken into account in the calculation of nomin / denom are now slower than new_feedrate, 
                    // which makes the new_feedrate lower than it should be.
                    // Re-run the calculation with a new min_feedrate limit, so that the segments with current feedrate lower than new_feedrate
                    // are not taken into account.
                    goto not_finished_yet;
            }
        goto finished;
not_finished_yet:
        min_feedrate = new_feedrate;
    }
    // Failed to find the new feedrate for the time_stretch.

finished:
    // Test whether the time_stretch was achieved.
#ifndef NDEBUG
    {
        float time_stretch_final = 0.f;
        for (auto it = it_begin; it != it_end; ++ it)
            time_stretch_final += (*it)->time_stretch_when_slowing_down_to_feedrate(new_feedrate, additional_slowdown_features);
        //assert(std::abs(time_stretch - time_stretch_final) < EPSILON);
    }
#endif /* NDEBUG */

	return new_feedrate;
}

std::string GCodeEditor::process_layer(std::string&&                        gcode,
                                       const size_t                         layer_id,
                                       std::vector<PerExtruderAdjustments>& per_extruder_adjustments,
                                       const std::vector<int>&              object_label,
                                       const bool                           flush,
                                       const bool                           spiral_vase)
{
    if (layer_id == 0 && m_config.print_sequence == PrintSequence::ByObject && !s_forced_first_layer_z) {
        double target_z = m_config.initial_layer_print_height.value + m_config.z_offset.value;
        if (std::abs(target_z) < GCodeFormatter::XYZ_EPSILON)
            target_z = 0.2;
        s_last_nonzero_z = float(target_z);
        s_forced_z_value = float(target_z);
        s_force_z_on_next_xy_move = true;
        s_forced_first_layer_z = true;
    }

    // Cache the input G-code.
    if (m_gcode.empty())
        m_gcode = std::move(gcode);
    else
        m_gcode += gcode;

    std::string out;
    if (flush) {
        // This is either an object layer or the very last print layer. Calculate cool down over the collected support layers
        // and one object layer.
        // record parse gcode info to per_extruder_adjustments
        per_extruder_adjustments = this->parse_layer_gcode(m_gcode, m_current_pos, object_label, spiral_vase, layer_id > 0);
        out                      = m_gcode;
        m_gcode.clear();
    }
    return out;
}

// Parse the layer G-code for the moves, which could be adjusted.
// Return the list of parsed lines, bucketed by an extruder.
std::vector<PerExtruderAdjustments> GCodeEditor::parse_layer_gcode(
    const std::string& gcode, std::vector<float>& current_pos, const std::vector<int>& object_label, bool spiral_vase, bool join_z_smooth)
{
    std::vector<PerExtruderAdjustments> per_extruder_adjustments(m_extruder_ids.size());
    std::vector<size_t>                 map_extruder_to_per_extruder_adjustment(m_num_extruders, 0);
    for (size_t i = 0; i < m_extruder_ids.size(); ++ i) {
        PerExtruderAdjustments &adj         = per_extruder_adjustments[i];
        unsigned int            extruder_id = m_extruder_ids[i];
        adj.extruder_id               = extruder_id;
        adj.cooling_slow_down_enabled = m_config.slow_down_for_layer_cooling.get_at(extruder_id);
        adj.cooling_slowdown_logic    = CoolingSlowdownLogicType(m_config.cooling_slowdown_logic.get_at(extruder_id));
        adj.slow_down_layer_time      = float(m_config.slow_down_layer_time.get_at(extruder_id));
        adj.slow_down_min_speed       = float(m_config.slow_down_min_speed.get_at(extruder_id));
        adj.cooling_slowdown_non_slow_outer_wall = adj.cooling_slowdown_logic == CoolingSlowdownLogicType::NonSlowOuterWalls;
        adj.cooling_slowdown_smart_zone          = adj.cooling_slowdown_logic == CoolingSlowdownLogicType::SmartCoolingZones;
      
        const std::string filament_type = m_config.filament_type.get_at(extruder_id);
        if (adj.cooling_slowdown_logic == CoolingSlowdownLogicType::SmartCoolingZones) {
            if (filament_type == "PLA" || filament_type == "PETG" || filament_type == "ABS") {
                adj.cooling_slowdown_smart_zone = true;
            }
        }
        map_extruder_to_per_extruder_adjustment[extruder_id] = i;
    }

    unsigned int      current_extruder  = m_parse_gcode_extruder;
    PerExtruderAdjustments *adjustment  = &per_extruder_adjustments[map_extruder_to_per_extruder_adjustment[current_extruder]];
    const char       *line_start = gcode.c_str();
    const char       *line_end   = line_start;
    // Index of an existing CoolingLine of the current adjustment, which holds the feedrate setting command
    // for a sequence of extrusion moves.
    size_t            active_speed_modifier = size_t(-1);
    int         object_id             = -1;
    int         cooling_node_id       = -1;
    std::string object_id_string      = "; OBJECT_ID: ";
    std::string cooling_node_label    = "; COOLING_NODE: ";
    bool        append_wall_ptr       = false;
    bool        append_inner_wall_ptr = false;
    float       current_path_width    = 0.f;

    std::pair<int, int> node_pos;
    int                 line_idx = -1;
    for (; *line_start != 0; line_start = line_end) 
    {
        while (*line_end != '\n' && *line_end != 0)
            ++ line_end;
        // sline will not contain the trailing '\n'.
        std::string sline(line_start, line_end);
        // CoolingLine will contain the trailing '\n'.
        if (*line_end == '\n')
            ++ line_end;
        CoolingLine line(0, line_start - gcode.c_str(), line_end - gcode.c_str());
        if (boost::starts_with(sline, "G0 "))
            line.type = CoolingLine::TYPE_G0;
        else if (boost::starts_with(sline, "G1 "))
            line.type = CoolingLine::TYPE_G1;
        else if (boost::starts_with(sline, "G92 "))
            line.type = CoolingLine::TYPE_G92;
        else if (boost::starts_with(sline, "G2 "))
            line.type = CoolingLine::TYPE_G2;
        else if (boost::starts_with(sline, "G3 "))
            line.type = CoolingLine::TYPE_G3;
        // BBS: parse object id & node id
        else if (boost::starts_with(sline, object_id_string)) {
            std::string sub = sline.substr(object_id_string.size());
            object_id       = std::stoi(sub);
        } else if (boost::starts_with(sline, cooling_node_label)) {
            std::string sub = sline.substr(cooling_node_label.size());
            cooling_node_id = std::stoi(sub);
        } else if (boost::starts_with(sline, ";WIDTH:")) {
            fast_float::from_chars(sline.data() + 7, sline.data() + sline.size(), current_path_width);
        }

            if (line.type) {
                // G0, G1 or G92
                // Parse the G-code line.
                std::vector<float> new_pos(current_pos);
                const char *c = sline.data() + 3;
            for (;;) {
                // Skip whitespaces.
                for (; *c == ' ' || *c == '\t'; ++ c);
                if (*c == 0 || *c == ';')
                    break;

                assert(is_decimal_separator_point()); // for atof
                //BBS: Parse the axis.
                size_t axis = (*c >= 'X' && *c <= 'Z') ? (*c - 'X') :
                              (*c == 'E')              ? AxisIdx::E :
                              (*c == 'F')              ? AxisIdx::F :
                              (*c >= 'I' && *c <= 'K') ? int(AxisIdx::I) + (*c - 'I') :
                              (*c == 'R')              ? AxisIdx::R :
                                                         size_t(-1);

                if (axis != size_t(-1)) {
                    fast_float::from_chars(&*(++c), sline.data() + sline.size(), new_pos[axis]);
                    if (axis == AxisIdx::F) {
                        // Convert mm/min to mm/sec.
                        new_pos[AxisIdx::F] /= 60.f;
                        if ((line.type & CoolingLine::TYPE_G92) == 0)
                            // This is G0 or G1 line and it sets the feedrate. This mark is used for reducing the duplicate F calls.
                            line.type |= CoolingLine::TYPE_HAS_F;
                    } else if (axis >= AxisIdx::I && axis <= AxisIdx::J) 
                        // BBS: get position of arc center
                        //new_pos[axis] += current_pos[axis - 5];
                        line.type |= CoolingLine::TYPE_G2G3_IJ;
                    else if (axis == AxisIdx::R)
                        line.type |= CoolingLine::TYPE_G2G3_R;
                    
                }
                // Skip this word.
                for (; *c != ' ' && *c != '\t' && *c != 0; ++ c);
            }
            // Recover Z if missing / zero.
            if (std::abs(new_pos[AxisIdx::Z]) < GCodeFormatter::XYZ_EPSILON && s_last_nonzero_z > GCodeFormatter::XYZ_EPSILON)
                new_pos[AxisIdx::Z] = s_last_nonzero_z;
            else if (new_pos[AxisIdx::Z] > GCodeFormatter::XYZ_EPSILON)
                s_last_nonzero_z = new_pos[AxisIdx::Z];

            //// If G2 or G3, then either center of the arc or radius has to be defined.
            //assert(!(line.type & CoolingLine::TYPE_G2G3) || (line.type & (CoolingLine::TYPE_G2G3_IJ | CoolingLine::TYPE_G2G3_R)));
            //// Arc is defined either by IJ or by R, not by both.
            //assert(!((line.type & CoolingLine::TYPE_G2G3_IJ) && (line.type & CoolingLine::TYPE_G2G3_R)));

            bool external_perimeter = boost::contains(sline, ";_EXTERNAL_PERIMETER");
            bool wipe               = boost::contains(sline, ";_WIPE");
            auto internal_perimeter_it_range = boost::find_last(sline, ";_INTERNAL_PERIMETER");
            bool large_range		= boost::contains(sline, ";_LARGE_RANGE");

            record_wall_lines(append_inner_wall_ptr, line_idx, adjustment, node_pos);

            if (wipe)
                line.type |= CoolingLine::TYPE_WIPE;
            if (large_range)
                line.type |= CoolingLine::TYPE_LARGE_RANGE;
            
            // ORCA: Dont slowdown external perimeters for layer time feature
            // use the adjustment pointer to ensure the value for the current extruder (filament) is used.
            bool adjust_external = true;
            if (adjustment->cooling_slowdown_non_slow_outer_wall && external_perimeter)
                adjust_external = false;

            if (adjustment->cooling_slowdown_smart_zone && large_range)
                adjust_external = false;
            
            // ORCA: Dont slowdown external perimeters for layer time works by not marking the external perimeter as adjustable, 
            // hence the slowdown algorithm ignores it.
            if (boost::contains(sline, ";_EXTRUDE_SET_SPEED") && ! wipe && adjust_external) {
                line.type |= CoolingLine::TYPE_ADJUSTABLE;
                active_speed_modifier = adjustment->lines.size();
            }

            record_wall_lines(append_wall_ptr, line_idx, adjustment, node_pos);

            if (external_perimeter) {
                line.type |= CoolingLine::TYPE_EXTERNAL_PERIMETER;
                line.perimeter_index = 0;
                if (line.type & CoolingLine::TYPE_ADJUSTABLE && join_z_smooth && !spiral_vase) {
                    // BBS: collect outwall info
                    mark_node_pos(append_wall_ptr, line_idx, node_pos, object_label, cooling_node_id, object_id, adjustment);
                }
            } else if (!internal_perimeter_it_range.empty()) {
                uint16_t    perimetr_index = 0;
                const char* start_ptr      = sline.data() + std::distance(sline.begin(), internal_perimeter_it_range.end());
                const char* end_ptr        = sline.data() + sline.size();
                const auto  res            = std::from_chars(start_ptr, end_ptr, perimetr_index);
                if (res.ec == std::errc()) {
                    line.type |= perimetr_index == 1 ? CoolingLine::TYPE_FIRST_INTERNAL_PERIMETER : CoolingLine::TYPE_INTERNAL_PERIMETER;
                    line.perimeter_index = perimetr_index;
                }
            }

            if ((line.type & CoolingLine::TYPE_G92) == 0) {
                //BBS: G0, G1, G2, G3. Calculate the duration.
                if (m_config.use_relative_e_distances.value)
                    // Reset extruder accumulator.
                    current_pos[AxisIdx::E] = 0.f;
                float dif[4];
                for (size_t i = 0; i < 4; ++ i)
                    dif[i] = new_pos[i] - current_pos[i];
                float dxy2 = 0;
                if (line.type & CoolingLine::TYPE_G2) {
                    // Measure arc length.
                    if (line.type & CoolingLine::TYPE_G2G3_IJ) {
                        dxy2 = sqr(Geometry::ArcWelder::arc_length(Vec2d(current_pos[AxisIdx::X], current_pos[AxisIdx::Y]),
                                                                   Vec2d(new_pos[AxisIdx::X], new_pos[AxisIdx::Y]),
                                                                   Vec2d(current_pos[AxisIdx::X] + new_pos[AxisIdx::I],
                                                                         current_pos[AxisIdx::Y] + new_pos[AxisIdx::J]),
                                                                   line.type & CoolingLine::TYPE_G3));
                    } else if (line.type & CoolingLine::TYPE_G2G3_R) {
                        dxy2 = sqr(Geometry::ArcWelder::arc_length(Vec2d(current_pos[AxisIdx::X], current_pos[AxisIdx::Y]),
                                                                   Vec2d(new_pos[AxisIdx::X], new_pos[AxisIdx::Y]),
                                                                   double(new_pos[AxisIdx::R])));
                    } else
                        dxy2 = 0;
                } else
                    dxy2 = sqr(dif[AxisIdx::X]) + sqr(dif[AxisIdx::Y]);

                const bool is_adjustable = (line.type & CoolingLine::TYPE_ADJUSTABLE) || active_speed_modifier != size_t(-1);
                float      dxyz2         = dxy2 + sqr(dif[AxisIdx::Z]);
                if (dxyz2 > 0.f) {
                    // Movement in xyz, calculate time from the xyz Euclidian distance.
                    if (is_adjustable)
                    {
                        line.adjustable_length = sqrt(dxyz2);
                        line.non_adjustable_length = 0.f;
                    } else {
                        line.adjustable_length = 0.f;
                        line.non_adjustable_length = sqrt(dxyz2);
                    }
                } else if (std::abs(dif[AxisIdx::E]) > 0.f) {
                    // Movement in the extruder axis.
                    if (is_adjustable) {
                        line.adjustable_length     = std::abs(dif[AxisIdx::E]);
                        line.non_adjustable_length = 0.f;
                    } else {
                        line.adjustable_length     = 0.f;
                        line.non_adjustable_length = std::abs(dif[AxisIdx::E]);
                    }
                }
                line.feedrate = new_pos[AxisIdx::F];
                line.origin_feedrate = new_pos[AxisIdx::F];

                assert((line.type & CoolingLine::TYPE_ADJUSTABLE) == 0 || line.feedrate > 0.f);
                if (line.length() > 0 && line.feedrate > 0)
                {
                    if (is_adjustable) {
                        line.adjustable_time = line.length() / line.feedrate;
                        line.non_adjustable_time = 0.f;
                    } else {
                        line.adjustable_time = 0.f;
                        line.non_adjustable_time = line.length() / line.feedrate;
                    }
                    assert(line.time() > 0);
                }

                if (line.feedrate == 0)
                    line.adjustable_time = line.non_adjustable_time = 0;

                if (is_adjustable) {
                    assert(adjustment->slow_down_min_speed >= 0);
                    line.adjustable_time_max = (adjustment->slow_down_min_speed == 0.f) ?
                                                   FLT_MAX :
                                                   std::max(line.time(), line.length() / adjustment->slow_down_min_speed);
                } else {
                    line.adjustable_time_max = 0.f;
                }
                line.origin_time_max = line.time_max();

                // BBS: add G2 and G3 support
                if (active_speed_modifier < adjustment->lines.size() && ((line.type & CoolingLine::TYPE_G1) ||
                                                                         (line.type & CoolingLine::TYPE_G2) ||
                                                                         (line.type & CoolingLine::TYPE_G3))) {
                    // Inside the ";_EXTRUDE_SET_SPEED" blocks, there must not be a G1 Fxx entry.
                    assert((line.type & CoolingLine::TYPE_HAS_F) == 0);
                    CoolingLine &sm = adjustment->lines[active_speed_modifier];
                    assert(sm.feedrate > 0.f);
                    sm.adjustable_length   += line.adjustable_length;
                    sm.adjustable_time     += line.adjustable_time;
                    if (sm.adjustable_time_max != FLT_MAX) {
                        if (line.adjustable_time_max == FLT_MAX)
                            sm.adjustable_time_max = FLT_MAX;
                        else
                            sm.adjustable_time_max += line.adjustable_time_max;

                        sm.origin_time_max = sm.time_max();
                    }

                    const Vec4f current_pos_v = Vec4f(current_pos[AxisIdx::X], current_pos[AxisIdx::Y], current_pos[AxisIdx::Z],
                                                      current_pos[AxisIdx::E]);
                    const Vec4f new_pos_v     = Vec4f(new_pos[AxisIdx::X], new_pos[AxisIdx::Y], new_pos[AxisIdx::Z], new_pos[AxisIdx::E]);

                    if (line.type & CoolingLine::TYPE_G2G3_IJ) {
                        sm.move_segments.emplace_back(current_pos_v, new_pos_v, line.length(), line.line_start, line.line_end, line.type,
                                                      Vec2f(new_pos[AxisIdx::I], new_pos[AxisIdx::J]));
                    } else {
                        assert(line.type & CoolingLine::TYPE_G1);
                        sm.move_segments.emplace_back(current_pos_v, new_pos_v, line.length(), line.line_start, line.line_end, line.type);
                    }

                    // Don't store this line.
                    line.type = 0;
                }
            }
            current_pos = std::move(new_pos);
            if (m_config.use_relative_e_distances.value && !m_config.z_direction_outwall_speed_continuous.value) {
                // Reset extruder accumulator.
                current_pos[AxisIdx::E] = 0.f;
            }
        } else if (boost::starts_with(sline, ";_EXTRUDE_END")) {
            line.type = CoolingLine::TYPE_EXTRUDE_END;
            if (active_speed_modifier != size_t(-1)) {
                assert(active_speed_modifier < adjustment->lines.size());
                CoolingLine& sm = adjustment->lines[active_speed_modifier];
                // There should be at least some extrusion move inside the adjustment block.
                // However if the block has no extrusion (which is wrong), fix it for the cooling buffer to work.
                // assert(sm.length() > 0);
                // assert(sm.time() > 0);
                if (sm.time() <= 0) {
                    // Likely a zero length extrusion, it should not be emitted, however the zero extrusions should not confuse firmware
                    // either. Prohibit time adjustment of a block of zero length extrusions by the cooling buffer.
                    sm.type &= ~CoolingLine::TYPE_ADJUSTABLE;
                    // But the start / end comment shall be removed.
                    sm.type |= CoolingLine::TYPE_ADJUSTABLE_EMPTY;
                }
            }
            active_speed_modifier = size_t(-1);
        } else if (boost::starts_with(sline, m_toolchange_prefix)) {
            unsigned int new_extruder = 0;
            auto ret = std::from_chars(sline.data() + m_toolchange_prefix.size(), sline.data() + sline.size(), new_extruder);
            if (std::errc::invalid_argument != ret.ec) {
                // Only change extruder in case the number is meaningful. User could provide an out-of-range index through custom gcodes -
                // those shall be ignored.
                if (new_extruder < map_extruder_to_per_extruder_adjustment.size()) {
                    if (new_extruder != current_extruder) {
                        // Switch the tool.
                        line.type        = CoolingLine::TYPE_SET_TOOL;
                        current_extruder = new_extruder;
                        adjustment       = &per_extruder_adjustments[map_extruder_to_per_extruder_adjustment[current_extruder]];
                    }
                } else {
                    // Only log the error in case of MM printer. Single extruder printers likely ignore any T anyway.
                    if (map_extruder_to_per_extruder_adjustment.size() > 1)
                        BOOST_LOG_TRIVIAL(error) << "CoolingBuffer encountered an invalid toolchange, maybe from a custom gcode: " << sline;
                }
            }
        } else if (boost::starts_with(sline, ";_OVERHANG_FAN_START")) {
            line.type = CoolingLine::TYPE_OVERHANG_FAN_START;
            line.overhang_fan_width = current_path_width;
        } else if (boost::starts_with(sline, ";_OVERHANG_FAN_END")) {
            line.type = CoolingLine::TYPE_OVERHANG_FAN_END;
            line.overhang_fan_width = current_path_width;
        } else if (boost::starts_with(sline, ";_SUPP_INTERFACE_FAN_START")) {
            line.type = CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_START;
        } else if (boost::starts_with(sline, ";_SUPP_INTERFACE_FAN_END")) {
            line.type = CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_END;
        } else if (boost::starts_with(sline, "G4 ")) {
            // Parse the wait time.
            line.type = CoolingLine::TYPE_G4;
            size_t pos_S = sline.find('S', 3);
            size_t pos_P = sline.find('P', 3);
            bool   has_S = pos_S > 0;
            bool   has_P = pos_P > 0;
            assert(is_decimal_separator_point()); // for atof
           /* line.time = line.time_max = float(
                (pos_S > 0) ? atof(sline.c_str() + pos_S + 1) :
                (pos_P > 0) ? atof(sline.c_str() + pos_P + 1) * 0.001 : 0.);*/
            if (has_S || has_P) {
                // auto [pend, ec] =
                fast_float::from_chars(sline.data() + (has_S ? pos_S : pos_P) + 1, sline.data() + sline.size(), line.non_adjustable_time);
                if (has_P)
                    line.non_adjustable_time *= 0.001f;
            } else {
                line.non_adjustable_time = 0.f;
            }
            line.origin_time_max = line.time_max();

            line.adjustable_time     = 0.f;
            line.adjustable_time_max = 0.f;
        } else if (boost::starts_with(sline, ";_FORCE_RESUME_FAN_SPEED")) {
            line.type = CoolingLine::TYPE_FORCE_RESUME_FAN;
        }
        if (line.type != 0)
            adjustment->lines.emplace_back(std::move(line));
    }
    m_parse_gcode_extruder = current_extruder;

    for (PerExtruderAdjustments& adj : per_extruder_adjustments) {
        const double perimeter_transition_distance = m_config.cooling_perimeter_transition_distance.get_at(m_current_extruder);
        if (adj.cooling_slowdown_logic == CoolingSlowdownLogicType::ConsistentSurface && perimeter_transition_distance >= 0.) {
            // Create non-adjustable segments for ConsistentSurface logic before sorting.
            adj.create_non_adjustable_segments(static_cast<float>(perimeter_transition_distance));
        }
    }

    return per_extruder_adjustments;
}

// Slow down an extruder range proportionally down to slow_down_layer_time.
// Return the total time for the complete layer.
static inline float extruder_range_slow_down_proportional(
    std::vector<PerExtruderAdjustments*>::iterator it_begin,
    std::vector<PerExtruderAdjustments*>::iterator it_end,
    // Elapsed time for the extruders already processed.
    float elapsed_time_total0,
    // Initial total elapsed time before slow down.
    float elapsed_time_before_slowdown,
    // Target time for the complete layer (all extruders applied).
    float slow_down_layer_time)
{
    // Total layer time after the slow down has been applied.
    float total_after_slowdown = elapsed_time_before_slowdown;
    // Now decide, whether the external perimeters shall be slowed down as well.
    float max_time_nep = elapsed_time_total0;
    for (auto it = it_begin; it != it_end; ++ it)
        max_time_nep += (*it)->maximum_time_after_slowdown(AdjustableFeatureType::None);
    if (max_time_nep > slow_down_layer_time) {
        // It is sufficient to slow down the non-external perimeter moves to reach the target layer time.
        // Slow down the non-external perimeters proportionally.
        float non_adjustable_time = elapsed_time_total0;
        for (auto it = it_begin; it != it_end; ++ it)
            non_adjustable_time += (*it)->non_adjustable_time(AdjustableFeatureType::None);
        // The following step is a linear programming task due to the minimum movement speeds of the print moves.
        // Run maximum 5 iterations until a good enough approximation is reached.
        for (size_t iter = 0; iter < 5; ++ iter) {
            float factor = (slow_down_layer_time - non_adjustable_time) / (total_after_slowdown - non_adjustable_time);
            assert(factor > 1.f);
            total_after_slowdown = elapsed_time_total0;
            for (auto it = it_begin; it != it_end; ++ it)
                total_after_slowdown += (*it)->slow_down_proportional(factor, AdjustableFeatureType::None);
            if (total_after_slowdown > 0.95f * slow_down_layer_time)
                break;
        }
    } else {
        // Slow down everything. First slow down the non-external perimeters to maximum.
        for (auto it = it_begin; it != it_end; ++ it)
            (*it)->slowdown_to_minimum_feedrate(AdjustableFeatureType::None);
        // Slow down the external perimeters proportionally.
        float non_adjustable_time = elapsed_time_total0;
        for (auto it = it_begin; it != it_end; ++ it)
            non_adjustable_time += (*it)->non_adjustable_time(AdjustableFeatureType::ExternalPerimeters | AdjustableFeatureType::FirstInternalPerimeters);
        for (size_t iter = 0; iter < 5; ++ iter) {
            float factor = (slow_down_layer_time - non_adjustable_time) / (total_after_slowdown - non_adjustable_time);
            assert(factor > 1.f);
            total_after_slowdown = elapsed_time_total0;
            for (auto it = it_begin; it != it_end; ++ it)
                total_after_slowdown += (*it)->slow_down_proportional(factor, AdjustableFeatureType::ExternalPerimeters | AdjustableFeatureType::FirstInternalPerimeters);
            if (total_after_slowdown > 0.95f * slow_down_layer_time)
                break;
        }
    }
    return total_after_slowdown;
}

// Slow down an extruder range to slow_down_layer_time.
// Return the total time for the complete layer.
static inline float extruder_range_slow_down_non_proportional(
    std::vector<PerExtruderAdjustments*>::iterator it_begin,
    std::vector<PerExtruderAdjustments*>::iterator it_end,
    float time_stretch,
    const AdjustableFeatureType                    additional_slowdown_features)
{
    // Slow down. Try to equalize the feedrates.
    std::vector<PerExtruderAdjustments*> by_min_print_speed(it_begin, it_end);
    // Find the next highest adjustable feedrate among the extruders.
    float feedrate = 0;
    for (PerExtruderAdjustments *adj : by_min_print_speed) {
        adj->idx_line_begin = 0;
        adj->idx_line_end   = 0;
        assert(adj->idx_line_begin < adj->n_lines_adjustable);
        if (adj->lines[adj->idx_line_begin].feedrate > feedrate)
            feedrate = adj->lines[adj->idx_line_begin].feedrate;
    }
    assert(feedrate > 0.f);
    // Sort by slow_down_min_speed, maximum speed first.
    std::sort(by_min_print_speed.begin(), by_min_print_speed.end(), 
        [](const PerExtruderAdjustments *p1, const PerExtruderAdjustments *p2){ return p1->slow_down_min_speed > p2->slow_down_min_speed; });
    // Slow down, fast moves first.
    for (;;) {
        // For each extruder, find the span of lines with a feedrate close to feedrate.
        for (PerExtruderAdjustments *adj : by_min_print_speed) {
            for (adj->idx_line_end = adj->idx_line_begin;
                adj->idx_line_end < adj->n_lines_adjustable && adj->lines[adj->idx_line_end].feedrate > feedrate - EPSILON;
                 ++ adj->idx_line_end) ;
        }
        // Find the next highest adjustable feedrate among the extruders.
        float feedrate_next = 0.f;
        for (PerExtruderAdjustments *adj : by_min_print_speed)
            if (adj->idx_line_end < adj->n_lines_adjustable && adj->lines[adj->idx_line_end].feedrate > feedrate_next)
                feedrate_next = adj->lines[adj->idx_line_end].feedrate;
        // Slow down, limited by max(feedrate_next, slow_down_min_speed).
        for (auto adj = by_min_print_speed.begin(); adj != by_min_print_speed.end();) {
            // Slow down at most by time_stretch.
            if ((*adj)->slow_down_min_speed == 0.f) {
                // All the adjustable speeds are now lowered to the same speed,
                // and the minimum speed is set to zero.
                float time_adjustable = 0.f;
                for (auto it = adj; it != by_min_print_speed.end(); ++ it)
                    time_adjustable += (*it)->adjustable_time(additional_slowdown_features);
                float rate = (time_adjustable + time_stretch) / time_adjustable;
                for (auto it = adj; it != by_min_print_speed.end(); ++ it)
                    (*it)->slow_down_proportional(rate, additional_slowdown_features);
                return 0.f;
            } else {
                float feedrate_limit = std::max(feedrate_next, (*adj)->slow_down_min_speed);
                bool  done           = false;
                float time_stretch_max = 0.f;
                for (auto it = adj; it != by_min_print_speed.end(); ++ it)
                    time_stretch_max += (*it)->time_stretch_when_slowing_down_to_feedrate(feedrate_limit, additional_slowdown_features);
                if (time_stretch_max >= time_stretch) {
                    feedrate_limit = new_feedrate_to_reach_time_stretch(adj, by_min_print_speed.end(), feedrate_limit, time_stretch, additional_slowdown_features, 20);
                    done = true;
                } else
                    time_stretch -= time_stretch_max;
                for (auto it = adj; it != by_min_print_speed.end(); ++ it)
                    (*it)->slow_down_to_feedrate(feedrate_limit, additional_slowdown_features);
                if (done)
                    return 0.f;
            }
            // Skip the other extruders with nearly the same slow_down_min_speed, as they have been processed already.
            auto next = adj;
            for (++ next; next != by_min_print_speed.end() && (*next)->slow_down_min_speed > (*adj)->slow_down_min_speed - EPSILON; ++ next);
            adj = next;
        }
        if (feedrate_next == 0.f)
            // There are no other extrusions available for slow down.
            break;
        for (PerExtruderAdjustments *adj : by_min_print_speed) {
            adj->idx_line_begin = adj->idx_line_end;
            feedrate = feedrate_next;
        }
    }
    return time_stretch;
}

// Calculate slow down for all the extruders.
float CoolingBuffer::calculate_layer_slowdown(std::vector<PerExtruderAdjustments> &per_extruder_adjustments)
{
    // Sort the extruders by an increasing slow_down_layer_time.
    // The layers with a lower slow_down_layer_time are slowed down
    // together with all the other layers with slow_down_layer_time above.
    std::vector<PerExtruderAdjustments*> by_slowdown_time;
    by_slowdown_time.reserve(per_extruder_adjustments.size());
    // Only insert entries, which are adjustable (have cooling enabled and non-zero stretchable time).
    // Collect total print time of non-adjustable extruders.
    float elapsed_time_total0 = 0.f;
    for (PerExtruderAdjustments &adj : per_extruder_adjustments) {
        // Curren total time for this extruder.
        adj.time_total  = adj.elapsed_time_total();
        // Maximum time for this extruder, when all extrusion moves are slowed down to min_extrusion_speed.
        adj.time_maximum = adj.maximum_time_after_slowdown(AdjustableFeatureType::ExternalPerimeters | AdjustableFeatureType::FirstInternalPerimeters);
        if (adj.cooling_slow_down_enabled && adj.lines.size() > 0) {
            by_slowdown_time.emplace_back(&adj);
            if (adj.cooling_slowdown_logic != CoolingSlowdownLogicType::Proportional)
                // sorts the lines, also sets adj.time_non_adjustable
                adj.sort_lines_by_decreasing_feedrate();
        } else
            elapsed_time_total0 += adj.elapsed_time_total();
    }
    std::sort(by_slowdown_time.begin(), by_slowdown_time.end(),
        [](const PerExtruderAdjustments *adj1, const PerExtruderAdjustments *adj2)
            { return adj1->slow_down_layer_time < adj2->slow_down_layer_time; });

    for (auto cur_begin = by_slowdown_time.begin(); cur_begin != by_slowdown_time.end(); ++ cur_begin) {
        PerExtruderAdjustments &adj = *(*cur_begin);
        // Calculate the current adjusted elapsed_time_total over the non-finalized extruders.
        float total = elapsed_time_total0;
        for (auto it = cur_begin; it != by_slowdown_time.end(); ++ it)
            total += (*it)->time_total;
        float slow_down_layer_time = adj.slow_down_layer_time * 1.001f;
        if (total > slow_down_layer_time) {
            // The current total time is above the minimum threshold of the rest of the extruders, don't adjust anything.
        } else {
            // Adjust this and all the following (higher m_config.slow_down_layer_time) extruders.
            // Sum maximum slow down time as if everything was slowed down including the external perimeters.
            float max_time = elapsed_time_total0;
            for (auto it = cur_begin; it != by_slowdown_time.end(); ++ it)
                max_time += (*it)->time_maximum;
            if (max_time > slow_down_layer_time) {
                /*if (m_cooling_logic_proportional)
                    extruder_range_slow_down_proportional(cur_begin, by_slowdown_time.end(), elapsed_time_total0, total, slow_down_layer_time);
                else
                    extruder_range_slow_down_non_proportional(cur_begin, by_slowdown_time.end(), slow_down_layer_time - total);*/
                if (adj.cooling_slowdown_logic == CoolingSlowdownLogicType::Proportional) {
                    extruder_range_slow_down_proportional(cur_begin, by_slowdown_time.end(), elapsed_time_total0, total,slow_down_layer_time);
                } else if (adj.cooling_slowdown_logic == CoolingSlowdownLogicType::ConsistentSurface) {
                    const float remaining_time_stretch = extruder_range_slow_down_non_proportional(cur_begin, by_slowdown_time.end(),slow_down_layer_time - total, AdjustableFeatureType::None);
                    if (remaining_time_stretch > 0.f) {
                        // We didn't achieve the requested time for the layer, so we allow a slowdown on the external and first internal perimeter.
                        extruder_range_slow_down_non_proportional(cur_begin, by_slowdown_time.end(), remaining_time_stretch,AdjustableFeatureType::ExternalPerimeters | AdjustableFeatureType::FirstInternalPerimeters);
                    }
                } else {
                    extruder_range_slow_down_non_proportional(cur_begin, by_slowdown_time.end(), slow_down_layer_time - total,AdjustableFeatureType::ExternalPerimeters | AdjustableFeatureType::FirstInternalPerimeters);
                }
            } else {
                // Slow down to maximum possible.
                for (auto it = cur_begin; it != by_slowdown_time.end(); ++ it)
                    (*it)->slowdown_to_minimum_feedrate(AdjustableFeatureType::ExternalPerimeters | AdjustableFeatureType::FirstInternalPerimeters);
            }
        }
        elapsed_time_total0 += adj.elapsed_time_total();
    }

    return elapsed_time_total0;
}

// Apply slow down over G-code lines stored in per_extruder_adjustments, enable fan if needed.
// Returns the adjusted G-code.
std::string GCodeEditor::write_layer_gcode(
    // Source G-code for the current layer.
    const std::string                      &gcode,
    // ID of the current layer, used to disable fan for the first n layers.
    size_t                                  layer_id, 
    // Total time of this layer after slow down, used to control the fan.
    float                                   layer_time,
    // Per extruder list of G-code lines and their cool down attributes.
    std::vector<PerExtruderAdjustments>    &per_extruder_adjustments)
{
    if (gcode.empty())
        return gcode;

    // First sort the adjustment lines by of multiple extruders by their position in the source G-code.
    std::vector<const CoolingLine*> lines;
    {
        size_t n_lines = 0;
        for (const PerExtruderAdjustments &adj : per_extruder_adjustments)
            n_lines += adj.lines.size();
        lines.reserve(n_lines);
        for (const PerExtruderAdjustments &adj : per_extruder_adjustments)
            for (const CoolingLine &line : adj.lines)
                lines.emplace_back(&line);
        std::sort(lines.begin(), lines.end(), [](const CoolingLine *ln1, const CoolingLine *ln2) { return ln1->line_start < ln2->line_start; } );
    }
    const std::unordered_set<const CoolingLine*> disabled_overhang_fan_markers = collect_disabled_overhang_fan_markers(lines);
    // Second generate the adjusted G-code.
    std::string new_gcode;
    new_gcode.reserve(gcode.size() * 2);
    bool overhang_fan_control= false;
    int  overhang_fan_speed   = 0;
    bool supp_interface_fan_control= false;
    int  supp_interface_fan_speed = 0;

#define EXTRUDER_CONFIG(OPT) m_config.OPT.get_at(m_current_extruder)
    //limit for height
    bool limit_height_fan = true;

    bool need_set_cds_fan = false;
    if (m_current_pos.size() > 2)
    {
        limit_height_fan = m_current_pos[2] >= EXTRUDER_CONFIG(cool_cds_fan_start_at_height);
    }
    auto change_extruder_set_fan = [ this, layer_id, layer_time, &new_gcode, &overhang_fan_control, &overhang_fan_speed, &supp_interface_fan_control, &supp_interface_fan_speed,&limit_height_fan](bool immediately_apply) {

        float fan_min_speed = EXTRUDER_CONFIG(fan_min_speed);
        float fan_speed_new = EXTRUDER_CONFIG(reduce_fan_stop_start_freq) ? fan_min_speed : 0;
        //BBS
        int additional_fan_speed_new = EXTRUDER_CONFIG(additional_cooling_fan_speed);
        int close_fan_the_first_x_layers = EXTRUDER_CONFIG(close_fan_the_first_x_layers);
        // Is the fan speed ramp enabled?
        int full_fan_speed_layer = EXTRUDER_CONFIG(full_fan_speed_layer);
        supp_interface_fan_speed = EXTRUDER_CONFIG(support_material_interface_fan_speed);

        if (close_fan_the_first_x_layers <= 0 && full_fan_speed_layer > 0) {
            // When ramping up fan speed from close_fan_the_first_x_layers to full_fan_speed_layer, force close_fan_the_first_x_layers above zero,
            // so there will be a zero fan speed at least at the 1st layer.
            close_fan_the_first_x_layers = 1;
        }
        if (int(layer_id) >= close_fan_the_first_x_layers) {
            float   fan_max_speed             = EXTRUDER_CONFIG(fan_max_speed);
            float slow_down_layer_time = float(EXTRUDER_CONFIG(slow_down_layer_time));
            float fan_cooling_layer_time      = float(EXTRUDER_CONFIG(fan_cooling_layer_time));
            //BBS: always enable the fan speed interpolation according to layer time
            //if (EXTRUDER_CONFIG(cooling)) {
                if (layer_time < slow_down_layer_time) {


                    // Layer time very short. Enable the fan to a full throttle.
                    fan_speed_new = fan_max_speed;
                } else if (layer_time < fan_cooling_layer_time) {
                    // Layer time quite short. Enable the fan proportionally according to the current layer time.
                    assert(layer_time >= slow_down_layer_time);
                    double t = (layer_time - slow_down_layer_time) / (fan_cooling_layer_time - slow_down_layer_time);
                    fan_speed_new = int(floor(t * fan_min_speed + (1. - t) * fan_max_speed) + 0.5);
                }
            //}
            overhang_fan_speed   = EXTRUDER_CONFIG(overhang_fan_speed);
            if (int(layer_id) >= close_fan_the_first_x_layers && int(layer_id) + 1 < full_fan_speed_layer) {
                // Ramp up the fan speed from close_fan_the_first_x_layers to full_fan_speed_layer.
                float factor = float(int(layer_id + 1) - close_fan_the_first_x_layers) / float(full_fan_speed_layer - close_fan_the_first_x_layers);
                fan_speed_new    = std::clamp(int(float(fan_speed_new) * factor + 0.5f), 0, 255);
                overhang_fan_speed = std::clamp(int(float(overhang_fan_speed) * factor + 0.5f), 0, 255);
            }
            supp_interface_fan_speed = EXTRUDER_CONFIG(support_material_interface_fan_speed);
            supp_interface_fan_control = supp_interface_fan_speed >= 0;

//#undef EXTRUDER_CONFIG
            overhang_fan_control= overhang_fan_speed > fan_speed_new;
        } else {
            overhang_fan_control= false;
            overhang_fan_speed   = 0;
            fan_speed_new      = 0;
            additional_fan_speed_new = 0;
            supp_interface_fan_control= false;
            supp_interface_fan_speed   = 0;
        }
        if (fan_speed_new != m_fan_speed) {
            m_fan_speed = fan_speed_new;
            m_current_fan_speed = fan_speed_new;
            if (immediately_apply)
                new_gcode  += GCodeWriter::set_fan(m_config.gcode_flavor, m_fan_speed);
        }
        if (m_fan_speed != m_current_fan_speed)
        {
            m_current_fan_speed = m_fan_speed;
            new_gcode += GCodeWriter::set_fan(m_config.gcode_flavor,m_fan_speed);
           
        }
        //BBS
        if (!EXTRUDER_CONFIG(enable_special_area_additional_cooling_fan) && limit_height_fan)
        {
            if (additional_fan_speed_new != m_additional_fan_speed) {
                m_additional_fan_speed = additional_fan_speed_new;
                if (immediately_apply && m_config.auxiliary_fan.value)
                    new_gcode += GCodeWriter::set_additional_fan(m_additional_fan_speed);
            }
        }
    };

    const char         *pos               = gcode.c_str();
    int                 current_feedrate  = 0;
    change_extruder_set_fan(true);

    // Orca: Reduce set fan commands by deferring the GCodeWriter::set_fan calls. Inspired by SuperSlicer
    // define fan_speed_change_requests and initialize it with all possible types fan speed change requests
    std::unordered_map<int, bool> fan_speed_change_requests = {{CoolingLine::TYPE_OVERHANG_FAN_START, false},
                                                               {CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_START, false},
                                                               {CoolingLine::TYPE_FORCE_RESUME_FAN, false},
                                                               {CoolingLine::TYPE_OVERHANG_FAN_END,false }};
    bool need_set_fan = false;
    bool have_type_overhang = false;
    const CoolingLine* line_waiting_for_split = nullptr;
    for (const CoolingLine *line : lines) {
        const char *line_start  = gcode.c_str() + line->line_start;
        const char *line_end    = gcode.c_str() + line->line_end;
        if (line_start > pos) {
            if (line_waiting_for_split != nullptr && line_waiting_for_split->move_segments.front().line_start == (pos - gcode.c_str()) &&
                line_waiting_for_split->move_segments.back().line_end == line->line_start) {
                assert(!line_waiting_for_split->move_segments.empty());

                // We need to split CoolingLine into two parts with different feedrates.
                const float segments_total_length    = std::accumulate(line_waiting_for_split->move_segments.begin(),
                                                                       line_waiting_for_split->move_segments.end(), 0.f,
                                                                       [](float sum, const GCodeMoveSegment& segment) {
                                                                        return sum + segment.length;
                                                                    });
                const float split_segments_at_length = segments_total_length - line_waiting_for_split->non_adjustable_length;

                float accumulated_length = 0.f;
                auto  segment_it         = line_waiting_for_split->move_segments.cbegin();
                for (; segment_it != line_waiting_for_split->move_segments.cend() &&
                       accumulated_length + segment_it->length <= split_segments_at_length;
                     ++segment_it) {
                    new_gcode.append(gcode.c_str() + segment_it->line_start, segment_it->line_end - segment_it->line_start);
                    accumulated_length += segment_it->length;
                }

                if (segment_it != line_waiting_for_split->move_segments.cend()) {
                    const GCodeMoveSegment& segment         = *segment_it;
                    const float             split_at_length = split_segments_at_length - accumulated_length;

                    if (split_at_length >= segment.length - SEGMENT_SPLIT_EPSILON) {
                        new_gcode.append(gcode.c_str() + segment_it->line_start, segment_it->line_end - segment_it->line_start);
                        ++segment_it;

                        if (segment_it != line_waiting_for_split->move_segments.cend()) {
                            new_gcode.append(emit_feedrate(line_waiting_for_split->origin_feedrate));
                        }
                    } else if (split_at_length <= SEGMENT_SPLIT_EPSILON) {
                        new_gcode.append(emit_feedrate(line_waiting_for_split->origin_feedrate));
                        new_gcode.append(gcode.c_str() + segment_it->line_start, segment_it->line_end - segment_it->line_start);
                        ++segment_it;
                    } else {
                        const std::pair<std::string, std::string> segment_parts =
                            segment.is_arc() ? split_arc_segment(segment, split_at_length, m_config.use_relative_e_distances) :
                                               split_linear_segment(segment, split_at_length, m_config.use_relative_e_distances);
                        new_gcode += segment_parts.first;

                        // Change the feedrate for the second part.
                        new_gcode.append(emit_feedrate(line_waiting_for_split->origin_feedrate));

                        new_gcode += segment_parts.second;

                        ++segment_it;
                    }
                }

                for (; segment_it != line_waiting_for_split->move_segments.end(); ++segment_it) {
                    new_gcode.append(gcode.c_str() + segment_it->line_start, segment_it->line_end - segment_it->line_start);
                }

                line_waiting_for_split = nullptr;
            } else {
                new_gcode.append(pos, line_start - pos);
            }
        }

        if (!disabled_overhang_fan_markers.empty() && disabled_overhang_fan_markers.find(line) != disabled_overhang_fan_markers.end()) {
            // Remove marker: this candidate overhang-fan region was merged away or filtered as isolated.
        } else if (line->type & CoolingLine::TYPE_SET_TOOL) {
            unsigned int new_extruder = 0;
            auto ret = std::from_chars(line_start + m_toolchange_prefix.size(), line_end, new_extruder);
            if (std::errc::invalid_argument != ret.ec) {
                if (new_extruder != m_current_extruder) {
                    m_current_extruder = new_extruder;
                    change_extruder_set_fan(true);
                }
            }
            new_gcode.append(line_start, line_end - line_start);
        } else if (line->type & CoolingLine::TYPE_OVERHANG_FAN_START) {
            have_type_overhang = true;
            if (overhang_fan_control && !fan_speed_change_requests[CoolingLine::TYPE_OVERHANG_FAN_START]) {
                need_set_fan = true;
                fan_speed_change_requests[CoolingLine::TYPE_OVERHANG_FAN_START] = true;
                fan_speed_change_requests[CoolingLine::TYPE_OVERHANG_FAN_END]   = false;
           }
        } else if (line->type & CoolingLine::TYPE_OVERHANG_FAN_END) {
            if (overhang_fan_control && fan_speed_change_requests[CoolingLine::TYPE_OVERHANG_FAN_START]) {
                fan_speed_change_requests[CoolingLine::TYPE_OVERHANG_FAN_START] = false;
            }
            fan_speed_change_requests[CoolingLine::TYPE_OVERHANG_FAN_END] = true;
            need_set_fan = true;
        } else if (line->type & CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_START) {
            if (supp_interface_fan_control && !fan_speed_change_requests[CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_START]) {
                fan_speed_change_requests[CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_START] = true;
                need_set_fan = true;
            }
        } else if (line->type & CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_END && fan_speed_change_requests[CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_START]) {
            if (supp_interface_fan_control) {
                fan_speed_change_requests[CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_START] = false;
            }
            need_set_fan = true;
        } else if (line->type & CoolingLine::TYPE_FORCE_RESUME_FAN) {
            // check if any fan speed change request is active
            if (m_fan_speed != -1 /*&& !std::any_of(fan_speed_change_requests.begin(), fan_speed_change_requests.end(), [](const std::pair<int, bool>& p) { return p.second; })*/){
                fan_speed_change_requests[CoolingLine::TYPE_FORCE_RESUME_FAN] = true;
                need_set_fan = true;
            }
            if (m_additional_fan_speed != -1 && m_config.auxiliary_fan.value && limit_height_fan)
                new_gcode += GCodeWriter::set_additional_fan(m_additional_fan_speed);
        }
        else if (line->type & CoolingLine::TYPE_EXTRUDE_END) {
            // Just remove this comment.
        } else if (line->type & (CoolingLine::TYPE_ADJUSTABLE | CoolingLine::TYPE_EXTERNAL_PERIMETER | CoolingLine::TYPE_FIRST_INTERNAL_PERIMETER | CoolingLine::TYPE_WIPE | CoolingLine::TYPE_HAS_F)) {
            // Find the start of a comment, or roll to the end of line.
            const char *end = line_start;
            for (; end < line_end && *end != ';'; ++ end);
            // Find the 'F' word.
            const char *fpos            = strstr(line_start + 2, " F") + 2;
            int         new_feedrate    = current_feedrate;
            // Modify the F word of the current G-code line.
            bool        modify          = false;
            // Remove the F word from the current G-code line.
            bool        remove          = false;
            assert(fpos != nullptr);
            new_feedrate = line->slowdown ? int(floor(60. * line->feedrate + 0.5)) : atoi(fpos);
            if (new_feedrate == current_feedrate) {
                // No need to change the F value.
                if ((line->type & (CoolingLine::TYPE_ADJUSTABLE | CoolingLine::TYPE_EXTERNAL_PERIMETER | CoolingLine::TYPE_FIRST_INTERNAL_PERIMETER | CoolingLine::TYPE_WIPE)) ||
                    line->length() == 0.)
                    // Feedrate does not change and this line does not move the print head. Skip the complete G-code line including the G-code comment.
                    end = line_end;
                else
                    // Remove the feedrate from the G0/G1 line. The G-code line may become empty!
                    remove = true;
            } else if (line->slowdown) {
                // The F value will be overwritten.
                modify = true;
            } else {
                // The F value is different from current_feedrate, but not slowed down, thus the G-code line will not be modified.
                // Emit the line without the comment.
                new_gcode.append(line_start, end - line_start);
                current_feedrate = new_feedrate;
            }
            if (modify || remove) {
                if (modify) {
                    // Replace the feedrate.
                    new_gcode.append(line_start, fpos - line_start);
                    current_feedrate = new_feedrate;
                    char buf[64];
                    sprintf(buf, "%d", int(current_feedrate));
                    new_gcode += buf;
                } else {
                    // Remove the feedrate word.
                    const char *f = fpos;
                    // Roll the pointer before the 'F' word.
                    for (f -= 2; f > line_start && (*f == ' ' || *f == '\t'); -- f);

                    if ((f - line_start == 1) && *line_start == 'G' && (*f == '1' || *f == '0')) {
                        // BBS: only remain "G1" or "G0" of this line after remove 'F' part, don't save
                    } else {
                        // Append up to the F word, without the trailing whitespace.
                        new_gcode.append(line_start, f - line_start + 1);
                    }
                }
                // Skip the non-whitespaces of the F parameter up the comment or end of line.
                for (; fpos != end && *fpos != ' ' && *fpos != ';' && *fpos != '\n'; ++ fpos);
                // Append the rest of the line without the comment.
                if (fpos < end)
                    // The G-code line is not empty yet. Emit the rest of it.
                    new_gcode.append(fpos, end - fpos);
                else if (remove && new_gcode == "G1") {
                    // The G-code line only contained the F word, now it is empty. Remove it completely including the comments.
                    new_gcode.resize(new_gcode.size() - 2);
                    end = line_end;
                }
            }
            // Process the rest of the line.
            if (end < line_end) {
                if (line->type & (CoolingLine::TYPE_ADJUSTABLE | CoolingLine::TYPE_ADJUSTABLE_EMPTY | CoolingLine::TYPE_EXTERNAL_PERIMETER | CoolingLine::TYPE_INTERNAL_PERIMETER | CoolingLine::TYPE_FIRST_INTERNAL_PERIMETER |
                                  CoolingLine::TYPE_WIPE | CoolingLine::TYPE_LARGE_RANGE)) {
                    // Process comments, remove ";_EXTRUDE_SET_SPEED", ";_EXTERNAL_PERIMETER", ";_WIPE"
                    std::string comment(end, line_end);
                    boost::replace_all(comment, ";_EXTRUDE_SET_SPEED", "");
                    if (line->type & CoolingLine::TYPE_EXTERNAL_PERIMETER)
                        boost::replace_all(comment, ";_EXTERNAL_PERIMETER", "");
                    else if (line->type & CoolingLine::TYPE_INTERNAL_PERIMETER || line->type & CoolingLine::TYPE_FIRST_INTERNAL_PERIMETER) {
                        assert(line->perimeter_index.has_value());
                        boost::replace_all(comment, ";_INTERNAL_PERIMETER" + std::to_string(*line->perimeter_index), "");
                    }

                    if (line->type & CoolingLine::TYPE_WIPE)
                        boost::replace_all(comment, ";_WIPE", "");
                    if (line->type & CoolingLine::TYPE_LARGE_RANGE)
                        boost::replace_all(comment, ";_LARGE_RANGE", "");
                    new_gcode += comment;
                } else {
                    // Just attach the rest of the source line.
                    new_gcode.append(end, line_end - end);
                }
            }

            if (line->slowdown && !line->move_segments.empty() && line->non_adjustable_length > 0.f) {
                assert(line_waiting_for_split == nullptr);
                line_waiting_for_split = line;
            }

        } else {
            new_gcode.append(line_start, line_end - line_start);
        }


        if (EXTRUDER_CONFIG(enable_special_area_additional_cooling_fan))
        {
            if (have_type_overhang && limit_height_fan)
            {
                float cool_special_cds_fan_speed = EXTRUDER_CONFIG(cool_special_cds_fan_speed);
                if (cool_special_cds_fan_speed != m_additional_fan_speed) {
                    m_additional_fan_speed = cool_special_cds_fan_speed;
                    if (m_config.auxiliary_fan.value)
                    {
                        need_set_cds_fan = true;
                    }
                }
            }
        }

        if (need_set_fan) {
            if (fan_speed_change_requests[CoolingLine::TYPE_FORCE_RESUME_FAN] && m_current_fan_speed != -1)
            {
                new_gcode += GCodeWriter::set_fan(m_config.gcode_flavor, m_current_fan_speed);
                fan_speed_change_requests[CoolingLine::TYPE_FORCE_RESUME_FAN] = false;
            }
            else if(fan_speed_change_requests[CoolingLine::TYPE_OVERHANG_FAN_START])
            {
                new_gcode += GCodeWriter::set_fan(m_config.gcode_flavor, overhang_fan_speed);
                m_current_fan_speed = overhang_fan_speed;
            }
            else if (fan_speed_change_requests[CoolingLine::TYPE_SUPPORT_INTERFACE_FAN_START]){
                new_gcode += GCodeWriter::set_fan(m_config.gcode_flavor, supp_interface_fan_speed);
                m_current_fan_speed = supp_interface_fan_speed;
            }
            else if (fan_speed_change_requests[CoolingLine::TYPE_OVERHANG_FAN_END]) {
                new_gcode += GCodeWriter::set_fan(m_config.gcode_flavor, m_fan_speed);
                m_current_fan_speed = m_fan_speed;
            }
            else
                new_gcode += GCodeWriter::set_fan(m_config.gcode_flavor, m_fan_speed);
            need_set_fan = false;
        }
        pos = line_end;
    }
    const char *gcode_end = gcode.c_str() + gcode.size();
    if (pos < gcode_end)
        new_gcode.append(pos, gcode_end - pos);

    if (EXTRUDER_CONFIG(enable_special_area_additional_cooling_fan))
    {
        if (have_type_overhang)
        {
            m_additional_fan_count = MAX_ADDITIONAL_LAYES;
        }
        else
        {
            m_additional_fan_count >= 0 ? m_additional_fan_count-- : 0;

            if (m_additional_fan_count <= 0 && limit_height_fan)
            {
                int additional_fan_speed_new = EXTRUDER_CONFIG(additional_cooling_fan_speed);
                if (additional_fan_speed_new != m_additional_fan_speed) {
                    m_additional_fan_speed = additional_fan_speed_new;
                    if (m_config.auxiliary_fan.value)
                    {
                        need_set_cds_fan = true;
                    }
                }
            }
        }
    }

    if (need_set_cds_fan)
    {
        std::string _gcode = GCodeWriter::set_additional_fan(m_additional_fan_speed);
        new_gcode = _gcode + new_gcode;
        need_set_cds_fan = false;
    }

#undef EXTRUDER_CONFIG
    return sanitize_buffer_z(new_gcode);
}
#undef MAX_ADDITIONAL_LAYES
} // namespace Slic3r
