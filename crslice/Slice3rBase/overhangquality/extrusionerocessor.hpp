#ifndef slic3r_ExtrusionProcessor_QUALITY_hpp_
#define slic3r_ExtrusionProcessor_QUALITY_hpp_


#include "Slice3rBase/AABBTreeLines.hpp"
#include "Slice3rBase/libslic3r.h"
#include "Slice3rBase/Point.hpp"
#include "Slice3rBase/BoundingBox.hpp"
#include "Slice3rBase/Polygon.hpp"
#include "Slice3rBase/ClipperUtils.hpp"
#include "overhanghead.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Slic3r {

class SlidingWindowCurvatureAccumulator
{
    float        window_size;
    float        total_distance  = 0; // accumulated distance
    float        total_curvature = 0; // accumulated signed ccw angles
    deque<float> distances;
    deque<float> angles;

public:
    SlidingWindowCurvatureAccumulator(float window_size) : window_size(window_size) {}

    void add_point(float distance, float angle)
    {
        total_distance += distance;
        total_curvature += angle;
        distances.push_back(distance);
        angles.push_back(angle);

        while (distances.size() > 1 && total_distance > window_size) {
            total_distance -= distances.front();
            total_curvature -= angles.front();
            distances.pop_front();
            angles.pop_front();
        }
    }

    float get_curvature() const
    {
        return total_curvature / window_size;
    }

    void reset()
    {
        total_curvature = 0;
        total_distance  = 0;
        distances.clear();
        angles.clear();
    }
};

class CurvatureEstimator
{
    static const size_t               sliders_count          = 3;
    SlidingWindowCurvatureAccumulator sliders[sliders_count] = {{1.0},{4.0}, {10.0}};

public:
    void add_point(float distance, float angle)
    {
        if (distance < EPSILON)
            return;
        for (SlidingWindowCurvatureAccumulator &slider : sliders) {
            slider.add_point(distance, angle);
        }
    }
    float get_curvature()
    {
        float max_curvature = 0.0f;
        for (const SlidingWindowCurvatureAccumulator &slider : sliders) {
            if (abs(slider.get_curvature()) > abs(max_curvature)) {
                max_curvature = slider.get_curvature();
            }
        }
        return max_curvature;
    }
    void reset()
    {
        for (SlidingWindowCurvatureAccumulator &slider : sliders) {
            slider.reset();
        }
    }
};

