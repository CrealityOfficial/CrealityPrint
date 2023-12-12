#include "utils/generateVaryingXYDisallowedArea.h"
#include  <assert.h>
#include <boost/polygon/voronoi.hpp>
#include <cassert>
#include <iostream>
#include "utils/Coord_t.h"

#include "communication/sliceDataStorage.h"
#include "skeletal/SkeletalTrapezoidation.h"

#include "tools/BoostInterface.h"
#include <boost/polygon/voronoi.hpp>
#include <cassert>
#include <iostream>

namespace cura52
{
    Polygons offset(Polygons& poly, const std::vector<coord_t>& offset_dists)
    {
        // we need as many offset-dists as points
        assert(poly.pointCount() == offset_dists.size());

        Polygons ret;
        int i = 0;
        //for (auto& poly_line : poly.paths | ranges::views::filter([](const auto& path) { return !path.empty(); }))
        for (auto& poly_line : poly.paths )
        {
            if (poly_line.empty())
                continue;
            std::vector<ClipperLib::IntPoint> ret_poly_line;

            auto prev_p = poly_line.back();
            auto prev_dist = offset_dists[i + poly_line.size() - 1];

            for (const auto& p : poly_line)
            {
                auto offset_dist = offset_dists[i];

                auto vec_dir = prev_p - p;

                constexpr coord_t min_vec_len = 10;
                if (vSize2(vec_dir) > min_vec_len * min_vec_len)
                {
                    auto offset_p1 = turn90CCW(normal(vec_dir, prev_dist));
                    auto offset_p2 = turn90CCW(normal(vec_dir, offset_dist));

                    ret_poly_line.emplace_back(prev_p + offset_p1);
                    ret_poly_line.emplace_back(p + offset_p2);
                }

                prev_p = p;
                prev_dist = offset_dist;
                i++;
            }

            ret.add(ret_poly_line);
        }

        ClipperLib::SimplifyPolygons(ret.paths, ClipperLib::PolyFillType::pftPositive);

        return ret;
    }

    double lerp( double a, double b, double t)
    {
        return( a + t*(b - a));
    }


