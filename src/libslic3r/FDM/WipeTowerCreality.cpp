#include "WipeTowerCreality.hpp"

#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/Fill/FillRectilinear.hpp"
#include "libslic3r/Geometry.hpp"

#include <boost/algorithm/string/predicate.hpp>


namespace Slic3r
{

static const int    arc_fit_size                 = 20;
static const double WIPE_TOWER_RESOLUTION = 0.1;
static const double WT_SIMPLIFY_TOLERANCE_SCALED = 0.001 / SCALING_FACTOR;
#define SCALED_WIPE_TOWER_RESOLUTION (WIPE_TOWER_RESOLUTION / SCALING_FACTOR)
enum class LimitFlow { None, LimitPrintFlow, LimitRammingFlow };
static bool     flat_ironing    = false; // Whether to enable flat ironing for the wipe tower
static float    flat_iron_area  = 4.f;
constexpr float flat_iron_speed = 10.f * 60.f;

inline float align_round(float value, float base) { return std::round(value / base) * base; }

inline float align_ceil(float value, float base) { return std::ceil(value / base) * base; }

inline float align_floor(float value, float base) { return std::floor((value) / base) * base; }
static WipeTower::box_coordinates expand_wipe_box(const WipeTower::box_coordinates& box, float x_outset, float y_bottom_outset, float y_top_outset)
{
    const float width  = box.rd.x() - box.ld.x();
    const float height = box.lu.y() - box.ld.y();
    if (width <= 0.f || height <= 0.f)
        return box;

    const float left_right = std::max(0.f, x_outset);
    const float bottom     = std::max(0.f, y_bottom_outset);
    const float top        = std::max(0.f, y_top_outset);
    return WipeTower::box_coordinates(box.ld - Vec2f(left_right, bottom),
                                      width + 2.f * left_right,
                                      height + bottom + top);
}

static WipeTower::box_coordinates expand_wipe_box(const WipeTower::box_coordinates& box, float outset)
{
    return expand_wipe_box(box, outset, outset, outset);
}

static WipeTower::box_coordinates expand_wipe_block_box(const WipeTower::box_coordinates& box, float outset)
{
    return expand_wipe_box(box, outset, 0.f, 0.f);
}

static WipeTower::box_coordinates shrink_wipe_box(const WipeTower::box_coordinates& box, float inset)
{
    const float width  = box.rd.x() - box.ld.x();
    const float height = box.lu.y() - box.ld.y();
    const float offset = std::max(0.f, inset);
    if (width <= 2.f * offset || height <= 2.f * offset)
        return box;

    return WipeTower::box_coordinates(box.ld + Vec2f(offset, offset),
                                      width - 2.f * offset,
                                      height - 2.f * offset);
}

static float wipe_block_wall_side_depth(bool first_layer, float perimeter_width, float extra_flow)
{
    return (first_layer ? 1.f : extra_flow) * perimeter_width;
}


static Vec2f closest_point_on_segment(const Vec2f& point, const Vec2f& a, const Vec2f& b)
{
    const Vec2f ab = b - a;
    const float len2 = ab.squaredNorm();
    if (len2 <= 1e-6f)
        return a;
    const float t = std::max(0.f, std::min(1.f, (point - a).dot(ab) / len2));
    return a + t * ab;
}

struct WipeWallEntry
{
    Vec2f approach;
    Vec2f entry;
    bool  valid{false};
};

static Vec2f closest_point_on_box(const WipeTower::box_coordinates& box, const Vec2f& point, float* distance_out = nullptr)
{
    const Vec2f corners[4] = { box.ld, box.rd, box.ru, box.lu };
    Vec2f best = corners[0];
    float best_distance = std::numeric_limits<float>::max();

    for (int i = 0; i < 4; ++i) {
        const Vec2f candidate = closest_point_on_segment(point, corners[i], corners[(i + 1) & 3]);
        const float distance = (candidate - point).squaredNorm();
        if (distance < best_distance) {
            best_distance = distance;
            best = candidate;
        }
    }

    if (distance_out != nullptr)
        *distance_out = best_distance;
    return best;
}

static WipeWallEntry wipe_wall_entry_from_gap_points(const WipeTower::box_coordinates& entry_box,
                                                     const std::vector<Vec2f>& gap_points)
{
    WipeWallEntry best;
    best.entry = entry_box.ld;
    best.approach = entry_box.ld;
    float best_distance = std::numeric_limits<float>::max();

    for (const Vec2f& gap : gap_points) {
        const Vec2f entry = closest_point_on_box(entry_box, gap);
        const float distance = (entry - gap).squaredNorm();
        if (distance < best_distance) {
            best_distance = distance;
            best.entry = entry;
            best.approach = gap;
            best.valid = true;
        }
    }

    return best;
}

static WipeTower::box_coordinates actual_serpentine_wipe_box(const WipeTower::box_coordinates& box, float line_width)
{
    const float width = box.rd.x() - box.ld.x();
    const float height = box.lu.y() - box.ld.y();
    if (width <= 0.f || height <= 0.f || line_width <= 0.f)
        return box;

    const int intervals = std::max(0, int(std::floor(std::max(0.f, height - 1e-3f) / line_width)));
    const float used_height = std::max(1e-3f, intervals * line_width);
    return WipeTower::box_coordinates(box.ld, width, used_height);
}

struct Segment
{
    Vec2f      start;
    Vec2f      end;
    bool       is_arc = false;
    ArcSegment arcsegment;
    Segment(const Vec2f& s, const Vec2f& e) : start(s), end(e) {}
    bool is_valid() const { return start.y() < end.y(); }
};

struct IntersectionInfo
{
    Vec2f pos;
    int   idx;
    int   pair_idx; // gap_pair idx
    float dis_from_idx;
    bool  is_forward;
};

struct PointWithFlag
{
    Vec2f pos;
    int   pair_idx; // gap_pair idx
    bool  is_forward;
};

std::vector<Segment> remove_points_from_segment(const Segment& segment, const std::vector<Vec2f>& skip_points, double range)
{
    std::vector<Segment> result;
    result.push_back(segment);
    float x = segment.start.x();

    for (const Vec2f& point : skip_points) {
        std::vector<Segment> newResult;
        for (const auto& seg : result) {
            if (point.y() + range <= seg.start.y() || point.y() - range >= seg.end.y()) {
                newResult.push_back(seg);
            } else {
                if (point.y() - range > seg.start.y()) {
                    newResult.push_back(Segment(Vec2f(x, seg.start.y()), Vec2f(x, point.y() - range)));
                }
                if (point.y() + range < seg.end.y()) {
                    newResult.push_back(Segment(Vec2f(x, point.y() + range), Vec2f(x, seg.end.y())));
                }
            }
        }

        result = newResult;
    }

    result.erase(std::remove_if(result.begin(), result.end(), [](const Segment& seg) { return !seg.is_valid(); }), result.end());
    return result;
}

inline std::pair<bool, Vec2f> ray_intersetion_line(const Vec2f& a, const Vec2f& v1, const Vec2f& b, const Vec2f& c)
{
    const Vec2f v2    = c - b;
    double      denom = cross2(v1, v2);
    if (fabs(denom) < EPSILON)
        return {false, Vec2f(0, 0)};
    const Vec2f v12    = (a - b);
    double      nume_a = cross2(v2, v12);
    double      nume_b = cross2(v1, v12);
    double      t1     = nume_a / denom;
    double      t2     = nume_b / denom;
    if (t1 >= 0 && t2 >= 0 && t2 <= 1.) {
        // Get the intersection point.
        Vec2f res = a + t1 * v1;
        return std::pair<bool, Vec2f>(true, res);
    }
    return std::pair<bool, Vec2f>(false, Vec2f{0, 0});
}

inline IntersectionInfo move_point_along_polygon(
    const std::vector<Vec2f>& points, const Vec2f& startPoint, int startIdx, float offset, bool forward, int pair_idx)
{
    float            remainingDistance = offset;
    IntersectionInfo res;
    int              mod = points.size();
    if (forward) {
        int next = (startIdx + 1) % mod;
        remainingDistance -= (points[next] - startPoint).norm();
        if (remainingDistance <= 0) {
            res.idx          = startIdx;
            res.pos          = startPoint + (points[next] - startPoint).normalized() * offset;
            res.pair_idx     = pair_idx;
            res.dis_from_idx = (points[startIdx] - res.pos).norm();
            return res;
        } else {
            for (int i = (startIdx + 1) % mod; i != startIdx; i = (i + 1) % mod) {
                float segmentLength = (points[(i + 1) % mod] - points[i]).norm();
                if (remainingDistance <= segmentLength) {
                    float ratio      = remainingDistance / segmentLength;
                    res.idx          = i;
                    res.pos          = points[i] + ratio * (points[(i + 1) % mod] - points[i]);
                    res.dis_from_idx = remainingDistance;
                    res.pair_idx     = pair_idx;
                    return res;
                }
                remainingDistance -= segmentLength;
            }
            res.idx          = (startIdx - 1 + mod) % mod;
            res.pos          = points[startIdx];
            res.pair_idx     = pair_idx;
            res.dis_from_idx = (res.pos - points[res.idx]).norm();
        }
    } else {
        int next = (startIdx + 1) % mod;
        remainingDistance -= (points[startIdx] - startPoint).norm();
        if (remainingDistance <= 0) {
            res.idx          = startIdx;
            res.pos          = startPoint - (points[next] - points[startIdx]).normalized() * offset;
            res.dis_from_idx = (res.pos - points[startIdx]).norm();
            res.pair_idx     = pair_idx;
            return res;
        }
        for (int i = (startIdx - 1 + mod) % mod; i != startIdx; i = (i - 1 + mod) % mod) {
            float segmentLength = (points[(i + 1) % mod] - points[i]).norm();
            if (remainingDistance <= segmentLength) {
                float ratio      = remainingDistance / segmentLength;
                res.idx          = i;
                res.pos          = points[(i + 1) % mod] - ratio * (points[(i + 1) % mod] - points[i]);
                res.dis_from_idx = segmentLength - remainingDistance;
                res.pair_idx     = pair_idx;
                return res;
            }
            remainingDistance -= segmentLength;
        }
        res.idx          = startIdx;
        res.pos          = points[res.idx];
        res.pair_idx     = pair_idx;
        res.dis_from_idx = 0;
    }
    return res;
};

inline void insert_points(std::vector<PointWithFlag>& pl, int idx, Vec2f pos, int pair_idx, bool is_forward)
{
    int   next = (idx + 1) % pl.size();
    Vec2f pos1 = pl[idx].pos;
    Vec2f pos2 = pl[next].pos;
    if ((pos - pos1).squaredNorm() < EPSILON) {
        pl[idx].pair_idx   = pair_idx;
        pl[idx].is_forward = is_forward;
    } else if ((pos - pos2).squaredNorm() < EPSILON) {
        pl[next].pair_idx   = pair_idx;
        pl[next].is_forward = is_forward;
    } else {
        pl.insert(pl.begin() + idx + 1, PointWithFlag{pos, pair_idx, is_forward});
    }
}

inline Polylines remove_points_from_polygon(
    const Polygon& polygon, const std::vector<Vec2f>& skip_points, double range, bool is_left, Polygon& insert_skip_pg)
{
    assert(polygon.size() > 2);
    Polylines                     result;
    std::vector<PointWithFlag>    new_pl; // add intersection points for gaps, where bool indicates whether it's a gap point.
    std::vector<IntersectionInfo> inter_info;
    Vec2f                         ray          = is_left ? Vec2f(-1, 0) : Vec2f(1, 0);
    auto                          polygon_box  = get_extents(polygon);
    Point                         anchor_point = is_left ? Point{polygon_box.max[0], polygon_box.min[1]} : polygon_box.min; // rd:ld
    std::vector<Vec2f>            points;
    {
        points.reserve(polygon.points.size());
        int      idx      = polygon.closest_point_index(anchor_point);
        Polyline tmp_poly = polygon.split_at_index(idx);
        for (auto& p : tmp_poly)
            points.push_back(unscale(p).cast<float>());
        points.pop_back();
    }

    for (int i = 0; i < skip_points.size(); i++) {
        for (int j = 0; j < points.size(); j++) {
            Vec2f& p1                  = points[j];
            Vec2f& p2                  = points[(j + 1) % points.size()];
            auto [is_inter, inter_pos] = ray_intersetion_line(skip_points[i], ray, p1, p2);
            if (is_inter) {
                IntersectionInfo forward  = move_point_along_polygon(points, inter_pos, j, range, true, i);
                IntersectionInfo backward = move_point_along_polygon(points, inter_pos, j, range, false, i);
                backward.is_forward       = false;
                forward.is_forward        = true;
                inter_info.push_back(backward);
                inter_info.push_back(forward);
                break;
            }
        }
    }

    // insert point to new_pl
    for (const auto& p : points)
        new_pl.push_back({p, -1});
    std::sort(inter_info.begin(), inter_info.end(), [](const IntersectionInfo& lhs, const IntersectionInfo& rhs) {
        if (rhs.idx == lhs.idx)
            return lhs.dis_from_idx < rhs.dis_from_idx;
        return lhs.idx < rhs.idx;
    });
    for (int i = inter_info.size() - 1; i >= 0; i--) {
        insert_points(new_pl, inter_info[i].idx, inter_info[i].pos, inter_info[i].pair_idx, inter_info[i].is_forward);
    }

    {
        // set insert_pg for wipe_path
        for (auto& p : new_pl)
            insert_skip_pg.points.push_back(scaled(p.pos));
    }

    int      beg  = 0;
    bool     skip = true;
    int      i    = beg;
    Polyline pl;

    do {
        if (skip || new_pl[i].pair_idx == -1) {
            pl.points.push_back(scaled(new_pl[i].pos));
            i    = (i + 1) % new_pl.size();
            skip = false;
        } else {
            if (!pl.points.empty()) {
                pl.points.push_back(scaled(new_pl[i].pos));
                result.push_back(pl);
                pl.points.clear();
            }
            int left = new_pl[i].pair_idx;
            int j    = (i + 1) % new_pl.size();
            while (j != beg && new_pl[j].pair_idx != left) {
                if (new_pl[j].pair_idx != -1 && !new_pl[j].is_forward)
                    left = new_pl[j].pair_idx;
                j = (j + 1) % new_pl.size();
            }
            i    = j;
            skip = true;
        }
    } while (i != beg);

    if (!pl.points.empty()) {
        if (new_pl[i].pair_idx == -1)
            pl.points.push_back(scaled(new_pl[i].pos));
        result.push_back(pl);
    }
    return result;
}

inline Polylines contrust_gap_for_skip_points(
    const Polygon& polygon, const std::vector<Vec2f>& skip_points, float wt_width, float gap_length, Polygon& insert_skip_polygon)
{
    if (skip_points.empty()) {
        insert_skip_polygon = polygon;
        return Polylines{to_polyline(polygon)};
    }
    bool        is_left = false;
    const auto& pt      = skip_points.front();
    if (abs(pt.x()) < wt_width / 2.f) {
        is_left = true;
    }
    return remove_points_from_polygon(polygon, skip_points, gap_length, is_left, insert_skip_polygon);
};

class WipeTowerWriterCreality
{
public:
	WipeTowerWriterCreality(float layer_height, float line_width, GCodeFlavor flavor, const std::vector<WipeTowerCreality::FilamentParameters>& filament_parameters) :
		m_current_pos(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
		m_current_z(0.f),
		m_current_feedrate(0.f),
		m_layer_height(layer_height),
		m_extrusion_flow(0.f),
		m_preview_suppressed(false),
		m_elapsed_time(0.f),
#if ENABLE_GCODE_VIEWER_DATA_CHECKING
        m_default_analyzer_line_width(line_width),
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING
        m_gcode_flavor(flavor),
        m_filpar(filament_parameters)
        {
            // ORCA: This class is only used by non BBL printers, so set the parameter appropriately.
            // This fixes an issue where the wipe tower was using BBL tags resulting in statistics for purging in the purge tower not being displayed.
            GCodeProcessor::s_IsBBLPrinter = false;
            // adds tag for analyzer:
            std::ostringstream str;
            str << ";" << GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Height) << m_layer_height << "\n"; // don't rely on GCodeAnalyzer knowing the layer height - it knows nothing at priming
            str << ";" << GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Role) << ExtrusionEntity::role_to_string(erWipeTower) << "\n";
            m_gcode += str.str();
            change_analyzer_line_width(line_width);
    }

    WipeTowerWriterCreality& change_analyzer_line_width(float line_width) {
        // adds tag for analyzer:
        std::stringstream str;
        str << ";" << GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Width) << line_width << "\n";
        m_gcode += str.str();
        return *this;
    }

#if ENABLE_GCODE_VIEWER_DATA_CHECKING
    WipeTowerWriterCreality& change_analyzer_mm3_per_mm(float len, float e) {
        static const float area = float(M_PI) * 1.75f * 1.75f / 4.f;
        float mm3_per_mm = (len == 0.f ? 0.f : area * e / len);
        // adds tag for processor:
        std::stringstream str;
        str << ";" << GCodeProcessor::Mm3_Per_Mm_Tag << mm3_per_mm << "\n";
        m_gcode += str.str();
        return *this;
    }
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING

	WipeTowerWriterCreality& set_initial_position(const Vec2f &pos, float width = 0.f, float depth = 0.f, float internal_angle = 0.f) {
        m_wipe_tower_width = width;
        m_wipe_tower_depth = depth;
        m_internal_angle = internal_angle;
		m_start_pos = this->rotate(pos);
		m_current_pos = pos;
		return *this;
	}

    WipeTowerWriterCreality& set_position(const Vec2f &pos) { m_current_pos = pos; return *this; }

    WipeTowerWriterCreality& set_initial_tool(size_t tool) { m_current_tool = tool; return *this; }

	WipeTowerWriterCreality& set_z(float z)
		{ m_current_z = z; return *this; }

	WipeTowerWriterCreality& set_extrusion_flow(float flow)
		{ m_extrusion_flow = flow; return *this; }

	WipeTowerWriterCreality& set_y_shift(float shift) {
        m_current_pos.y() -= shift-m_y_shift;
        m_y_shift = shift;
        return (*this);
    }

    WipeTowerWriterCreality& disable_linear_advance() {
        if (m_gcode_flavor == gcfRepRapSprinter || m_gcode_flavor == gcfRepRapFirmware)
            m_gcode += (std::string("M572 D") + std::to_string(m_current_tool) + " S0\n");
        else if (m_gcode_flavor == gcfKlipper)
            m_gcode += "SET_PRESSURE_ADVANCE ADVANCE=0\n";
        else
            m_gcode += "M900 K0\n";
        return *this;
    }

	// Suppress / resume G-code preview in Slic3r. Slic3r will have difficulty to differentiate the various
	// filament loading and cooling moves from normal extrusion moves. Therefore the writer
	// is asked to suppres output of some lines, which look like extrusions.
#if ENABLE_GCODE_VIEWER_DATA_CHECKING
    WipeTowerWriterCreality& suppress_preview() { change_analyzer_line_width(0.f); m_preview_suppressed = true; return *this; }
    WipeTowerWriterCreality& resume_preview() { change_analyzer_line_width(m_default_analyzer_line_width); m_preview_suppressed = false; return *this; }
