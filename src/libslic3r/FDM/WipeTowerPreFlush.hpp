#ifndef SLIC3R_WIPE_TOWER_PRE_FLUSH_HPP
#define SLIC3R_WIPE_TOWER_PRE_FLUSH_HPP

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace Slic3r {
namespace wipe_tower_pre_flush {

struct Point {
    double x = 0.;
    double y = 0.;
};

struct Geometry {
    double width = 0.;
    double pitch = 0.;
    double left = 0.;
    double right = 0.;
    double e_per_mm = 0.;
};

struct Move {
    Point from;
    Point to;
    double filament_length = 0.;
    double width = 0.;
    bool coarse = false;
};

struct Parameters {
    double nozzle_diameter = 0.4;
    double base_width = 0.5;
    double layout_width = 0.5;
    double tower_width = 35.;
    double layer_height = 0.2;
    double filament_area = 0.;
    double extra_flow = 1.;
    double spacing = 1.;
    double start_offset = 0.;
    double pre_flush_volume = 0.;
    double wipe_volume = 0.;
    bool first_layer = false;
};

struct Layout {
    Geometry coarse;
    Geometry fine;
    std::vector<Move> moves;
    Point start;
    Point end;
    double coarse_volume = 0.;
    double depth = 0.; // Extrusion footprint only; structural margins are added by the tower.
    double min_x = 0.;
    double max_x = 0.;
};

inline double coarse_base_width(double diameter)
{
    const double diameters[] = {0.2, 0.4, 0.6, 0.8};
    const double widths[] = {0.5, 1.0, 1.2, 1.4};
    for (int i = 0; i < 4; ++i)
        if (std::abs(diameter - diameters[i]) < 1e-4)
            return widths[i];
    throw std::invalid_argument("Unsupported nozzle diameter for wipe tower pre-flush.");
}

inline Layout make_layout(const Parameters& p)
{
    Layout result;
    if (p.pre_flush_volume <= 0. || p.wipe_volume <= 0.)
        return result;
    if (!(p.layer_height > 0.) || !(p.filament_area > 0.) ||
        !(p.base_width > 0.) || !(p.spacing >= 1.))
        throw std::invalid_argument("Invalid wipe tower pre-flush geometry.");

    const double cap = p.layer_height * (1. - std::acos(-1.) / 4.);
    const double fine_width = p.base_width * (p.first_layer ? 1. : p.extra_flow);
    const double coarse_width = std::max(coarse_base_width(p.nozzle_diameter), fine_width);
    const double gap = p.first_layer ? 0. : fine_width * (p.spacing - 1.);
    const double coarse_gap =
        p.first_layer ? 0. : fine_width * (std::max(p.spacing, 1.2) - 1.);
    auto geometry = [&](double width, double pitch, bool coarse) {
        Geometry g;
        g.width = width;
        g.pitch = pitch;
        g.left = 0.5 * (p.layout_width + width);
        g.right = p.tower_width - g.left;
        g.e_per_mm = p.layer_height * (width - cap) / p.filament_area;
        if (!coarse && p.first_layer)
            g.e_per_mm *= 1.18;
        if (!(g.right > g.left) || !(g.e_per_mm > 0.))
            throw std::invalid_argument("The prime tower is too narrow for the active nozzle widths.");
        return g;
    };
    result.coarse = geometry(coarse_width, coarse_width + coarse_gap, true);
    result.fine = geometry(fine_width, fine_width + gap, false);

    const double total_e = p.wipe_volume / p.filament_area;
    // Share the existing tower purge budget, then convert volume to filament length.
    const double coarse_volume = std::min(p.pre_flush_volume, p.wipe_volume);
    const double coarse_e = coarse_volume / p.filament_area;
    result.coarse_volume = coarse_volume;
    Point current{result.coarse.left + std::min(std::max(0., p.start_offset),
                     0.5 * (result.coarse.right - result.coarse.left)), coarse_width * 0.5};
    result.start = current;
    double min_y = 0.;
    double max_y = coarse_width;
    result.min_x = p.tower_width;
    result.max_x = 0.;

    // Keep the coarse budget exact. Fine moves consume the remaining minimum
    // purge budget, but always finish their segment so the final row is complete.
    auto append = [&](Point target, double& remaining, const Geometry& g, bool coarse) {
        const double length = std::hypot(target.x - current.x, target.y - current.y);
        if (length <= 1e-12 || (coarse && remaining <= 0.))
            return;
        const double requested_e = length * g.e_per_mm;
        const double e = coarse ? std::min(requested_e, remaining) : requested_e;
        const double ratio = e / requested_e;
        const Point actual = e == requested_e ? target :
            Point{current.x + (target.x - current.x) * ratio,
                  current.y + (target.y - current.y) * ratio};
        // The first-layer fine flow boost changes its deposited footprint too.
        const double actual_width = g.e_per_mm * p.filament_area / p.layer_height + cap;
        result.moves.push_back(Move{current, actual, e, actual_width, coarse});
        min_y = std::min(min_y, std::min(current.y, actual.y) - actual_width * 0.5);
        max_y = std::max(max_y, std::max(current.y, actual.y) + actual_width * 0.5);
        result.min_x = std::min(result.min_x, std::min(current.x, actual.x) - actual_width * 0.5);
        result.max_x = std::max(result.max_x, std::max(current.x, actual.x) + actual_width * 0.5);
        current = actual;
        remaining = std::max(0., remaining - e);
    };

    bool left_to_right = true;
    bool coarse_ended_in_turn = false;
    double coarse_row_y = current.y;
    double turn_y = current.y;
    double coarse_remaining = coarse_e;
    while (coarse_remaining > 0.) {
        coarse_ended_in_turn = false;
        coarse_row_y = current.y;
        append(Point{left_to_right ? result.coarse.right : result.coarse.left, current.y},
               coarse_remaining, result.coarse, true);
        if (coarse_remaining <= 0.)
            break;
        coarse_ended_in_turn = true;
        turn_y = current.y + result.coarse.pitch;
        append(Point{current.x, turn_y}, coarse_remaining, result.coarse, true);
        left_to_right = !left_to_right;
    }

    double fine_e = std::max(0., total_e - coarse_e);
    if (fine_e > 0.) {
        const double fine_footprint_width = result.fine.e_per_mm * p.filament_area / p.layer_height + cap;
        const double mixed_row_pitch = 0.5 * (std::max(coarse_width, fine_footprint_width) +
                                                   fine_footprint_width) + gap;
        bool mixed_row = true;
        if (coarse_ended_in_turn) {
            // Finish a partially extruded turn before traversing the next row.
            append(Point{current.x, std::max(turn_y, coarse_row_y + mixed_row_pitch)},
                   fine_e, result.fine, false);
            mixed_row = false;
        } else if (std::abs(current.x - (left_to_right ? result.coarse.right : result.coarse.left)) <= 1e-12) {
            // A complete coarse row already reaches the wall; do not add a
            // short fine extension just because fine center bounds are wider.
            append(Point{current.x, current.y + mixed_row_pitch}, fine_e, result.fine, false);
            left_to_right = !left_to_right;
            mixed_row = false;
        }

        while (true) {
            // Preserve the coarse direction when switching partway along a row.
            append(Point{left_to_right ? result.fine.right : result.fine.left, current.y},
                   fine_e, result.fine, false);
            // Ignore roundoff at a row boundary instead of adding another row.
            if (fine_e <= 1e-10 * std::max(1., total_e))
                break;
            append(Point{current.x, current.y + (mixed_row ? mixed_row_pitch : result.fine.pitch)},
                   fine_e, result.fine, false);
            left_to_right = !left_to_right;
            mixed_row = false;
        }
    }

    // Normalize the full footprint so block placement and mirroring use the
    // same bounds, including the fine first-layer flow boost.
    for (Move& move : result.moves) {
        move.from.y -= min_y;
        move.to.y -= min_y;
    }
    result.start.y -= min_y;
    current.y -= min_y;
    result.end = current;
    result.depth = max_y - min_y;
    return result;
}

inline Point place(const Layout& layout, Point point, double tower_width,
                   unsigned direction, double origin_y)
{
    direction %= 4;
    const bool reverse_x = direction == 1 || direction == 2;
    const bool reverse_y = direction == 1 || direction == 3;
    return Point{reverse_x ? tower_width - point.x : point.x,
                 origin_y + (reverse_y ? layout.depth - point.y : point.y)};
}

} // namespace wipe_tower_pre_flush
} // namespace Slic3r

#endif