    Polygons  generateVaryingXYDisallowedArea(const SliceMeshStorage& storage, const Settings& infill_settings, const LayerIndex layer_idx)
    {
        const auto& mesh_group_settings = storage.settings;
        coord_t max_resolution = mesh_group_settings.get<coord_t>("meshfix_maximum_resolution");
        coord_t max_deviation = mesh_group_settings.get<coord_t>("meshfix_maximum_deviation");
        coord_t max_area_deviation = mesh_group_settings.get<coord_t>("meshfix_maximum_extrusion_area_deviation");

        const Simplify simplify(max_resolution, max_deviation, max_area_deviation);

        const auto layer_thickness = mesh_group_settings.get<coord_t>("layer_height");
        const auto support_distance_top = static_cast<double>(mesh_group_settings.get<coord_t>("support_top_distance"));
        const auto support_distance_bot = static_cast<double>(mesh_group_settings.get<coord_t>("support_bottom_distance"));
        const auto overhang_angle = mesh_group_settings.get<AngleRadians>("support_angle");
        const auto xy_distance = static_cast<double>(mesh_group_settings.get<coord_t>("support_xy_distance"));
        const auto xy_distance_overhang = infill_settings.get<coord_t>("support_xy_distance_overhang");

        constexpr coord_t snap_radius = 10;
        constexpr coord_t close_dist = snap_radius + 5; // needs to be larger than the snap radius!
        constexpr coord_t search_radius = 0;

        auto layer_current = simplify.polygon(storage.layers[layer_idx].getOutlines()
            .offset(-close_dist)
            .offset(close_dist));

        // sparse grid for storing the offset distances at each point. For each point there can be multiple offset
        // values as multiple may be calculated when multiple layers are used for z-smoothing of the offsets.
        // The average of all offset dists is taken for the used varying offset. To account for this the commutative
        // offset, and the number of offsets $n$ are stored simultaneously. The final offset used is then commutative
        // equal to commutative_offset / n.
        using point_pair_t = std::pair<size_t, double>;
        using grid_t = SparsePointGridInclusive<point_pair_t>;
        grid_t offset_dist_at_point{ snap_radius };

        // Collection of the various areas we used to calculate the areas for. This is a combination
        //  - the support distance (this is the support top distance for overhang areas, and support
        //    bottom thickness for sloped areas)
        //  - of the delta z between the current layer and layer below (this can vary between the areas
        //    when we use multiple layers for z-smoothing)
        //  - the polygon delta; the xy-distance is calculated separately for overhang and sloped areas.
        //    here either the slope or overhang area is stored
        std::vector<std::tuple<double, double, Polygons>> z_distances_layer_deltas;

        constexpr LayerIndex layer_index_offset{ 1 };

        const LayerIndex layer_idx_below{ std::max(layer_idx - layer_index_offset, LayerIndex { 0 }) };
        if (layer_idx_below != layer_idx)
        {
            auto layer_below = simplify.polygon(storage.layers[layer_idx_below].getOutlines()
                .offset(-close_dist)
                .offset(close_dist));

            z_distances_layer_deltas.emplace_back(
                support_distance_top,
                static_cast<double>(layer_index_offset * layer_thickness),
                layer_current.difference(layer_below)
            );

            z_distances_layer_deltas.emplace_back(
                support_distance_bot,
                static_cast<double>(layer_index_offset * layer_thickness),
                layer_below.difference(layer_current)
            );
        }

        const LayerIndex layer_idx_above{ std::min(layer_idx + layer_index_offset, LayerIndex(static_cast<int>(storage.layers.size()) - 1)) };
        if (layer_idx_above != layer_idx)
        {
            auto layer_above = simplify.polygon(storage.layers[layer_idx_below].getOutlines()
                .offset(-close_dist)
                .offset(close_dist));

            z_distances_layer_deltas.emplace_back(
                support_distance_bot,
                static_cast<double>(layer_index_offset * layer_thickness),
                layer_current.difference(layer_above)
            );

            z_distances_layer_deltas.emplace_back(
                support_distance_top,
                static_cast<double>(layer_index_offset * layer_thickness),
                layer_above.difference(layer_current)
            );
        }

        for (auto& [support_distance, delta_z, layer_delta_] : z_distances_layer_deltas)
        {
            const auto xy_distance_natural = support_distance * std::tan(overhang_angle);

            // perform a close operation to remove narrow areas; these cannot easily be turned into a voronoi diagram
            // we might "miss" some vertices in the resulting git map, this is not a problem
            auto layer_delta = layer_delta_.offset(-close_dist).offset(close_dist);

            if (layer_delta.empty())
            {
                continue;
            }

            // grid for storing the "slope" (wall overhang area at that specific point in the polygon)
            grid_t slope_at_point{ snap_radius };

            // construct a voronoi diagram. The slope is calculated based
            // on the edge length from the boundary to the center edge(s)
            std::vector<Segment> segments;

            int poly_idx = 0;
            for (auto poly : layer_delta)
            {
                int point_idx = 0;
                for (auto _p : poly)
                {
                    segments.emplace_back(&layer_delta, poly_idx, point_idx);

                     ++point_idx;
                }
                ++poly_idx;
            }
            segments.begin();

            boost::polygon::voronoi_diagram<double> vonoroi_diagram;
            //Ring const inputs[] = {
            //Ring { {0,0}, {8, 3}, {10, 7}, {8, 9}, {0, 6}, }, // {0, 0},
            //Ring { {0,0}, {8, 3}, {8, 5}, {10, 7}, {8, 9}, {0, 6}, } // {0, 0},
            //};
            //for (auto input : inputs) {
            boost::polygon::construct_voronoi(segments.begin(), segments.end(), &vonoroi_diagram);
           // }


            for (const auto& edge : vonoroi_diagram.edges())
            {
                if (edge.is_infinite())
                {
                    continue;
                }

                auto p0 = VoronoiUtils::p(edge.vertex0());
                auto p1 = VoronoiUtils::p(edge.vertex1());

                // skip edges that move "outside" the polygon;
                // these are st edges that are inside polygon-holes
                if (!layer_delta.inside(p0) && !layer_delta.inside(p1))
                {
                    continue;
                }

                auto dist_to_center_edge = static_cast<double>(cura52::vSize(p0 - p1));

                if (dist_to_center_edge < snap_radius)
                {
                    continue;
                }

                // p0 to p1 is the distance to the center between the two polygons; two times
                // this distance is (approximately) the distance between the boundaries
                auto dist_to_boundary = 2. * dist_to_center_edge;
                auto slope = dist_to_boundary / delta_z;

                auto nearby_vals = slope_at_point.getNearbyVals(p0, search_radius);

                //auto n = ranges::accumulate(nearby_vals | views::get(&point_pair_t::first), 0);
                //auto cumulative_slope = ranges::accumulate(nearby_vals | views::get(&point_pair_t::second), 0.);

                //n += 1;
                //cumulative_slope += slope;
                size_t n_alter = 0;
                double cumulative_slope_alter = 0;
                for (int i = 0; i < nearby_vals.size(); i++)
                {
                    n_alter += nearby_vals.at(i).first;
                    cumulative_slope_alter += nearby_vals.at(i).second;
                }

                n_alter += 1;
                cumulative_slope_alter += slope;

                //update cumulative_slope in sparse grid
                //slope_at_point.insert(p0, { n, cumulative_slope });
                slope_at_point.insert(p0, { n_alter, cumulative_slope_alter });
            }

            for (const auto& poly : layer_current)
            {


                for (const auto& point : poly)
                {  
                    size_t n_alter = 0;
                    double cumulative_slope_alter = 0;
                    auto nearby_vals = slope_at_point.getNearbyVals(point, search_radius);
                    //auto n = ranges::accumulate(nearby_vals | views::get(&point_pair_t::first), 0);
                    //auto cumulative_slope = ranges::accumulate(nearby_vals | views::get(&point_pair_t::second), 0.);
                    for (int i = 0; i < nearby_vals.size(); i++)
                    {
                        n_alter += nearby_vals.at(0).first;
                        cumulative_slope_alter += nearby_vals.at(0).second;
                    }

                    if (n_alter != 0)
                    {
                        auto slope = cumulative_slope_alter / static_cast<double>(n_alter);
                        double wall_angle = std::atan(slope);
                        auto ratio = std::min(wall_angle / overhang_angle, 1.);

                        auto xy_distance_varying = lerp(xy_distance, xy_distance_natural, ratio);

                        auto nearby_vals_offset_dist = offset_dist_at_point.getNearbyVals(point, search_radius);

                        // update and insert cumulative varying xy distance in one go
                        //offset_dist_at_point.insert(point, {
                        //                                   ranges::accumulate(nearby_vals_offset_dist | views::get(&point_pair_t::first), 0) + 1,
                        //                                   ranges::accumulate(nearby_vals_offset_dist | views::get(&point_pair_t::second), 0.) + xy_distance_varying
                        //    });
                        size_t re_0 = 0;
                        double re_1 = 0;
                        for (int i = 0; i < nearby_vals_offset_dist.size(); i++)
                        {   
                            re_0 += nearby_vals_offset_dist.at(i).first;
                            re_1 += nearby_vals_offset_dist.at(i).second;
                        }

                        offset_dist_at_point.insert(point, {
                                                           re_0 + 1,
                                                           re_1+ xy_distance_varying
                            });
                    }
                }
            }
        }

        std::vector<coord_t> varying_offsets;
        for (const auto& poly : layer_current)
        {
            for (const auto& point : poly)
            {
                auto nearby_vals = offset_dist_at_point.getNearbyVals(point, search_radius);

                //auto n = ranges::accumulate(nearby_vals | views::get(&point_pair_t::first), 0);
                //auto cumulative_offset_dist = ranges::accumulate(nearby_vals | views::get(&point_pair_t::second), 0.);
                size_t n_alter = 0;
                double cumulative_offset_dist_alter = 0;
                double offset_dist{};
                for (int i = 0; i < nearby_vals.size(); i++)
                {
                    n_alter += nearby_vals.at(i).first;
                    cumulative_offset_dist_alter += nearby_vals.at(i).second;
                }
                if (n_alter == 0)
                {
                    // if there are no offset dists generated for a vertex $p$ this must mean that vertex $p$ was not
                    // present in any of the delta areas. This can only happen if the areas for the current layer and
                    // the layer(s) below are perfectly aligned at vertex $p$; the walls at vertex $p$ are vertical.
                    // As the wall is vertical the xy_distance is taken at vertex $p$.
                    offset_dist = xy_distance;
                }
                else
                {
                    auto avg_offset_dist = cumulative_offset_dist_alter / static_cast<double>(n_alter);
                    offset_dist = avg_offset_dist;
                }

                varying_offsets.push_back(static_cast<coord_t>(offset_dist));
            }
        }

        const auto smooth_dist = xy_distance / 2.0;
        Polygons varying_xy_disallowed_areas2 =  offset(layer_current, varying_offsets);
        Polygons varying_xy_disallowed_areas = varying_xy_disallowed_areas2
            // offset using the varying offset distances we calculated previously
         
            // close operation to smooth the x/y disallowed area boundary. With varying xy distances we see some jumps in the boundary.
            // As the x/y disallowed areas "cut in" to support the xy-disallowed area may propagate through the support area. If the
            // x/y disallowed area is not smoothed boost has trouble generating a voronoi diagram.
            .offset(smooth_dist).offset(-smooth_dist);
        //scripta::log("support_varying_xy_disallowed_areas", varying_xy_disallowed_areas, SectionType::SUPPORT, layer_idx);
        return varying_xy_disallowed_areas;
    }
















}



