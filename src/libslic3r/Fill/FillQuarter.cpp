#include "../ClipperUtils.hpp"
#include "../ExPolygon.hpp"
#include "../Surface.hpp"
#include "../VariableWidth.hpp"
#include "Arachne/WallToolPaths.hpp"

#include "FillQuarter.hpp"
#include <cstddef>
#include <libslic3r/ShortestPath.hpp>
#include "Fill/Quarter/NoZigZagConnectorProcessor.h"

namespace Slic3r {
   

    void FillQuarter::setOrigin(const InfillPattern& _pattern, const Point& _infill_origin, const Point& _offset, const coord_t& _z, const coord_t& _line_distance, const coord_t& _infill_line_width)
    {
        infill_origin = _infill_origin + _offset;
        offset = _offset;
        line_distance = _line_distance;
        infill_line_width = _infill_line_width;
        z = _z;
        m_pattern = _pattern;
    }

    static constexpr double one_over_sqrt_2 = 0.7071067811865475244008443621048490392848359376884740;


    void applyMatrix(Polygons& outline,const PointMatrix& matrix)
    {
        for (Polygon& poly : outline)
        {
            for (auto&p : poly.points)
            {
                p = matrix.apply(p);
            }
        }
    }

    static inline int computeScanSegmentIdx(int x, int line_width)
    {
        if (x < 0)
        {
            return (x + 1) / line_width - 1;
            // - 1 because -1 belongs to scansegment -1
            // + 1 because -line_width belongs to scansegment -1
        }
        return x / line_width;
    }

    float vSize2f(const Point& p0)
    {
        return static_cast<float>(p0.x()) * static_cast<float>(p0.x()) + static_cast<float>(p0.y()) * static_cast<float>(p0.y());
    }

    Point closestPointTo(Polygon& poly,Point p)
    {
        Point ret = p;
        float bestDist = FLT_MAX;
        for (unsigned int n = 0; n < poly.points.size(); n++)
        {
            float dist = vSize2f(p - poly.points[n]);
            if (dist < bestDist)
            {
                ret = poly.points[n];
                bestDist = dist;
            }
        }
        return ret;
    }

    void FillQuarter::addLineInfill(Polygons& result, const PointMatrix& rotation_matrix, const int scanline_min_idx, const int line_distance, const BoundingBox boundary, std::vector<std::vector<coord_t>>& cut_list, coord_t shift)
    {
        //assert(!connect_lines && "connectLines() should add the infill lines, not addLineInfill");

        unsigned int scanline_idx = 0;
        for (coord_t x = scanline_min_idx * line_distance + shift; x < boundary.max.x(); x += line_distance)
        {
            if (scanline_idx >= cut_list.size())
            {
                break;
            }
            std::vector<coord_t>& crossings = cut_list[scanline_idx];
            std::sort(crossings.begin(), crossings.end()); // sort by increasing Y coordinates
            for (unsigned int crossing_idx = 0; crossing_idx + 1 < crossings.size(); crossing_idx += 2)
            {
                if (crossings[crossing_idx + 1] - crossings[crossing_idx] < infill_line_width / 5)
                { // segment is too short to create infill
                    continue;
                }
                result.push_back(Polygon{ rotation_matrix.unapply(Point(x, crossings[crossing_idx])), rotation_matrix.unapply(Point(x, crossings[crossing_idx + 1])) });
                //result.addLine(rotation_matrix.unapply(Point(x, crossings[crossing_idx])), rotation_matrix.unapply(Point(x, crossings[crossing_idx + 1])));
            }
            scanline_idx += 1;
        }
    }

