#include "../../src/libslic3r/FDM/WipeTowerPreFlush.hpp"

#include <cstdlib>
#include <iostream>

using namespace Slic3r::wipe_tower_pre_flush;

static size_t checks = 0;
static void require(bool condition, const char* message)
{
    ++checks;
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

static bool near(double a, double b, double tolerance = 1e-8)
{
    return std::abs(a - b) <= tolerance * std::max(1., std::max(std::abs(a), std::abs(b)));
}

static Parameters defaults()
{
    Parameters p;
    p.filament_area = std::acos(-1.) * 1.75 * 1.75 / 4.;
    p.pre_flush_volume = 10. * p.filament_area;
    p.wipe_volume = 100.;
    p.spacing = 1.5;
    return p;
}

static void verify(const Parameters& p, const Layout& layout)
{
    require(!layout.moves.empty(), "An enabled purge must have extrusion moves");
    double coarse_e = 0., fine_e = 0.;
    bool fine_started = false;
    Point previous = layout.start;
    double row_y = -1., row_width = 0.;
    bool row_has_coarse = false;
    const Move* last_coarse = nullptr;
    for (const auto& move : layout.moves) {
        require(near(previous.x, move.from.x) && near(previous.y, move.from.y), "Planned segments must be continuous");
        const double length = std::hypot(move.to.x - move.from.x, move.to.y - move.from.y);
        require(length > 0. && move.filament_length > 0., "Extrusion segments must have positive length and E");
        require(!fine_started || !move.coarse, "Coarse extrusion must precede all fine extrusion");
        if (!move.coarse && !fine_started && last_coarse && near(last_coarse->from.y, last_coarse->to.y)) {
            const bool coarse_reached_edge = near(last_coarse->to.x, layout.coarse.left) ||
                                             near(last_coarse->to.x, layout.coarse.right);
            if (!coarse_reached_edge) {
                require(near(move.from.y, move.to.y), "A mid-row width change must not start with a turn");
                require((move.to.x - move.from.x) * (last_coarse->to.x - last_coarse->from.x) > 0.,
                        "A mid-row width change must preserve the travel direction");
            }
        }
        fine_started |= !move.coarse;
        if (move.coarse)
            last_coarse = &move;
        (move.coarse ? coarse_e : fine_e) += move.filament_length;
        const auto& g = move.coarse ? layout.coarse : layout.fine;
        require(near(move.filament_length / length, g.e_per_mm), "Clipping must preserve the phase flow");
        require(move.from.x >= g.left - 1e-8 && move.from.x <= g.right + 1e-8 &&
                move.to.x >= g.left - 1e-8 && move.to.x <= g.right + 1e-8, "Move centers must stay inside their phase boundaries");
        require(std::min(move.from.y, move.to.y) - move.width * .5 >= -1e-8 &&
                std::max(move.from.y, move.to.y) + move.width * .5 <= layout.depth + 1e-8,
                "Reserved depth must contain the complete extrusion footprint");
        require(std::min(move.from.x, move.to.x) - move.width * .5 >= -1e-8 &&
                std::max(move.from.x, move.to.x) + move.width * .5 <= p.tower_width + 1e-8,
                "Extrusions must remain inside the actual tower width");
        if (near(move.from.y, move.to.y)) {
            if (row_y >= 0. && !near(move.from.y, row_y)) {
                const double pitch = move.from.y - row_y;
                if (move.coarse || !row_has_coarse)
                    require(near(pitch, g.pitch), "Uniform-width rows must use their planned center pitch");
                if (!p.first_layer || row_has_coarse) {
                    const double gap = p.first_layer ? 0. : layout.fine.width * (p.spacing - 1.);
                    require(pitch + 1e-8 >= .5 * (row_width + move.width) + gap,
                            "Rows beside coarse extrusion must leave room for its complete footprint");
                }
                row_width = 0.;
                row_has_coarse = false;
            }
            row_y = move.from.y;
            row_width = std::max(row_width, move.width);
            row_has_coarse |= move.coarse;
        }
        previous = move.to;
    }
    require(near(previous.x, layout.end.x) && near(previous.y, layout.end.y), "The planned end must be the actual last endpoint");
    const double required_e = p.wipe_volume / p.filament_area;
    require(coarse_e + fine_e + 1e-8 * std::max(1., required_e) >= required_e,
            "Coarse plus fine must satisfy the minimum tower volume");
    const double expected_coarse_e = std::min(p.pre_flush_volume, p.wipe_volume) / p.filament_area;
    require(near(coarse_e, expected_coarse_e), "The configured volume must define the coarse budget");
    require(near(layout.coarse_volume, coarse_e * p.filament_area), "Stored coarse volume must agree with the paths");
    if (p.pre_flush_volume >= p.wipe_volume) {
        require(fine_e == 0., "No fine tail may be forced when the coarse phase uses the entire budget");
        require(near(coarse_e, required_e), "Coarse-only extrusion must retain its exact budget");
    } else {
        const auto& last = layout.moves.back();
        require(!last.coarse && near(last.from.y, last.to.y), "A fine tail must end in a complete horizontal row");
        require(near(last.to.x, layout.fine.left) || near(last.to.x, layout.fine.right),
                "The final fine row must reach its side boundary");
        double before_last_row = coarse_e + fine_e - last.filament_length;
        if (layout.moves.size() > 1) {
            const auto& turn = layout.moves[layout.moves.size() - 2];
            if (!turn.coarse && near(turn.from.x, turn.to.x))
                before_last_row -= turn.filament_length;
        }
        require(before_last_row < required_e + 1e-8 * std::max(1., required_e),
                "Row completion must not add another row after the minimum volume is met");
    }
}

int main()
{
    auto p = defaults();
    auto layout = make_layout(p);
    require(near(layout.coarse.width, 1.), "0.4 mm nozzle must use a 1 mm coarse line");
    require(near(layout.coarse.pitch, 1.25), "150 percent spacing must leave a 0.25 mm gap");
    require(near(layout.coarse.right - layout.coarse.left, 33.5), "35 mm tower must allow a 33.5 mm coarse horizontal");
    verify(p, layout);

    for (double nozzle : {0.2, 0.4, 0.6, 0.8})
        for (bool first : {false, true})
            for (double flow : {1., 2.})
                for (double spacing : {1., 1.5, 3.})
                    for (double length : {0.001, 1., 10., 1000.})
                        for (double volume : {0.01, 15., 100.})
                            for (double height : {nozzle * .25, nozzle * .5})
                                for (double offset : {0., 2., 100.}) {
                                    p = defaults();
                                    p.nozzle_diameter = nozzle;
                                    p.base_width = nozzle * 1.25;
                                    p.layout_width = std::max(p.base_width, 0.75); // mixed-nozzle boundary width
                                    p.extra_flow = flow;
                                    p.first_layer = first;
                                    p.spacing = spacing;
                                    p.pre_flush_volume = length * p.filament_area;
                                    p.wipe_volume = volume;
                                    p.layer_height = height;
                                    p.start_offset = offset;
                                    layout = make_layout(p);
                                    verify(p, layout);
                                    require(layout.coarse.width >= layout.fine.width, "Coarse width must never be less than the configured fine width");
                                    const double cap = height * (1. - std::acos(-1.) / 4.);
                                    require(near(layout.coarse.e_per_mm,
                                            height * (layout.coarse.width - cap) / p.filament_area),
                                            "Coarse flow must never receive the first-layer 18 percent boost");
                                    require(near(layout.fine.e_per_mm,
                                            height * (layout.fine.width - cap) / p.filament_area * (first ? 1.18 : 1.)),
                                            "Fine flow must retain the existing first-layer boost");
                                    if (first)
                                        require(near(layout.coarse.pitch, layout.coarse.width), "First-layer coarse lines must be tightly spaced");
                                }

    // A volume setting must produce the same coarse path for different filament diameters.
    for (double volume : {0.001, 10., 15., 20.}) {
        double reference_path_length = -1.;
        for (double diameter : {1.75, 2.85}) {
            p = defaults();
            p.filament_area = std::acos(-1.) * diameter * diameter / 4.;
            p.pre_flush_volume = volume;
            p.wipe_volume = 15.;
            layout = make_layout(p);
            verify(p, layout);

            double coarse_e = 0., coarse_path_length = 0.;
            for (const auto& move : layout.moves) {
                if (!move.coarse)
                    continue;
                coarse_e += move.filament_length;
                coarse_path_length += std::hypot(move.to.x - move.from.x, move.to.y - move.from.y);
            }
            const double expected_volume = std::min(volume, p.wipe_volume);
            const double cap = p.layer_height * (1. - std::acos(-1.) / 4.);
            const double coarse_area = p.layer_height * (layout.coarse.width - cap);
            require(near(coarse_e * p.filament_area, expected_volume),
                    "Coarse feed volume must match the setting and respect the tower budget");
            require(near(coarse_path_length * coarse_area, expected_volume),
                    "The coarse path geometry must deposit the configured volume");
            if (reference_path_length < 0.)
                reference_path_length = coarse_path_length;
            else
                require(near(coarse_path_length, reference_path_length),
                        "Filament diameter must not change the path for a fixed coarse volume");
        }
    }

    p = defaults();
    p.pre_flush_volume = 0.;
    require(make_layout(p).moves.empty(), "Zero must bypass pre-flush planning");
    p = defaults();
    p.wipe_volume = 0.;
    require(make_layout(p).moves.empty(), "Zero tower budget must not add purge material");
    p = defaults();
    p.pre_flush_volume = 0.001 * p.filament_area;
    layout = make_layout(p);
    require(layout.moves.front().to.x < layout.coarse.right, "A small coarse budget must truncate its first horizontal");
    verify(p, layout);
    p = defaults();
    p.wipe_volume = p.filament_area * 0.0011;
    p.pre_flush_volume = 0.001 * p.filament_area;
    layout = make_layout(p);
    verify(p, layout);
    require(layout.moves.size() == 2 && !layout.moves.back().coarse &&
            near(layout.moves.back().from.y, layout.moves.back().to.y) && near(layout.end.x, layout.fine.right),
            "Even a tiny fine budget must finish the same row as the partial coarse line");

    // Check both directions, including switches beyond the row midpoint.
    for (int row = 0; row < 3; ++row)
        for (double fraction : {.25, .75}) {
            p = defaults();
            const auto shape = make_layout(p);
            const double row_e = (shape.coarse.right - shape.coarse.left) * shape.coarse.e_per_mm;
            const double turn_e = shape.coarse.pitch * shape.coarse.e_per_mm;
            p.pre_flush_volume = (row * (row_e + turn_e) + fraction * row_e) * p.filament_area;
            p.wipe_volume = p.pre_flush_volume + .0001 * p.filament_area;
            layout = make_layout(p);
            verify(p, layout);
            const auto& fine = layout.moves.back();
            require(near(fine.from.y, fine.to.y), "A partial coarse row must finish horizontally with fine extrusion");
            require(near(fine.to.x, row % 2 == 0 ? layout.fine.right : layout.fine.left),
                    "Fine extrusion must continue in the coarse row direction on either side of its midpoint");
        }

    // Set coarse E/mm to exactly one to exercise exact row and turn boundaries.
    for (double fraction : {0., .25, .75, 1.}) {
        p = defaults();
        const double cap = p.layer_height * (1. - std::acos(-1.) / 4.);
        p.filament_area = p.layer_height * (1. - cap);
        const auto shape = make_layout(p);
        p.pre_flush_volume = (shape.coarse.right - shape.coarse.left + fraction * shape.coarse.pitch) * p.filament_area;
        p.wipe_volume = p.pre_flush_volume + .0001 * p.filament_area;
        layout = make_layout(p);
        verify(p, layout);
        size_t first_fine = 0;
        while (layout.moves[first_fine].coarse)
            ++first_fine;
        if (fraction < 1.)
            require(near(layout.moves[first_fine].from.x, layout.moves[first_fine].to.x),
                    "A complete coarse row or partial coarse turn must step to the next row first");
        require(near(layout.end.x, layout.fine.left), "A fine row after a coarse turn must reverse direction");
        require(near(layout.end.y - layout.start.y, fraction == 0. ? 1. : 1.25),
                "Switching during a coarse turn must finish its planned pitch before the fine row");
    }

    // A target exactly at a row edge must stop there; a target in the following
    // turn must finish that turn and the next horizontal instead of leaving a stub.
    for (double tail : {0., .5}) {
        p = defaults();
        const auto shape = make_layout(p);
        const double coarse_length = .25 * (shape.coarse.right - shape.coarse.left);
        p.pre_flush_volume = coarse_length * shape.coarse.e_per_mm * p.filament_area;
        const double fine_length = shape.fine.right - shape.start.x - coarse_length;
        const double mixed_pitch = .5 * (shape.coarse.width + shape.fine.width) + .25;
        p.wipe_volume = p.pre_flush_volume + (fine_length + tail * mixed_pitch) * shape.fine.e_per_mm * p.filament_area;
        layout = make_layout(p);
        verify(p, layout);
        size_t fine_rows = 0;
        for (const auto& move : layout.moves)
            if (!move.coarse && near(move.from.y, move.to.y))
                ++fine_rows;
        require(fine_rows == (tail == 0. ? 1u : 2u), "Only the final required fine row may be completed");
    }
    // A partial first line has asymmetric X bounds: mirror its enclosing wall
    // together with the path, rather than retaining the original left bounds.
    p = defaults();
    p.pre_flush_volume = 1000.;
    p.wipe_volume = .01;
    layout = make_layout(p);
    for (unsigned direction = 0; direction < 4; ++direction) {
        const auto a = place(layout, {layout.min_x, 0.}, p.tower_width, direction, 7.);
        const auto b = place(layout, {layout.max_x, layout.depth}, p.tower_width, direction, 7.);
        const double left = std::min(a.x, b.x), right = std::max(a.x, b.x);
        require(near(left, direction == 1 || direction == 2 ? p.tower_width - layout.max_x : layout.min_x),
                "The footprint must mirror in the same direction as the extrusion");
        for (const auto& move : layout.moves) {
            const auto from = place(layout, move.from, p.tower_width, direction, 7.);
            const auto to = place(layout, move.to, p.tower_width, direction, 7.);
            require(std::min(from.x,to.x)-move.width*.5 >= left-1e-8 &&
                    std::max(from.x,to.x)+move.width*.5 <= right+1e-8,
                    "Mirrored wall bounds must enclose the extrusion");
            require(std::min(from.y,to.y)-move.width*.5 >= 7.-1e-8 &&
                    std::max(from.y,to.y)+move.width*.5 <= 7.+layout.depth+1e-8,
                    "Mirrored extrusion must remain in its reserved block");
        }
    }
    p = defaults();
    p.tower_width = 1.;
    bool rejected = false;
    try { make_layout(p); } catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "A tower too narrow for the coarse width must be rejected");
    std::cout << "Pre-flush geometry and budget checks passed: " << checks << '\n';
}