#else
    WipeTowerWriterCreality& suppress_preview() { m_preview_suppressed = true; return *this; }
	WipeTowerWriterCreality& resume_preview()   { m_preview_suppressed = false; return *this; }
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING

	WipeTowerWriterCreality& feedrate(float f)
	{
        if (f != m_current_feedrate) {
			m_gcode += "G1" + set_format_F(f) + "\n";
            m_current_feedrate = f;
        }
		return *this;
	}
    WipeTowerWriterCreality& feedrate_(float f)
    {
        if (f != m_current_feedrate) {
            m_gcode += "G1" + set_format_F(f);
            m_current_feedrate = f;
        }
        return *this;
    }

    WipeTowerWriterCreality& set_feedrate(float f)
    {
        if (f != m_current_feedrate) {
            m_current_feedrate = f;
        }
        return *this;
    }

	const std::string&   gcode() const { return m_gcode; }
	const std::vector<WipeTower::Extrusion>& extrusions() const { return m_extrusions; }
	float                x()     const { return m_current_pos.x(); }
	float                y()     const { return m_current_pos.y(); }
	const Vec2f& 		 pos()   const { return m_current_pos; }
	const Vec2f	 		 start_pos_rotated() const { return m_start_pos; }
	const Vec2f  		 point_rotated(const Vec2f &pt) const { return this->rotate(pt); }
	const Vec2f  		 pos_rotated() const { return this->rotate(m_current_pos); }
	float 				 elapsed_time() const { return m_elapsed_time; }
    float                get_wipe_maxe_y() const { return m_wipe_max_y; }
    float                get_wipe_maxe_x() const { return m_wipe_max_x; }
    float                get_and_reset_used_filament_length() { float temp = m_used_filament_length; m_used_filament_length = 0.f; return temp; }

	// Extrude with an explicitely provided amount of extrusion.
    WipeTowerWriterCreality& extrude_explicit(float x, float y, float e, float f = 0.f, bool record_length = false, bool limit_volumetric_flow = true)
    {
		if (x == m_current_pos.x() && y == m_current_pos.y() && e == 0.f && (f == 0.f || f == m_current_feedrate))
			// Neither extrusion nor a travel move.
			return *this;

		float dx = x - m_current_pos.x();
		float dy = y - m_current_pos.y();
        float len = std::sqrt(dx*dx+dy*dy);
        if (record_length)
            m_used_filament_length += e;

		// Now do the "internal rotation" with respect to the wipe tower center
		Vec2f rotated_current_pos(this->pos_rotated());
		Vec2f rot(this->rotate(Vec2f(x,y)));                               // this is where we want to go

        if (! m_preview_suppressed && e > 0.f && len > 0.f) {
#if ENABLE_GCODE_VIEWER_DATA_CHECKING
            change_analyzer_mm3_per_mm(len, e);
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING
            // Width of a squished extrusion, corrected for the roundings of the squished extrusions.
			// This is left zero if it is a travel move.
            float width = e * m_filpar[0].filament_area / (len * m_layer_height);
			// Correct for the roundings of a squished extrusion.
			width += m_layer_height * float(1. - M_PI / 4.);
			if (m_extrusions.empty() || m_extrusions.back().pos != rotated_current_pos)
				m_extrusions.emplace_back(WipeTower::Extrusion(rotated_current_pos, 0, m_current_tool));
			m_extrusions.emplace_back(WipeTower::Extrusion(rot, width, m_current_tool));
		}

        if (m_wipe_max_y < rot.y()) {
            m_wipe_max_y = rot.y();
        }
        if (m_wipe_max_x < rot.x()) {
            m_wipe_max_x = rot.x();
        }


		m_gcode += "G1";
        if (std::abs(rot.x() - rotated_current_pos.x()) > (float)EPSILON)
			m_gcode += set_format_X(rot.x());

        if (std::abs(rot.y() - rotated_current_pos.y()) > (float)EPSILON)
			m_gcode += set_format_Y(rot.y());


		if (e != 0.f)
			m_gcode += set_format_E(e);

		if (f != 0.f && f != m_current_feedrate) {
            if (limit_volumetric_flow) {
                float e_speed = e / (((len == 0.f) ? std::abs(e) : len) / f * 60.f);
                f /= std::max(1.f, e_speed / m_filpar[m_current_tool].max_e_speed);
            }
			m_gcode += set_format_F(f);
        }

        // Append newline if at least one of X,Y,E,F was changed.
        // Otherwise, remove the "G1".
        if (! boost::ends_with(m_gcode, "G1"))
            m_gcode += "\n";
        else
            m_gcode.erase(m_gcode.end()-2, m_gcode.end());

        m_current_pos.x() = x;
        m_current_pos.y() = y;
		// Update the elapsed time with a rough estimate.
        m_elapsed_time += ((len == 0.f) ? std::abs(e) : len) / m_current_feedrate * 60.f;
		return *this;
	}

    WipeTowerWriterCreality& extrude_explicit(const Vec2f &dest, float e, float f = 0.f, bool record_length = false, bool limit_volumetric_flow = true)
    { return extrude_explicit(dest.x(), dest.y(), e, f, record_length); }

    WipeTowerWriterCreality& extrude_arc_explicit(ArcSegment& arc,
                                          float       f             = 0.f,
                                          bool        record_length = false,
                                          LimitFlow   limit_flow    = LimitFlow::LimitPrintFlow)
    {
        float x   = (float) unscale(arc.end_point).x();
        float y   = (float) unscale(arc.end_point).y();
        float len = unscaled<float>(arc.length);
        float e   = len * m_extrusion_flow;
        if (len < (float) EPSILON && e == 0.f && (f == 0.f || f == m_current_feedrate))
            // Neither extrusion nor a travel move.
            return *this;
        if (record_length)
            m_used_filament_length += e;

        // Now do the "internal rotation" with respect to the wipe tower center
        Vec2f rotated_current_pos(this->pos_rotated());
        Vec2f rot(this->rotate(Vec2f(x, y))); // this is where we want to go

        if (!m_preview_suppressed && e > 0.f && len > 0.f) {
#if ENABLE_GCODE_VIEWER_DATA_CHECKING
            change_analyzer_mm3_per_mm(len, e);
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING
       // Width of a squished extrusion, corrected for the roundings of the squished extrusions.
       // This is left zero if it is a travel move.
            float width = e * m_filpar[0].filament_area / (len * m_layer_height);
            // Correct for the roundings of a squished extrusion.
            width += m_layer_height * float(1. - M_PI / 4.);
            if (m_extrusions.empty() || m_extrusions.back().pos != rotated_current_pos)
                m_extrusions.emplace_back(WipeTower::Extrusion(rotated_current_pos, 0, m_current_tool));
            {
                int n = arc_fit_size;
                for (int j = 0; j < n; j++) {
                    float cur_angle = arc.polar_start_theta + (float) j / n * arc.angle_radians;
                    if (cur_angle > 2 * PI)
                        cur_angle -= 2 * PI;
                    else if (cur_angle < 0)
                        cur_angle += 2 * PI;
                    Point tmp = arc.center + Point{arc.radius * std::cos(cur_angle), arc.radius * std::sin(cur_angle)};
                    m_extrusions.emplace_back(WipeTower::Extrusion(this->rotate(unscaled<float>(tmp)), width, m_current_tool));
                }
                m_extrusions.emplace_back(WipeTower::Extrusion(rot, width, m_current_tool));
            }
        }

        if (e == 0.f) {
           // m_gcode += set_travel_acceleration();
        } else {
           // m_gcode += set_normal_acceleration();
        }

        m_gcode += arc.direction == ArcDirection::Arc_Dir_CCW ? "G3" : "G2";
        const Vec2f center_offset = this->rotate(unscaled<float>(arc.center)) - rotated_current_pos;
        m_gcode += set_format_X(rot.x());
        m_gcode += set_format_Y(rot.y());
        m_gcode += set_format_I(center_offset.x());
        m_gcode += set_format_J(center_offset.y());

        if (e != 0.f)
            m_gcode += set_format_E(e);

        if (f != 0.f && f != m_current_feedrate) {
            if (limit_flow != LimitFlow::None) {
                float e_speed = e / (((len == 0.f) ? std::abs(e) : len) / f * 60.f);
                float tmp     = m_filpar[m_current_tool].max_e_speed;
               /* if (limit_flow == LimitFlow::LimitRammingFlow)
                    tmp = m_filpar[m_current_tool].max_e_ramming_speed;*/
                f /= std::max(1.f, e_speed / tmp);
            }
            m_gcode += set_format_F(f);
        }

        m_current_pos.x() = x;
        m_current_pos.y() = y;
        // Update the elapsed time with a rough estimate.
        m_elapsed_time += ((len == 0.f) ? std::abs(e) : len) / m_current_feedrate * 60.f;
        m_gcode += "\n";
        return *this;
    }

    // Travel to a new XY position. f=0 means use the current value.
	WipeTowerWriterCreality& travel(float x, float y, float f = 0.f)
    { return extrude_explicit(x, y, 0.f, f); }

    WipeTowerWriterCreality& travel(const Vec2f &dest, float f = 0.f)
    { return extrude_explicit(dest.x(), dest.y(), 0.f, f); }

    // Extrude a line from current position to x, y with the extrusion amount given by m_extrusion_flow.
	WipeTowerWriterCreality& extrude(float x, float y, float f = 0.f)
	{
		float dx = x - m_current_pos.x();
		float dy = y - m_current_pos.y();
        return extrude_explicit(x, y, std::sqrt(dx*dx+dy*dy) * m_extrusion_flow, f, true);
	}

    WipeTowerWriterCreality& extrude(const Vec2f &dest, const float f = 0.f)
    { return extrude(dest.x(), dest.y(), f); }

    WipeTowerWriterCreality& extrude_arc(ArcSegment& arc, float f = 0.f, LimitFlow limit_flow = LimitFlow::LimitPrintFlow)
    {
        return extrude_arc_explicit(arc, f, false, limit_flow);
    }

    WipeTowerWriterCreality& rectangle(const Vec2f& ld,float width,float height,const float f = 0.f)
    {
        Vec2f corners[4];
        corners[0] = ld;
        corners[1] = ld + Vec2f(width,0.f);
        corners[2] = ld + Vec2f(width,height);
        corners[3] = ld + Vec2f(0.f,height);
        int index_of_closest = 0;
        if (x()-ld.x() > ld.x()+width-x())    // closer to the right
            index_of_closest = 1;
        if (y()-ld.y() > ld.y()+height-y())   // closer to the top
            index_of_closest = (index_of_closest==0 ? 3 : 2);

        travel(corners[index_of_closest].x(), y());      // travel to the closest corner
        travel(x(),corners[index_of_closest].y());

        int i = index_of_closest;
        do {
            ++i;
            if (i==4) i=0;
            extrude(corners[i], f);
        } while (i != index_of_closest);
        return (*this);
    }

    WipeTowerWriterCreality& rectangle_fill_box(
        const Vec2f& ld, float width, float height, const float retractlength, const float retractspeed, const float f = 0.f)
    {
        Vec2f corners[4];
        corners[0]           = ld;
        corners[1]           = ld + Vec2f(width, 0.f);
        corners[2]           = ld + Vec2f(width, height);
        corners[3]           = ld + Vec2f(0.f, height);
        int index_of_closest = 0;
        if (x() - ld.x() > ld.x() + width - x()) // closer to the right
            index_of_closest = 1;
        if (y() - ld.y() > ld.y() + height - y()) // closer to the top
            index_of_closest = (index_of_closest == 0 ? 3 : 2);

        travel(corners[index_of_closest].x(), y()); // travel to the closest corner
        travel(x(), corners[index_of_closest].y());

        int i = index_of_closest;
        do {
          /*  if (i == index_of_closest) {
                travel(corners[i], f);
                retract(-retractlength, retractspeed);
            }*/
            ++i;
            if (i == 4)
                i = 0;
            extrude(corners[i], f);
        } while (i != index_of_closest);
        return (*this);
    }

    WipeTowerWriterCreality& line(const WipeTowerCreality* wipe_tower, Vec2f p0, Vec2f p1, const float f = 0.f)
    {
        if (abs(x() - p0.x()) > abs(x() - p1.x()))
            std::swap(p0, p1);
        travel(p0.x(), y());
        travel(x(), p0.y());
        extrude(p1, f);
        return (*this);
    }

    WipeTowerWriterCreality& rectangle_fill_box(const WipeTowerCreality*          wipe_tower,
                                                const WipeTower::box_coordinates& fill_box,
                                                std::vector<Vec2f>&               finish_rect_wipe_path,
                                                const float                       retractlength,
                                                const float                       retractspeed,
                                                const float                       f = 0.f)
    {
        float width  = fill_box.rd.x() - fill_box.ld.x();
        float height = fill_box.ru.y() - fill_box.rd.y();
        if (height > wipe_tower->m_perimeter_width - wipe_tower->WT_EPSILON) {
            rectangle_fill_box(fill_box.ld, fill_box.rd.x() - fill_box.ld.x(), fill_box.ru.y() - fill_box.rd.y(), retractlength,
                                       retractspeed, f);
            Vec2f target = (pos() == fill_box.ld ?
                                fill_box.rd :
                                (pos() == fill_box.rd ? fill_box.ru : (pos() == fill_box.ru ? fill_box.lu : fill_box.ld)));
            finish_rect_wipe_path.emplace_back(pos());
            finish_rect_wipe_path.emplace_back(target);
        } else if (height > wipe_tower->WT_EPSILON) {
            line(wipe_tower, fill_box.ld, fill_box.rd);
            Vec2f target = (pos() == fill_box.ld ? fill_box.rd : fill_box.ld);
            finish_rect_wipe_path.emplace_back(pos());
            finish_rect_wipe_path.emplace_back(target);
        }
        return (*this);
    }

    WipeTowerWriterCreality& rectangle(const WipeTower::box_coordinates& box, const float f = 0.f)
    {
        rectangle(Vec2f(box.ld.x(), box.ld.y()),
                  box.ru.x() - box.lu.x(),
                  box.ru.y() - box.rd.y(), f);
        return (*this);
    }

    WipeTowerWriterCreality& polygon(const Polygon& wall_polygon, const float f = 0.f)
    {
        Polyline pl = to_polyline(wall_polygon);
        pl.simplify(WT_SIMPLIFY_TOLERANCE_SCALED);
        pl.simplify_by_fitting_arc(SCALED_WIPE_TOWER_RESOLUTION);

        auto get_closet_idx = [this](std::vector<Segment>& corners) -> int {
            Vec2f anchor{this->m_current_pos.x(), this->m_current_pos.y()};
            int   closestIndex = -1;
            float minDistance  = std::numeric_limits<float>::max();
            for (int i = 0; i < corners.size(); ++i) {
                float distance = (corners[i].start - anchor).squaredNorm();
                if (distance < minDistance) {
                    minDistance  = distance;
                    closestIndex = i;
                }
            }
            return closestIndex;
        };
        std::vector<Segment> segments;
        for (int i = 0; i < pl.fitting_result.size(); i++) {
            if (pl.fitting_result[i].path_type == EMovePathType::Linear_move) {
                for (int j = pl.fitting_result[i].start_point_index; j < pl.fitting_result[i].end_point_index; j++)
                    segments.push_back({unscaled<float>(pl.points[j]), unscaled<float>(pl.points[j + 1])});
            } else {
                int beg = pl.fitting_result[i].start_point_index;
                int end = pl.fitting_result[i].end_point_index;
                segments.push_back({unscaled<float>(pl.points[beg]), unscaled<float>(pl.points[end])});
                segments.back().is_arc     = true;
                segments.back().arcsegment = pl.fitting_result[i].arc_data;
            }
        }

        int index_of_closest = get_closet_idx(segments);
        int i                = index_of_closest;
        travel(segments[i].start); // travel to the closest points
        segments[i].is_arc ? extrude_arc(segments[i].arcsegment, f) : extrude(segments[i].end, f);
        do {
            i = (i + 1) % segments.size();
            if (i == index_of_closest)
                break;
            segments[i].is_arc ? extrude_arc(segments[i].arcsegment, f) : extrude(segments[i].end, f);
        } while (1);
        return (*this);
    }

	WipeTowerWriterCreality& load(float e, float f = 0.f)
	{
		if (e == 0.f && (f == 0.f || f == m_current_feedrate))
			return *this;
		m_gcode += "G1";
		if (e != 0.f)
			m_gcode += set_format_E(e);
		if (f != 0.f && f != m_current_feedrate)
			m_gcode += set_format_F(f);
		m_gcode += "\n";
		return *this;
	}

	WipeTowerWriterCreality& retract(float e, float f = 0.f)
		{ return load(-e, f); }

	// Elevate the extruder head above the current print_z position.
    WipeTowerWriterCreality& z_hop(float hop, float f = 0.f, std::string _str = "G1")
	{
		m_gcode += _str + set_format_Z(m_current_z + hop);
		if (f != 0 && f != m_current_feedrate)
			m_gcode += set_format_F(f);
		m_gcode += "\n";
		return *this;
	}

    // Elevate the extruder head above the current print_z position.
    WipeTowerWriterCreality& relative_zhop(float hop, float f = 0.f, std::string _str = "G1")
    {
        m_gcode += _str + set_format_Z(hop);
        if (f != 0 && f != m_current_feedrate)
            m_gcode += set_format_F(f);
        m_gcode += "\n";
        return *this;
    }

	// Lower the extruder head back to the current print_z position.
	WipeTowerWriterCreality& z_hop_reset(float f = 0.f)
		{ return z_hop(0, f); }

	// Move to x1, +y_increment,
	// extrude quickly amount e to x2 with feed f.
	WipeTowerWriterCreality& ram(float x1, float x2, float dy, float e0, float e, float f)
	{
        extrude_explicit(x1, m_current_pos.y() + dy, e0, f, true, false);
        extrude_explicit(x2, m_current_pos.y(), e, 0.f, true, false);
		return *this;
	}

	// Let the end of the pulled out filament cool down in the cooling tube
	// by moving up and down and moving the print head left / right
	// at the current Y position to spread the leaking material.
	WipeTowerWriterCreality& cool(float x1, float x2, float e1, float e2, float f)
	{
		extrude_explicit(x1, m_current_pos.y(), e1, f, false, false);
		extrude_explicit(x2, m_current_pos.y(), e2, false, false);
		return *this;
	}

    WipeTowerWriterCreality& set_tool(size_t tool)
	{
		m_current_tool = tool;
		return *this;
	}

	// Set extruder temperature, don't wait by default.
	WipeTowerWriterCreality& set_extruder_temp(int temperature, bool wait = false)
	{
        m_gcode += "M" + std::to_string(wait ? 109 : 104) + " S" + std::to_string(temperature) + "\n";
        return *this;
    }

    // Wait for a period of time (seconds).
	WipeTowerWriterCreality& wait(float time)
	{
        if (time==0.f)
            return *this;
        m_gcode += "G4 S" + Slic3r::float_to_string_decimal_point(time, 3) + "\n";
		return *this;
    }

	// Set speed factor override percentage.
	WipeTowerWriterCreality& speed_override(int speed)
	{


    //creality 暂时屏蔽M220指令,固件不支持
#if 0
       m_gcode += "M220 S" + std::to_string(speed) + "\n";
#endif

		return *this;
    }

	// Let the firmware back up the active speed override value.
	WipeTowerWriterCreality& speed_override_backup()
    {
        // This is only supported by Prusa at this point (https://github.com/prusa3d/PrusaSlicer/issues/3114)
        if (m_gcode_flavor == gcfMarlinLegacy || m_gcode_flavor == gcfMarlinFirmware)
            m_gcode += "M220 B\n";
		return *this;
    }

	// Let the firmware restore the active speed override value.
	WipeTowerWriterCreality& speed_override_restore()
	{
        if (m_gcode_flavor == gcfMarlinLegacy || m_gcode_flavor == gcfMarlinFirmware)
            m_gcode += "M220 R\n";
		return *this;
    }

	WipeTowerWriterCreality& flush_planner_queue()
	{
		m_gcode += "G4 S0\n";
		return *this;
	}

	// Reset internal extruder counter.
	WipeTowerWriterCreality& reset_extruder()
	{
		m_gcode += "G92 E0\n";
		return *this;
	}

	WipeTowerWriterCreality& comment_with_value(const char *comment, int value)
    {
        m_gcode += std::string(";") + comment + std::to_string(value) + "\n";
		return *this;
    }

    WipeTowerWriterCreality& set_fan(unsigned speed)
	{
		if (speed == m_last_fan_speed)
			return *this;
		if (speed == 0)
			m_gcode += "M107\n";
        else
            m_gcode += "M106 S" + std::to_string(unsigned(255.0 * speed / 100.0)) + "\n";
		m_last_fan_speed = speed;
		return *this;
	}

	WipeTowerWriterCreality& append(const std::string& text) { m_gcode += text; return *this; }
    WipeTowerWriterCreality& prefix(const std::string& text)
    {
        m_gcode = text + m_gcode;
        return *this;
    }

    const std::vector<Vec2f>& wipe_path() const
    {
        return m_wipe_path;
    }

    const std::vector<std::vector<Vec2f>>& wipe_path_group() const
    {
        return m_wipe_path_group;
    }

    WipeTowerWriterCreality& add_wipe_group(const Vec2f& pt, const Vec2f& pd)
    {
        std::vector<Vec2f> group(2);
        group[0] = rotate(pt);
        group[1] = rotate(pd);
        m_wipe_path_group.push_back(group);
        add_wipe_point(pt);
        add_wipe_point(pd);
        return *this;
    }

    WipeTowerWriterCreality& add_wipe_point(const Vec2f& pt)
    {
        m_wipe_path.push_back(rotate(pt));
        return *this;
    }

    WipeTowerWriterCreality& add_wipe_point(float x, float y)
    {
        return add_wipe_point(Vec2f(x, y));
    }

    WipeTowerWriterCreality& add_wipe_path(const Polygon& polygon, double wipe_dist)
    {
        int      closest_idx = polygon.closest_point_index(scaled(m_current_pos));
        Polyline wipe_path   = polygon.split_at_index(closest_idx);
        wipe_path.reverse();
        for (int i = 0; i < wipe_path.size(); ++i) {
            if (wipe_dist < EPSILON)
                break;
            add_wipe_point(unscaled<float>(wipe_path[i]));
            if (i != 0)
                wipe_dist -= (unscaled(wipe_path[i]) - unscaled(wipe_path[i - 1])).norm();
        }
        return *this;
    }

    void generate_path(Polylines& pls, float feedrate, float retract_length, float retract_speed, float travel_speed)
    {
        auto get_closet_idx = [this](std::vector<Segment>& corners) -> int {
            Vec2f anchor{this->m_current_pos.x(), this->m_current_pos.y()};
            int   closestIndex = -1;
            float minDistance  = std::numeric_limits<float>::max();
            for (int i = 0; i < corners.size(); ++i) {
                float distance = (corners[i].start - anchor).squaredNorm();
                if (distance < minDistance) {
                    minDistance  = distance;
                    closestIndex = i;
                }
            }
            return closestIndex;
        };
        for (auto& pl : pls)
            pl.simplify_by_fitting_arc(SCALED_WIPE_TOWER_RESOLUTION);

        std::vector<Segment> segments;
        for (const auto& pl : pls) {
            if (pl.points.size() < 2)
                continue;
            for (int i = 0; i < pl.fitting_result.size(); i++) {
                if (pl.fitting_result[i].path_type == EMovePathType::Linear_move) {
                    for (int j = pl.fitting_result[i].start_point_index; j < pl.fitting_result[i].end_point_index; j++)
                        segments.push_back({unscaled<float>(pl.points[j]), unscaled<float>(pl.points[j + 1])});
                } else {
                    int beg = pl.fitting_result[i].start_point_index;
                    int end = pl.fitting_result[i].end_point_index;
                    segments.push_back({unscaled<float>(pl.points[beg]), unscaled<float>(pl.points[end])});
                    segments.back().is_arc     = true;
                    segments.back().arcsegment = pl.fitting_result[i].arc_data;
                }
            }
        }
        int index_of_closest = get_closet_idx(segments);
        int i                = index_of_closest;
        //retract(-retract_length, retract_speed);
        travel(segments[i].start); // travel to the closest points

        segments[i].is_arc ? extrude_arc(segments[i].arcsegment, feedrate) : extrude(segments[i].end, feedrate);
        do {
            i = (i + 1) % segments.size();
            if (i == index_of_closest)
                break;
            float dx  = segments[i].start.x() - m_current_pos.x();
            float dy  = segments[i].start.y() - m_current_pos.y();
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > EPSILON) {
                retract(retract_length, retract_speed);
                travel(segments[i].start, travel_speed);
                retract(-retract_length, retract_speed);
            }
            segments[i].is_arc ? extrude_arc(segments[i].arcsegment, feedrate) : extrude(segments[i].end, feedrate);
        } while (1);
    }

    void spiral_flat_ironing(const Vec2f& center, float area, float step_length, float feedrate)
    {
        float edge_length = std::sqrt(area);
        Vec2f box_max     = center + Vec2f{step_length, step_length};
        Vec2f box_min     = center - Vec2f{step_length, step_length};
        int   n           = std::ceil(edge_length / step_length / 2.f);
        assert(n > 0);
        while (n--) {
            travel(box_max.x(), m_current_pos.y(), feedrate);
            travel(m_current_pos.x(), box_max.y(), feedrate);
            travel(box_min.x(), m_current_pos.y(), feedrate);
            travel(m_current_pos.x(), box_min.y(), feedrate);

            box_max += Vec2f{step_length, step_length};
            box_min -= Vec2f{step_length, step_length};
        }
    }

