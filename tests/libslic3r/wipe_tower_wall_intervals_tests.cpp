#include "../../src/libslic3r/FDM/WipeTowerWallIntervals.hpp"

#include <cstdlib>
#include <iostream>

using Slic3r::merge_wipe_tower_wall_gaps;
using Intervals = std::vector<std::pair<double, double>>;

static void require_intervals(const Intervals& actual, const Intervals& expected, const char* message)
{
    bool equal = actual.size() == expected.size();
    for (size_t i = 0; equal && i < actual.size(); ++i)
        equal = std::abs(actual[i].first - expected[i].first) < 1e-7 &&
                std::abs(actual[i].second - expected[i].second) < 1e-7;
    if (!equal) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

int main()
{
    // Layer 103 of the reported i8 cube: two 2.5 mm gaps share an endpoint.
    // Three gaps must remove only 7.5 mm, leaving the rest of the wall intact.
    require_intervals(merge_wipe_tower_wall_gaps(159.072, {10., 14., 16.5}, 1.25),
                      {{8.75, 11.25}, {12.75, 17.75}}, "Touching gaps must merge without losing a boundary");
    require_intervals(merge_wipe_tower_wall_gaps(40., {10., 8., 9., 8.}, 2.),
                      {{6., 12.}}, "Overlapping and duplicate gaps must form one interval");
    require_intervals(merge_wipe_tower_wall_gaps(40., {7., 4.5, 2.}, 1.25),
                      {{.75, 8.25}}, "A reversed chain of touching gaps must merge");
    require_intervals(merge_wipe_tower_wall_gaps(40., {5., 20.}, 1.),
                      {{4., 6.}, {19., 21.}}, "Separate gaps must preserve the wall between them");
    require_intervals(merge_wipe_tower_wall_gaps(40., {0.}, 1.25),
                      {{0., 1.25}, {38.75, 40.}}, "A gap at the contour origin must wrap");
    require_intervals(merge_wipe_tower_wall_gaps(40., {39., 1.}, 2.),
                      {{0., 3.}, {37., 40.}}, "Overlapping gaps across the contour origin must merge");
    require_intervals(merge_wipe_tower_wall_gaps(40., {1., 39.}, 1.),
                      {{0., 2.}, {38., 40.}}, "Gap boundaries may lie exactly at the contour origin");
    require_intervals(merge_wipe_tower_wall_gaps(40., {0., 10., 20., 30.}, 5.),
                      {{0., 40.}}, "Multiple gaps may cover the whole wall");
    require_intervals(merge_wipe_tower_wall_gaps(40., {5.}, 20.),
                      {{0., 40.}}, "A gap as long as the perimeter must cover the whole wall");
    require_intervals(merge_wipe_tower_wall_gaps(40., {}, 1.), {}, "No centers must preserve the closed wall");
    require_intervals(merge_wipe_tower_wall_gaps(40., {5.}, 0.), {}, "Zero-size gaps must preserve the wall");
    require_intervals(merge_wipe_tower_wall_gaps(0., {5.}, 1.), {}, "An empty contour must not divide by zero");
    using Slic3r::fit_wipe_tower_gap_on_edge;
    auto require_gap = [](double center, double edge_end, const Intervals& expected, const char* message) {
        const auto gap = fit_wipe_tower_gap_on_edge(center, 1.25, 0., edge_end, .5, .5);
        require_intervals(gap ? Intervals{*gap} : Intervals{}, expected, message);
    };
    require_gap(5., 10., {{3.75, 6.25}}, "Safe openings must stay unchanged");
    require_gap(8.5, 10., {{7., 9.5}}, "Move gap off the upper corner without shortening the opening");
    require_gap(1., 10., {{.5, 3.}}, "Move gap off the lower corner without shortening the opening");
    require_gap(1.15, 2.3, {{.5, 1.8}}, "Short edges may narrow an opening while retaining entry clearance");
    require_gap(9.5, 10., {}, "An entry too close to a corner must be moved before cutting");
    require_gap(.75, 1.5, {}, "Reject an edge too short for corners and nozzle clearance");
    require_gap(1., 2., {{.5, 1.5}}, "Exact minimum clearance must fit");
    require_intervals(Slic3r::merge_wipe_tower_wall_intervals({{.5, 3.}, {3., 5.5}, {7., 9.5}}),
                      {{.5, 5.5}, {7., 9.5}}, "Merging adjusted gaps must retain protected corners");
    std::cout << "20 wipe tower wall interval regression cases passed\n";
}
