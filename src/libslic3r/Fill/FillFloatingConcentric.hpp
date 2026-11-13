#ifndef slic3r_FillFloatingConcentric_hpp_
#define slic3r_FillFloatingConcentric_hpp_

#include "FillConcentric.hpp"
#include "../Arachne/WallToolPaths.hpp"

namespace Slic3r {

struct FloatingThickline : public ThickLine
{
    FloatingThickline(const Point& a, const Point& b, double wa, double wb, bool a_floating, bool b_floating)
        : ThickLine(a, b, wa, wb), is_a_floating(a_floating), is_b_floating(b_floating) {}

    bool is_a_floating;
    bool is_b_floating;
};
using FloatingThicklines = std::vector<FloatingThickline>;

// The marker is attached to vertices; an edge floats only when both ends are marked.
struct FloatingThickPolyline : public ThickPolyline
{
    std::vector<bool> is_floating;

    FloatingThickPolyline rebase_at(size_t idx) const;
    FloatingThicklines floating_thicklines() const;
    // Local to narrow floating infill; resolution is in mm.
    ExtrusionPaths to_extrusion_paths(ExtrusionRole role, const Flow& flow, double resolution) const;
    void clip_end(double distance);
};
using FloatingThickPolylines = std::vector<FloatingThickPolyline>;

class FillFloatingConcentric : public FillConcentric
{
public:
    ~FillFloatingConcentric() override = default;

    // Sparse/void areas determine the slowdown marker. Actual lower sparse beads
    // are used separately to choose a supported start on a closed path.
    ExPolygons lower_layer_unsupport_areas;
    Polygons lower_sparse_polys;

    void fill_surface_extrusion(const Surface* surface, const FillParams& params, ExtrusionEntitiesPtr& out) override;

protected:
    Fill* clone() const override { return new FillFloatingConcentric(*this); }

    static FloatingThickPolyline detect_floating_line(const ThickPolyline& line,
        const ExPolygons& floating_areas, double default_width, bool force_no_detect);

    void _fill_surface_single(const FillParams& params, unsigned int thickness_layers,
        const std::pair<float, Point>& direction, ExPolygon expolygon, FloatingThickPolylines& thick_polylines_out);
    FloatingThickPolylines fill_surface_arachne_floating(const Surface* surface, const FillParams& params);
    FloatingThickPolylines resplit_order_loops(Point curr_point, const std::vector<const Arachne::ExtrusionLine*>& all_extrusions,
        const ExPolygons& floating_areas, const Polygons& sparse_polys, coord_t default_width);
};

} // namespace Slic3r

#endif // slic3r_FillFloatingConcentric_hpp_