private:
    float         m_wipe_max_y = std::numeric_limits<double>::lowest();
    float         m_wipe_max_x = std::numeric_limits<double>::lowest();
	Vec2f         m_start_pos;
	Vec2f         m_current_pos;
    std::vector<Vec2f>  m_wipe_path;
    std::vector<std::vector<Vec2f>> m_wipe_path_group;
	float    	  m_current_z;
	float 	  	  m_current_feedrate;
    size_t        m_current_tool;
	float 		  m_layer_height;
	float 	  	  m_extrusion_flow;
	bool		  m_preview_suppressed;
	std::string   m_gcode;
	std::vector<WipeTower::Extrusion> m_extrusions;
	float         m_elapsed_time;
	float   	  m_internal_angle = 0.f;
	float		  m_y_shift = 0.f;
	float 		  m_wipe_tower_width = 0.f;
	float		  m_wipe_tower_depth = 0.f;
    unsigned      m_last_fan_speed = 0;
    int           current_temp = -1;
#if ENABLE_GCODE_VIEWER_DATA_CHECKING
    const float   m_default_analyzer_line_width;
#endif // ENABLE_GCODE_VIEWER_DATA_CHECKING
    float         m_used_filament_length = 0.f;
    GCodeFlavor   m_gcode_flavor;
    const std::vector<WipeTowerCreality::FilamentParameters>& m_filpar;

	std::string   set_format_X(float x)
    {
        m_current_pos.x() = x;
        return " X" + Slic3r::float_to_string_decimal_point(x, 3);
	}

	std::string   set_format_Y(float y) {
        m_current_pos.y() = y;
        return " Y" + Slic3r::float_to_string_decimal_point(y, 3);
	}

	std::string   set_format_Z(float z) {
        return " Z" + Slic3r::float_to_string_decimal_point(z, 3);
	}

	std::string   set_format_E(float e) {
        return " E" + Slic3r::float_to_string_decimal_point(e, 4);
	}

	std::string   set_format_F(float f) {
        char buf[64];
        sprintf(buf, " F%d", int(floor(f + 0.5f)));
        m_current_feedrate = f;
        return buf;
	}

    std::string set_format_I(float i) { return " I" + Slic3r::float_to_string_decimal_point(i, 3); }
    std::string set_format_J(float j) { return " J" + Slic3r::float_to_string_decimal_point(j, 3); }

	WipeTowerWriterCreality& operator=(const WipeTowerWriterCreality &rhs);

	// Rotate the point around center of the wipe tower about given angle (in degrees)
	Vec2f rotate(Vec2f pt) const
	{
		pt.x() -= m_wipe_tower_width / 2.f;
		pt.y() += m_y_shift - m_wipe_tower_depth / 2.f;
	    double angle = m_internal_angle * float(M_PI/180.);
	    double c = cos(angle);
	    double s = sin(angle);
	    return Vec2f(float(pt.x() * c - pt.y() * s) + m_wipe_tower_width / 2.f, float(pt.x() * s + pt.y() * c) + m_wipe_tower_depth / 2.f);
	}

}; // class WipeTowerWriterCreality



WipeTower::ToolChangeResult WipeTowerCreality::construct_tcr(
    WipeTowerWriterCreality& writer, bool priming, size_t old_tool, bool is_finish, float purge_volume, bool is_tool_change) const
{
    WipeTower::ToolChangeResult result;
    result.priming      = priming;
    result.initial_tool = int(old_tool);
    result.new_tool     = int(m_current_tool);
    result.print_z      = m_z_pos;
    result.layer_height = m_layer_height;
    result.elapsed_time = writer.elapsed_time();
    result.start_pos    = writer.start_pos_rotated();
    result.end_pos      = priming ? writer.pos() : writer.pos_rotated();
    result.gcode        = std::move(writer.gcode());
#if ORCA_CHECK_GCODE_PLACEHOLDERS
    result.gcode += is_finish ? ";toolchange_change finished ################\n" : ";toolchange_change ################\n";
#endif

    result.extrusions   = std::move(writer.extrusions());
    result.wipe_path    = std::move(writer.wipe_path());
    result.wipe_paths   = std::move(writer.wipe_path_group());
    result.is_finish_first = is_finish;
    result.m_wipe_max_y          = writer.get_wipe_maxe_y();
    result.m_wipe_max_x          = writer.get_wipe_maxe_x();
    result.is_tool_change  = is_tool_change;
    result.tool_change_start_pos = is_tool_change ? result.start_pos : Vec2f(0, 0);
    result.purge_volume = purge_volume;
    return result;
}

WipeTower::ToolChangeResult WipeTowerCreality::construct_block_tcr(
    WipeTowerWriterCreality& writer, bool priming, size_t filament_id, bool is_finish) const
{
    WipeTower::ToolChangeResult result;
    result.priming      = priming;
    result.initial_tool = int(filament_id);
    result.new_tool     = int(filament_id);
    result.print_z      = m_z_pos;
    result.layer_height = m_layer_height;
    result.elapsed_time = writer.elapsed_time();
    result.start_pos    = writer.start_pos_rotated();
    result.end_pos      = priming ? writer.pos() : writer.pos_rotated();
    result.gcode        = std::move(writer.gcode());
#if ORCA_CHECK_GCODE_PLACEHOLDERS
    result.gcode += is_finish ? ";toolchange_change finished ################\n" : ";toolchange_change ################\n";
#endif

    result.extrusions            = std::move(writer.extrusions());
    result.wipe_path  = std::move(writer.wipe_path());
    result.wipe_paths = std::move(writer.wipe_path_group());
    //result.wipe_paths.push_back(result.wipe_path);
    result.is_finish_first       = is_finish;
    result.is_tool_change        = false;
    //result.tool_change_start_pos = is_tool_change ? result.start_pos : Vec2f(0, 0);
    return result;
}

WipeTowerCreality::WipeTowerCreality(const PrintConfig& config, const PrintRegionConfig& default_region_config,int plate_idx, Vec3d plate_origin, const std::vector<std::vector<float>>& wiping_matrix, size_t initial_tool) :
    m_wipe_tower_pos(config.wipe_tower_x.get_at(plate_idx), config.wipe_tower_y.get_at(plate_idx)),
    m_wipe_tower_width(float(config.prime_tower_width)),
    m_wipe_tower_rotation_angle(float(config.wipe_tower_rotation_angle)),
    m_wipe_tower_brim_width(float(config.prime_tower_brim_width)),
    m_wipe_tower_cone_angle(float(config.wipe_tower_cone_angle)),
    m_wipe_tower_rib_wall(float(config.prime_tower_rib_wall.value)),
    m_wipe_tower_gap_wall(float(config.prime_tower_skip_points.value)),
    m_wipe_tower_framework(config.prime_tower_enable_framework.value),
    m_extra_flow(float(config.wipe_tower_extra_flow / 100.)),
    m_extra_spacing(float(config.wipe_tower_extra_spacing / 100.)),
    m_y_shift(0.f),
    m_z_pos(0.f),
    m_z_offset(config.z_offset),
    m_bridging(float(config.wipe_tower_bridging)),
    m_no_sparse_layers(config.wipe_tower_no_sparse_layers),
    m_gcode_flavor(config.gcode_flavor),
    m_travel_speed(config.travel_speed),
    m_infill_speed(default_region_config.sparse_infill_speed),
    m_perimeter_speed(default_region_config.inner_wall_speed),
    m_current_tool(initial_tool),
    wipe_volumes(wiping_matrix),
    m_wipe_tower_max_purge_speed(float(config.wipe_tower_max_purge_speed)),
    m_enable_timelapse_print(config.timelapse_type.value == TimelapseType::tlSmooth)
{
    // Read absolute value of first layer speed, if given as percentage,
    // it is taken over following default. Speeds from config are not
    // easily accessible here.
    const float default_speed = 60.f;
    m_first_layer_speed = config.initial_layer_speed;
    if (m_first_layer_speed == 0.f) // just to make sure autospeed doesn't break it.
        m_first_layer_speed = default_speed / 2.f;

    // Autospeed may be used...
    if (m_infill_speed == 0.f)
        m_infill_speed = 80.f;
    if (m_perimeter_speed == 0.f)
        m_perimeter_speed = 80.f;

    // Calculate where the priming lines should be - very naive test not detecting parallelograms etc.
    const std::vector<Vec2d>& bed_points = config.printable_area.values;
    BoundingBoxf bb(bed_points);
    m_bed_width = float(bb.size().x());
    m_bed_shape = (bed_points.size() == 4 ? RectangularBed : CircularBed);

    if (m_bed_shape == CircularBed) {
        // this may still be a custom bed, check that the points are roughly on a circle
        double r2 = std::pow(m_bed_width/2., 2.);
        double lim2 = std::pow(m_bed_width/10., 2.);
        Vec2d center = bb.center();
        for (const Vec2d& pt : bed_points)
            if (std::abs(std::pow(pt.x()-center.x(), 2.) + std::pow(pt.y()-center.y(), 2.) - r2) > lim2) {
                m_bed_shape = CustomBed;
                break;
            }
    }

    m_bed_bottom_left = m_bed_shape == RectangularBed
                  ? Vec2f(bed_points.front().x(), bed_points.front().y())
                  : Vec2f::Zero();

    m_prime_tower_enhance_type = config.prime_tower_enhance_type;

    if (m_wipe_tower_rib_wall) {
        // Square shape : initialize the tower width (not the final value)
        double layer_height = config.initial_layer_print_height.value;
        float  wipe_volume  = config.prime_volume.value;

        if (layer_height > 0 && wipe_volume > 0) {
            float length_to_extrude = volume_to_length(wipe_volume, m_perimeter_width * m_extra_flow, layer_height);
            m_wipe_tower_width      = align_ceil(std::sqrt(length_to_extrude * m_perimeter_width * m_extra_flow), m_perimeter_width) +
                                 m_perimeter_width;
        }
    }
}



void WipeTowerCreality::set_extruder(size_t idx, const PrintConfig& config)
{
    //while (m_filpar.size() < idx+1)   // makes sure the required element is in the vector
    m_filpar.push_back(FilamentParameters());

    m_filpar[idx].material = config.filament_type.get_at(idx);
    m_filpar[idx].is_soluble = config.filament_soluble.get_at(idx);
    m_filpar[idx].temperature = config.nozzle_temperature.get_at(idx);
    m_filpar[idx].first_layer_temperature = config.nozzle_temperature_initial_layer.get_at(idx);
    m_filpar[idx].category                = config.filament_adhesiveness_category.get_at(idx);
    m_filpar[idx].filament_area = float((M_PI/4.f) * pow(config.filament_diameter.get_at(idx), 2)); // all extruders are assumed to have the same filament diameter at this point
    float nozzle_diameter = float(config.nozzle_diameter.get_at(idx));
    m_filpar[idx].nozzle_diameter = nozzle_diameter; // to be used in future with (non-single) multiextruder MM

    float max_vol_speed = float(config.filament_max_volumetric_speed.get_at(idx));
    if (max_vol_speed!= 0.f)
        m_filpar[idx].max_e_speed = (max_vol_speed / filament_area());

    m_perimeter_width = nozzle_diameter * Width_To_Nozzle_Ratio; // all extruders are now assumed to have the same diameter

    {
        std::istringstream stream{config.filament_ramming_parameters.get_at(idx)};
        float speed = 0.f;
        stream >> m_filpar[idx].ramming_line_width_multiplicator >> m_filpar[idx].ramming_step_multiplicator;
        m_filpar[idx].ramming_line_width_multiplicator /= 100;
        m_filpar[idx].ramming_step_multiplicator /= 100;
        while (stream >> speed)
            m_filpar[idx].ramming_speed.push_back(speed);
        // ramming_speed now contains speeds to be used for every 0.25s piece of the ramming line.
        // This allows to have the ramming flow variable. The 0.25s value is how it is saved in config
        // and the same time step has to be used when the ramming is performed.
    }

    m_used_filament_length.resize(std::max(m_used_filament_length.size(), idx + 1)); // makes sure that the vector is big enough so we don't have to check later
    m_filpar[idx].retract_length = config.retraction_length.get_at(idx);
    m_filpar[idx].retract_speed  = config.retraction_speed.get_at(idx);
    m_filpar[idx].wipe_dist      = config.wipe_distance.get_at(idx);
}