template<bool SCALED_INPUT, bool ADD_INTERSECTIONS, bool PREV_LAYER_BOUNDARY_OFFSET, bool SIGNED_DISTANCE, typename P, typename L>
std::vector<ExtendedPoint> estimate_points_properties(const std::vector<P>                   &input_points,
                                                      const AABBTreeLines::LinesDistancer<L> &unscaled_prev_layer,
                                                      float                                   flow_width,
                                                      float                                   max_line_length = -1.0f)
{
    using AABBScalar = typename AABBTreeLines::LinesDistancer<L>::Scalar;
    if (input_points.empty())
        return {};
    float              boundary_offset = PREV_LAYER_BOUNDARY_OFFSET ? 0.5 * flow_width : 0.0f;
    CurvatureEstimator cestim;
    auto maybe_unscale = [](const P &p) { return SCALED_INPUT ? unscaled(p) : p.template cast<double>(); };
    float check_min_line_len = 25 * flow_width;
    float min_bridge_len = 10 * flow_width;

    std::vector<ExtendedPoint> points;
    points.reserve(input_points.size() * (ADD_INTERSECTIONS ? 1.5 : 1));

    {
        ExtendedPoint start_point{maybe_unscale(input_points.front())};
        auto [distance, nearest_line, x]    = unscaled_prev_layer.template distance_from_lines_extra<SIGNED_DISTANCE>(start_point.position.cast<AABBScalar>());
        start_point.distance                = distance + boundary_offset;
        start_point.nearest_prev_layer_line = nearest_line;
        points.push_back(start_point);
    }
    for (size_t i = 1; i < input_points.size(); i++) {
        ExtendedPoint next_point{maybe_unscale(input_points[i])};
        auto [distance, nearest_line, x]   = unscaled_prev_layer.template distance_from_lines_extra<SIGNED_DISTANCE>(next_point.position.cast<AABBScalar>());
        next_point.distance                = distance + boundary_offset;
        next_point.nearest_prev_layer_line = nearest_line;

        if (ADD_INTERSECTIONS)
        {
            if (((points.back().distance > boundary_offset + EPSILON) != (next_point.distance > boundary_offset + EPSILON)))
            {
                const ExtendedPoint& prev_point = points.back();
                auto intersections = unscaled_prev_layer.template intersections_with_line<true>(L{ prev_point.position.cast<AABBScalar>(), next_point.position.cast<AABBScalar>() });
                for (const auto& intersection : intersections) {
                    points.emplace_back(intersection.first.template cast<double>(), boundary_offset, intersection.second);
                }
            }
            else
            {
                const ExtendedPoint& prev_point = points.back();
                if ((prev_point.position - next_point.position).norm() > check_min_line_len)
                {
                    auto intersections = unscaled_prev_layer.template intersections_with_line<true>(L{ prev_point.position.cast<AABBScalar>(), next_point.position.cast<AABBScalar>() });
                    for (int i = 0; i < intersections.size(); i += 2)
                    {
                        if (i + 1 > intersections.size() - 1)
                            break;
                        const auto& intersection_0 = intersections[i];
                        const auto& intersection_1 = intersections[i + 1];
                        if ((intersection_0.first - intersection_1.first).norm() > min_bridge_len)
                        {
                            points.emplace_back(intersection_0.first.template cast<double>(), boundary_offset, intersection_0.second);
                            points.emplace_back(intersection_1.first.template cast<double>(), boundary_offset, intersection_1.second);
                        }
                    }
                }
            }
        }
        points.push_back(next_point);
    }

    if (PREV_LAYER_BOUNDARY_OFFSET && ADD_INTERSECTIONS) {
        std::vector<ExtendedPoint> new_points;
        new_points.reserve(points.size()*2);
        new_points.push_back(points.front());
        for (int point_idx = 0; point_idx < int(points.size()) - 1; ++point_idx) {
            const ExtendedPoint &curr = points[point_idx];
            const ExtendedPoint &next = points[point_idx + 1];

            if ((curr.distance > 0 && curr.distance < boundary_offset + 2.0f) ||
                (next.distance > 0 && next.distance < boundary_offset + 2.0f)) {
                double line_len = (next.position - curr.position).norm();
                if (line_len > 4.0f) {
                    double a0 = std::clamp((curr.distance + 2 * boundary_offset) / line_len, 0.0, 1.0);
                    double a1 = std::clamp(1.0f - (next.distance + 2 * boundary_offset) / line_len, 0.0, 1.0);
                    double t0 = std::min(a0, a1);
                    double t1 = std::max(a0, a1);

                    if (t0 < 1.0) {
                        auto p0                         = curr.position + t0 * (next.position - curr.position);
                        auto [p0_dist, p0_near_l, p0_x] = unscaled_prev_layer.template distance_from_lines_extra<SIGNED_DISTANCE>(p0.cast<AABBScalar>());
                        new_points.push_back(ExtendedPoint{p0, float(p0_dist + boundary_offset), p0_near_l});
                    }
                    if (t1 > 0.0) {
                        auto p1                         = curr.position + t1 * (next.position - curr.position);
                        auto [p1_dist, p1_near_l, p1_x] = unscaled_prev_layer.template distance_from_lines_extra<SIGNED_DISTANCE>(p1.cast<AABBScalar>());
                        new_points.push_back(ExtendedPoint{p1, float(p1_dist + boundary_offset), p1_near_l});
                    }
                }
            }
            new_points.push_back(next);
        }
        points = new_points;
    }

    if (max_line_length > 0) {
        std::vector<ExtendedPoint> new_points;
        new_points.reserve(points.size()*2);
        {
            for (size_t i = 0; i + 1 < points.size(); i++) {
                const ExtendedPoint &curr = points[i];
                const ExtendedPoint &next = points[i + 1];
                new_points.push_back(curr);
                double len             = (next.position - curr.position).squaredNorm();
                double t               = sqrt((max_line_length * max_line_length) / len);
                size_t new_point_count = 1.0 / t;
                for (size_t j = 1; j < new_point_count + 1; j++) {
                    Vec2d pos  = curr.position * (1.0 - j * t) + next.position * (j * t);
                    auto [p_dist, p_near_l,
                          p_x] = unscaled_prev_layer.template distance_from_lines_extra<SIGNED_DISTANCE>(pos.cast<AABBScalar>());
                    new_points.push_back(ExtendedPoint{pos, float(p_dist + boundary_offset), p_near_l});
                }
            }
            new_points.push_back(points.back());
        }
        points = new_points;
    }

    for (int point_idx = 0; point_idx < int(points.size()); ++point_idx) {
        ExtendedPoint &a    = points[point_idx];
        ExtendedPoint &prev = points[point_idx > 0 ? point_idx - 1 : point_idx];

        int prev_point_idx = point_idx;
        while (prev_point_idx > 0) {
            prev_point_idx--;
            if ((a.position - points[prev_point_idx].position).squaredNorm() > EPSILON) { break; }
        }

        int next_point_index = point_idx;
        while (next_point_index < int(points.size()) - 1) {
            next_point_index++;
            if ((a.position - points[next_point_index].position).squaredNorm() > EPSILON) { break; }
        }

        if (prev_point_idx != point_idx && next_point_index != point_idx) {
            float distance = (prev.position - a.position).norm();
            float alfa     = angle(a.position - points[prev_point_idx].position, points[next_point_index].position - a.position);
            cestim.add_point(distance, alfa);
        }

        a.curvature = cestim.get_curvature();
    }

    return points;
}