    void FillQuarter::generateLinearBasedInfill(Polygons& result, const int line_distance, const PointMatrix& rotation_matrix, ZigzagConnectorProcessor& zigzag_connector_processor, const bool connected_zigzags, coord_t extra_shift)
    {
        if (line_distance == 0 || inner_contour.empty()) // No infill to generate (0% density) or no area to generate it in.
        {
            return;
        }

        Polygons outline = inner_contour; // Make a copy. We'll be rotating this outline to make intersections always horizontal, for better performance.
        applyMatrix(outline,rotation_matrix);

        coord_t shift = extra_shift; /*+ this->shift;*/
        if (shift < 0)
        {
            shift = line_distance - (-shift) % line_distance;
        }
        else
        {
            shift = shift % line_distance;
        }

        BoundingBox boundary= get_extents(outline);
        //AABB boundary(outline);

        int scanline_min_idx = computeScanSegmentIdx(boundary.min.x() - shift, line_distance);
        int line_count = computeScanSegmentIdx(boundary.max.x() - shift, line_distance) + 1 - scanline_min_idx;

        std::vector<std::vector<coord_t>> cut_list(line_count); // mapping from scanline to all intersections with polygon segments

        // When we find crossings, keep track of which crossing belongs to which scanline and to which polygon line segment.
        // Then we can later join two crossings together to form lines and still know what polygon line segments that infill line connected to.
        struct Crossing
        {
            Crossing(Point coordinate, size_t polygon_index, size_t vertex_index) : coordinate(coordinate), polygon_index(polygon_index), vertex_index(vertex_index) {};
            Point coordinate;
            size_t polygon_index;
            size_t vertex_index;
            bool operator<(const Crossing& other) const // Crossings will be ordered by their Y coordinate so that they get ordered along the scanline.
            {
                return coordinate.y() < other.coordinate.y();
            }
        };
        std::vector<std::vector<Crossing>> crossings_per_scanline; // For each scanline, a list of crossings.
        const int min_scanline_index = computeScanSegmentIdx(boundary.min.x() - shift, line_distance) + 1;
        const int max_scanline_index = computeScanSegmentIdx(boundary.max.x() - shift, line_distance) + 1;
        crossings_per_scanline.resize(max_scanline_index - min_scanline_index);
        //if (connect_lines)
        //{
        //    crossings_on_line.resize(outline.size()); // One for each polygon.
        //}

        auto getStartPointAndIdx = [](Polygon& poly, const Point prePoint, Point& p0)
        {
            size_t startIdx = 0;
            if (prePoint != Point())
            {
                //p0 = poly.closestPointTo(prePoint);
                p0 = closestPointTo(poly, prePoint);
                for (size_t point_idx = 0; point_idx < poly.size(); point_idx++)
                {
                    if (poly[point_idx] == p0)
                        startIdx = (point_idx + 1) % poly.size();
                }
            }
            return startIdx;
        };

        for (size_t poly_idx = 0; poly_idx < outline.size(); poly_idx++)
        {
            Polygon poly = outline[poly_idx];
            //if (connect_lines)
            //{
            //    crossings_on_line[poly_idx].resize(poly.size()); // One for each line in this polygon.
            //}
            Point p0 = poly.back();
            size_t startIdx = getStartPointAndIdx(poly, rotation_matrix.apply(current_position), p0);
            //zigzag_connector_processor.registerVertex(p0); // always adds the first point to ZigzagConnectorProcessorEndPieces::first_zigzag_connector when using a zigzag infill type

            for (size_t i = startIdx; i < poly.size() + startIdx; i++)
            {
                size_t point_idx = i % poly.size();
                Point p1 = poly[point_idx];
                if (p1.x() == p0.x())
                {
                    zigzag_connector_processor.registerVertex(p1);
                    // TODO: how to make sure it always adds the shortest line? (in order to prevent overlap with the zigzag connectors)
                    // note: this is already a problem for normal infill, but hasn't really bothered anyone so far.
                    p0 = p1;
                    continue;
                }

                int scanline_idx0;
                int scanline_idx1;
                // this way of handling the indices takes care of the case where a boundary line segment ends exactly on a scanline:
                // in case the next segment moves back from that scanline either 2 or 0 scanline-boundary intersections are created
                // otherwise only 1 will be created, counting as an actual intersection
                int direction = 1;
                if (p0.x() < p1.x())
                {
                    scanline_idx0 = computeScanSegmentIdx(p0.x() - shift, line_distance) + 1; // + 1 cause we don't cross the scanline of the first scan segment
                    scanline_idx1 = computeScanSegmentIdx(p1.x() - shift, line_distance); // -1 cause the vertex point is handled in the next segment (or not in the case which looks like >)
                }
                else
                {
                    direction = -1;
                    scanline_idx0 = computeScanSegmentIdx(p0.x() - shift, line_distance); // -1 cause the vertex point is handled in the previous segment (or not in the case which looks like >)
                    scanline_idx1 = computeScanSegmentIdx(p1.x() - shift, line_distance) + 1; // + 1 cause we don't cross the scanline of the first scan segment
                }

                for (int scanline_idx = scanline_idx0; scanline_idx != scanline_idx1 + direction; scanline_idx += direction)
                {
                    int x = scanline_idx * line_distance + shift;
                    int y = p1.y() + (p0.y() - p1.y()) * (x - p1.x()) / (p0.x() - p1.x());
                    assert(scanline_idx - scanline_min_idx >= 0 && scanline_idx - scanline_min_idx < int(cut_list.size()) && "reading infill cutlist index out of bounds!");
                    cut_list[scanline_idx - scanline_min_idx].push_back(y);
                    Point scanline_linesegment_intersection(x, y);
                    zigzag_connector_processor.registerScanlineSegmentIntersection(scanline_linesegment_intersection, scanline_idx);
                    crossings_per_scanline[scanline_idx - min_scanline_index].emplace_back(scanline_linesegment_intersection, poly_idx, point_idx);
                }
                zigzag_connector_processor.registerVertex(p1);
                p0 = p1;
            }
            zigzag_connector_processor.registerPolyFinished();
        }

        //if (connect_lines)
        //{
        //    //// Gather all crossings per scanline and find out which crossings belong together, then store them in crossings_on_line.
        //    //for (int scanline_index = min_scanline_index; scanline_index < max_scanline_index; scanline_index++)
        //    //{
        //    //    auto& crossings = crossings_per_scanline[scanline_index - min_scanline_index];
        //    //    // Sorts them by Y coordinate.
        //    //    std::sort(crossings.begin(), crossings.end());
        //    //    // Combine each 2 subsequent crossings together.
        //    //    for (long crossing_index = 0; crossing_index < static_cast<long>(crossings.size()) - 1; crossing_index += 2)
        //    //    {
        //    //        const Crossing& first = crossings[crossing_index];
        //    //        const Crossing& second = crossings[crossing_index + 1];
        //    //        // Avoid creating zero length crossing lines
        //    //        const Point unrotated_first = rotation_matrix.unapply(first.coordinate);
        //    //        const Point unrotated_second = rotation_matrix.unapply(second.coordinate);
        //    //        if (unrotated_first == unrotated_second)
        //    //        {
        //    //            continue;
        //    //        }
        //    //        InfillLineSegment* new_segment = new InfillLineSegment(unrotated_first, first.vertex_index, first.polygon_index, unrotated_second, second.vertex_index, second.polygon_index);
        //    //        // Put the same line segment in the data structure twice: Once for each of the polygon line segment that it crosses.
        //    //        crossings_on_line[first.polygon_index][first.vertex_index].push_back(new_segment);
        //    //        crossings_on_line[second.polygon_index][second.vertex_index].push_back(new_segment);
        //    //    }
        //    //}
        //}
        //else
        {
            if (cut_list.size() == 0)
            {
                return;
            }
            if (connected_zigzags && cut_list.size() == 1 && cut_list[0].size() <= 2)
            {
                return; // don't add connection if boundary already contains whole outline!
            }

            // We have to create our own lines when they are not created by the method connectLines.
            addLineInfill(result, rotation_matrix, scanline_min_idx, line_distance, boundary, cut_list, shift);
        }
    }