WipeTower::ToolChangeResult WipeTowerCreality::tool_change(size_t tool, bool extrude_perimeter, bool first_toolchange_to_nonsoluble)
{
    size_t old_tool = m_current_tool;
    float  wipe_length = 0.f;
    float wipe_area = 0.f;
	float wipe_volume = 0.f;
    float purge_volume = 0.0f;
    float wipe_depth  = 0.f;
    float planned_depth = 0.f;
    bool  round_wipe_wall_gap = false;
    float round_wipe_wall_bottom_depth = 0.f;
    float round_wipe_wall_top_depth = 0.f;
	// Finds this toolchange info
	if (tool != (unsigned int)(-1))
	{
		for (const auto &b : m_layer_info->tool_changes)
			if ( b.new_tool == tool ) {
                wipe_length = b.wipe_length;
                wipe_volume = b.wipe_volume;
                purge_volume = b.purge_volume;
                planned_depth = b.required_depth;
                wipe_depth  = b.required_depth;
                round_wipe_wall_gap = b.round_wipe_wall;
                round_wipe_wall_bottom_depth = b.round_wipe_wall_bottom_depth;
                round_wipe_wall_top_depth = b.round_wipe_wall_top_depth;
				wipe_area = b.required_depth * m_layer_info->extra_spacing;
				break;
			}
	}
	else {
		// Otherwise we are going to Unload only. And m_layer_info would be invalid.
	}

    bool  first_layer = is_first_layer();
    float factor      = first_layer ? 2.f : (1.f + m_extra_flow);
    round_wipe_wall_gap = tool != (unsigned int)(-1) && round_wipe_wall_gap;
    const float wipe_wall_gap = round_wipe_wall_gap ? round_wipe_wall_bottom_depth + round_wipe_wall_top_depth : 0.f;
    const float wipe_wall_inset = round_wipe_wall_gap ? round_wipe_wall_bottom_depth * m_layer_info->extra_spacing : 0.f;
    if (tool != (unsigned int)(-1))
        wipe_depth = std::max(0.f, planned_depth - wipe_wall_gap * m_layer_info->extra_spacing);

    WipeTower::WipeTowerBlock* block     = nullptr;
    float cur_depth = factor / 2.f * m_perimeter_width;
    if (tool != (unsigned) (-1)) {
        block = get_block_by_category(m_filpar[tool].category, false);
        if (!block) {
            assert(block != nullptr);
            return WipeTower::ToolChangeResult();
        }
        // m_cur_block = block;
        cur_depth = block->cur_depth;
    }

   /* WipeTower::box_coordinates cleaning_box(Vec2f(m_perimeter_width, cur_depth),
		m_wipe_tower_width - m_perimeter_width, wipe_depth);*/
    WipeTower::box_coordinates cleaning_box(Vec2f(factor / 2.f * m_perimeter_width, cur_depth + wipe_wall_inset), m_wipe_tower_width - factor * m_perimeter_width, wipe_depth);
    const float wipe_line_width = (first_layer ? 1.f : m_extra_flow) * m_perimeter_width;
    const float wipe_wall_x_outset = round_wipe_wall_gap ? wipe_line_width : 0.f;
    const float wipe_wall_bottom_outset = round_wipe_wall_gap ? round_wipe_wall_bottom_depth : 0.f;
    const float wipe_wall_top_outset = round_wipe_wall_gap ? round_wipe_wall_top_depth : 0.f;
    const WipeTower::box_coordinates actual_wipe_box = round_wipe_wall_gap ? actual_serpentine_wipe_box(cleaning_box, wipe_line_width) : cleaning_box;
    WipeTower::box_coordinates wipe_wall_box = round_wipe_wall_gap ?
        expand_wipe_box(actual_wipe_box, wipe_wall_x_outset, wipe_wall_bottom_outset, wipe_wall_top_outset) :
        cleaning_box;

    WipeTower::box_coordinates toolchange_wipe_box = cleaning_box;

	WipeTowerWriterCreality writer(m_layer_height, m_perimeter_width, m_gcode_flavor, m_filpar);
    writer.prefix(";will_change_tool\n");
	writer.set_extrusion_flow(m_extrusion_flow)
		.set_z(m_z_pos)
		.set_initial_tool(m_current_tool)
        .set_y_shift(m_y_shift + (tool!=(unsigned int)(-1) && (m_current_shape == SHAPE_REVERSED) ? m_layer_info->depth - m_layer_info->toolchanges_depth(): 0.f))
		.append(";--------------------\n"
				"; CP TOOLCHANGE START\n")
		.comment_with_value(" toolchange #", m_num_tool_changes + 1); // the number is zero-based

    if (tool != (unsigned)(-1)){
        writer.append(std::string("; material : " + (m_current_tool < m_filpar.size() ? m_filpar[m_current_tool].material : "(NONE)") + " -> " + m_filpar[tool].material + "\n").c_str())
            .append(";--------------------\n");
        //writer.append(";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_Start) + "\n");
    }

    writer.speed_override_backup();
	writer.speed_override(100);

	//Vec2f initial_position = cleaning_box.ld + Vec2f(0.f, m_depth_traversed);
    //writer.set_initial_position(initial_position, m_wipe_tower_width, m_wipe_tower_depth, m_internal_rotation);

    //writer.travel(cleaning_box.ld);

    // Ram the hot material out of the melt zone, retract the filament into the cooling tubes and let it cool.
    if (tool != (unsigned int)-1){ 			// This is not the last change.
        Vec2f initial_position = get_next_pos(cleaning_box, wipe_length);
        writer.set_initial_position(initial_position, m_wipe_tower_width, m_wipe_tower_depth, m_internal_rotation);
        writer.append(";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_Start) + "\n");
        toolchange_Unload(writer, cleaning_box, m_filpar[m_current_tool].material,
                          is_first_layer() ? m_filpar[tool].first_layer_temperature : m_filpar[tool].temperature);
        toolchange_Change(writer, tool, m_filpar[tool].material); // Change the tool, set a speed override for soluble and flex materials.

        //writer.travel(writer.x(), writer.y()-m_perimeter_width); // cooling and loading were done a bit down the road

        toolchange_Wipe(writer, toolchange_wipe_box, wipe_volume, false);     // Wipe the newly loaded filament until the end of the assigned wipe area.

        writer.append(";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_End) + "\n");
        ++ m_num_tool_changes;
    } else {
        writer.set_initial_position(cleaning_box.ld);
        toolchange_Unload(writer, cleaning_box, m_filpar[m_current_tool].material, m_filpar[m_current_tool].temperature);
    }

    //m_depth_traversed += wipe_area;

    if (tool != (unsigned) (-1)) {
        block->cur_depth += planned_depth;
        block->last_filament_change_id = tool;
    }

	writer.speed_override_restore();
    writer.feedrate(m_travel_speed * 60.f)
          .flush_planner_queue()
          .reset_extruder()
          .append("; CP TOOLCHANGE END\n"
                  ";----------------\n"
                  "\n\n");

    // Ask our writer about how much material was consumed:
    if (m_current_tool < m_used_filament_length.size())
        m_used_filament_length[m_current_tool] += writer.get_and_reset_used_filament_length();

    WipeTower::ToolChangeResult result = construct_tcr(writer, false, old_tool, false, purge_volume, true);
    if (tool != (unsigned)(-1) && round_wipe_wall_gap) {
        auto rotated_point = [&writer](const Vec2f &pt) { return writer.point_rotated(pt); };
        result.wipe_tower_inner_wall_box_valid = true;
        result.wipe_tower_inner_wall_box[0] = rotated_point(wipe_wall_box.ld);
        result.wipe_tower_inner_wall_box[1] = rotated_point(wipe_wall_box.rd);
        result.wipe_tower_inner_wall_box[2] = rotated_point(wipe_wall_box.ru);
        result.wipe_tower_inner_wall_box[3] = rotated_point(wipe_wall_box.lu);
        const WipeWallEntry wall_entry = wipe_wall_entry_from_gap_points(wipe_wall_box, m_wall_skip_points);
        result.wipe_tower_inner_wall_start_pos_valid = wall_entry.valid;
        result.wipe_tower_inner_wall_start_pos = rotated_point(wall_entry.entry);
        result.wipe_tower_inner_wall_approach_pos_valid = wall_entry.valid;
        result.wipe_tower_inner_wall_approach_pos = rotated_point(wall_entry.approach);
    }
    return result;
}


// Ram the hot material out of the melt zone, retract the filament into the cooling tubes and let it cool.
void WipeTowerCreality::toolchange_Unload(
	WipeTowerWriterCreality &writer,
	const WipeTower::box_coordinates 	&cleaning_box,
	const std::string&		 current_material,
	const int 				 new_temperature)
{
#if 0
	float xl = cleaning_box.ld.x() + 1.f * m_perimeter_width;
	float xr = cleaning_box.rd.x() - 1.f * m_perimeter_width;

    const float line_width = m_perimeter_width * m_filpar[m_current_tool].ramming_line_width_multiplicator;       // desired ramming line thickness
	const float y_step = line_width * m_filpar[m_current_tool].ramming_step_multiplicator * m_extra_spacing; // spacing between lines in mm

    const Vec2f ramming_start_pos = Vec2f(xl, cleaning_box.ld.y() + m_depth_traversed + y_step/2.f);

    writer.append("; CP TOOLCHANGE UNLOAD\n")
        .change_analyzer_line_width(line_width);

	unsigned i = 0;										// iterates through ramming_speed
	m_left_to_right = true;								// current direction of ramming
	float remaining = xr - xl ;							// keeps track of distance to the next turnaround
	float e_done = 0;									// measures E move done from each segment

    writer.set_position(ramming_start_pos);

	Vec2f end_of_ramming(writer.x(),writer.y());
    writer.change_analyzer_line_width(m_perimeter_width);   // so the next lines are not affected by ramming_line_width_multiplier

    // Retraction:
    float old_x = writer.x();
    float turning_point = (!m_left_to_right ? xl : xr );
    // Wipe tower should only change temperature with single extruder MM. Otherwise, all temperatures should
    // be already set and there is no need to change anything. Also, the temperature could be changed
    // for wrong extruder.
    {
        if (new_temperature != 0 && (new_temperature != m_old_temperature || is_first_layer()) ) { 	// Set the extruder temperature, but don't wait.
            // If the required temperature is the same as last time, don't emit the M104 again (if user adjusted the value, it would be reset)
            // However, always change temperatures on the first layer (this is to avoid issues with priming lines turned off).
            writer.set_extruder_temp(new_temperature, false);
            m_old_temperature = new_temperature;
        }
    }

    // this is to align ramming and future wiping extrusions, so the future y-steps can be uniform from the start:
    // the perimeter_width will later be subtracted, it is there to not load while moving over just extruded material
    Vec2f pos = Vec2f(end_of_ramming.x(), end_of_ramming.y() + (y_step/m_extra_spacing-m_perimeter_width) / 2.f + m_perimeter_width);
    writer.set_position(pos);

    writer.resume_preview()
          .flush_planner_queue();
#endif
}

// Change the tool, set a speed override for soluble and flex materials.
void WipeTowerCreality::toolchange_Change(
	WipeTowerWriterCreality &writer,
    const size_t 	new_tool,
    const std::string&  new_material)
{
#if ORCA_CHECK_GCODE_PLACEHOLDERS
    writer.append("; CP TOOLCHANGE CHANGE \n");
#endif
    // Ask the writer about how much of the old filament we consumed:
    if (m_current_tool < m_used_filament_length.size())
    	m_used_filament_length[m_current_tool] += writer.get_and_reset_used_filament_length();

    // This is where we want to place the custom gcodes. We will use placeholders for this.
    // These will be substituted by the actual gcodes when the gcode is generated.
    writer.append("[change_filament_gcode]\n");
    //std::string z_up_for_firmware = "[z_up_for_firmware]"

     //writer.z_hop(0.4f + m_z_offset, 1200.0f, "G0"); // 固件bug,临时抬升0.4,防止Z高度错误导致的移动到擦拭塔时产生剐蹭
    writer.relative_zhop(0.4f + m_z_offset, 1200.0f, "relative_zhop_up_for_firmware G0"); // 固件bug,临时抬升0.4,防止Z高度错误导致的移动到擦拭塔时产生剐蹭
    writer.append("[move_around_wipe_tower]\n");
    // Travel to where we assume we are. Custom toolchange or some special T code handling (parking extruder etc)
    // gcode could have left the extruder somewhere, we cannot just start extruding. We should also inform the
    // postprocessor that we absolutely want to have this in the gcode, even if it thought it is the same as before.
    //writer.relative_zhop(m_z_offset, 0.0, "relative_zhop_recovery_for_firmware G1");

    Vec2f current_pos = writer.pos_rotated();
    writer
        .feedrate_(m_travel_speed * 60.f) // see https://github.com/prusa3d/PrusaSlicer/issues/5483
        .append(std::string("X") + Slic3r::float_to_string_decimal_point(current_pos.x()) + " Y" +
                Slic3r::float_to_string_decimal_point(current_pos.y()) + WipeTower::never_skip_tag() + "\n");

    writer.relative_zhop(m_z_offset, 0.0, "relative_zhop_recovery_for_firmware G1");

    writer.append("[deretraction_from_wipe_tower_generator]");

     // Orca TODO: handle multi extruders
    // The toolchange Tn command will be inserted later, only in case that the user does
    // not provide a custom toolchange gcode.
	writer.set_tool(new_tool); // This outputs nothing, the writer just needs to know the tool has changed.
    // writer.append("[filament_start_gcode]\n");


	writer.flush_planner_queue();
	m_current_tool = new_tool;
}

// Wipe the newly loaded filament until the end of the assigned wipe area.
void WipeTowerCreality::toolchange_Wipe(
	WipeTowerWriterCreality &writer,
	const WipeTower::box_coordinates  &cleaning_box,
	float wipe_volume,
    bool round_wipe_wall)
{
    // Increase flow on first layer, slow down print.
    writer.set_extrusion_flow(m_extrusion_flow * (is_first_layer() ? 1.18f : 1.f)).append("; CP TOOLCHANGE WIPE\n");
    float retract_length = m_filpar[m_current_tool].retract_length;
    float retract_speed  = m_filpar[m_current_tool].retract_speed * 60;

    bool  first_layer = is_first_layer();
    const float line_width = first_layer ? m_perimeter_width : m_perimeter_width * m_extra_flow;
    if (!first_layer) {
        writer.set_extrusion_flow(m_extrusion_flow * m_extra_flow);
        writer.change_analyzer_line_width(line_width);
    }

    const float target_speed = first_layer || (m_num_tool_changes <= 1 && m_no_sparse_layers) ?
                                   m_first_layer_speed * 60.f :
                                   std::min(m_wipe_tower_max_purge_speed * 60.f, m_infill_speed * 60.f);
    float wipe_speed = 0.33f * target_speed;

    m_left_to_right = ((m_cur_layer_id + 3) % 4 >= 2);

    const WipeTower::box_coordinates wipe_box = round_wipe_wall ? shrink_wipe_box(cleaning_box, line_width) : cleaning_box;

    const float wipe_width  = wipe_box.rd.x() - wipe_box.ld.x();
    const float wipe_height = wipe_box.lu.y() - wipe_box.ld.y();
    Vec2f last_wipe_extrude_from = writer.pos();
    bool  has_last_wipe_extrude = false;
    auto emit_entrance_wipe = [&](bool left_to_right, float y) {
        constexpr float entrance_wipe_length = 3.f;
        const float entry_len = std::min(entrance_wipe_length, wipe_width);
        if (entry_len <= WT_EPSILON)
            return;

        const float dir = left_to_right ? 1.f : -1.f;
        const Vec2f entry_from(left_to_right ? wipe_box.ld.x() : wipe_box.rd.x(), y);
        const Vec2f entry_to(entry_from.x() + dir * entry_len, y);
        writer.travel(entry_from, wipe_speed);
        writer.extrude(entry_to, wipe_speed);

        if (retract_length > WT_EPSILON) {
            const float back_len = std::min(entry_len * 0.5f, 1.5f);
            writer.retract(retract_length, retract_speed);
            writer.travel(Vec2f(entry_from.x() - dir * back_len, y), retract_speed * 0.25f);
            writer.travel(entry_to, retract_speed * 0.10f);
            writer.retract(-retract_length, retract_speed);
        }
    };

    if (wipe_width > WT_EPSILON && wipe_height > WT_EPSILON) {
        const float bottom_y    = wipe_box.ld.y();
        const float top_y       = wipe_box.lu.y();
        const float inner_top_y = std::max(bottom_y, top_y - line_width);
        const bool  top_to_bottom = std::abs(writer.y() - top_y) < std::abs(writer.y() - bottom_y);
        const float first_y     = top_to_bottom ? inner_top_y : bottom_y;
        const float last_y      = top_to_bottom ? bottom_y : inner_top_y;
        float y = first_y;
        const float y_step = top_to_bottom ? -line_width : line_width;
        bool  left_to_right = std::abs(writer.x() - wipe_box.ld.x()) <= std::abs(writer.x() - wipe_box.rd.x());

        emit_entrance_wipe(left_to_right, y);
        while (true) {
            if (wipe_speed < 0.34f * target_speed)
                wipe_speed = 0.375f * target_speed;
            else if (wipe_speed < 0.377f * target_speed)
                wipe_speed = 0.458f * target_speed;
            else if (wipe_speed < 0.46f * target_speed)
                wipe_speed = 0.875f * target_speed;
            else
                wipe_speed = std::min(target_speed, wipe_speed + 50.f);

            last_wipe_extrude_from = writer.pos();
            has_last_wipe_extrude = true;
            writer.extrude(Vec2f(left_to_right ? wipe_box.rd.x() : wipe_box.ld.x(), y), wipe_speed);

            const float next_y = y + y_step;
            if (top_to_bottom ? next_y < last_y - WT_EPSILON : next_y > last_y + WT_EPSILON)
                break;
            y = top_to_bottom ? std::max(next_y, last_y) : std::min(next_y, last_y);
            writer.extrude(Vec2f(writer.x(), y), wipe_speed);
            left_to_right = !left_to_right;
        }
    } else if (wipe_width > WT_EPSILON) {
        const bool left_to_right = std::abs(writer.x() - wipe_box.ld.x()) <= std::abs(writer.x() - wipe_box.rd.x());
        emit_entrance_wipe(left_to_right, wipe_box.ld.y());
        last_wipe_extrude_from = writer.pos();
        has_last_wipe_extrude = true;
        writer.extrude(left_to_right ? wipe_box.rd : wipe_box.ld, wipe_speed);
    }
    // We may be going back to the model - wipe the nozzle. If this is followed
    //擦拭完去打产品（擦拭-抬升-产品）
    //writer.add_wipe_point(writer.x(), writer.y()).add_wipe_point(!m_left_to_right ? m_wipe_tower_width : 0.f, writer.y());
    //擦拭完继续填充擦拭塔下一个block（擦拭-空走-继续填充下一block）  暂时不加  否则跳转不对
    Vec2f wipe_target = writer.pos();
    if (has_last_wipe_extrude) {
        const Vec2f wipe_dir = last_wipe_extrude_from - writer.pos();
        const float wipe_len = wipe_dir.norm();
        if (wipe_len > WT_EPSILON)
            wipe_target = writer.pos() + wipe_dir * (std::min(2.f, wipe_len) / wipe_len);
    }
    writer.add_wipe_group(writer.pos(), wipe_target);
    writer.append(";wipe_finish_path\n");
    writer.set_feedrate(retract_speed);

    if (m_layer_info != m_plan.end() && m_current_tool != m_layer_info->tool_changes.back().new_tool)
        m_left_to_right = !m_left_to_right;

    writer.set_extrusion_flow(m_extrusion_flow); // Reset the extrusion flow.
    if (!first_layer) {
        writer.change_analyzer_line_width(m_perimeter_width);
    }
}

// BBS
WipeTower::box_coordinates WipeTowerCreality::align_perimeter(const WipeTower::box_coordinates& perimeter_box)
{
    WipeTower::box_coordinates aligned_box = perimeter_box;

    float spacing = m_extra_spacing * m_perimeter_width;
    float up      = perimeter_box.lu(1) - m_perimeter_width;
    up            = align_ceil(up, spacing);
    up += m_perimeter_width;
    up = std::min(up, m_wipe_tower_depth);

    float down = perimeter_box.ld(1) - m_perimeter_width;
    down       = align_floor(down, spacing);
    down += m_perimeter_width;
    down = std::max(down, -m_y_shift);

    aligned_box.lu(1) = aligned_box.ru(1) = up;
    aligned_box.ld(1) = aligned_box.rd(1) = down;

    return aligned_box;
}

WipeTower::ToolChangeResult WipeTowerCreality::finish_layer(bool extrude_perimeter, bool extruder_fill)
{
	assert(! this->layer_finished());
    m_current_layer_finished = true;

    size_t old_tool = m_current_tool;

	WipeTowerWriterCreality writer(m_layer_height, m_perimeter_width, m_gcode_flavor, m_filpar);
	writer.set_extrusion_flow(m_extrusion_flow)
		.set_z(m_z_pos)
		.set_initial_tool(m_current_tool)
        .set_y_shift(m_y_shift - (m_current_shape == SHAPE_REVERSED ? m_layer_info->toolchanges_depth() : 0.f));


	// Slow down on the 1st layer.
    // If spare layers are excluded -> if 1 or less toolchange has been done, it must be still the first layer, too. So slow down.
    bool first_layer = is_first_layer() || (m_num_tool_changes <= 1 && m_no_sparse_layers);
    float                      feedrate      = first_layer ? m_first_layer_speed * 60.f : std::min(m_wipe_tower_max_purge_speed * 60.f, m_infill_speed * 60.f);
    float current_depth = m_layer_info->depth - m_layer_info->toolchanges_depth();
    WipeTower::box_coordinates fill_box(Vec2f(m_perimeter_width, m_layer_info->depth-(current_depth-m_perimeter_width)),
                             m_wipe_tower_width - 2 * m_perimeter_width, current_depth-m_perimeter_width);


    writer.set_initial_position((m_left_to_right ? fill_box.ru : fill_box.lu), m_wipe_tower_width, m_wipe_tower_depth, m_internal_rotation);

    bool toolchanges_on_layer = m_layer_info->toolchanges_depth() > WT_EPSILON;
    const WipeTower::box_coordinates fill_wall_box = expand_wipe_block_box(fill_box, m_perimeter_width);
    WipeTower::box_coordinates wt_box(Vec2f(0.f, (m_current_shape == SHAPE_REVERSED ? m_layer_info->toolchanges_depth() : 0.f)),
                        m_wipe_tower_width, m_layer_info->depth + m_perimeter_width);
    // Keep the public tower wall on the nominal tower boundary so it touches the inner wipe-block wall.

    float retract_length = m_filpar[m_current_tool].retract_length;
    float retract_speed  = m_filpar[m_current_tool].retract_speed * 60;
    // inner perimeter of the sparse section, if there is space for it:
    if (fill_wall_box.ru.y() - fill_wall_box.rd.y() > m_perimeter_width - WT_EPSILON)
        writer.rectangle_fill_box(fill_wall_box.ld, fill_wall_box.rd.x() - fill_wall_box.ld.x(), fill_wall_box.ru.y() - fill_wall_box.rd.y(), retract_length,
                                  retract_speed, feedrate);

    // we are in one of the corners, travel to ld along the perimeter:
    if (std::abs(writer.x() - fill_box.ld.x()) > EPSILON) writer.travel(fill_box.ld.x(),writer.y());
    if (std::abs(writer.y() - fill_box.ld.y()) > EPSILON) writer.travel(writer.x(),fill_box.ld.y());

    // Extrude infill to support the material to be printed above.
    const float dy = (fill_box.lu.y() - fill_box.ld.y() - m_perimeter_width);
    float left = fill_box.lu.x() + 2 * m_perimeter_width;
    float right = fill_box.ru.x() - 2 * m_perimeter_width;
    //if (extruder_fill && dy > m_perimeter_width)
    if ( dy > m_perimeter_width)
    {
        writer.travel(fill_box.ld + Vec2f(m_perimeter_width * 2, 0.f))
              .append(";--------------------\n"
                      "; CP EMPTY GRID START\n")
              .comment_with_value(wipe_tower_layer_change_tag, m_num_layer_changes + 1);

        // Is there a soluble filament wiped/rammed at the next layer?
        // If so, the infill should not be sparse.
        bool solid_infill = m_layer_info+1 == m_plan.end()
                          ? false
                          : std::any_of((m_layer_info+1)->tool_changes.begin(),
                                        (m_layer_info+1)->tool_changes.end(),
                                   [this](const WipeTowerInfo::ToolChange& tch) {
                                       return m_filpar[tch.new_tool].is_soluble
                                           || m_filpar[tch.old_tool].is_soluble;
                                   });
        solid_infill |= first_layer && m_adhesion;

        if (solid_infill) {
            float sparse_factor = 1.5f; // 1=solid, 2=every other line, etc.
            if (first_layer) { // the infill should touch perimeters
                left  -= m_perimeter_width;
                right += m_perimeter_width;
                sparse_factor = 1.f;
            }
            float y = fill_box.ld.y() + m_perimeter_width;
            int n = dy / (m_perimeter_width * sparse_factor);
            float spacing = (dy-m_perimeter_width)/(n-1);
            int i=0;
            for (i=0; i<n; ++i) {
                writer.extrude(writer.x(), y, feedrate)
                      .extrude(i%2 ? left : right, y);
                y = y + spacing;
            }
            writer.extrude(writer.x(), fill_box.lu.y());
        } else {
            // Extrude an inverse U at the left of the region and the sparse infill.
            writer.extrude(fill_box.lu + Vec2f(m_perimeter_width * 2, 0.f), feedrate);

            const int n = 1+int((right-left)/m_bridging);
            const float dx = (right-left)/n;
            for (int i=1;i<=n;++i) {
                float x=left+dx*i;
                writer.travel(x,writer.y());
                writer.extrude(x,i%2 ? fill_box.rd.y() : fill_box.ru.y());
            }
        }

        writer.append("; CP EMPTY GRID END\n"
                      ";------------------\n\n\n\n\n\n\n");
    }

    // outer perimeter (always):
    /* WipeTower::box_coordinates wt_box(Vec2f(0.f, (m_current_shape == SHAPE_REVERSED ? m_layer_info->toolchanges_depth() : 0.f)),
                                      m_wipe_tower_width, m_layer_info->depth + m_perimeter_width);
    wt_box = this->align_perimeter(wt_box);
    if (extrude_perimeter) {
        writer.rectangle(wt_box, feedrate);
    }*/

    const float spacing = m_perimeter_width - m_layer_height*float(1.-M_PI_4);

    // This block creates the stabilization cone.
    // First define a lambda to draw the rectangle with stabilization.
    auto supported_rectangle = [this, &writer, spacing](const WipeTower::box_coordinates& wt_box, double feedrate, bool infill_cone) -> Polygon {
        const auto [R, support_scale] = WipeTower2::get_wipe_tower_cone_base(m_wipe_tower_width, m_wipe_tower_height, m_wipe_tower_depth, m_wipe_tower_cone_angle);

        double z = m_no_sparse_layers ? (m_current_height + m_layer_info->height) : m_layer_info->z; // the former should actually work in both cases, but let's stay on the safe side (the 2.6.0 is close)

        double r = std::tan(Geometry::deg2rad(m_wipe_tower_cone_angle/2.f)) * (m_wipe_tower_height - z);
        Vec2f center = (wt_box.lu + wt_box.rd) / 2.;
        double w = wt_box.lu.y() - wt_box.ld.y();
        enum Type {
            Arc,
            Corner,
            ArcStart,
            ArcEnd
        };

        // First generate vector of annotated point which form the boundary.
        std::vector<std::pair<Vec2f, Type>> pts = {{wt_box.ru, Corner}};
        if (double alpha_start = std::asin((0.5*w)/r); ! std::isnan(alpha_start) && r > 0.5*w+0.01) {
            for (double alpha = alpha_start; alpha < M_PI-alpha_start+0.001; alpha+=(M_PI-2*alpha_start) / 40.)
                pts.emplace_back(Vec2f(center.x() + r*std::cos(alpha)/support_scale, center.y() + r*std::sin(alpha)), alpha == alpha_start ? ArcStart : Arc);
            pts.back().second = ArcEnd;
        }
        pts.emplace_back(wt_box.lu, Corner);
        pts.emplace_back(wt_box.ld, Corner);
        for (int i=int(pts.size())-3; i>0; --i)
            pts.emplace_back(Vec2f(pts[i].first.x(), 2*center.y()-pts[i].first.y()), i == int(pts.size())-3 ? ArcStart : i == 1 ? ArcEnd : Arc);
        pts.emplace_back(wt_box.rd, Corner);

        // Create a Polygon from the points.
        Polygon poly;
        for (const auto& [pt, tag] : pts)
            poly.points.push_back(Point::new_scale(pt));

        // Prepare polygons to be filled by infill.
        Polylines polylines;
        if (infill_cone && m_wipe_tower_width > 2*spacing && m_wipe_tower_depth > 2*spacing) {
            ExPolygons infill_areas;
            ExPolygon wt_contour(poly);
            Polygon wt_rectangle(Points{Point::new_scale(wt_box.ld), Point::new_scale(wt_box.rd), Point::new_scale(wt_box.ru), Point::new_scale(wt_box.lu)});
            wt_rectangle = offset(wt_rectangle, scale_(-spacing/2.)).front();
            wt_contour = offset_ex(wt_contour, scale_(-spacing/2.)).front();
            infill_areas = diff_ex(wt_contour, wt_rectangle);
            if (infill_areas.size() == 2) {
                ExPolygon& bottom_expoly = infill_areas.front().contour.points.front().y() < infill_areas.back().contour.points.front().y() ? infill_areas[0] : infill_areas[1];
                std::unique_ptr<Fill> filler(Fill::new_from_type(ipMonotonicLine));
                filler->angle = Geometry::deg2rad(45.f);
                filler->spacing = spacing;
                FillParams params;
                params.density = 1.f;
                Surface surface(stBottom, bottom_expoly);
                filler->bounding_box = get_extents(bottom_expoly);
                polylines = filler->fill_surface(&surface, params);
                if (! polylines.empty()) {
                    if (polylines.front().points.front().x() > polylines.back().points.back().x()) {
                        std::reverse(polylines.begin(), polylines.end());
                        for (Polyline& p : polylines)
                            p.reverse();
                    }
                }
            }
        }

        // Find the closest corner and travel to it.
        int start_i = 0;
        double min_dist = std::numeric_limits<double>::max();
        for (int i=0; i<int(pts.size()); ++i) {
            if (pts[i].second == Corner) {
                double dist = (pts[i].first - Vec2f(writer.x(), writer.y())).squaredNorm();
                if (dist < min_dist) {
                    min_dist = dist;
                    start_i = i;
                }
            }
        }
        writer.travel(pts[start_i].first);

        // Now actually extrude the boundary (and possibly infill):
        int i = start_i+1 == int(pts.size()) ? 0 : start_i + 1;
        while (i != start_i) {
            writer.extrude(pts[i].first, feedrate);
            if (pts[i].second == ArcEnd) {
                // Extrude the infill.
                if (! polylines.empty()) {
                    // Extrude the infill and travel back to where we were.
                    bool mirror = ((pts[i].first.y() - center.y()) * (unscale(polylines.front().points.front()).y() - center.y())) < 0.;
                    for (const Polyline& line : polylines) {
                        writer.travel(center - (mirror ? 1.f : -1.f) * (unscale(line.points.front()).cast<float>() - center));
                        for (size_t i=0; i<line.points.size(); ++i)
                            writer.extrude(center - (mirror ? 1.f : -1.f) * (unscale(line.points[i]).cast<float>() - center));
                    }
                    writer.travel(pts[i].first);
                }
            }
            if (++i == int(pts.size()))
                i = 0;
        }
        writer.extrude(pts[start_i].first, feedrate);
        return poly;
    };

    auto chamfer = [this, &writer, spacing, first_layer](const WipeTower::box_coordinates& wt_box, double feedrate)->Polygon{
        WipeTower::box_coordinates _wt_box = wt_box; // align_perimeter(wt_box);
        if (true) {
            writer.rectangle(_wt_box, feedrate);
        }

        Polygon poly;
        int loops_num = (m_wipe_tower_brim_width + spacing / 2.f) / spacing;
        const float max_chamfer_width = 3.f;
        if (!first_layer) {
            // stop print chamfer if depth changes
            if (m_layer_info->depth != m_plan.front().depth) {
                loops_num = 0;
            }
            else {
                // limit max chamfer width to 3 mm
                int chamfer_loops_num = (int)(max_chamfer_width / spacing);
                int dist_to_1st = m_layer_info - m_plan.begin() - m_first_layer_idx;
                loops_num = std::min(loops_num, chamfer_loops_num) - dist_to_1st;
            }
        }


        WipeTower::box_coordinates box = _wt_box;
        if (loops_num > 0) {
            for (size_t i = 0; i < loops_num; ++i) {
                box.expand(spacing);
                writer.rectangle(box);
            }
        }

        if(first_layer)
            m_wipe_tower_brim_width_real += loops_num * spacing;
        poly.points.emplace_back(Point::new_scale(box.ru));
        poly.points.emplace_back(Point::new_scale(box.lu));
        poly.points.emplace_back(Point::new_scale(box.ld));
        poly.points.emplace_back(Point::new_scale(box.rd));
        return poly;
    };

    feedrate = first_layer ? m_first_layer_speed * 60.f : m_perimeter_speed * 60.f;

    if(first_layer)
        m_wipe_tower_brim_width_real = 0.0f;
    // outer contour (always)
    bool use_cone = m_prime_tower_enhance_type == PrimeTowerEnhanceType::pteCone;
    Polygon poly;
    if(use_cone){
        bool infill_cone = first_layer && m_wipe_tower_width > 2*spacing && m_wipe_tower_depth > 2*spacing;
        poly = supported_rectangle(wt_box, feedrate, infill_cone);
    }else{
        poly = chamfer(wt_box, feedrate);
    }


    // brim (first layer only)
    if (first_layer) {
        size_t loops_num = (m_wipe_tower_brim_width + spacing/2.f) / spacing;

        for (size_t i = 0; i < loops_num; ++ i) {
            poly = offset(poly, scale_(spacing)).front();
            int cp = poly.closest_point_index(Point::new_scale(writer.x(), writer.y()));
            writer.travel(unscale(poly.points[cp]).cast<float>());
            for (int i=cp+1; true; ++i ) {
                if (i==int(poly.points.size()))
                    i = 0;
                writer.extrude(unscale(poly.points[i]).cast<float>());
                if (i == cp)
                    break;
            }
        }

        // Save actual brim width to be later passed to the Print object, which will use it
        // for skirt calculation and pass it to GLCanvas for precise preview box
        m_wipe_tower_brim_width_real += loops_num * spacing;
    }

    // Now prepare future wipe.
    int i = poly.closest_point_index(Point::new_scale(writer.x(), writer.y()));
    writer.add_wipe_point(writer.pos()).add_wipe_point(unscale(poly.points[i==0 ? int(poly.points.size())-1 : i-1]).cast<float>());

    // Ask our writer about how much material was consumed.
    // Skip this in case the layer is sparse and config option to not print sparse layers is enabled.
    if (! m_no_sparse_layers || toolchanges_on_layer || first_layer) {
        if (m_current_tool < m_used_filament_length.size())
            m_used_filament_length[m_current_tool] += writer.get_and_reset_used_filament_length();
        m_current_height += m_layer_info->height;
    }
    //writer.append(";current_path_finish\n");
    return construct_tcr(writer,false, old_tool, true, 0.f);
}

// Appends a toolchange into m_plan and calculates neccessary depth of the corresponding box
void WipeTowerCreality::plan_toolchange(float z_par, float layer_height_par, unsigned int old_tool, unsigned int new_tool, float wipe_volume, float purge_volume, bool flush_into_skeleton, bool round_wipe_wall)
{
	assert(m_plan.empty() || m_plan.back().z <= z_par + WT_EPSILON);	// refuses to add a layer below the last one

	if (m_plan.empty() || m_plan.back().z + WT_EPSILON < z_par) // if we moved to a new layer, we'll add it to m_plan first
		m_plan.push_back(WipeTowerInfo(z_par, layer_height_par));

    m_plan.back().round_wipe_wall = m_plan.back().round_wipe_wall || round_wipe_wall;

    if (m_first_layer_idx == size_t(-1) && (! m_no_sparse_layers || old_tool != new_tool || m_plan.size() == 1))
        m_first_layer_idx = m_plan.size() - 1;

    if (old_tool == new_tool)	// new layer without toolchanges - we are done
        return;

    float depth = 0.f;
    // The first layer does not use wipe_tower_extra_flow.
    float first_layer_z = m_plan[m_first_layer_idx].z;
    bool  first_layer   = first_layer_z - WT_EPSILON <= z_par && z_par <= first_layer_z + WT_EPSILON;
    // The width of outer wall is not affected by wipe_tower_extra_flow.
    float width = m_wipe_tower_width - m_perimeter_width - m_perimeter_width * (first_layer ? 1.0f : m_extra_flow);

    // BBS: if the wipe tower width is too small, the depth will be infinity
    if (width <= EPSILON)
        return;

    float length_to_extrude = volume_to_length(wipe_volume, m_perimeter_width * m_extra_flow, layer_height_par);
    depth += std::ceil(length_to_extrude / width) * m_perimeter_width * m_extra_flow;
	m_plan.back().tool_changes.push_back(WipeTowerInfo::ToolChange(old_tool, new_tool, depth, 0.0f, 0.0f, wipe_volume, length_to_extrude, purge_volume, flush_into_skeleton, round_wipe_wall));
}

void WipeTowerCreality::plan_tower()
{
    if (m_wipe_tower_brim_width < 0)
        m_wipe_tower_brim_width = WipeTower::get_auto_brim_by_height(m_wipe_tower_height);

    auto clear_round_wipe_wall_depths = [this]() {
        for (WipeTowerInfo& info : m_plan) {
            for (WipeTowerInfo::ToolChange& toolchange : info.tool_changes) {
                toolchange.required_depth -= toolchange.round_wipe_wall_bottom_depth + toolchange.round_wipe_wall_top_depth;
                toolchange.round_wipe_wall_bottom_depth = 0.f;
                toolchange.round_wipe_wall_top_depth = 0.f;
            }
        }
    };

    auto apply_round_wipe_wall_depths = [this]() {
        for (size_t layer_idx = 0; layer_idx < m_plan.size(); ++layer_idx) {
            WipeTowerInfo& info = m_plan[layer_idx];
            const bool first_layer = m_first_layer_idx != size_t(-1) && layer_idx == m_first_layer_idx;
            const float side_depth = wipe_block_wall_side_depth(first_layer, m_perimeter_width, m_extra_flow);

            for (WipeTowerInfo::ToolChange& toolchange : info.tool_changes) {
                if (!toolchange.round_wipe_wall)
                    continue;

                toolchange.round_wipe_wall = true;
                toolchange.round_wipe_wall_bottom_depth = side_depth;
                toolchange.round_wipe_wall_top_depth = side_depth;
                toolchange.required_depth += toolchange.round_wipe_wall_bottom_depth + toolchange.round_wipe_wall_top_depth;
            }
        }
    };

    clear_round_wipe_wall_depths();
    apply_round_wipe_wall_depths();

    if (m_wipe_tower_rib_wall)
    {
        // recalculate wipe_tower_with and layer's depth
        generate_wipe_tower_blocks();
        float max_depth = std::accumulate(m_wipe_tower_blocks.begin(), m_wipe_tower_blocks.end(), 0.f,
                                          [](float a, const auto& t) { return a + t.depth; }) +
                          m_perimeter_width;
        float square_width = align_ceil(std::sqrt(max_depth * m_extra_spacing * m_wipe_tower_width), m_perimeter_width);
        // std::cout << " before  m_wipe_tower_width = " << m_wipe_tower_width << "  max_depth = " << max_depth << std::endl;
        m_wipe_tower_width = square_width;
        float width        = m_wipe_tower_width - 2 * m_perimeter_width;
        clear_round_wipe_wall_depths();
        for (int idx = 0; idx < m_plan.size(); idx++) {
            for (auto& toolchange : m_plan[idx].tool_changes) {
                float length_to_extrude   = toolchange.wipe_length;
                float depth               = std::ceil(length_to_extrude / width) * m_perimeter_width * m_extra_flow;
                float nozzle_change_depth = 0;
                if (!m_filament_map.empty() && m_filament_map[toolchange.old_tool] != m_filament_map[toolchange.new_tool]) {
                    /*double e_flow                   = nozzle_change_extrusion_flow(m_plan[idx].height);
                    double length                   = m_filaments_change_length[toolchange.old_tool] / e_flow;
                    int    nozzle_change_line_count = std::ceil(length / (m_wipe_tower_width - 2 * m_nozzle_change_perimeter_width));
                    if (m_need_reverse_travel)
                        nozzle_change_depth = m_tpu_fixed_spacing * nozzle_change_line_count * m_nozzle_change_perimeter_width;
                    else
                        nozzle_change_depth = nozzle_change_line_count * m_nozzle_change_perimeter_width;*/
                    depth += nozzle_change_depth;
                }
                toolchange.nozzle_change_depth = nozzle_change_depth;
                toolchange.required_depth      = depth;
            }
        }
        apply_round_wipe_wall_depths();
    }
    generate_wipe_tower_blocks();

    // BBS
    // calculate extra spacing
    float max_depth = 0.f;
    for (const auto& block : m_wipe_tower_blocks) {
        max_depth += block.depth;
    }

    float min_wipe_tower_depth = 0.f;
    auto  iter                 = WipeTower::min_depth_per_height.begin();
    while (iter != WipeTower::min_depth_per_height.end()) {
        auto curr_height_to_depth = *iter;

        // This is the case that wipe tower height is lower than the first min_depth_to_height member.
        if (curr_height_to_depth.first >= m_wipe_tower_height) {
            min_wipe_tower_depth = curr_height_to_depth.second;
            break;
        }

        iter++;

        // If curr_height_to_depth is the last member, use its min_depth.
        if (iter == WipeTower::min_depth_per_height.end()) {
            min_wipe_tower_depth = curr_height_to_depth.second;
            break;
        }

        // If wipe tower height is between the current and next member, set the min_depth as linear interpolation between them
        auto next_height_to_depth = *iter;
        if (next_height_to_depth.first > m_wipe_tower_height) {
            float height_base    = curr_height_to_depth.first;
            float height_diff    = next_height_to_depth.first - curr_height_to_depth.first;
            float min_depth_base = curr_height_to_depth.second;
            float depth_diff     = next_height_to_depth.second - curr_height_to_depth.second;

            min_wipe_tower_depth = min_depth_base + (m_wipe_tower_height - curr_height_to_depth.first) / height_diff * depth_diff;
            break;
        }
    }

    if (max_depth < EPSILON && m_enable_timelapse_print)
    {
        if (m_enable_timelapse_print && max_depth < EPSILON)
            max_depth = min_wipe_tower_depth;

        if (max_depth + EPSILON < min_wipe_tower_depth)
            m_extra_spacing = min_wipe_tower_depth / max_depth;
        else
            m_extra_spacing = 1.f;

        for (int idx = 0; idx < m_plan.size(); idx++) {
            auto& info = m_plan[idx];
            if (idx == 0 && m_extra_spacing > 1.f + EPSILON) {
                // apply solid fill for the first layer
                info.extra_spacing = 1.f;
                for (auto& toolchange : info.tool_changes) {
                    float x_to_wipe     = volume_to_length(toolchange.wipe_volume, m_perimeter_width * m_extra_flow, info.height);
                    float line_len      = m_wipe_tower_width - 2 * m_perimeter_width;
                    float x_to_wipe_new = x_to_wipe * m_extra_spacing;
                    x_to_wipe_new       = std::floor(x_to_wipe_new / line_len) * line_len;
                    x_to_wipe_new       = std::max(x_to_wipe_new, x_to_wipe);

                    int line_count            = std::ceil((x_to_wipe_new - WT_EPSILON) / line_len);
                    toolchange.required_depth = line_count * m_perimeter_width * m_extra_flow;
                    toolchange.wipe_volume    = x_to_wipe_new / x_to_wipe * toolchange.wipe_volume;
                    //toolchange.wipe_length    = x_to_wipe_new;
                }
            } else {
                info.extra_spacing = m_extra_spacing;
                for (auto& toolchange : info.tool_changes) {
                    toolchange.required_depth *= m_extra_spacing;
                    toolchange.wipe_length = volume_to_length(toolchange.wipe_volume, m_perimeter_width * m_extra_flow, info.height);
                }
            }
        }
    }

    update_all_layer_depth(max_depth);
}

// Return index of first toolchange that switches to non-soluble extruder
// ot -1 if there is no such toolchange.
int WipeTowerCreality::first_toolchange_to_nonsoluble(
        const std::vector<WipeTowerInfo::ToolChange>& tool_changes) const
{
    for (size_t idx=0; idx<tool_changes.size(); ++idx)
#if 1
        return 0;
#else
        if (! m_filpar[tool_changes[idx].new_tool].is_soluble)
            return idx;
#endif

    return -1;
}

static WipeTower::ToolChangeResult merge_tcr(WipeTower::ToolChangeResult& first,
                                             WipeTower::ToolChangeResult& second)
{
    //assert(first.new_tool == second.initial_tool);
    WipeTower::ToolChangeResult out = first;
    /*if ((first.end_pos - second.start_pos).norm() > (float) EPSILON)*/ {
        std::string travel_gcode = "G1 X" + Slic3r::float_to_string_decimal_point(second.start_pos.x(), 3) + " Y" +
                                   Slic3r::float_to_string_decimal_point(second.start_pos.y(), 3) + "F5400" + WipeTower::never_skip_tag() +
                                   "\n";
        bool need_insert_travel = true;
        if (second.is_tool_change && is_approx(second.start_pos.x(), second.tool_change_start_pos.x()) &&
            is_approx(second.start_pos.y(), second.tool_change_start_pos.y())) {
            // will insert travel in gcode.cpp
            need_insert_travel = false;
        }

        if (need_insert_travel)
            out.gcode += travel_gcode;
    }
    out.gcode += second.gcode;
    out.extrusions.insert(out.extrusions.end(), second.extrusions.begin(), second.extrusions.end());
    out.end_pos = second.end_pos;
    out.wipe_path = second.wipe_path;
    out.initial_tool = first.initial_tool;
    out.new_tool = second.new_tool;
    auto copy_inner_wall_box = [](WipeTower::ToolChangeResult& dst, const WipeTower::ToolChangeResult& src) {
        dst.wipe_tower_inner_wall_box_valid = src.wipe_tower_inner_wall_box_valid;
        for (int i = 0; i < 4; ++i)
            dst.wipe_tower_inner_wall_box[i] = src.wipe_tower_inner_wall_box[i];
        dst.wipe_tower_inner_wall_start_pos_valid = src.wipe_tower_inner_wall_start_pos_valid;
        dst.wipe_tower_inner_wall_start_pos = src.wipe_tower_inner_wall_start_pos;
        dst.wipe_tower_inner_wall_approach_pos_valid = src.wipe_tower_inner_wall_approach_pos_valid;
        dst.wipe_tower_inner_wall_approach_pos = src.wipe_tower_inner_wall_approach_pos;
    };
    if (second.is_tool_change && second.wipe_tower_inner_wall_box_valid)
        copy_inner_wall_box(out, second);
    else if (first.is_tool_change && first.wipe_tower_inner_wall_box_valid)
        copy_inner_wall_box(out, first);
    else if (second.wipe_tower_inner_wall_box_valid && !out.wipe_tower_inner_wall_box_valid)
        copy_inner_wall_box(out, second);

    if (first.is_tool_change)
    {
        out.is_tool_change        = true;
        out.tool_change_start_pos = first.tool_change_start_pos;
        out.purge_volume = first.purge_volume;
    }
    else if (second.is_tool_change)
    {
        out.is_tool_change        = true;
        out.tool_change_start_pos = second.tool_change_start_pos;
        out.purge_volume = second.purge_volume;
    }
    else
    {
        out.is_tool_change = false;
        out.purge_volume = 0.0;
    }
    out.wipe_paths = first.wipe_paths;
    for (auto & it : second.wipe_paths)
    {
        out.wipe_paths.push_back(it);
    }

    return out;
}

WipeTower::ToolChangeResult WipeTowerCreality::only_generate_out_wall()
{
    size_t old_tool = m_current_tool;

    WipeTowerWriterCreality writer(m_layer_height, m_perimeter_width, m_gcode_flavor, m_filpar);
    writer.set_extrusion_flow(m_extrusion_flow)
        .set_z(m_z_pos)
        .set_initial_tool(m_current_tool)
        .set_y_shift(m_y_shift - (m_current_shape == SHAPE_REVERSED ? m_layer_info->toolchanges_depth() : 0.f));

    // Slow down on the 1st layer.
    bool first_layer = is_first_layer();
    // BBS: speed up perimeter speed to 90mm/s for non-first layer
    float                      feedrate   = first_layer ? std::min(m_first_layer_speed * 60.f, 5400.f) :
                                                          std::min(60.0f * m_filpar[m_current_tool].max_e_speed / m_extrusion_flow, 5400.f);
    float                      fill_box_y = m_layer_info->toolchanges_depth() + m_perimeter_width;
    WipeTower::box_coordinates fill_box(Vec2f(m_perimeter_width, fill_box_y), m_wipe_tower_width - 2 * m_perimeter_width,
                                        m_layer_info->depth - fill_box_y);

    writer.set_initial_position((m_left_to_right ? fill_box.ru : fill_box.lu), m_wipe_tower_width, m_wipe_tower_depth, m_internal_rotation);

    bool toolchanges_on_layer = m_layer_info->toolchanges_depth() > WT_EPSILON;

    // we are in one of the corners, travel to ld along the perimeter:
    // BBS: Delete some unnecessary travel
    // if (writer.x() > fill_box.ld.x() + EPSILON) writer.travel(fill_box.ld.x(), writer.y());
    // if (writer.y() > fill_box.ld.y() + EPSILON) writer.travel(writer.x(), fill_box.ld.y());
    writer.append(";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_Start) + "\n");
    // outer perimeter (always):
    // BBS
    WipeTower::box_coordinates wt_box(Vec2f(0.f, (m_current_shape == SHAPE_REVERSED ? m_layer_info->toolchanges_depth() : 0.f)),
                                      m_wipe_tower_width, m_layer_info->depth + m_perimeter_width);
    if (m_layer_info->round_wipe_wall) {
        const float spacing = m_perimeter_width - m_layer_height * float(1. - M_PI_4);
        wt_box = expand_wipe_block_box(wt_box, spacing);
    }
    wt_box = align_perimeter(wt_box);
    // Align the depth endpoints while retaining the selected X contour.
    writer.rectangle(wt_box, feedrate);

    // Now prepare future wipe. box contains rectangle that was extruded last (ccw).
    Vec2f target = (writer.pos() == wt_box.ld ?
                        wt_box.rd :
                        (writer.pos() == wt_box.rd ? wt_box.ru : (writer.pos() == wt_box.ru ? wt_box.lu : wt_box.ld)));
    writer.add_wipe_point(writer.pos()).add_wipe_point(target);

    writer.append(";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_End) + "\n");

    // Ask our writer about how much material was consumed.
    // Skip this in case the layer is sparse and config option to not print sparse layers is enabled.
    if (!m_no_sparse_layers || toolchanges_on_layer)
        if (m_current_tool < m_used_filament_length.size())
            m_used_filament_length[m_current_tool] += writer.get_and_reset_used_filament_length();
    //writer.append(";current_path_finish\n");
    return construct_tcr(writer, false, old_tool, true, 0.f);
}

// Processes vector m_plan and calls respective functions to generate G-code for the wipe tower
// Resulting ToolChangeResults are appended into vector "result"
void WipeTowerCreality::generate(std::vector<std::vector<WipeTower::ToolChangeResult>> &result)
{
	if (m_plan.empty())
        return;
    m_wipe_tower_height = m_plan.back().z; // real wipe_tower_height
	plan_tower();
    // for (int i=0;i<5;++i) {
    //     save_on_last_wipe();
    //     plan_tower();
    // }
    m_layer_info = m_plan.begin();
    m_current_height = 0.f;

    // we don't know which extruder to start with - we'll set it according to the first toolchange
    for (const auto& layer : m_plan) {
        if (!layer.tool_changes.empty()) {
            m_current_tool = layer.tool_changes.front().old_tool;
            break;
        }
    }

    for (auto& used : m_used_filament_length) // reset used filament stats
        used = 0.f;

    m_old_temperature = -1; // reset last temperature written in the gcode
    int                                      wall_filament = get_wall_filament_for_all_layer();
    int                                      index = 0;
    std::vector<WipeTower::ToolChangeResult> layer_result;
    std::unordered_set<int>                  solid_blocks_id; // The contact surface of different bonded materials is solid.
	for (const WipeTowerCreality::WipeTowerInfo& layer : m_plan)
	{
        reset_block_status();
        m_cur_layer_id = index++;
        set_layer(layer.z, layer.height, 0, false/*layer.z == m_plan.front().z*/, layer.z == m_plan.back().z);
        //m_internal_rotation += 180.f;

       /* if (m_layer_info->depth < m_wipe_tower_depth - m_perimeter_width)
			m_y_shift = (m_wipe_tower_depth-m_layer_info->depth-m_perimeter_width)/2.f;*/
        if (m_wipe_tower_blocks.size() == 1) {
            if (m_layer_info->depth < m_wipe_tower_depth - m_perimeter_width) {
                // align y shift to perimeter width
                float dy  = m_extra_spacing * m_perimeter_width;
                m_y_shift = (m_wipe_tower_depth - m_layer_info->depth) / 2.f;
                m_y_shift = align_round(m_y_shift, dy);
            }
        }


        get_wall_skip_points(layer);

        //int idx = first_toolchange_to_nonsoluble(layer.tool_changes);
        WipeTower::ToolChangeResult finish_layer_tcr;
        WipeTower::ToolChangeResult timelapse_wall;


        auto get_wall_filament_for_this_layer = [this, &layer, &wall_filament]() -> int {
            if (layer.tool_changes.size() == 0)
                return -1;

            int candidate_id = -1;
            for (size_t idx = 0; idx < layer.tool_changes.size(); ++idx) {
                if (idx == 0) {
                    if (layer.tool_changes[idx].old_tool == wall_filament && is_valid_last_layer(layer.tool_changes[idx].old_tool))
                        return wall_filament;
                    else if (m_filpar[layer.tool_changes[idx].old_tool].category == m_filpar[wall_filament].category &&
                             is_valid_last_layer(layer.tool_changes[idx].old_tool)) {
                        candidate_id = layer.tool_changes[idx].old_tool;
                    }
                }
                if (layer.tool_changes[idx].new_tool == wall_filament) {
                    return wall_filament;
                }

                if ((candidate_id == -1) && (m_filpar[layer.tool_changes[idx].new_tool].category == m_filpar[wall_filament].category))
                    candidate_id = layer.tool_changes[idx].new_tool;
            }
            return candidate_id == -1 ? layer.tool_changes[0].new_tool : candidate_id;
        };

        int wall_idx = get_wall_filament_for_this_layer();

         // this layer has no tool_change
        if (wall_idx == -1) {
            bool need_insert_solid_infill = false;
            for (const WipeTower::WipeTowerBlock& block : m_wipe_tower_blocks) {
                if (block.solid_infill[m_cur_layer_id] && (block.filament_adhesiveness_category != m_filament_categories[m_current_tool])) {
                    need_insert_solid_infill = true;
                    break;
                }
            }

            if (need_insert_solid_infill) {
                wall_idx = m_current_tool;
            } else {
                if (m_enable_timelapse_print) {
                    timelapse_wall = only_generate_out_wall();
                }
                finish_layer_tcr = finish_layer_new(m_enable_timelapse_print ? false : true, layer.extruder_fill);
                std::for_each(m_wipe_tower_blocks.begin(), m_wipe_tower_blocks.end(),
                              [this](WipeTower::WipeTowerBlock& block) { block.finish_depth[this->m_cur_layer_id] = block.start_depth; });
            }
        }

        int insert_finish_layer_idx = -1;
        if (wall_idx != -1 && m_enable_timelapse_print) {
             timelapse_wall = only_generate_out_wall();
        }

        float layer_max_y = std::numeric_limits<double>::lowest();
        float layer_max_x = std::numeric_limits<double>::lowest();
        int   num_tool_change = int(layer.tool_changes.size());

        for (int i = 0; i < num_tool_change; ++i) {

            if (i == 0 && (layer.tool_changes[i].old_tool == wall_idx)) {
                finish_layer_tcr = finish_layer_new(m_enable_timelapse_print ? false : true, false, false);//不生成格子
            }

           const auto* block = get_block_by_category(m_filpar[layer.tool_changes[i].new_tool].category, false);
            int         id    = std::find_if(m_wipe_tower_blocks.begin(), m_wipe_tower_blocks.end(),
                                             [&](const WipeTower::WipeTowerBlock& b) { return &b == block; }) -
                     m_wipe_tower_blocks.begin();
            bool solid_toolchange = solid_blocks_id.count(id);

            const auto* block2 = get_block_by_category(m_filpar[layer.tool_changes[i].old_tool].category, false);
            id                 = std::find_if(m_wipe_tower_blocks.begin(), m_wipe_tower_blocks.end(),
                                              [&](const WipeTower::WipeTowerBlock& b) { return &b == block2; }) -
                 m_wipe_tower_blocks.begin();
            bool solid_nozzlechange = solid_blocks_id.count(id);

            layer_result.emplace_back(tool_change(layer.tool_changes[i].new_tool, solid_toolchange, solid_nozzlechange));
            if (i == 0 && (layer.tool_changes[i].old_tool == wall_idx)) {
            } else if (layer.tool_changes[i].new_tool == wall_idx) {
               finish_layer_tcr = finish_layer_new(m_enable_timelapse_print ? false : true, false, false);
               insert_finish_layer_idx = i;
            }
        }

       std::unordered_set<int> next_solid_blocks_id;
       if (wall_idx != -1) {
            if (layer.tool_changes.empty()) {
                finish_layer_tcr = finish_layer_new(m_enable_timelapse_print ? false : true, false, false);
            }

            for (WipeTower::WipeTowerBlock& block : m_wipe_tower_blocks) {
                block.finish_depth[m_cur_layer_id] = block.start_depth + block.depth;
                if (block.cur_depth + EPSILON >= block.start_depth + block.layer_depths[m_cur_layer_id] - m_perimeter_width) {
                    continue;
                }
                int id = std::find_if(m_wipe_tower_blocks.begin(), m_wipe_tower_blocks.end(),
                                      [&](const WipeTower::WipeTowerBlock& b) { return &b == &block; }) -
                         m_wipe_tower_blocks.begin();
                bool interface_solid       = solid_blocks_id.count(id);
                int  finish_layer_filament = -1;
                if (block.last_filament_change_id != -1) {
                    finish_layer_filament = block.last_filament_change_id;
                } else if (block.last_nozzle_change_id != -1) {
                    finish_layer_filament = block.last_nozzle_change_id;
                }

                if (!layer.tool_changes.empty()) {
                    WipeTower::WipeTowerBlock* last_layer_finish_block = get_block_by_category(get_filament_category(
                                                                                        layer.tool_changes.front().old_tool),
                                                                                    false);
                    if (last_layer_finish_block && last_layer_finish_block->block_id == block.block_id && finish_layer_filament == -1)
                        finish_layer_filament = layer.tool_changes.front().old_tool;
                }

                if (finish_layer_filament == -1) {
                    finish_layer_filament = wall_idx;
                }
                // Cancel the block of the last layer
                if (!is_valid_last_layer(finish_layer_filament))
                    continue;
                WipeTower::ToolChangeResult finish_block_tcr;
                if (interface_solid || (block.solid_infill[m_cur_layer_id] &&
                                        block.filament_adhesiveness_category != m_filament_categories[finish_layer_filament])) {
                    interface_solid = interface_solid && !((block.solid_infill[m_cur_layer_id] &&
                                                            block.filament_adhesiveness_category !=
                                                                m_filament_categories[finish_layer_filament])); // noly reduce speed when
                    if (!interface_solid) {
                        int tmp_id = std::find_if(m_wipe_tower_blocks.begin(), m_wipe_tower_blocks.end(),
                                                  [&](const WipeTower::WipeTowerBlock& b) { return &b == &block; }) -
                                     m_wipe_tower_blocks.begin();
                        next_solid_blocks_id.insert(tmp_id);
                    }
                    finish_block_tcr = finish_block_solid(block, finish_layer_filament, layer.extruder_fill, interface_solid);
                    block.finish_depth[m_cur_layer_id] = block.start_depth + block.depth;
                } else {
                    finish_block_tcr                   = finish_block(block, finish_layer_filament, layer.extruder_fill);
                    block.finish_depth[m_cur_layer_id] = block.cur_depth;
                }

                bool has_inserted = false;
                {
                    auto fc_iter = std::find_if(layer_result.begin(), layer_result.end(),
                                                [&finish_layer_filament](const WipeTower::ToolChangeResult& item) {
                                                    return item.new_tool == finish_layer_filament;
                                                });
                    if (fc_iter != layer_result.end()) {
                        *fc_iter     = merge_tcr(*fc_iter, finish_block_tcr);
                        has_inserted = true;
                    }
                }

                if (block.last_filament_change_id == -1 && !has_inserted) {
                    auto nc_iter = std::find_if(layer_result.begin(), layer_result.end(),
                                                [&finish_layer_filament](const WipeTower::ToolChangeResult& item) {
                                                    return item.initial_tool == finish_layer_filament;
                                                });
                    if (nc_iter != layer_result.end()) {
                        *nc_iter     = merge_tcr(finish_block_tcr, *nc_iter);
                        has_inserted = true;
                    }
                }

                if (!has_inserted) {
                    if (finish_block_tcr.gcode.empty())
                        finish_block_tcr = finish_block_tcr;
                    else
                        finish_layer_tcr = merge_tcr(finish_layer_tcr, finish_block_tcr);
                }
            }
        }

        //record the contact layers of different categories
        solid_blocks_id = next_solid_blocks_id;

        if (layer_result.empty()) {
            // there is nothing to merge finish_layer with
            layer_result.emplace_back(std::move(finish_layer_tcr));
        }
        else {
            if (insert_finish_layer_idx == -1) {
                layer_result[0] = merge_tcr(finish_layer_tcr, layer_result[0]);
            }
            else
                layer_result[insert_finish_layer_idx] = merge_tcr(layer_result[insert_finish_layer_idx], finish_layer_tcr);
        }

        for (auto& tcr : layer_result) {
            tcr.m_wipe_max_y = layer_max_y;
            tcr.m_wipe_max_x = layer_max_x;
        }
		result.emplace_back(std::move(layer_result));
	}
}

std::vector<std::pair<float, float>> WipeTowerCreality::get_z_and_depth_pairs() const
{
    std::vector<std::pair<float, float>> out = {{0.f, m_wipe_tower_depth}};
    for (const WipeTowerInfo& wti : m_plan) {
        assert(wti.depth < wti.depth + WT_EPSILON);
        if (wti.depth < out.back().second - WT_EPSILON)
            out.emplace_back(wti.z, wti.depth);
    }
    if (out.back().first < m_wipe_tower_height - WT_EPSILON)
        out.emplace_back(m_wipe_tower_height, 0.f);
    return out;
}


void WipeTowerCreality::generate_wipe_tower_blocks()
{
    // 1. generate all layer depth
    m_all_layers_depth.clear();
    m_all_layers_depth.resize(m_plan.size());
    m_cur_layer_id = 0;
    for (auto& info : m_plan) {
        for (const WipeTowerInfo::ToolChange& tool_change : info.tool_changes) {
            if (is_in_same_extruder(tool_change.old_tool, tool_change.new_tool)) {
                int filament_adhesiveness_category = get_filament_category(tool_change.new_tool);
                add_depth_to_block(tool_change.new_tool, filament_adhesiveness_category, tool_change.required_depth);
            } else {
                int old_filament_category = get_filament_category(tool_change.old_tool);
                add_depth_to_block(tool_change.old_tool, old_filament_category, tool_change.nozzle_change_depth, true);
                int new_filament_category = get_filament_category(tool_change.new_tool);
                add_depth_to_block(tool_change.new_tool, new_filament_category,
                                   tool_change.required_depth - tool_change.nozzle_change_depth);
            }
        }
        ++m_cur_layer_id;
    }

    // 2. generate all layer depth
    std::vector<std::unordered_map<int, float>> all_layer_category_to_depth(m_plan.size());
    for (size_t layer_id = 0; layer_id < m_all_layers_depth.size(); ++layer_id) {
        const auto&                     layer_blocks      = m_all_layers_depth[layer_id];
        std::unordered_map<int, float>& category_to_depth = all_layer_category_to_depth[layer_id];
        for (auto block : layer_blocks) {
            category_to_depth[block.category] = block.depth; // m_all_layers_depth    bambu是统一值
        }
    }

    // 3. generate wipe tower block
    m_wipe_tower_blocks.clear();
    for (int layer_id = 0; layer_id < all_layer_category_to_depth.size(); ++layer_id) {
        const auto& layer_category_depths = all_layer_category_to_depth[layer_id];
        for (auto iter = layer_category_depths.begin(); iter != layer_category_depths.end(); ++iter) {
            auto* block = get_block_by_category(iter->first, true);
            if (block->layer_depths.empty()) {
                block->layer_depths.resize(all_layer_category_to_depth.size(), 0);
                block->solid_infill.resize(all_layer_category_to_depth.size(), false);
                block->finish_depth.resize(all_layer_category_to_depth.size(), 0);
            }
            block->depth                  = std::max(block->depth, iter->second);
            block->layer_depths[layer_id] = iter->second;
        }
    }

    // add solid infill flag
    int solid_infill_layer = 4;
    for (WipeTower::WipeTowerBlock& block : m_wipe_tower_blocks) {
        for (int layer_id = 0; layer_id < all_layer_category_to_depth.size(); ++layer_id) {
            std::unordered_map<int, float>& category_to_depth = all_layer_category_to_depth[layer_id];
            if (is_approx(category_to_depth[block.filament_adhesiveness_category], 0.f)) {
                int layer_count = solid_infill_layer;
                while (layer_count > 0) {
                    if (layer_id + layer_count < all_layer_category_to_depth.size()) {
                        std::unordered_map<int, float>& up_layer_depth = all_layer_category_to_depth[layer_id + layer_count];
                        if (!is_approx(up_layer_depth[block.filament_adhesiveness_category], 0.f)) {
                            block.solid_infill[layer_id] = true;
                            break;
                        }
                    }
                    --layer_count;
                }
            }
        }
    }

    // 4. get real depth for every layer
    for (int layer_id = m_plan.size() - 1; layer_id >= 0; --layer_id) {
        m_plan[layer_id].depth = 0;
        for (auto& block : m_wipe_tower_blocks) {
            if (layer_id < m_plan.size() - 1)
                block.layer_depths[layer_id] = std::max(block.layer_depths[layer_id], block.layer_depths[layer_id + 1]);
            m_plan[layer_id].depth += block.layer_depths[layer_id];
        }
    }

    if (m_wipe_tower_framework) {
        for (int layer_id = 1; layer_id < m_plan.size(); ++layer_id) {
            m_plan[layer_id].depth = 0;
            for (auto& block : m_wipe_tower_blocks) {
                block.layer_depths[layer_id] = block.layer_depths[0];
                m_plan[layer_id].depth += block.layer_depths[layer_id];
            }
        }
    }
}

void WipeTowerCreality::reset_block_status()
{
    for (auto& block : m_wipe_tower_blocks) {
        block.cur_depth               = block.start_depth;
        block.last_filament_change_id = -1;
        block.last_nozzle_change_id   = -1;
    }
}

bool WipeTowerCreality::is_in_same_extruder(int filament_id_1, int filament_id_2)
{
    if (filament_id_1 >= m_filament_map.size() || filament_id_2 >= m_filament_map.size())
        return true;

    return m_filament_map[filament_id_1] == m_filament_map[filament_id_2];
}

void WipeTowerCreality::add_depth_to_block(int filament_id, int filament_adhesiveness_category, float depth, bool is_nozzle_change)
{
    std::vector<WipeTower::BlockDepthInfo>& layer_depth = m_all_layers_depth[m_cur_layer_id];
    auto                                    iter        = std::find_if(layer_depth.begin(), layer_depth.end(),
                                                                       [&filament_adhesiveness_category](const WipeTower::BlockDepthInfo& item) {
                                 return item.category == filament_adhesiveness_category;
                             });

    if (iter != layer_depth.end()) {
        iter->depth += depth;
        if (is_nozzle_change)
            iter->nozzle_change_depth += depth;
    } else {
        WipeTower::BlockDepthInfo new_block;
        new_block.category = filament_adhesiveness_category;
        new_block.depth    = depth;
        if (is_nozzle_change)
            new_block.nozzle_change_depth += depth;
        layer_depth.emplace_back(std::move(new_block));
    }
}

int WipeTowerCreality::get_filament_category(int filament_id)
{
    if (filament_id >= m_filament_categories.size())
        return 0;
    return m_filament_categories[filament_id];
}

WipeTower::WipeTowerBlock* WipeTowerCreality::get_block_by_category(int filament_adhesiveness_category, bool create)
{
    auto iter = std::find_if(m_wipe_tower_blocks.begin(), m_wipe_tower_blocks.end(),
                             [&filament_adhesiveness_category](const WipeTower::WipeTowerBlock& item) {
                                 return item.filament_adhesiveness_category == filament_adhesiveness_category;
                             });

    if (iter != m_wipe_tower_blocks.end()) {
        return &(*iter);
    }

    if (create) {
        WipeTower::WipeTowerBlock new_block;
        new_block.block_id                       = m_wipe_tower_blocks.size();
        new_block.filament_adhesiveness_category = filament_adhesiveness_category;
        m_wipe_tower_blocks.emplace_back(new_block);
        return &m_wipe_tower_blocks.back();
    }

    return nullptr;
}

void WipeTowerCreality::update_all_layer_depth(float wipe_tower_depth)
{
    m_wipe_tower_depth = 0.f;
    float start_offset = m_perimeter_width;
    float start_depth  = start_offset;
    for (auto& block : m_wipe_tower_blocks) {
        block.depth *= m_extra_spacing;
        block.start_depth = start_depth;
        start_depth += block.depth;
        m_wipe_tower_depth += block.depth;

        for (auto& layer_depth : block.layer_depths) {
            layer_depth *= m_extra_spacing;
        }

        for (WipeTowerInfo& plan_info : m_plan) {
            plan_info.depth *= m_extra_spacing;
        }
    }
    if (m_wipe_tower_depth > 0)
        m_wipe_tower_depth += start_offset;

    if (m_enable_timelapse_print) {
        if (is_approx(m_wipe_tower_depth, 0.f))
            m_wipe_tower_depth = wipe_tower_depth;
        for (WipeTowerInfo& plan_info : m_plan) {
            plan_info.depth = m_wipe_tower_depth;
        }
    }
}

int WipeTowerCreality::get_wall_filament_for_all_layer()
{
    std::map<int, int> category_counts;
    std::map<int, int> filament_counts;
    int                current_tool = m_current_tool;
    for (const auto& layer : m_plan) {
        if (layer.tool_changes.empty()) {
            filament_counts[current_tool]++;
            category_counts[get_filament_category(current_tool)]++;
            continue;
        }
        std::unordered_set<int> used_tools;
        std::unordered_set<int> used_category;
        for (size_t i = 0; i < layer.tool_changes.size(); ++i) {
            if (i == 0) {
                filament_counts[layer.tool_changes[i].old_tool]++;
                category_counts[get_filament_category(layer.tool_changes[i].old_tool)]++;
                used_tools.insert(layer.tool_changes[i].old_tool);
                used_category.insert(get_filament_category(layer.tool_changes[i].old_tool));
            }
            if (!used_category.count(get_filament_category(layer.tool_changes[i].new_tool)))
                category_counts[get_filament_category(layer.tool_changes[i].new_tool)]++;
            if (!used_tools.count(layer.tool_changes[i].new_tool))
                filament_counts[layer.tool_changes[i].new_tool]++;
            used_tools.insert(layer.tool_changes[i].new_tool);
            used_category.insert(get_filament_category(layer.tool_changes[i].new_tool));
        }
        current_tool = layer.tool_changes.empty() ? current_tool : layer.tool_changes.back().new_tool;
    }

    // std::vector<std::pair<int, int>> category_counts_vec;
    int selected_category = -1;
    int selected_count    = 0;

    for (auto iter = category_counts.begin(); iter != category_counts.end(); ++iter) {
        if (iter->second > selected_count) {
            selected_category = iter->first;
            selected_count    = iter->second;
        }
    }

    // std::sort(category_counts_vec.begin(), category_counts_vec.end(), [](const std::pair<int, int> &left, const std::pair<int, int>&
    // right) {
    //     return left.second > right.second;
    // });

    int filament_id    = -1;
    int filament_count = 0;
    for (auto iter = filament_counts.begin(); iter != filament_counts.end(); ++iter) {
        if (m_filament_categories[iter->first] == selected_category && iter->second > filament_count) {
            filament_id    = iter->first;
            filament_count = iter->second;
        }
    }
    return filament_id;
}

bool WipeTowerCreality::is_valid_last_layer(int tool) const
{
    int nozzle_id = -1;
    if (tool >= 0 && tool < m_filament_map.size())
        nozzle_id = m_filament_map[tool] - 1;
    if (nozzle_id < 0 || nozzle_id >= m_printable_height.size())
        return true;
    if (m_last_layer_id[nozzle_id] == m_cur_layer_id && m_z_pos > m_printable_height[nozzle_id])
        return false;
    return true;
}

WipeTower::ToolChangeResult WipeTowerCreality::finish_block(const WipeTower::WipeTowerBlock& block, int filament_id, bool extrude_fill)
{
    WipeTowerWriterCreality writer(m_layer_height, m_perimeter_width, m_gcode_flavor, m_filpar);
    writer.set_extrusion_flow(m_extrusion_flow)
        .set_z(m_z_pos)
        .set_initial_tool(filament_id)
        .set_y_shift(m_y_shift - (m_current_shape == SHAPE_REVERSED ? m_layer_info->toolchanges_depth() : 0.f));

    writer.append(";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_Start) + "\n");
    writer.append(";avoiding_repeat_unretract\n");

    // Slow down on the 1st layer.
    bool first_layer = is_first_layer();
    // BBS: speed up perimeter speed to 90mm/s for non-first layer
    float feedrate = first_layer ? std::min(m_first_layer_speed * 60.f, m_max_speed) :
                                   std::min(60.0f * m_filpar[filament_id].max_e_speed / m_extrusion_flow, m_max_speed);

    float factor   = first_layer ? 2.f : (1.f + m_extra_flow);

    WipeTower::box_coordinates fill_box(Vec2f(0, 0), 0, 0);
    fill_box = WipeTower::box_coordinates(Vec2f(factor / 2.f * m_perimeter_width,  block.cur_depth),
                                          m_wipe_tower_width - factor * m_perimeter_width,
                               block.start_depth + block.layer_depths[m_cur_layer_id] - block.cur_depth - factor / 2.f * m_perimeter_width);

    float line_width = m_perimeter_width;
    if (!first_layer)
        line_width *= m_extra_flow;
    const WipeTower::box_coordinates block_wall_box = expand_wipe_block_box(fill_box, line_width);
    Vec2f initial_pos = m_left_to_right ? fill_box.ru : fill_box.lu;
    if (block_wall_box.ru.y() - block_wall_box.rd.y() > WT_EPSILON)
        initial_pos = m_left_to_right ? block_wall_box.ru : block_wall_box.lu;
    writer.set_initial_position(initial_pos, m_wipe_tower_width, m_wipe_tower_depth, m_internal_rotation);



    float retract_length = m_filpar[filament_id].retract_length;
    float retract_speed  = m_filpar[filament_id].retract_speed * 60;
    /* if (m_no_sparse_layers) {
        writer.travel(writer.pos(), m_travel_speed * 60.f);
        writer.relative_zhop(m_z_offset, 0.0, "relative_zhop_recovery_for_firmware G1");
        writer.retract(-retract_length, retract_speed);
    } else {
        writer.travel(writer.pos(), m_travel_speed * 60.f);
    }*/

    writer.travel(writer.pos(), m_travel_speed * 60.f);
    writer.relative_zhop(m_z_offset, 0.0, "relative_zhop_recovery_for_firmware G1");
    writer.retract(-retract_length, retract_speed); // 装填

    if (!first_layer) {
        writer.change_analyzer_line_width(line_width);
    }

    bool toolchanges_on_layer = m_layer_info->toolchanges_depth() > WT_EPSILON;

    std::vector<Vec2f> finish_rect_wipe_path;
    // inner perimeter of the sparse section, if there is space for it:
    if (block_wall_box.ru.y() - block_wall_box.rd.y() > WT_EPSILON) {
        //writer.rectangle_fill_box(fill_box.ld, fill_box.rd.x() - fill_box.ld.x(), fill_box.ru.y() - fill_box.rd.y(), retract_length,
        //                          retract_speed, feedrate);//格子外框
       // writer.retract(-retract_length, retract_speed);
        writer.rectangle_fill_box(this, block_wall_box, finish_rect_wipe_path, retract_length, retract_speed, feedrate);//格子外框
    }

    WipeTower::box_coordinates wt_box(block_wall_box.ld, block_wall_box.ru.x() - block_wall_box.lu.x(), block_wall_box.ru.y() - block_wall_box.rd.y());
    wt_box = align_perimeter(wt_box);

    // Now prepare future wipe. box contains rectangle that was extruded last (ccw).逆时针
    Vec2f target = (writer.pos() == wt_box.ld ?
                        wt_box.rd :
                        (writer.pos() == wt_box.rd ? wt_box.ru : (writer.pos() == wt_box.ru ? wt_box.lu : wt_box.ld)));

    // Extrude infill to support the material to be printed above.
    const float dy    = (fill_box.lu.y() - fill_box.ld.y() - line_width);
    float       left  = fill_box.lu.x() + 2 * line_width;
    float       right = fill_box.ru.x() - 2 * line_width;
    if (extrude_fill && dy > line_width) {
        float distance = (fill_box.ld - writer.pos()).norm();
        if (distance > line_width) {
            //writer.add_wipe_point(writer.x(), writer.y()).add_wipe_point(target);
            writer.add_wipe_group(Vec2f(writer.x(), writer.y()),target);
            writer.append(";wipe_finish_path\n");//框结束擦拭，为打印栅格填充做准备
            writer.set_feedrate(retract_speed);
        }

        writer.travel(fill_box.ld + Vec2f(line_width * 2, 0.f), feedrate)
            .append(";---------------\n"
                    "; CP EMPTY GRID START\n")
            .comment_with_value(" level #", m_num_layer_changes + 1);
        if (distance > m_perimeter_width) {

            writer.retract(-retract_length, retract_speed);
        }
        // Is there a soluble filament wiped/rammed at the next layer?
        // If so, the infill should not be sparse.
        bool solid_infill = m_layer_info + 1 == m_plan.end() ?
                                false :
                                std::any_of((m_layer_info + 1)->tool_changes.begin(), (m_layer_info + 1)->tool_changes.end(),
                                            [this](const WipeTowerInfo::ToolChange& tch) {
                                                return m_filpar[tch.new_tool].is_soluble || m_filpar[tch.old_tool].is_soluble;
                                            });
        solid_infill |= first_layer && m_adhesion;

        if (solid_infill) {
            float sparse_factor = 1.5f; // 1=solid, 2=every other line, etc.
            if (first_layer) {          // the infill should touch perimeters
                left -= line_width;
                right += line_width;
                sparse_factor = 1.f;
            }
            float y       = fill_box.ld.y() + line_width;
            int   n       = dy / (line_width * sparse_factor);
            float spacing = (dy - line_width) / (n - 1);
            int   i       = 0;

            //finish_rect_wipe_path.clear();
            for (i = 0; i < n; ++i) {
                writer.extrude(writer.x(), y, feedrate).extrude(i % 2 ? left : right, y);
                //finish_rect_wipe_path.emplace_back(Vec2f(writer.x(), y));
                y = y + spacing;
            }
            //writer.extrude(writer.x(), fill_box.lu.y());
            //finish_rect_wipe_path.emplace_back(writer.x(), fill_box.lu.y());
            //添加擦拭路径
            //writer/*.add_wipe_point(writer.x(), writer.y())*/
            //    .add_wipe_point(writer.x(), writer.y() - line_width)
            //    .add_wipe_point(!m_left_to_right ? m_wipe_tower_width : 0.f, writer.y() - line_width);
            //writer.add_wipe_point(writer.x(), writer.y()).add_wipe_point(!m_left_to_right ? m_wipe_tower_width : 0.f, writer.y());
            //writer.retract(retract_length * 0.7, retract_speed);
            writer.add_wipe_group(Vec2f(writer.x(), writer.y()), Vec2f(!m_left_to_right ? m_wipe_tower_width : 0.f, writer.y()));
            writer.append(";wipe_finish_path\n");
            writer.set_feedrate(retract_speed);

        } else {
            // Extrude an inverse U at the left of the region and the sparse infill.
            writer.extrude(fill_box.lu + Vec2f(line_width * 2, 0.f), feedrate);

            const int   n  = 1 + int((right - left) / m_bridging);
            const float dx = (right - left) / n;
            for (int i = 1; i <= n; ++i) {
                float x = left + dx * i;
                writer.travel(x, writer.y());
                writer.extrude(x, i % 2 ? fill_box.rd.y() : fill_box.ru.y());
            }
            finish_rect_wipe_path.clear();
            // BBS: add wipe_path for this case: only with finish rectangle
            finish_rect_wipe_path.emplace_back(writer.pos());
            finish_rect_wipe_path.emplace_back(Vec2f(left + dx * n, n % 2 ? fill_box.ru.y() : fill_box.rd.y()));

                // BBS: add wipe_path for this case: only with finish rectangle
            if (finish_rect_wipe_path.size() == 2 && finish_rect_wipe_path[0] == writer.pos())
                target = finish_rect_wipe_path[1];
            // 添加擦拭路径
            //writer.add_wipe_point(writer.x(), writer.y()).add_wipe_point(target);
            //writer.retract(retract_length * 0.7, retract_speed);
            writer.add_wipe_group(Vec2f(writer.x(), writer.y()), target);
            writer.append(";wipe_finish_path\n");
            writer.set_feedrate(retract_speed);
        }

        writer.append("; CP EMPTY GRID END\n"
                      ";-------------\n\n\n\n\n\n\n");
    }

    if (!first_layer)
        writer.change_analyzer_line_width(m_perimeter_width);

    writer.append(";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_End) + "\n");

    // Ask our writer about how much material was consumed.
    // Skip this in case the layer is sparse and config option to not print sparse layers is enabled.
    if (!m_no_sparse_layers || toolchanges_on_layer)
        if (filament_id < m_used_filament_length.size())
            m_used_filament_length[filament_id] += writer.get_and_reset_used_filament_length();

    return construct_tcr(writer, false, filament_id, true, 0.f);
}

WipeTower::ToolChangeResult WipeTowerCreality::finish_block_solid(const WipeTower::WipeTowerBlock& block,
                                                          int                   filament_id,
                                                          bool                  extrude_fill,
                                                          bool                  interface_solid)
{
    float layer_height = m_layer_height;
    float e_flow       = m_extrusion_flow;
    if (m_cur_layer_id > 1 && !block.solid_infill[m_cur_layer_id - 1] && m_extrusion_flow < extrusion_flow(0.2)) {
        layer_height = 0.2;
        e_flow       = extrusion_flow(0.2);
    }

    WipeTowerWriterCreality writer(layer_height, m_perimeter_width, m_gcode_flavor, m_filpar);
    writer.set_extrusion_flow(e_flow)
        .set_z(m_z_pos)
        .set_initial_tool(filament_id)
        .set_y_shift(m_y_shift - (m_current_shape == SHAPE_REVERSED ? m_layer_info->toolchanges_depth() : 0.f));

    //set_for_wipe_tower_writer(writer);

    writer.append(";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_Start) + "\n");
    // toolchange_Change(writer, m_current_tool, "", m_travel_speed * 60.f); // Change the tool, set a speed override for soluble and flex materials.

    // Slow down on the 1st layer.
    bool first_layer = is_first_layer();
    // BBS: speed up perimeter speed to 90mm/s for non-first layer
    float feedrate = first_layer ? std::min(m_first_layer_speed * 60.f, m_max_speed) :
                                   std::min(60.0f * m_filpar[filament_id].max_e_speed / m_extrusion_flow, m_max_speed);
    feedrate       = interface_solid ? 20.f * 60.f : feedrate;

    float factor   = first_layer ? 2.f : (1.f + m_extra_flow);
    WipeTower::box_coordinates fill_box(Vec2f(0, 0), 0, 0);
    fill_box = WipeTower::box_coordinates(Vec2f(factor / 2.f * m_perimeter_width, block.cur_depth), m_wipe_tower_width - factor * m_perimeter_width,
                               block.start_depth + block.layer_depths[m_cur_layer_id] - block.cur_depth - factor / 2.f * m_perimeter_width);

    float line_width = m_perimeter_width;
    if (!first_layer)
        line_width *= m_extra_flow;
    const WipeTower::box_coordinates block_wall_box = expand_wipe_block_box(fill_box, line_width);
    Vec2f initial_pos = m_left_to_right ? fill_box.rd : fill_box.ld;
    if (block_wall_box.ru.y() - block_wall_box.rd.y() > line_width - WT_EPSILON)
        initial_pos = m_left_to_right ? block_wall_box.rd : block_wall_box.ld;
    writer.set_initial_position(initial_pos, m_wipe_tower_width, m_wipe_tower_depth, m_internal_rotation);


    /* if (m_no_sparse_layers) {
        writer.travel(writer.pos(), m_travel_speed * 60.f);
        writer.relative_zhop(m_z_offset, 0.0, "relative_zhop_recovery_for_firmware G1");
    } else {
        writer.travel(writer.pos(), m_travel_speed * 60.f);
    }*/
    writer.travel(writer.pos(), m_travel_speed * 60.f);
    writer.relative_zhop(m_z_offset, 0.0, "relative_zhop_recovery_for_firmware G1");

    if (!first_layer) {
        writer.change_analyzer_line_width(line_width);
    }

    float retract_speed  = m_filpar[m_current_tool].retract_speed * 60;
    float retract_length = m_filpar[m_current_tool].retract_length;
    writer.retract(-retract_length, retract_speed);

    m_left_to_right           = !m_left_to_right;
    bool toolchanges_on_layer = m_layer_info->toolchanges_depth() > WT_EPSILON;

    // Extrude infill to support the material to be printed above.
    const float        dy    = (fill_box.lu.y() - fill_box.ld.y());
    float              left  = fill_box.lu.x();
    float              right = fill_box.ru.x();
    std::vector<Vec2f> finish_rect_wipe_path;
    if (block_wall_box.ru.y() - block_wall_box.rd.y() > line_width - WT_EPSILON)
        writer.rectangle_fill_box(this, block_wall_box, finish_rect_wipe_path, retract_length, retract_speed, feedrate);
    writer.travel(Vec2f(m_left_to_right ? left : right, fill_box.ld.y()), feedrate);
    {
        writer
            .append(";----------------\n"
                    "; CP EMPTY GRID START\n")
            .comment_with_value(" level #", m_num_layer_changes + 1);

        float y       = fill_box.ld.y();
        int   n       = (dy + 0.25 * line_width) / line_width + 1;
        float spacing = line_width;
        int   i       = 0;
        for (i = 0; i < n; ++i) {
            writer.extrude(m_left_to_right ? right : left, writer.y(), feedrate);
            if (i == n - 1) {
                writer.add_wipe_point(writer.pos()).add_wipe_point(Vec2f(m_left_to_right ? left : right, writer.y()));
                finish_rect_wipe_path.emplace_back(Vec2f(m_left_to_right ? left : right, writer.y()));
                break;
            }
            m_left_to_right = !m_left_to_right;
            y               = y + spacing;
            writer.extrude(writer.x(), y, feedrate);
        }

        writer.append("; CP EMPTY GRID END\n"
                      ";---------------\n\n\n\n\n\n\n");
    }



    writer.add_wipe_group(Vec2f(writer.x(), writer.y()), finish_rect_wipe_path.back());
    writer.append(";wipe_finish_path\n"); // 框结束擦拭，为打印栅格填充做准备
    writer.set_feedrate(retract_speed);


    if (!first_layer)
        writer.change_analyzer_line_width(m_perimeter_width);

    writer.append(";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_End) + "\n");

    //writer.retract(retract_length, retract_speed);

    // Ask our writer about how much material was consumed.
    // Skip this in case the layer is sparse and config option to not print sparse layers is enabled.
    if (!m_no_sparse_layers || toolchanges_on_layer)
        if (filament_id < m_used_filament_length.size())
            m_used_filament_length[filament_id] += writer.get_and_reset_used_filament_length();

    return construct_block_tcr(writer, false, filament_id, true);
}

WipeTower::ToolChangeResult WipeTowerCreality::finish_layer_new(bool extrude_perimeter, bool extrude_fill, bool extrude_fill_wall)
{
    assert(!this->layer_finished());
    m_current_layer_finished = true;

    size_t old_tool = m_current_tool;

    WipeTowerWriterCreality writer(m_layer_height, m_perimeter_width, m_gcode_flavor, m_filpar);
    writer.set_extrusion_flow(m_extrusion_flow)
        .set_z(m_z_pos)
        .set_initial_tool(m_current_tool)
        .set_y_shift(m_y_shift - (m_current_shape == SHAPE_REVERSED ? m_layer_info->toolchanges_depth() : 0.f));

    writer.append(";avoiding_repeat_unretract\n");

    // Slow down on the 1st layer.
    // If spare layers are excluded -> if 1 or less toolchange has been done, it must be still the first layer, too. So slow down.
    bool  first_layer   = is_first_layer() || (m_num_tool_changes <= 1 && m_no_sparse_layers);
    float feedrate      = first_layer ? m_first_layer_speed * 60.f : std::min(m_wipe_tower_max_purge_speed * 60.f, m_infill_speed * 60.f);

    float factor = first_layer ? 2.f : (1.f + m_extra_flow);

    float fill_box_depth = m_wipe_tower_depth - factor * m_perimeter_width;
    if (m_wipe_tower_blocks.size() == 1) {
        fill_box_depth = m_layer_info->depth - (first_layer ? 1.f : m_extra_flow) * m_perimeter_width;
    }
    WipeTower::box_coordinates fill_box(Vec2f(factor / 2.f * m_perimeter_width, factor / 2.f * m_perimeter_width), m_wipe_tower_width - factor * m_perimeter_width,
                                      fill_box_depth);

    float line_width = m_perimeter_width;
    if (!first_layer)
        line_width *= m_extra_flow;
    const bool has_toolchange_round_wipe_wall =
        std::any_of(m_layer_info->tool_changes.begin(), m_layer_info->tool_changes.end(),
                    [](const WipeTowerInfo::ToolChange& tc) { return tc.round_wipe_wall; });
    const bool print_standalone_round_wipe_wall = m_layer_info->round_wipe_wall && !has_toolchange_round_wipe_wall;
    const WipeTower::box_coordinates fill_wall_box =
        print_standalone_round_wipe_wall ? expand_wipe_block_box(fill_box, line_width) : fill_box;
    Vec2f initial_pos = m_left_to_right ? fill_box.ru : fill_box.lu;
    if (extrude_fill_wall && fill_wall_box.ru.y() - fill_wall_box.rd.y() > line_width - WT_EPSILON)
        initial_pos = m_left_to_right ? fill_wall_box.ru : fill_wall_box.lu;
    else if (extrude_fill && (fill_box.lu.y() - fill_box.ld.y() - line_width) > line_width)
        initial_pos = fill_box.ld + Vec2f(line_width * 2, 0.f);
    writer.set_initial_position(initial_pos,  m_wipe_tower_width, m_wipe_tower_depth, m_internal_rotation);

    float retract_length = m_filpar[m_current_tool].retract_length;
    float retract_speed  = m_filpar[m_current_tool].retract_speed * 60;

     /* if (m_no_sparse_layers) {
       // writer.comment_with_value("test---------------", m_num_layer_changes + 1);
        writer.travel(writer.pos(), m_travel_speed * 60.f);
        writer.relative_zhop(m_z_offset, 0.0, "relative_zhop_recovery_for_firmware G1");
        writer.retract(-retract_length, retract_speed);
    } else {
        writer.travel(writer.pos(), m_travel_speed * 60.f);
        writer.relative_zhop(m_z_offset, 0.0, "relative_zhop_recovery_for_firmware G1");
        writer.retract(-retract_length, retract_speed);
    }*/
    //writer.travel(writer.pos(), m_travel_speed * 60.f);

    writer.relative_zhop(m_z_offset, 0.0, "relative_zhop_recovery_for_firmware G1");
    if (m_no_sparse_layers) {
        writer.append(";deretraction_from_wipe_tower_zhop\n");
    }else
        writer.retract(-retract_length, retract_speed);

    if (!first_layer) {
        writer.change_analyzer_line_width(line_width);
    }

    bool                       toolchanges_on_layer = m_layer_info->toolchanges_depth() > WT_EPSILON;
    //writer.append("[deretraction_from_wipe_tower_generator]");


    // inner perimeter of the sparse section, if there is space for it:
    std::vector<Vec2f> finish_rect_wipe_path;
    if (extrude_fill_wall) {
        if (fill_wall_box.ru.y() - fill_wall_box.rd.y() > line_width - WT_EPSILON) {
            if (print_standalone_round_wipe_wall)
                writer.append("; CP SKELETON FLUSH WIPE WALL\n");
            writer.rectangle_fill_box(this, fill_wall_box, finish_rect_wipe_path, retract_length, retract_speed, feedrate);

            Vec2f target = (writer.pos() == fill_wall_box.ld ?
                                fill_wall_box.rd :
                                (writer.pos() == fill_wall_box.rd ? fill_wall_box.ru : (writer.pos() == fill_wall_box.ru ? fill_wall_box.lu : fill_wall_box.ld)));

            // BBS: add wipe_path for this case: only with finish rectangle
            if (finish_rect_wipe_path.size() == 2 && finish_rect_wipe_path[0] == writer.pos())
                 target = finish_rect_wipe_path[1];
            writer.add_wipe_group(writer.pos(), target);
            writer.append(";wipe_finish_path\n");
            writer.set_feedrate(retract_speed);
        }
    }

    // Extrude infill to support the material to be printed above.
    const float dy    = (fill_box.lu.y() - fill_box.ld.y() - line_width);
    float       left  = fill_box.lu.x() + 2 * line_width;
    float       right = fill_box.ru.x() - 2 * line_width;
    if (extrude_fill && dy > line_width) {
        const float fill_start_inset = first_layer && print_standalone_round_wipe_wall ? 0.f : 2.f * line_width;
        writer.travel(fill_box.ld + Vec2f(fill_start_inset, 0.f), feedrate)
            .append(";--------------\n"
                    "; CP EMPTY GRID START\n")
            .comment_with_value(wipe_tower_layer_change_tag, m_num_layer_changes + 1);


        writer.retract(-retract_length, retract_speed);
        // Is there a soluble filament wiped/rammed at the next layer?
        // If so, the infill should not be sparse.
        bool solid_infill = m_layer_info + 1 == m_plan.end() ?
                                false :
                                std::any_of((m_layer_info + 1)->tool_changes.begin(), (m_layer_info + 1)->tool_changes.end(),
                                            [this](const WipeTowerInfo::ToolChange& tch) {
                                                return m_filpar[tch.new_tool].is_soluble || m_filpar[tch.old_tool].is_soluble;
                                            });
        solid_infill |= first_layer && m_adhesion;

        if (solid_infill) {
            float sparse_factor = 1.5f; // 1=solid, 2=every other line, etc.
            if (first_layer) {          // the infill should touch perimeters
                left -= line_width;
                right += line_width;
                if (print_standalone_round_wipe_wall) {
                    left -= line_width;
                    right += line_width;
                }
                sparse_factor = 1.f;
            }
            float y       = fill_box.ld.y() + line_width;
            int   n       = dy / (line_width * sparse_factor);
            float spacing = (dy - line_width) / (n - 1);
            int   i       = 0;
            for (i = 0; i < n; ++i) {
                writer.extrude(writer.x(), y, feedrate).extrude(i % 2 ? left : right, y);
                y = y + spacing;
            }
            writer.extrude(writer.x(), fill_box.lu.y());
        } else {
            // Extrude an inverse U at the left of the region and the sparse infill.
            writer.extrude(fill_box.lu + Vec2f(line_width * 2, 0.f), feedrate);

            const int   n  = 1 + int((right - left) / m_bridging);
            const float dx = (right - left) / n;
            for (int i = 1; i <= n; ++i) {
                float x = left + dx * i;
                writer.travel(x, writer.y());
                writer.extrude(x, i % 2 ? fill_box.rd.y() : fill_box.ru.y());
            }

            finish_rect_wipe_path.clear();
            // BBS: add wipe_path for this case: only with finish rectangle
            finish_rect_wipe_path.emplace_back(writer.pos());
            finish_rect_wipe_path.emplace_back(Vec2f(left + dx * n, n % 2 ? fill_box.ru.y() : fill_box.rd.y()));
        }

        writer.append("; CP EMPTY GRID END\n"
                      ";------------------\n\n\n\n\n\n\n");
    }

    if (!first_layer)
        writer.change_analyzer_line_width(m_perimeter_width);

    const float spacing          = m_perimeter_width - m_layer_height * float(1. - M_PI_4);
    float wipe_tower_depth = m_wipe_tower_depth;
    if (m_wipe_tower_blocks.size() == 1) {
        wipe_tower_depth = m_layer_info->depth + m_perimeter_width;
    }
    WipeTower::box_coordinates wt_box(Vec2f(0.f,(m_current_shape == SHAPE_REVERSED ? m_layer_info->toolchanges_depth() : 0.f)), m_wipe_tower_width, wipe_tower_depth);
    // Keep every tower layer at the wider skeleton-flush contour. Only the side walls
    // need clearance; the depth endpoints already have the normal tower wall.
    if (m_layer_info->round_wipe_wall)
        wt_box = expand_wipe_block_box(wt_box, spacing);
    // This block creates the stabilization cone.
    // First define a lambda to draw the rectangle with stabilization.
    auto supported_rectangle = [this, &writer, spacing](const WipeTower::box_coordinates& wt_box, double feedrate,
                                                        bool infill_cone) -> Polygon {
        const auto [R, support_scale] = WipeTower2::get_wipe_tower_cone_base(m_wipe_tower_width, m_wipe_tower_height, m_wipe_tower_depth,
                                                                             m_wipe_tower_cone_angle);

        double z = m_no_sparse_layers ?
                       (m_current_height + m_layer_info->height) :
                       m_layer_info
                           ->z; // the former should actually work in both cases, but let's stay on the safe side (the 2.6.0 is close)

        double r      = std::tan(Geometry::deg2rad(m_wipe_tower_cone_angle / 2.f)) * (m_wipe_tower_height - z);
        Vec2f  center = (wt_box.lu + wt_box.rd) / 2.;
        double w      = wt_box.lu.y() - wt_box.ld.y();
        enum Type { Arc, Corner, ArcStart, ArcEnd };

        // First generate vector of annotated point which form the boundary.
        std::vector<std::pair<Vec2f, Type>> pts = {{wt_box.ru, Corner}};
        if (double alpha_start = std::asin((0.5 * w) / r); !std::isnan(alpha_start) && r > 0.5 * w + 0.01) {
            for (double alpha = alpha_start; alpha < M_PI - alpha_start + 0.001; alpha += (M_PI - 2 * alpha_start) / 40.)
                pts.emplace_back(Vec2f(center.x() + r * std::cos(alpha) / support_scale, center.y() + r * std::sin(alpha)),
                                 alpha == alpha_start ? ArcStart : Arc);
            pts.back().second = ArcEnd;
        }
        pts.emplace_back(wt_box.lu, Corner);
        pts.emplace_back(wt_box.ld, Corner);
        for (int i = int(pts.size()) - 3; i > 0; --i)
            pts.emplace_back(Vec2f(pts[i].first.x(), 2 * center.y() - pts[i].first.y()), i == int(pts.size()) - 3 ? ArcStart :
                                                                                         i == 1                   ? ArcEnd :
                                                                                                                    Arc);
        pts.emplace_back(wt_box.rd, Corner);

        // Create a Polygon from the points.
        Polygon poly;
        for (const auto& [pt, tag] : pts)
            poly.points.push_back(Point::new_scale(pt));

        // Prepare polygons to be filled by infill.
        Polylines polylines;
        if (infill_cone && m_wipe_tower_width > 2 * spacing && m_wipe_tower_depth > 2 * spacing) {
            ExPolygons infill_areas;
            ExPolygon  wt_contour(poly);
            Polygon    wt_rectangle(
                Points{Point::new_scale(wt_box.ld), Point::new_scale(wt_box.rd), Point::new_scale(wt_box.ru), Point::new_scale(wt_box.lu)});
            wt_rectangle = offset(wt_rectangle, scale_(-spacing / 2.)).front();
            wt_contour   = offset_ex(wt_contour, scale_(-spacing / 2.)).front();
            infill_areas = diff_ex(wt_contour, wt_rectangle);
            if (infill_areas.size() == 2) {
                ExPolygon&            bottom_expoly = infill_areas.front().contour.points.front().y() <
                                                   infill_areas.back().contour.points.front().y() ?
                                                          infill_areas[0] :
                                                          infill_areas[1];
                std::unique_ptr<Fill> filler(Fill::new_from_type(ipMonotonicLine));
                filler->angle   = Geometry::deg2rad(45.f);
                filler->spacing = spacing;
                FillParams params;
                params.density = 1.f;
                Surface surface(stBottom, bottom_expoly);
                filler->bounding_box = get_extents(bottom_expoly);
                polylines            = filler->fill_surface(&surface, params);
                if (!polylines.empty()) {
                    if (polylines.front().points.front().x() > polylines.back().points.back().x()) {
                        std::reverse(polylines.begin(), polylines.end());
                        for (Polyline& p : polylines)
                            p.reverse();
                    }
                }
            }
        }

        // Find the closest corner and travel to it.
        int    start_i  = 0;
        double min_dist = std::numeric_limits<double>::max();
        for (int i = 0; i < int(pts.size()); ++i) {
            if (pts[i].second == Corner) {
                double dist = (pts[i].first - Vec2f(writer.x(), writer.y())).squaredNorm();
                if (dist < min_dist) {
                    min_dist = dist;
                    start_i  = i;
                }
            }
        }
        return poly;
    };

    auto chamfer = [this, &writer, spacing, first_layer](const WipeTower::box_coordinates& wt_box) -> Polygon {
        WipeTower::box_coordinates box = wt_box;
        Polygon     poly;
        poly.points.emplace_back(Point::new_scale(box.ru));
        poly.points.emplace_back(Point::new_scale(box.lu));
        poly.points.emplace_back(Point::new_scale(box.ld));
        poly.points.emplace_back(Point::new_scale(box.rd));
        return poly;
    };

    feedrate = first_layer ? m_first_layer_speed * 60.f : m_perimeter_speed * 60.f;

    if (first_layer)
        m_wipe_tower_brim_width_real = 0.0f;
    // outer contour (always)
    bool    use_cone = m_prime_tower_enhance_type == PrimeTowerEnhanceType::pteCone;
    Polygon poly;
    if (use_cone) {
        bool infill_cone = first_layer && m_wipe_tower_width > 2 * spacing && m_wipe_tower_depth > 2 * spacing;
        poly             = supported_rectangle(wt_box, feedrate, infill_cone);
        if (first_layer)
            m_wipe_tower_stable_cone = poly;
    } else {
        poly = chamfer(wt_box);
    }

    Polylines skip_wall;
    Polygon   outer_skip_wall;
    if (m_wipe_tower_gap_wall)
    {
        skip_wall = contrust_gap_for_skip_points(poly, m_wall_skip_points, m_wipe_tower_width, 2.5 * m_perimeter_width, outer_skip_wall);
    }
    else
    {
        skip_wall.push_back(to_polyline(poly));
        outer_skip_wall = poly;
    }

    writer.generate_path(skip_wall, feedrate, retract_length, retract_speed, m_travel_speed * 60.0);

    if (extrude_perimeter) {
        Polyline shift_polyline = to_polyline(outer_skip_wall);
        shift_polyline.translate(0, scaled(m_y_shift));
        m_outer_wall[m_z_pos].push_back(shift_polyline);
    }

#if 1 //"外墙缺口"
    // How many perimeters shall the brim have?
    int         loops_num         = (m_wipe_tower_brim_width + spacing / 2.f) / spacing;
    const float max_chamfer_width = 3.f;
    if (!first_layer) {
        // stop print chamfer if depth changes
        if (m_layer_info->depth != m_plan.front().depth) {
            loops_num = 0;
        } else {
            // limit max chamfer width to 3 mm
            int chamfer_loops_num = (int) (max_chamfer_width / spacing);
            int dist_to_1st       = m_layer_info - m_plan.begin() - m_first_layer_idx;
            loops_num             = std::min(loops_num, chamfer_loops_num) - dist_to_1st;
        }
    }

    if (loops_num > 0) {
        for (size_t i = 0; i < loops_num; ++i) {
            outer_skip_wall = offset(outer_skip_wall, scaled(spacing)).front();
            writer.polygon(outer_skip_wall, feedrate);
            m_outer_wall[m_z_pos].push_back(to_polyline(outer_skip_wall));
        }
        if (first_layer) {
            // Save actual brim width to be later passed to the Print object, which will use it
            // for skirt calculation and pass it to GLCanvas for precise preview box
            m_wipe_tower_brim_width_real = loops_num * spacing + spacing / 2.f;
            // m_wipe_tower_brim_width_real = wt_box.ld.x() - box.ld.x() + spacing / 2.f;
        }
    }

    if (extrude_perimeter /*|| loops_num > 0*/) {
        //writer.add_wipe_path(outer_skip_wall, m_filpar[m_current_tool].wipe_dist);
        BoundingBox skip_wall_vertex = get_extents(outer_skip_wall);
        Vec2f       corners[4];
        corners[0] = unscaled<float>(skip_wall_vertex[0]);
        corners[1] = unscaled<float>(skip_wall_vertex[1]);
        corners[2] = unscaled<float>(skip_wall_vertex[2]);
        corners[3] = unscaled<float>(skip_wall_vertex[3]);
        int index_of_closest = 0;
        if (writer.x() - corners[0].x() > corners[0].x() + corners[2].x() - writer.x()) // closer to the right
            index_of_closest = 1;
        if (writer.y() - corners[0].y() > corners[0].y() + corners[2].y() - writer.y()) // closer to the top
            index_of_closest = (index_of_closest == 0 ? 3 : 2);

        writer.add_wipe_group(writer.pos(), corners[(index_of_closest + 4 - 1) % 4]);
        writer.append(";wipe_finish_path\n");
        writer.set_feedrate(retract_speed);
    }
    else
    {
        //Now prepare future wipe. box contains rectangle that was extruded last (ccw).
        //Vec2f target = (writer.pos() == wt_box.ld ?
        //                    wt_box.rd :
        //                    (writer.pos() == wt_box.rd ? wt_box.ru : (writer.pos() == wt_box.ru ? wt_box.lu : wt_box.ld)));

        //// BBS: add wipe_path for this case: only with finish rectangle
        //if (finish_rect_wipe_path.size() == 2 && finish_rect_wipe_path[0] == writer.pos())
        //    target = finish_rect_wipe_path[1];

        //writer.add_wipe_group(writer.pos(), target);
        //BoundingBox skip_wall_vertex = get_extents(outer_skip_wall);
        //Vec2f       corners[4];
        //corners[0]           = unscaled<float>(skip_wall_vertex[0]);
        //corners[1]           = unscaled<float>(skip_wall_vertex[1]);
        //corners[2]           = unscaled<float>(skip_wall_vertex[2]);
        //corners[3]           = unscaled<float>(skip_wall_vertex[3]);
        //int index_of_closest = 0;
        //if (writer.x() - corners[0].x() > corners[0].x() + corners[2].x() - writer.x()) // closer to the right
        //    index_of_closest = 1;
        //if (writer.y() - corners[0].y() > corners[0].y() + corners[2].y() - writer.y()) // closer to the top
        //    index_of_closest = (index_of_closest == 0 ? 3 : 2);

        //writer.add_wipe_group(writer.pos(), corners[(index_of_closest + 4 - 1) % 4]);
        //writer.append(";wipe_finish_path\n");
    }



    writer.append(";" + GCodeProcessor::reserved_tag(GCodeProcessor::ETags::Wipe_Tower_End) + "\n\n\n");

#endif
    // Ask our writer about how much material was consumed.
    // Skip this in case the layer is sparse and config option to not print sparse layers is enabled.
    if (!m_no_sparse_layers || toolchanges_on_layer || first_layer) {
        if (m_current_tool < m_used_filament_length.size())
            m_used_filament_length[m_current_tool] += writer.get_and_reset_used_filament_length();
        m_current_height += m_layer_info->height;
    }

    return construct_tcr(writer, false, old_tool, true, 0.f);
}

void WipeTowerCreality::get_wall_skip_points(const WipeTowerInfo& layer)
{
    m_wall_skip_points.clear();
    std::unordered_map<int, float> cur_block_depth;
    for (int i = 0; i < int(layer.tool_changes.size()); ++i) {
        const WipeTowerInfo::ToolChange& tool_change  = layer.tool_changes[i];
        size_t                           old_filament = tool_change.old_tool;
        size_t                           new_filament = tool_change.new_tool;
        float                            spacing      = m_layer_info->extra_spacing;
      /*  if (m_need_reverse_travel && m_layer_info->extra_spacing < m_tpu_fixed_spacing)
            spacing = 1;
        else if (m_need_reverse_travel)
            spacing = spacing / m_tpu_fixed_spacing;*/
        float nozzle_change_depth = tool_change.nozzle_change_depth * spacing;
        // float                            nozzle_change_depth = tool_change.nozzle_change_depth * (has_tpu_filament() ?
        // m_tpu_fixed_spacing : layer.extra_spacing);
        auto* block = get_block_by_category(m_filpar[new_filament].category, false);
        if (!block)
            continue;
        // float wipe_depth    = tool_change.required_depth - nozzle_change_depth;
        float wipe_depth = ceil(tool_change.wipe_length / (m_wipe_tower_width - 2 * m_perimeter_width)) * m_perimeter_width *
                           layer.extra_spacing * m_extra_flow;
        float process_depth = 0.f;
        if (!cur_block_depth.count(m_filpar[new_filament].category))
            cur_block_depth[m_filpar[new_filament].category] = block->start_depth;
        process_depth = cur_block_depth[m_filpar[new_filament].category];
        if (!m_filament_map.empty() && new_filament < m_filament_map.size() &&
            m_filament_map[old_filament] != m_filament_map[new_filament]) {
            if (m_filament_categories[new_filament] == m_filament_categories[old_filament])
                process_depth += nozzle_change_depth;
            else {
                if (!cur_block_depth.count(m_filpar[old_filament].category)) {
                    auto* old_block = get_block_by_category(m_filpar[old_filament].category, false);
                    if (!old_block)
                        continue;
                    cur_block_depth[m_filpar[old_filament].category] = old_block->start_depth;
                }
                cur_block_depth[m_filpar[old_filament].category] += nozzle_change_depth;
            }
        }

        {
            Vec2f res;
            int   index = m_cur_layer_id % 4;
            const float gap_block_depth = tool_change.round_wipe_wall ?
                                              tool_change.required_depth - tool_change.nozzle_change_depth * layer.extra_spacing :
                                              wipe_depth;
            const float gap_depth = process_depth;
            const float gap_top_depth = process_depth + gap_block_depth - layer.extra_spacing * m_perimeter_width;
            switch (index % 4) {
            case 0: res = Vec2f(0, gap_depth); break;
            case 1: res = Vec2f(m_wipe_tower_width, gap_top_depth); break;
            case 2: res = Vec2f(m_wipe_tower_width, gap_depth); break;
            case 3: res = Vec2f(0, gap_top_depth); break;
            default: break;
            }

            m_wall_skip_points.emplace_back(res);
        }

        cur_block_depth[m_filpar[new_filament].category] = process_depth + tool_change.required_depth -
                                                           tool_change.nozzle_change_depth * layer.extra_spacing;
    }
}

Vec2f WipeTowerCreality::get_next_pos(const WipeTower::box_coordinates& cleaning_box, float wipe_length)
{
    const float& xl         = cleaning_box.ld.x();
    const float& xr         = cleaning_box.rd.x();
    int          line_count = wipe_length / (xr - xl);

    float       dy         = m_layer_info->extra_spacing * m_perimeter_width * m_extra_flow;
    float       y_offset   = float(line_count) * dy;
    const Vec2f pos_offset = Vec2f(0.f, m_depth_traversed);

    Vec2f res;
    int   index = m_cur_layer_id % 4;
    // Vec2f offset = m_use_gap_wall ? Vec2f(5 * m_perimeter_width, 0) : Vec2f{0, 0};
    Vec2f offset = Vec2f{0, 0};
    switch (index % 4) {
    case 0: res = offset + cleaning_box.ld + pos_offset; break;
    case 1: res = -offset + cleaning_box.rd + pos_offset + Vec2f(0, y_offset); break;
    case 2: res = -offset + cleaning_box.rd + pos_offset; break;
    case 3: res = offset + cleaning_box.ld + pos_offset + Vec2f(0, y_offset); break;
    default: break;
    }
    return res;
}

Polygon WipeTowerCreality::generate_support_wall_new(WipeTowerWriterCreality&  writer,
                                             const WipeTower::box_coordinates& wt_box,
                                             double                 feedrate,
                                             bool                   first_layer,
                                             bool                   cone_wall,
                                             bool                   extrude_perimeter,
                                             bool                   skip_points)
{
    auto get_closet_idx = [this, &writer](Polylines& pls) -> std::pair<int, int> {
        Vec2f anchor{writer.x(), writer.y()};
        int   closestIndex = -1;
        int   closestPl    = -1;
        float minDistance  = std::numeric_limits<float>::max();
        for (int i = 0; i < pls.size(); ++i) {
            for (int j = 0; j < pls[i].size(); ++j) {
                float distance = (unscaled<float>(pls[i][j]) - anchor).squaredNorm();
                if (distance < minDistance) {
                    minDistance  = distance;
                    closestPl    = i;
                    closestIndex = j;
                }
            }
        }
        return {closestPl, closestIndex};
    };

    float retract_length = m_filpar[m_current_tool].retract_length;
    float retract_speed  = m_filpar[m_current_tool].retract_speed * 60;
    Polygon wall_polygon = cone_wall ? generate_cone_polygon(wt_box) : generate_rectange_polygon(wt_box.ld, wt_box.ru);

    Polylines result_wall;
    Polygon   insert_skip_polygon;

    if (skip_points) {
        result_wall = contrust_gap_for_skip_points(wall_polygon, m_wall_skip_points, m_wipe_tower_width, 2.5 * m_perimeter_width,
                                                   insert_skip_polygon);
    }
    else
    {
        result_wall.push_back(to_polyline(wall_polygon));
        insert_skip_polygon = wall_polygon;
    }
    writer.generate_path(result_wall, feedrate, retract_length, retract_speed, m_travel_speed * 60.0);

    // if (m_cur_layer_id == 0) {
    //     BoundingBox bbox = get_extents(result_wall);
    //     m_rib_offset     = Vec2f(-unscaled<float>(bbox.min.x()), -unscaled<float>(bbox.min.y()));
    // }

    return insert_skip_polygon;
}

Polygon WipeTowerCreality::generate_cone_polygon(const WipeTower::box_coordinates& wt_box)
{
    // 稳定锥体
    //   This block creates the stabilization cone.
    //  First define a lambda to draw the rectangle with stabilization.
    float spacing             = m_perimeter_width - m_layer_height * float(1. - M_PI_4);
    auto  supported_rectangle = [this, spacing](const WipeTower::box_coordinates& wt_box, double feedrate, bool infill_cone) -> Polygon {
        const auto [R, support_scale] = WipeTower2::get_wipe_tower_cone_base(m_wipe_tower_width, m_wipe_tower_height, m_wipe_tower_depth,
                                                                              m_wipe_tower_cone_angle);

        double z = m_no_sparse_layers ?
                        (m_current_height + m_layer_info->height) :
                        m_layer_info
                           ->z; // the former should actually work in both cases, but let's stay on the safe side (the 2.6.0 is close)

        double r      = std::tan(Geometry::deg2rad(m_wipe_tower_cone_angle / 2.f)) * (m_wipe_tower_height - z);
        Vec2f  center = (wt_box.lu + wt_box.rd) / 2.;
        double w      = wt_box.lu.y() - wt_box.ld.y();
        enum Type { Arc, Corner, ArcStart, ArcEnd };

        // First generate vector of annotated point which form the boundary.
        std::vector<std::pair<Vec2f, Type>> pts = {{wt_box.ru, Corner}};
        if (double alpha_start = std::asin((0.5 * w) / r); !std::isnan(alpha_start) && r > 0.5 * w + 0.01) {
            for (double alpha = alpha_start; alpha < M_PI - alpha_start + 0.001; alpha += (M_PI - 2 * alpha_start) / 40.)
                pts.emplace_back(Vec2f(center.x() + r * std::cos(alpha) / support_scale, center.y() + r * std::sin(alpha)),
                                 alpha == alpha_start ? ArcStart : Arc);
            pts.back().second = ArcEnd;
        }
        pts.emplace_back(wt_box.lu, Corner);
        pts.emplace_back(wt_box.ld, Corner);
        for (int i = int(pts.size()) - 3; i > 0; --i)
            pts.emplace_back(Vec2f(pts[i].first.x(), 2 * center.y() - pts[i].first.y()), i == int(pts.size()) - 3 ? ArcStart :
                                                                                          i == 1                   ? ArcEnd :
                                                                                                                     Arc);
        pts.emplace_back(wt_box.rd, Corner);

        // Create a Polygon from the points.
        Polygon poly;
        for (const auto& [pt, tag] : pts)
            poly.points.push_back(Point::new_scale(pt));

        // Prepare polygons to be filled by infill.
        Polylines polylines;
        if (infill_cone && m_wipe_tower_width > 2 * spacing && m_wipe_tower_depth > 2 * spacing) {
            ExPolygons infill_areas;
            ExPolygon  wt_contour(poly);
            Polygon    wt_rectangle(
                Points{Point::new_scale(wt_box.ld), Point::new_scale(wt_box.rd), Point::new_scale(wt_box.ru), Point::new_scale(wt_box.lu)});
            wt_rectangle = offset(wt_rectangle, scale_(-spacing / 2.)).front();
            wt_contour   = offset_ex(wt_contour, scale_(-spacing / 2.)).front();
            infill_areas = diff_ex(wt_contour, wt_rectangle);
            if (infill_areas.size() == 2) {
                ExPolygon&            bottom_expoly = infill_areas.front().contour.points.front().y() <
                                                   infill_areas.back().contour.points.front().y() ?
                                                           infill_areas[0] :
                                                           infill_areas[1];
                std::unique_ptr<Fill> filler(Fill::new_from_type(ipMonotonicLine));
                filler->angle   = Geometry::deg2rad(45.f);
                filler->spacing = spacing;
                FillParams params;
                params.density = 1.f;
                Surface surface(stBottom, bottom_expoly);
                filler->bounding_box = get_extents(bottom_expoly);
                polylines            = filler->fill_surface(&surface, params);
                if (!polylines.empty()) {
                    if (polylines.front().points.front().x() > polylines.back().points.back().x()) {
                        std::reverse(polylines.begin(), polylines.end());
                        for (Polyline& p : polylines)
                            p.reverse();
                    }
                }
            }
        }

        return poly;
    };

    bool first_layer = is_first_layer() || (m_num_tool_changes <= 1 && m_no_sparse_layers); //"无稀疏层beta"

    float feedrate = first_layer ? std::min(m_first_layer_speed * 60.f, m_max_speed) :
                                   std::min(60.0f * m_filpar[m_current_tool].max_e_speed / m_extrusion_flow, m_max_speed);

    feedrate = first_layer ? m_first_layer_speed * 60.f : m_perimeter_speed * 60.f;

    Polygon poly;
    bool    infill_cone = first_layer && m_wipe_tower_width > 2 * spacing && m_wipe_tower_depth > 2 * spacing;
    poly                = supported_rectangle(wt_box, feedrate, infill_cone);

    if (first_layer) {
        m_wipe_tower_stable_cone = poly;
    }

    return poly;
};

Polygon WipeTowerCreality::generate_rectange_polygon(const Vec2f& wt_box_min, const Vec2f& wt_box_max)
{
    Polygon res;
    res.points.push_back(scaled(wt_box_min));
    res.points.push_back(scaled(Vec2f{wt_box_max[0], wt_box_min[1]}));
    res.points.push_back(scaled(wt_box_max));
    res.points.push_back(scaled(Vec2f{wt_box_min[0], wt_box_max[1]}));
    return res;
}


} // namespace Slic3r