struct ProcessedPoint
{
    Point p;
    float speed = 1.0f;
    float overlap = 1.0f;
};

class ExtrusionQualityEstimator
{
    //std::unordered_map<const PrintObject *, AABBTreeLines::LinesDistancer<Linef>> prev_layer_boundaries;
    //std::unordered_map<const PrintObject *, AABBTreeLines::LinesDistancer<Linef>> next_layer_boundaries;
    //const PrintObject                                                            *current_object;
public:
    std::unordered_map<int, AABBTreeLines::LinesDistancer<Linef>> prev_layer_boundaries;
    std::unordered_map<int,AABBTreeLines::LinesDistancer<Linef>> next_layer_boundaries;
    int current_object;

public:
    //void set_current_object(const PrintObject *object) { current_object = object; }

    //void prepare_for_new_layer(const PrintObject * obj, const Layer *layer)
    //{
    //    if (layer == nullptr) return;
    //    const PrintObject *object = obj;
    //    prev_layer_boundaries[object] = next_layer_boundaries[object];
    //    next_layer_boundaries[object] = AABBTreeLines::LinesDistancer<Linef>{to_unscaled_linesf(layer->lslices)};
    //}

    void set_current_object(int object) { current_object = object; }

    void prepare_for_new_layer(const int layer, const ExPolygons& src)
    {
        if (src.empty()) return;
        //prev_layer_boundaries[layer] = next_layer_boundaries[layer];
        //next_layer_boundaries[layer] = AABBTreeLines::LinesDistancer<Linef>{ to_unscaled_linesf(src) };
        prev_layer_boundaries[layer] = AABBTreeLines::LinesDistancer<Linef>{ to_unscaled_linesf(src) };
    }


    std::vector<ProcessedPoint> estimate_extrusion_quality(const Polygon                &path,
                                                           const std::vector<double>     &overlaps,
                                                           const std::vector<float>        &speeds,
                                                           float                               width,
                                                           float                               ext_perimeter_speed,
                                                           float                               original_speed)
    {
        size_t                               speed_sections_count = std::min(overlaps.size(), speeds.size());
        std::vector<std::pair<float, float>> speed_sections;
        for (size_t i = 0; i < speed_sections_count; i++) {
            float distance = width * (1.0 - (overlaps.at(i) / 100.0));
            float speed    = speeds.at(i) > 1 ? (ext_perimeter_speed * speeds.at(i) / 100.0) : speeds.at(i);
            speed_sections.push_back({distance, speed});
        }
        std::sort(speed_sections.begin(), speed_sections.end(),
                  [](const std::pair<float, float> &a, const std::pair<float, float> &b) { 
                    if (a.first == b.first) {
                        return a.second > b.second;
                    }
                    return a.first < b.first; });

        std::pair<float, float> last_section{INFINITY, 0};
        for (auto &section : speed_sections) {
            if (section.first == last_section.first) {
                section.second = last_section.second;
            } else {
                last_section = section;
            }
        }

        std::vector<ExtendedPoint> extended_points =
            estimate_points_properties<true, true, true, true>(path.points, prev_layer_boundaries[current_object], width);
        const auto width_inv = 1.0f / width;
        std::vector<ProcessedPoint> processed_points;
        processed_points.reserve(extended_points.size());
        for (size_t i = 0; i < extended_points.size(); i++) {
            const ExtendedPoint &curr = extended_points[i];
            const ExtendedPoint &next = extended_points[i + 1 < extended_points.size() ? i + 1 : i];

            auto calculate_speed = [&speed_sections, &original_speed](float distance) {
                float final_speed;
                if (distance <= speed_sections.front().first) {
                    final_speed = original_speed;
                } else if (distance >= speed_sections.back().first) {
                    final_speed = speed_sections.back().second;
                } else {
                    size_t section_idx = 0;
                    while (distance > speed_sections[section_idx + 1].first) {
                        section_idx++;
                    }
                    float t = (distance - speed_sections[section_idx].first) /
                              (speed_sections[section_idx + 1].first - speed_sections[section_idx].first);
                    t           = std::clamp(t, 0.0f, 1.0f);
                    final_speed = (1.0f - t) * speed_sections[section_idx].second + t * speed_sections[section_idx + 1].second;
                }
                return final_speed;
            };

            float extrusion_speed = std::min(calculate_speed(curr.distance), calculate_speed(next.distance));
            float overlap = std::min(1 - curr.distance * width_inv, 1 - next.distance * width_inv);

            processed_points.push_back({ scaled(curr.position), extrusion_speed, overlap });
        }
        return processed_points;
    }
};

} // namespace Slic3r

#endif // slic3r_ExtrusionProcessor_QUALITY_hpp_