    coord_t FillQuarter::getShiftOffsetFromInfillOriginAndRotation(const double& infill_rotation)
    {
        if (infill_origin.x() != 0 || infill_origin.y() != 0)
        {
            const double rotation_rads = infill_rotation * M_PI / 180;
            return infill_origin.x() * std::cos(rotation_rads) - infill_origin.y() * std::sin(rotation_rads);
        }
        return 0;
    }

    void FillQuarter::generateLineInfill(Polygons& result, int line_distance, const double& infill_rotation, coord_t shift)
    {
        shift += getShiftOffsetFromInfillOriginAndRotation(infill_rotation);
        PointMatrix rotation_matrix(infill_rotation);
        NoZigZagConnectorProcessor lines_processor(rotation_matrix, result);
        bool connected_zigzags = false;
        generateLinearBasedInfill(result, line_distance, rotation_matrix, lines_processor, connected_zigzags, shift);
    }

    void FillQuarter::generateHalfTetrahedralInfill(float pattern_z_shift, int angle_shift, Polygons& result)
    {
        const coord_t period = line_distance * 2;
        coord_t shift = coord_t(one_over_sqrt_2 * (z + pattern_z_shift * period * 2)) % period;
        shift = std::min(shift, period - shift); // symmetry due to the fact that we are applying the shift in both directions
        shift = std::min(shift, period / 2 - infill_line_width / 2); // don't put lines too close to each other
        shift = std::max(shift, infill_line_width / 2); // don't put lines too close to each other
        generateLineInfill(result, period, fill_angle + angle_shift, shift);
        generateLineInfill(result, period, fill_angle + angle_shift, -shift);
    }


void FillQuarter::_fill_surface_single(
    const FillParams                &params, 
    unsigned int                     thickness_layers,
    const std::pair<float, Point>   &direction, 
    ExPolygon                        expolygon,
    Polylines                       &polylines_out)
{
    Slic3r::Polygons result;

    outer_contour.push_back(expolygon.contour);
    Polygon poly;
    for (auto& p : expolygon.contour)
    {
        poly.points.push_back(Point(p.x()/1000.0f, p.y() / 1000.0f));
    }
    inner_contour.push_back(poly);

    try {
        if (m_pattern == ipquarter_cubic)
        {
            generateHalfTetrahedralInfill(0.0, 0, result);
            generateHalfTetrahedralInfill(0.5, 90, result);
        }
        else if (m_pattern == iptetrahedral)
        {
            generateHalfTetrahedralInfill(0.0, 0, result);
            generateHalfTetrahedralInfill(0.0, 90, result);
        }
    }
    catch (const std::exception& ex)
    {
        return;
    }

    Polylines result_pl;
    size_t n = (size_t)result.size();

    if(n > 0)
    {
        result_pl.resize(n);
        for (size_t i = 0; i < n; i++)
        {
            Polygon& poly = result[i];
            Polyline& pl = result_pl[i];
            for(const Point& p : poly.points)
            {
                Point pp = p - offset;
                pl.points.push_back(Point(pp.x() * 1000, pp.y() * 1000));
            }
        }
    }

    Slic3r::Polygons   loops = to_polygons(expolygon);
    polylines_out  = chain_polylines(intersection_pl(result_pl, loops));
}

void FillQuarter::_fill_surface_single(const FillParams& params,
    unsigned int                   thickness_layers,
    const std::pair<float, Point>& direction,
    ExPolygon                      expolygon,
    ThickPolylines& thick_polylines_out)
{

}

} // namespace Slic3r
