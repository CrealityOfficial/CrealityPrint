#ifndef slic3r_WipeTowerWallIntervals_hpp_
#define slic3r_WipeTowerWallIntervals_hpp_

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace Slic3r {

inline std::vector<std::pair<double, double>> merge_wipe_tower_wall_intervals(
    std::vector<std::pair<double, double>> gaps)
{
    std::sort(gaps.begin(), gaps.end());
    std::size_t count = 0;
    constexpr double tolerance = 1e-6;
    for (const auto gap : gaps) {
        if (count > 0 && gap.first <= gaps[count - 1].second + tolerance)
            gaps[count - 1].second = std::max(gaps[count - 1].second, gap.second);
        else
            gaps[count++] = gap;
    }
    gaps.resize(count);
    return gaps;
}

// Keep the stored float entry inside the double-precision safe interval.
// Rounding a boundary outwards would fail the subsequent gap clearance check.
inline std::optional<float> clamp_wipe_tower_entry(float requested, double begin, double end)
{
    if (begin > end)
        return std::nullopt;
    float entry = float(std::clamp(double(requested), begin, end));
    if (entry < begin)
        entry = std::nextafter(entry, std::numeric_limits<float>::infinity());
    else if (entry > end)
        entry = std::nextafter(entry, -std::numeric_limits<float>::infinity());
    if (entry < begin || entry > end)
        return std::nullopt;
    return entry;
}

// Slide a gap along one edge, preserving both corners and entry clearance.
inline std::optional<std::pair<double, double>> fit_wipe_tower_gap_on_edge(
    double center, double radius, double edge_begin, double edge_end,
    double corner_margin, double entry_clearance)
{
    const double begin = edge_begin + corner_margin;
    const double end = edge_end - corner_margin;
    const double width = std::min(2. * radius, end - begin);
    constexpr double tolerance = 1e-6;
    if (width <= 0. || width + tolerance < 2. * entry_clearance ||
        center < begin + entry_clearance - tolerance || center > end - entry_clearance + tolerance)
        return std::nullopt;
    const double gap_begin = std::clamp(center - 0.5 * width, begin, end - width);
    return std::make_pair(gap_begin, gap_begin + width);
}

// Distances are measured along the closed wall, in millimeters. Split gaps
// crossing the contour origin before merging, so coincident endpoints do not
// need to carry (and potentially overwrite) multiple gap-pair identifiers.
inline std::vector<std::pair<double, double>> merge_wipe_tower_wall_gaps(
    double perimeter, const std::vector<double>& centers, double radius)
{
    std::vector<std::pair<double, double>> gaps;
    if (perimeter <= 0. || radius <= 0. || centers.empty())
        return gaps;
    if (2. * radius >= perimeter)
        return {{0., perimeter}};

    for (double center : centers) {
        center = std::fmod(center, perimeter);
        if (center < 0.)
            center += perimeter;
        const double begin = center - radius;
        const double end = center + radius;
        if (begin < 0.) {
            gaps.emplace_back(0., end);
            gaps.emplace_back(begin + perimeter, perimeter);
        } else if (end > perimeter) {
            gaps.emplace_back(begin, perimeter);
            gaps.emplace_back(0., end - perimeter);
        } else {
            gaps.emplace_back(begin, end);
        }
    }

    return merge_wipe_tower_wall_intervals(std::move(gaps));
}

} // namespace Slic3r

#endif
