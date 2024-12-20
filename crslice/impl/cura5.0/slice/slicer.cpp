// Copyright (c) 2022 Ultimaker B.V.
// CuraEngine is released under the terms of the AGPLv3 or higher

#include <algorithm> // remove_if
#include <stdio.h>

#include "magic/AdaptiveLayerHeights.h"

#include "slicer.h"
#include "utils/SettingsWrapper.h"
#include "utils/SparsePointGridInclusive.h"
#include "trimesh2/Vec.h"

#include <fstream>
#include "communication/slicecontext.h"
#include "utils/paintdata.h"

namespace cura52
{

constexpr int largest_neglected_gap_first_phase = MM2INT(0.01); //!< distance between two line segments regarded as connected
constexpr int largest_neglected_gap_second_phase = MM2INT(0.02); //!< distance between two line segments regarded as connected
constexpr int max_stitch1 = MM2INT(1.0); //!< maximal distance stitched between open polylines to form polygons
constexpr int max_stitch2 = MM2INT(10.0); //!< maximal distance stitched between open polylines to form polygons

void SlicerLayer::makeBasicPolygonLoops(Polygons& open_polylines)
{
    for (size_t start_segment_idx = 0; start_segment_idx < segments.size(); start_segment_idx++)
    {
        if (! segments[start_segment_idx].addedToPolygon)
        {
            makeBasicPolygonLoop(open_polylines, start_segment_idx);
        }
    }
    // Clear the segmentList to save memory, it is no longer needed after this point.
    segments.clear();
}

void SlicerLayer::makeBasicPolygonLoop(Polygons& open_polylines, const size_t start_segment_idx)
{
    Polygon poly;
    poly.add(segments[start_segment_idx].start);

    for (int segment_idx = start_segment_idx; segment_idx != -1;)
    {
        SlicerSegment& segment = segments[segment_idx];
        poly.add(segment.end);
        segment.addedToPolygon = true;
        segment_idx = getNextSegmentIdx(segment, start_segment_idx);
        if (segment_idx == static_cast<int>(start_segment_idx))
        { // polyon is closed
            polygons.add(poly);
            return;
        }
    }
    // polygon couldn't be closed
    open_polylines.add(poly);
}

int SlicerLayer::tryFaceNextSegmentIdx(const SlicerSegment& segment, const int face_idx, const size_t start_segment_idx) const
{
    decltype(face_idx_to_segment_idx.begin()) it;
    auto it_end = face_idx_to_segment_idx.end();
    it = face_idx_to_segment_idx.find(face_idx);
    if (it != it_end)
    {
        const int segment_idx = (*it).second;
        Point p1 = segments[segment_idx].start;
        Point diff = segment.end - p1;
        if (shorterThen(diff, largest_neglected_gap_first_phase))
        {
            if (segment_idx == static_cast<int>(start_segment_idx))
            {
                return start_segment_idx;
            }
            if (segments[segment_idx].addedToPolygon)
            {
                return -1;
            }
            return segment_idx;
        }
    }

    return -1;
}

int SlicerLayer::getNextSegmentIdx(const SlicerSegment& segment, const size_t start_segment_idx) const
{
    int next_segment_idx = -1;

    const bool segment_ended_at_edge = segment.endVertex == nullptr;
    if (segment_ended_at_edge)
    {
        const int face_to_try = segment.endOtherFaceIdx;
        if (face_to_try == -1)
        {
            return -1;
        }
        return tryFaceNextSegmentIdx(segment, face_to_try, start_segment_idx);
    }
    else
    {
        // segment ended at vertex

        const std::vector<uint32_t>& faces_to_try = segment.endVertex->connected_faces;
        for (int face_to_try : faces_to_try)
        {
            const int result_segment_idx = tryFaceNextSegmentIdx(segment, face_to_try, start_segment_idx);
            if (result_segment_idx == static_cast<int>(start_segment_idx))
            {
                return start_segment_idx;
            }
            else if (result_segment_idx != -1)
            {
                // not immediately returned since we might still encounter the start_segment_idx
                next_segment_idx = result_segment_idx;
            }
        }
    }

    return next_segment_idx;
}

void SlicerLayer::connectOpenPolylines(Polygons& open_polylines)
{
    constexpr bool allow_reverse = false;
    // Search a bit fewer cells but at cost of covering more area.
    // Since acceptance area is small to start with, the extra is unlikely to hurt much.
    constexpr coord_t cell_size = largest_neglected_gap_first_phase * 2;
    connectOpenPolylinesImpl(open_polylines, largest_neglected_gap_second_phase, cell_size, allow_reverse);
}

void SlicerLayer::stitch(Polygons& open_polylines)
{
    bool allow_reverse = true;
	//先正向缝合，剩余的再参与反向缝合
	connectOpenPolylinesImpl(open_polylines, max_stitch1, max_stitch1, false);
#ifndef CUSTOM_KEY
    connectOpenPolylinesImpl(open_polylines, max_stitch2, max_stitch2, allow_reverse);
#endif
}

const SlicerLayer::Terminus SlicerLayer::Terminus::INVALID_TERMINUS{ ~static_cast<Index>(0U) };

bool SlicerLayer::PossibleStitch::operator<(const PossibleStitch& other) const
{
    // better if lower distance
    if (dist2 > other.dist2)
    {
        return true;
    }
    else if (dist2 < other.dist2)
    {
        return false;
    }

    // better if in order instead of reversed
    if (! in_order() && other.in_order())
    {
        return true;
    }

    // better if lower Terminus::Index for terminus_0
    // This just defines a more total order and isn't strictly necessary.
    if (terminus_0.asIndex() > other.terminus_0.asIndex())
    {
        return true;
    }
    else if (terminus_0.asIndex() < other.terminus_0.asIndex())
    {
        return false;
    }

    // better if lower Terminus::Index for terminus_1
    // This just defines a more total order and isn't strictly necessary.
    if (terminus_1.asIndex() > other.terminus_1.asIndex())
    {
        return true;
    }
    else if (terminus_1.asIndex() < other.terminus_1.asIndex())
    {
        return false;
    }

    // The stitches have equal goodness
    return false;
}

std::priority_queue<SlicerLayer::PossibleStitch> SlicerLayer::findPossibleStitches(const Polygons& open_polylines, coord_t max_dist, coord_t cell_size, bool allow_reverse) const
{
    std::priority_queue<PossibleStitch> stitch_queue;

    // maximum distance squared
    int64_t max_dist2 = max_dist * max_dist;

    // Represents a terminal point of a polyline in open_polylines.
    struct StitchGridVal
    {
        unsigned int polyline_idx;
        // Depending on the SparsePointGridInclusive, either the start point or the
        // end point of the polyline
        Point polyline_term_pt;
    };

    struct StitchGridValLocator
    {
        Point operator()(const StitchGridVal& val) const
        {
            return val.polyline_term_pt;
        }
    };

    // Used to find nearby end points within a fixed maximum radius
    SparsePointGrid<StitchGridVal, StitchGridValLocator> grid_ends(cell_size);
    // Used to find nearby start points within a fixed maximum radius
    SparsePointGrid<StitchGridVal, StitchGridValLocator> grid_starts(cell_size);

    // populate grids

    // Inserts the ends of all polylines into the grid (does not
    //   insert the starts of the polylines).
    for (unsigned int polyline_0_idx = 0; polyline_0_idx < open_polylines.size(); polyline_0_idx++)
    {
        ConstPolygonRef polyline_0 = open_polylines[polyline_0_idx];

        if (polyline_0.size() < 1)
            continue;

        StitchGridVal grid_val;
        grid_val.polyline_idx = polyline_0_idx;
        grid_val.polyline_term_pt = polyline_0.back();
        grid_ends.insert(grid_val);
    }

    // Inserts the start of all polylines into the grid.
    if (allow_reverse)
    {
        for (unsigned int polyline_0_idx = 0; polyline_0_idx < open_polylines.size(); polyline_0_idx++)
        {
            ConstPolygonRef polyline_0 = open_polylines[polyline_0_idx];

            if (polyline_0.size() < 1)
                continue;

            StitchGridVal grid_val;
            grid_val.polyline_idx = polyline_0_idx;
            grid_val.polyline_term_pt = polyline_0[0];
            grid_starts.insert(grid_val);
        }
    }

    // search for nearby end points
    for (unsigned int polyline_1_idx = 0; polyline_1_idx < open_polylines.size(); polyline_1_idx++)
    {
        ConstPolygonRef polyline_1 = open_polylines[polyline_1_idx];

        if (polyline_1.size() < 1)
            continue;

        std::vector<StitchGridVal> nearby_ends;

        // Check for stitches that append polyline_1 onto polyline_0
        // in natural order.  These are stitches that use the end of
        // polyline_0 and the start of polyline_1.
        nearby_ends = grid_ends.getNearby(polyline_1[0], max_dist);
        for (const auto& nearby_end : nearby_ends)
        {
            Point diff = nearby_end.polyline_term_pt - polyline_1[0];
            int64_t dist2 = vSize2(diff);
            if (dist2 < max_dist2)
            {
                PossibleStitch poss_stitch;
                poss_stitch.dist2 = dist2;
                poss_stitch.terminus_0 = Terminus{ nearby_end.polyline_idx, true };
                poss_stitch.terminus_1 = Terminus{ polyline_1_idx, false };
                stitch_queue.push(poss_stitch);
            }
        }

        if (allow_reverse)
        {
            // Check for stitches that append polyline_1 onto polyline_0
            // by reversing order of polyline_1.  These are stitches that
            // use the end of polyline_0 and the end of polyline_1.
            nearby_ends = grid_ends.getNearby(polyline_1.back(), max_dist);
            for (const auto& nearby_end : nearby_ends)
            {
                // Disallow stitching with self with same end point
                if (nearby_end.polyline_idx == polyline_1_idx)
                {
                    continue;
                }

                Point diff = nearby_end.polyline_term_pt - polyline_1.back();
                int64_t dist2 = vSize2(diff);
                if (dist2 < max_dist2)
                {
                    PossibleStitch poss_stitch;
                    poss_stitch.dist2 = dist2;
                    poss_stitch.terminus_0 = Terminus{ nearby_end.polyline_idx, true };
                    poss_stitch.terminus_1 = Terminus{ polyline_1_idx, true };
                    stitch_queue.push(poss_stitch);
                }
            }

            // Check for stitches that append polyline_1 onto polyline_0
            // by reversing order of polyline_0.  These are stitches that
            // use the start of polyline_0 and the start of polyline_1.
            std::vector<StitchGridVal> nearby_starts = grid_starts.getNearby(polyline_1[0], max_dist);
            for (const auto& nearby_start : nearby_starts)
            {
                // Disallow stitching with self with same end point
                if (nearby_start.polyline_idx == polyline_1_idx)
                {
                    continue;
                }

                Point diff = nearby_start.polyline_term_pt - polyline_1[0];
                int64_t dist2 = vSize2(diff);
                if (dist2 < max_dist2)
                {
                    PossibleStitch poss_stitch;
                    poss_stitch.dist2 = dist2;
                    poss_stitch.terminus_0 = Terminus{ nearby_start.polyline_idx, false };
                    poss_stitch.terminus_1 = Terminus{ polyline_1_idx, false };
                    stitch_queue.push(poss_stitch);
                }
            }
        }
    }

    return stitch_queue;
}

void SlicerLayer::planPolylineStitch(const Polygons& open_polylines, Terminus& terminus_0, Terminus& terminus_1, bool reverse[2]) const
{
    size_t polyline_0_idx = terminus_0.getPolylineIdx();
    size_t polyline_1_idx = terminus_1.getPolylineIdx();
    bool back_0 = terminus_0.isEnd();
    bool back_1 = terminus_1.isEnd();
    reverse[0] = false;
    reverse[1] = false;
    if (back_0)
    {
        if (back_1)
        {
            // back of both polylines
            // we can reverse either one and then append onto the other
            // reverse the smaller polyline
            if (open_polylines[polyline_0_idx].size() < open_polylines[polyline_1_idx].size())
            {
                std::swap(terminus_0, terminus_1);
            }
            reverse[1] = true;
        }
        else
        {
            // back of 0, front of 1
            // already in order, nothing to do
        }
    }
    else
    {
        if (back_1)
        {
            // front of 0, back of 1
            // in order if we swap 0 and 1
            std::swap(terminus_0, terminus_1);
        }
        else
        {
            // front of both polylines
            // we can reverse either one and then prepend to the other
            // reverse the smaller polyline
            if (open_polylines[polyline_0_idx].size() > open_polylines[polyline_1_idx].size())
            {
                std::swap(terminus_0, terminus_1);
            }
            reverse[0] = true;
        }
    }
}

void SlicerLayer::joinPolylines(PolygonRef& polyline_0, PolygonRef& polyline_1, const bool reverse[2]) const
{
    if (reverse[0])
    {
        // reverse polyline_0
        size_t size_0 = polyline_0.size();
        for (size_t idx = 0U; idx != size_0 / 2; ++idx)
        {
            std::swap(polyline_0[idx], polyline_0[size_0 - 1 - idx]);
        }
    }
    if (reverse[1])
    {
        // reverse polyline_1 by adding in reverse order
        for (int poly_idx = polyline_1.size() - 1; poly_idx >= 0; poly_idx--)
            polyline_0.add(polyline_1[poly_idx]);
    }
    else
    {
        // append polyline_1 onto polyline_0
        for (Point& p : polyline_1)
            polyline_0.add(p);
    }
    polyline_1.clear();
}

SlicerLayer::TerminusTrackingMap::TerminusTrackingMap(Terminus::Index end_idx) : m_terminus_old_to_cur_map(end_idx)
{
    // Initialize map to everything points to itself since nothing has moved yet.
    for (size_t idx = 0U; idx != end_idx; ++idx)
    {
        m_terminus_old_to_cur_map[idx] = Terminus{ idx };
    }
    m_terminus_cur_to_old_map = m_terminus_old_to_cur_map;
}

void SlicerLayer::TerminusTrackingMap::updateMap(size_t num_terms, const Terminus* cur_terms, const Terminus* next_terms, size_t num_removed_terms, const Terminus* removed_cur_terms)
{
    // save old locations
    std::vector<Terminus> old_terms(num_terms);
    for (size_t idx = 0U; idx != num_terms; ++idx)
    {
        old_terms[idx] = getOldFromCur(cur_terms[idx]);
    }
    // update using maps old <-> cur and cur <-> next
    for (size_t idx = 0U; idx != num_terms; ++idx)
    {
        m_terminus_old_to_cur_map[old_terms[idx].asIndex()] = next_terms[idx];
        Terminus next_term = next_terms[idx];
        if (next_term != Terminus::INVALID_TERMINUS)
        {
            m_terminus_cur_to_old_map[next_term.asIndex()] = old_terms[idx];
        }
    }
    // remove next locations that no longer exist
    for (size_t rem_idx = 0U; rem_idx != num_removed_terms; ++rem_idx)
    {
        m_terminus_cur_to_old_map[removed_cur_terms[rem_idx].asIndex()] = Terminus::INVALID_TERMINUS;
    }
}

void SlicerLayer::connectOpenPolylinesImpl(Polygons& open_polylines, coord_t max_dist, coord_t cell_size, bool allow_reverse)
{
    // below code closes smallest gaps first

    std::priority_queue<PossibleStitch> stitch_queue = findPossibleStitches(open_polylines, max_dist, cell_size, allow_reverse);

    static const Terminus INVALID_TERMINUS = Terminus::INVALID_TERMINUS;
    Terminus::Index terminus_end_idx = Terminus::endIndexFromPolylineEndIndex(open_polylines.size());
    // Keeps track of how polyline end point locations move around
    TerminusTrackingMap terminus_tracking_map(terminus_end_idx);

    while (! stitch_queue.empty())
    {
        // Get the next best stitch
        PossibleStitch next_stitch;
        next_stitch = stitch_queue.top();
        stitch_queue.pop();
        Terminus old_terminus_0 = next_stitch.terminus_0;
        Terminus terminus_0 = terminus_tracking_map.getCurFromOld(old_terminus_0);
        if (terminus_0 == INVALID_TERMINUS)
        {
            // if we already used this terminus, then this stitch is no longer usable
            continue;
        }
        Terminus old_terminus_1 = next_stitch.terminus_1;
        Terminus terminus_1 = terminus_tracking_map.getCurFromOld(old_terminus_1);
        if (terminus_1 == INVALID_TERMINUS)
        {
            // if we already used this terminus, then this stitch is no longer usable
            continue;
        }

        size_t best_polyline_0_idx = terminus_0.getPolylineIdx();
        size_t best_polyline_1_idx = terminus_1.getPolylineIdx();

        // check to see if this completes a polygon
        bool completed_poly = best_polyline_0_idx == best_polyline_1_idx;
        if (completed_poly)
        {
            // finished polygon
            PolygonRef polyline_0 = open_polylines[best_polyline_0_idx];
            polygons.add(polyline_0);
            polyline_0.clear();
            Terminus cur_terms[2] = { { best_polyline_0_idx, false }, { best_polyline_0_idx, true } };
            for (size_t idx = 0U; idx != 2U; ++idx)
            {
                terminus_tracking_map.markRemoved(cur_terms[idx]);
            }
            continue;
        }

        // we need to join these polylines

        // plan how to join polylines
        bool reverse[2];
        planPolylineStitch(open_polylines, terminus_0, terminus_1, reverse);

        // need to reread since planPolylineStitch can swap terminus_0/1
        best_polyline_0_idx = terminus_0.getPolylineIdx();
        best_polyline_1_idx = terminus_1.getPolylineIdx();
        PolygonRef polyline_0 = open_polylines[best_polyline_0_idx];
        PolygonRef polyline_1 = open_polylines[best_polyline_1_idx];

        // join polylines according to plan
        joinPolylines(polyline_0, polyline_1, reverse);

        // update terminus_tracking_map
        Terminus cur_terms[4] = { { best_polyline_0_idx, false }, { best_polyline_0_idx, true }, { best_polyline_1_idx, false }, { best_polyline_1_idx, true } };
        Terminus next_terms[4] = { { best_polyline_0_idx, false }, INVALID_TERMINUS, INVALID_TERMINUS, { best_polyline_0_idx, true } };
        if (reverse[0])
        {
            std::swap(next_terms[0], next_terms[1]);
        }
        if (reverse[1])
        {
            std::swap(next_terms[2], next_terms[3]);
        }
        // cur_terms -> next_terms has movement map
        // best_polyline_1 is always removed
        terminus_tracking_map.updateMap(4U, cur_terms, next_terms, 2U, &cur_terms[2]);
    }
}

void SlicerLayer::stitch_extensive(Polygons& open_polylines)
{
    // For extensive stitching find 2 open polygons that are touching 2 closed polygons.
    //  Then find the shortest path over this polygon that can be used to connect the open polygons,
    //  And generate a path over this shortest bit to link up the 2 open polygons.
    //  (If these 2 open polygons are the same polygon, then the final result is a closed polyon)

    while (1)
    {
        unsigned int best_polyline_1_idx = -1;
        unsigned int best_polyline_2_idx = -1;
        GapCloserResult best_result;
        best_result.len = POINT_MAX;
        best_result.polygonIdx = -1;
        best_result.pointIdxA = -1;
        best_result.pointIdxB = -1;

        for (unsigned int polyline_1_idx = 0; polyline_1_idx < open_polylines.size(); polyline_1_idx++)
        {
            PolygonRef polyline_1 = open_polylines[polyline_1_idx];
            if (polyline_1.size() < 1)
                continue;

            {
                GapCloserResult res = findPolygonGapCloser(polyline_1[0], polyline_1.back());
                if (res.len > 0 && res.len < best_result.len)
                {
                    best_polyline_1_idx = polyline_1_idx;
                    best_polyline_2_idx = polyline_1_idx;
                    best_result = res;
                }
            }

            for (unsigned int polyline_2_idx = 0; polyline_2_idx < open_polylines.size(); polyline_2_idx++)
            {
                PolygonRef polyline_2 = open_polylines[polyline_2_idx];
                if (polyline_2.size() < 1 || polyline_1_idx == polyline_2_idx)
                    continue;

                GapCloserResult res = findPolygonGapCloser(polyline_1[0], polyline_2.back());
                if (res.len > 0 && res.len < best_result.len)
                {
                    best_polyline_1_idx = polyline_1_idx;
                    best_polyline_2_idx = polyline_2_idx;
                    best_result = res;
                }
            }
        }

        if (best_result.len < POINT_MAX)
        {
            if (best_polyline_1_idx == best_polyline_2_idx)
            {
                if (best_result.pointIdxA == best_result.pointIdxB)
                {
                    polygons.add(open_polylines[best_polyline_1_idx]);
                    open_polylines[best_polyline_1_idx].clear();
                }
                else if (best_result.AtoB)
                {
                    PolygonRef poly = polygons.newPoly();
                    for (unsigned int j = best_result.pointIdxA; j != best_result.pointIdxB; j = (j + 1) % polygons[best_result.polygonIdx].size())
                        poly.add(polygons[best_result.polygonIdx][j]);
                    for (unsigned int j = open_polylines[best_polyline_1_idx].size() - 1; int(j) >= 0; j--)
                        poly.add(open_polylines[best_polyline_1_idx][j]);
                    open_polylines[best_polyline_1_idx].clear();
                }
                else
                {
                    unsigned int n = polygons.size();
                    polygons.add(open_polylines[best_polyline_1_idx]);
                    for (unsigned int j = best_result.pointIdxB; j != best_result.pointIdxA; j = (j + 1) % polygons[best_result.polygonIdx].size())
                        polygons[n].add(polygons[best_result.polygonIdx][j]);
                    open_polylines[best_polyline_1_idx].clear();
                }
            }
            else
            {
                if (best_result.pointIdxA == best_result.pointIdxB)
                {
                    for (unsigned int n = 0; n < open_polylines[best_polyline_1_idx].size(); n++)
                        open_polylines[best_polyline_2_idx].add(open_polylines[best_polyline_1_idx][n]);
                    open_polylines[best_polyline_1_idx].clear();
                }
                else if (best_result.AtoB)
                {
                    Polygon poly;
                    for (unsigned int n = best_result.pointIdxA; n != best_result.pointIdxB; n = (n + 1) % polygons[best_result.polygonIdx].size())
                        poly.add(polygons[best_result.polygonIdx][n]);
                    for (unsigned int n = poly.size() - 1; int(n) >= 0; n--)
                        open_polylines[best_polyline_2_idx].add(poly[n]);
                    for (unsigned int n = 0; n < open_polylines[best_polyline_1_idx].size(); n++)
                        open_polylines[best_polyline_2_idx].add(open_polylines[best_polyline_1_idx][n]);
                    open_polylines[best_polyline_1_idx].clear();
                }
                else
                {
                    for (unsigned int n = best_result.pointIdxB; n != best_result.pointIdxA; n = (n + 1) % polygons[best_result.polygonIdx].size())
                        open_polylines[best_polyline_2_idx].add(polygons[best_result.polygonIdx][n]);
                    for (unsigned int n = open_polylines[best_polyline_1_idx].size() - 1; int(n) >= 0; n--)
                        open_polylines[best_polyline_2_idx].add(open_polylines[best_polyline_1_idx][n]);
                    open_polylines[best_polyline_1_idx].clear();
                }
            }
        }
        else
        {
            break;
        }
    }
}

GapCloserResult SlicerLayer::findPolygonGapCloser(Point ip0, Point ip1)
{
    GapCloserResult ret;
    ClosePolygonResult c1 = findPolygonPointClosestTo(ip0);
    ClosePolygonResult c2 = findPolygonPointClosestTo(ip1);
    if (c1.polygonIdx < 0 || c1.polygonIdx != c2.polygonIdx)
    {
        ret.len = -1;
        return ret;
    }
    ret.polygonIdx = c1.polygonIdx;
    ret.pointIdxA = c1.pointIdx;
    ret.pointIdxB = c2.pointIdx;
    ret.AtoB = true;

    if (ret.pointIdxA == ret.pointIdxB)
    {
        // Connection points are on the same line segment.
        ret.len = vSize(ip0 - ip1);
    }
    else
    {
        // Find out if we have should go from A to B or the other way around.
        Point p0 = polygons[ret.polygonIdx][ret.pointIdxA];
        int64_t lenA = vSize(p0 - ip0);
        for (unsigned int i = ret.pointIdxA; i != ret.pointIdxB; i = (i + 1) % polygons[ret.polygonIdx].size())
        {
            Point p1 = polygons[ret.polygonIdx][i];
            lenA += vSize(p0 - p1);
            p0 = p1;
        }
        lenA += vSize(p0 - ip1);

        p0 = polygons[ret.polygonIdx][ret.pointIdxB];
        int64_t lenB = vSize(p0 - ip1);
        for (unsigned int i = ret.pointIdxB; i != ret.pointIdxA; i = (i + 1) % polygons[ret.polygonIdx].size())
        {
            Point p1 = polygons[ret.polygonIdx][i];
            lenB += vSize(p0 - p1);
            p0 = p1;
        }
        lenB += vSize(p0 - ip0);

        if (lenA < lenB)
        {
            ret.AtoB = true;
            ret.len = lenA;
        }
        else
        {
            ret.AtoB = false;
            ret.len = lenB;
        }
    }
    return ret;
}

ClosePolygonResult SlicerLayer::findPolygonPointClosestTo(Point input)
{
    ClosePolygonResult ret;
    for (unsigned int n = 0; n < polygons.size(); n++)
    {
        Point p0 = polygons[n][polygons[n].size() - 1];
        for (unsigned int i = 0; i < polygons[n].size(); i++)
        {
            Point p1 = polygons[n][i];

            // Q = A + Normal( B - A ) * ((( B - A ) dot ( P - A )) / VSize( A - B ));
            Point pDiff = p1 - p0;
            int64_t lineLength = vSize(pDiff);
            if (lineLength > 1)
            {
                int64_t distOnLine = dot(pDiff, input - p0) / lineLength;
                if (distOnLine >= 0 && distOnLine <= lineLength)
                {
                    Point q = p0 + pDiff * distOnLine / lineLength;
                    if (shorterThen(q - input, MM2INT(0.1)))
                    {
                        ret.polygonIdx = n;
                        ret.pointIdx = i;
                        return ret;
                    }
                }
            }
            p0 = p1;
        }
    }
    ret.polygonIdx = -1;
    return ret;
}
void SlicerLayer::SplitSlicerTopAndBottomSegmentBycolor(Polygons& open_polylines){
    std::map<int, std::vector<SlicerSegment>> mapsegments;
    // int segmentsflag[ segments.size()] = { 0 };
     std::map<int, ClipperLib::Paths> mappaths;
    //总路径
     ClipperLib::Paths subpaths;
   std::vector<std::vector<SlicerSegment>> vecsegments;


    for(auto it :skincolorpaths){
        ClipperLib::Paths paths = it.second;
        subpaths.insert(subpaths.end(),paths.begin(),paths.end());
    }


    ClipperLib::ClipperOffset co1;
    co1.AddPaths(subpaths, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);
    co1.Execute(subpaths, 100);

    ClipperLib::Clipper c;
    c.AddPaths(subpaths, ClipperLib::ptSubject, true);
    // c.AddPath(path, ClipperLib::ptClip, true);
    c.Execute(ClipperLib::ctUnion, subpaths, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
    ClipperLib::ClipperOffset co2;
    co2.AddPaths(subpaths, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);
    co2.Execute(subpaths, -100);

    // 内缩，做渗透
    ClipperLib::ClipperOffset co;
    co.AddPaths(subpaths, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);
    co.Execute(subpaths, -inner * 2 * 200);

    skinpath.insert(skinpath.end(), subpaths.begin(), subpaths.end());
    std::map<int, ClipperLib::Paths>::iterator iterator;
    for (iterator = skincolorpaths.begin(); iterator != skincolorpaths.end(); iterator++)
    {
        ClipperLib::Paths cpaths = iterator->second;
        int color = iterator->first;

        // 执行交运算
        ClipperLib::Clipper clipper;
        clipper.AddPaths(subpaths, ClipperLib::ptSubject, true);
        clipper.AddPaths(cpaths, ClipperLib::ptClip, true);
        clipper.Execute(ClipperLib::ctIntersection, cpaths, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

        ClipperLib::ClipperOffset co1;
        co1.AddPaths(cpaths, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);
        co1.Execute(cpaths, -50);
        for (auto path : cpaths)
        {
            Polygon poly(path);
            polygons.add(poly);
            polygons.colors.push_back(color);
            std::unique_ptr<Polygon> obj = std::make_unique<Polygon>(poly);
            polygons.polys.push_back(std::move(obj));
        }
    }
}

int IsInsidePath(const ClipperLib::Paths& path1, const ClipperLib::Path& path2) {
    //被包含次数
    int num = 0;
    for (auto& path : path1) {
        bool flag = true;
        for (auto& point: path2) {
             if (!ClipperLib::PointInPolygon(point, path)) {
                flag = false;  
            }
        }
        if(flag) {
            num+=1;
        }
       
    }
    return num;
}


#if 1

void SlicerLayer::SplitSlicerSegmentBycolor(Polygons& open_polylines)
{
    std::vector<std::vector<SlicerSegment>> vecsegments;

    int *segmentsflag = (int * ) malloc (segments.size() * sizeof(int));
    memset(segmentsflag, 0, segments.size() * sizeof(int));
    for (int i = 0; i < segments.size(); i++){
        if (segmentsflag[i]){
            continue;
        }
        std::vector<SlicerSegment> psegments;
        psegments.push_back(segments[i]);
        segmentsflag[i]=1;
        for (int j = 0; j < segments.size(); j++){
            if (segmentsflag[j]){
                continue;
            }
            Point start = psegments.front().start;
            Point end = psegments.back().end;
            if(std::abs( segments[j].start.X - end.X)<=2&&std::abs( segments[j].start.Y - end.Y)<=2){
                segmentsflag[j]=1;
                psegments.push_back(segments[j]);
                j=0;
            }

            if(std::abs(segments[j].end.X - start.X)<=2 && std::abs(segments[j].end.Y - start.Y)<=2){
                segmentsflag[j]=1;
                psegments.insert(psegments.begin(),segments[j]);
                j=0;
            }
        }
        vecsegments.push_back(psegments);
    }
    // 总路径
    ClipperLib::Paths subpath;
    for (int i = 0; i < vecsegments.size(); i++)
    {
        ClipperLib::Path path;
        if (vecsegments[i].size() > 0)
        {
            path.push_back(vecsegments[i][0].start);
            for (int j = 0; j < vecsegments[i].size(); j++)
            {
                SlicerSegment s = vecsegments[i][j];
                // if(s.color = startcolor){
                path.push_back(s.end);
                // }
            }
            subpath.push_back(path);
        }

        // ClipperLib::Paths solution;
        // ClipperLib::Clipper c;
        // c.AddPaths(subpath, ClipperLib::ptSubject, true);
        // c.AddPath(path, ClipperLib::ptClip, true);
        // c.Execute(ClipperLib::ctUnion, subpath, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
    }

     ClipperLib::Clipper clpr = ClipperLib::Clipper();
     clpr.AddPaths(subpath, ClipperLib::ptSubject,true);
     ClipperLib::IntRect bounds = clpr.GetBounds();
     // 输出边界框的坐标

    for (int i = 0; i < vecsegments.size(); i++)
    {
        std::vector<SlicerSegment> psegments = vecsegments[i];
        int crrcolor = psegments[0].color;
        int startcolor = psegments[0].color;
        // 单一颜色
        std::vector<SlicerSegment> singlecolorsegments;
        // 拆分出的线段组成多边形
        std::vector<SlicerSegment> anothersegments;
        //   Path subj;

        int centerx = 0;
        int centery = 0;

        int minX = 0;
        int minY = 0;
        int maxX = 0;
        int maxY = 0;
        if (psegments.size() > 0)
        {
            minX = psegments[0].start.X;
            minY = psegments[0].start.Y;
            maxX = psegments[0].start.X;
            maxY = psegments[0].start.Y;
        }
        for (int j = 0; j < psegments.size(); j++)
        {
            minX = psegments[j].end.X < minX ? psegments[j].end.X : minX;
            minY = psegments[j].end.Y < minY ? psegments[j].end.Y : minY;

            maxX = psegments[j].end.X > maxX ? psegments[j].end.X : maxX;
            maxY = psegments[j].end.Y > maxY ? psegments[j].end.Y : maxY;
        }
        centerx = (maxX + minX) / 2;
        centery = (maxY + minY) / 2;




        ClipperLib::Paths cpath(1);
        cpath[0].push_back(psegments[0].start);
        for (int j = 0; j < psegments.size(); j++)
        {
            SlicerSegment &s = psegments.at(j);

            // if(s.color = startcolor){
            cpath[0].push_back(s.end);
            // }
        }

        //  // 判断路径2是否是路径1的内部路径
        int InsidePath = IsInsidePath(subpath, cpath[0]);

        std::map<int, std::vector<ClipperLib::Paths>> mappaths;

        std::vector<std::pair<int, ClipperLib::Path>> colorpaths;

        ClipperLib::Path path;
        if (psegments.size() > 0)
        {
            path.push_back(psegments[0].start);
            int lastcolor = psegments[0].color;
            SlicerSegment lastSegment = psegments[0];
            for (int j = 0; j < psegments.size(); j++)
            {
                SlicerSegment s = psegments[j];
                path.push_back(s.end);
                if (s.color != lastcolor)
                {
                    colorpaths.push_back(std::pair<int, ClipperLib::Path>(lastcolor, path));
                    path.clear();
                    path.push_back(s.end);
                }
                lastcolor = s.color;
            }
            if (colorpaths.size() > 0)
            {
                if (colorpaths.front().first == lastcolor)
                {

                    // for(int p_idx = path.size()-1;p_idx>=0;p_idx--){
                    //     // colorpaths.front().second.push_back(path[p_idx]);
                    //     colorpaths.front().second.insert(path[p_idx]);

                    // }
                    for (int p_idx = 0; p_idx < colorpaths.front().second.size(); p_idx++)
                    {
                        path.push_back(colorpaths.front().second[p_idx]);
                    }
                    colorpaths.front().second = path;
                    // colorpaths.push_back(std::pair<int,ClipperLib::Path>(lastcolor,path));
                }
                else
                {
                    colorpaths.push_back(std::pair<int, ClipperLib::Path>(lastcolor, path));
                }
            }
            else
            {
                colorpaths.push_back(std::pair<int, ClipperLib::Path>(lastcolor, path));
            }
        }

        std::vector<Polygon> plist;
        for (auto it : colorpaths)
        {
            ClipperLib::Path cpath = it.second;
            int color = it.first;
            if(cpath.size()<2){
                continue;
            }
            //计算中点
            if (0)
            {
                // 最后一条线
                ClipperLib::IntPoint point1 = cpath[cpath.size() - 1];
                ClipperLib::IntPoint point2 = cpath[cpath.size() - 2];
                // 线段中点
                ClipperLib::IntPoint linecenter = ClipperLib::IntPoint((point1.X + point2.X) / 2, (point1.Y + point2.Y) / 2);

                ClipperLib::IntPoint verticalpoint1;
                ClipperLib::IntPoint verticalpoint2;
                if (point1.X == point2.X)
                {
                    verticalpoint1 = ClipperLib::IntPoint(bounds.left, linecenter.Y);
                    verticalpoint2 = ClipperLib::IntPoint(bounds.right, linecenter.Y);

                    centerx = (verticalpoint2.X + verticalpoint1.X) / 2;
                    centery = (verticalpoint2.Y + verticalpoint1.Y) / 2;
                }
                else if (point1.Y == point2.Y)
                {
                    verticalpoint1 = ClipperLib::IntPoint(bounds.left, linecenter.Y);
                    verticalpoint2 = ClipperLib::IntPoint(bounds.right, linecenter.Y);
                    centerx = (verticalpoint2.X + verticalpoint1.X) / 2;
                    centery = (verticalpoint2.Y + verticalpoint1.Y) / 2;
                }
                else
                {
                    double m = (double)(point2.Y - point1.Y) / (double)(point2.X - point1.X);
                }
            }
            cpath.push_back(ClipperLib::IntPoint(centerx, centery));
            cpath.push_back(cpath[0]);
            ClipperLib::Paths cpaths(1);
            cpaths[0] = cpath;

            //  Polygon poly;
            // 偏移一
            ClipperLib::ClipperOffset co;
            co.AddPaths(cpaths, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);
            co.Execute(cpaths, 1);

            if (isSkin)
            {

                // 减去表面的路径
                ClipperLib::Clipper clipper;
                // clipper.AddPaths(cpaths, ClipperLib::ptSubject, true);

                // continue;
                ClipperLib::Paths skinofpath = skinpath;
                ClipperLib::ClipperOffset co;
                co.AddPaths(skinpath, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);
                co.Execute(skinofpath, 1);

                // 计算交集
                ClipperLib::Paths intersectionpath;
                ClipperLib::Clipper intersectionCo;
                if(cpaths.size()==0){
                    continue;
                }
                intersectionpath.push_back(cpaths[0]);
                intersectionCo.AddPaths(skinofpath, ClipperLib::ptSubject, true);

                intersectionCo.AddPaths(cpaths, ClipperLib::ptClip, true);
                intersectionCo.Execute(ClipperLib::ctIntersection, intersectionpath);

                clipper.AddPaths(cpaths, ClipperLib::ptSubject, true);
                clipper.AddPaths(intersectionpath, ClipperLib::ptClip, true);
                // 创建一个路径对象来存储结果
                ClipperLib::Paths solution;

                // 进行路径相减操作
                clipper.Execute(ClipperLib::ctDifference, solution, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);

                if (solution.empty())
                {
                    continue;
                }
                cpaths = solution;
            }
            //偏移一
             ClipperLib::ClipperOffset co1;
             co1.AddPaths(cpaths, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);
             co1.Execute(cpaths, -2);
            

            if (cpaths.size() <= 0)
            {
                return;
            }
            if (cpaths[0].size() <= 0)
            {
                return;
            }
            for(auto path :cpaths){
                path.push_back(path[0]);
                Polygon polygon(path);
                polygon.color = color;
                plist.push_back(polygon);
            }
           
            // polygons.colors[i] = color;
            // std::unique_ptr<Polygon> obj = std::make_unique<Polygon>(poly);
            // polygons.polys.push_back(std::move(obj));
            // polygons.polys[i]->color==s.color;
        }
        // for(int m=0;m<plist.size();m++){
        //     for(int n=m;n<plist.size();n++){
        //         if(plist[n].color<plist[m].color){
        //             std::swap(plist[n],plist[m]);
        //         }
        //     }
        // }
       
        for (int m = 0; m < plist.size(); m++)
        {

            polygons.add(plist[m]);
            // polygons.colors.push_back(s.color);
            polygons.colors.push_back(plist[m].color);
            std::unique_ptr<Polygon> obj = std::make_unique<Polygon>(plist[m]);
            polygons.polys.push_back(std::move(obj));
        }
    }
}

#endif 

#if 0
void SlicerLayer::SplitSlicerSegmentBycolor(Polygons& open_polylines)
{
    std::vector<std::vector<SlicerSegment>> vecsegments;
    int* segmentsflag = new int[1000000]();
    for (size_t start_segment_idx = 0; start_segment_idx < segments.size(); start_segment_idx++)
    {
        if (! segmentsflag[start_segment_idx])
        {
            std::vector<SlicerSegment> psegments;
            double startx = segments[start_segment_idx].start.X;
            double starty = segments[start_segment_idx].start.Y;
            for (int segment_idx = start_segment_idx; segment_idx != -1;)
            {
                SlicerSegment segment = segments[segment_idx];
                SlicerSegment s;
                segmentsflag[segment_idx] = 1;
                s.start.X = startx;
                s.start.Y = starty;
                s.end.X = segment.end.X;
                s.end.Y = segment.end.Y;
                startx = segment.end.X;
                starty = segment.end.Y;
                s.color = segment.color;
                segment_idx = getNextSegmentIdx(segment, start_segment_idx);
                psegments.push_back(s);

                if (segment_idx == static_cast<int>(start_segment_idx))
                {
                    break;
                }
            }
            vecsegments.push_back(psegments);
        }
    }



    for (int i = 0; i < vecsegments.size(); i++)
    {
        std::vector<SlicerSegment> psegments = vecsegments[i];
        int crrcolor = psegments[0].color;
        int startcolor = psegments[0].color;
        // 单一颜色
        std::vector<SlicerSegment> singlecolorsegments;
        // 拆分出的线段组成多边形
        std::vector<SlicerSegment> anothersegments;
        //   Path subj;

        int centerx = 0;
        int centery = 0;

        int minX = 0;
        int minY = 0;
        int maxX = 0;
        int maxY = 0;
        if (psegments.size() > 0)
        {
            minX = psegments[0].start.X;
            minY = psegments[0].start.Y;
            maxX = psegments[0].start.X;
            maxY = psegments[0].start.Y;
        }
        for (int j = 0; j < psegments.size(); j++)
        {
            minX = psegments[j].end.X < minX ? psegments[j].end.X : minX;
            minY = psegments[j].end.Y < minY ? psegments[j].end.Y : minY;

            maxX = psegments[j].end.X > maxX ? psegments[j].end.X : maxX;
            maxY = psegments[j].end.Y > maxY ? psegments[j].end.Y : maxY;
        }
        centerx = (maxX + minX) / 2;
        centery = (maxY + minY) / 2;
        for (int j = 0; j < psegments.size(); j++)
        {
            SlicerSegment s = psegments[j];


            if (crrcolor != s.color)
            {
                ClipperLib::Paths cpath(1);
                int cpathcolor = s.color;
                s.end.X = centerx;
                s.end.Y = centery;

                if (crrcolor == startcolor)
                {
                    singlecolorsegments.push_back(s);
                }
                crrcolor = s.color;

                j++;
                for (; j < psegments.size(); j++)
                {
                    //  anothersegments.push_back(s);

                    ClipperLib::IntPoint p = ClipperLib::IntPoint(s.start.X, s.start.Y);
                    cpath[0].push_back(p);
                    p = ClipperLib::IntPoint(s.end.X, s.end.Y);
                    cpath[0].push_back(p);

                    s = psegments[j];

                    if (crrcolor != s.color)
                    {
                        SlicerSegment s2;
                        s2.end = s.start;
                        s2.start.X = centerx;
                        s2.start.Y = centery;
                        if (s.color == startcolor)
                        {
                            singlecolorsegments.push_back(s2);
                        }


                        p = ClipperLib::IntPoint(s2.end.X, s2.end.Y);
                        cpath[0].push_back(p);

                        //
                        crrcolor = s.color;
                        if (s.color == startcolor)
                        {
                            singlecolorsegments.push_back(s);
                            crrcolor = s.color;
                        }
                        else
                        {
                            j--;
                        }

                        break;
                    }
                }


                ClipperLib::ClipperOffset co;
                co.AddPaths(cpath, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
                
                co.Execute(cpath, -1);
                if (cpath[0].size() > 0)
                {
                    for (size_t i = 1; i < cpath[0].size(); i++)
                    {
                        SlicerSegment s;
                        s.start.X = cpath[0][i - 1].X;
                        s.start.Y = cpath[0][i - 1].Y;
                        s.end.X = cpath[0][i].X;
                        s.end.Y = cpath[0][i].Y;
                        s.color = cpathcolor;
                        anothersegments.push_back(s);

                        //
                    }
                    vecsegments.push_back(anothersegments);
                    anothersegments.clear();
                }
            }
            else
            {
                singlecolorsegments.push_back(s);
            }
        }
        vecsegments[i] = singlecolorsegments;
    }


    std::vector<std::unique_ptr<Polygon>> polys;


    // 生成多边形
    for (int i = 0; i < vecsegments.size(); i++)
    {
        std::vector<SlicerSegment> psegments = vecsegments[i];
        Polygon poly;
        poly.add(psegments[0].start);
        ClipperLib::Paths cpath(1);
        cpath[0].push_back(psegments[0].start);
        SlicerSegment s;
        for (int j = 0; j < psegments.size(); j++)
        {
            s = psegments[j];
            poly.add(s.end);
            cpath[0].push_back(s.end);
        }
        poly.color = s.color;

        if(isSkin){
                // 减去表面的路径
            ClipperLib::Clipper clipper;
            clipper.AddPaths(cpath, ClipperLib::ptSubject, true);
            clipper.AddPaths(skinpath, ClipperLib::ptClip, true);
            // 创建一个路径对象来存储结果
            ClipperLib::Paths solution;

            // 进行路径相减操作
            clipper.Execute(ClipperLib::ctDifference, solution);
    
            if(solution.empty()){
                continue;
            }
            cpath[0] = solution[0];
        }
       
        Polygon polygon(cpath[0]);
        polygons.add(polygon);
        // polygons.colors.push_back(s.color);
        polygons.colors.push_back(s.color);
        polygons.colors[i] = s.color;
        std::unique_ptr<Polygon> obj = std::make_unique<Polygon>(poly);
        polygons.polys.push_back(std::move(obj));
        polygons.polys[i]->color==s.color;
       
    }
}


#endif
void SlicerLayer::makePolygons(const Mesh* mesh)
{
    Polygons open_polylines;

    SplitSlicerTopAndBottomSegmentBycolor(open_polylines);
    SplitSlicerSegmentBycolor(open_polylines);

    connectOpenPolylines(open_polylines);

    // TODO: (?) for mesh surface mode: connect open polygons. Maybe the above algorithm can create two open polygons which are actually connected when the starting segment is in the middle between the two open polygons.

    if (mesh->settings.get<ESurfaceMode>("magic_mesh_surface_mode") == ESurfaceMode::NORMAL)
    { // don't stitch when using (any) mesh surface mode, i.e. also don't stitch when using mixed mesh surface and closed polygons, because then polylines which are supposed to be open will be closed
        stitch(open_polylines);
    }

    if (mesh->settings.get<bool>("meshfix_extensive_stitching"))
    {
        stitch_extensive(open_polylines);
    }

    if (mesh->settings.get<bool>("meshfix_keep_open_polygons"))
    {
        for (PolygonRef polyline : open_polylines)
        {
            if (polyline.size() > 0)
                polygons.add(polyline);
        }
    }

    for (PolygonRef polyline : open_polylines)
    {
        if (polyline.size() > 0)
        {
            openPolylines.add(polyline);
        }
    }

    // Remove all the tiny polygons, or polygons that are not closed. As they do not contribute to the actual print.
    const coord_t snap_distance = std::max(mesh->settings.get<coord_t>("minimum_polygon_circumference"), static_cast<coord_t>(1));
    auto it = std::remove_if(polygons.begin(), polygons.end(), [snap_distance](PolygonRef poly) { return poly.shorterThan(snap_distance); });
    polygons.erase(it, polygons.end());

    // Finally optimize all the polygons. Every point removed saves time in the long run.
    polygons = simplifyPolygon(polygons, mesh->settings);

    polygons.removeDegenerateVerts(); // remove verts connected to overlapping line segments

    // Clean up polylines for Surface Mode printing
    it = std::remove_if(openPolylines.begin(), openPolylines.end(), [snap_distance](PolygonRef poly) { return poly.shorterThan(snap_distance); });
    openPolylines.erase(it, openPolylines.end());

    openPolylines.removeDegenerateVertsPolyline();
}

Slicer::Slicer(SliceContext* _application, Mesh* i_mesh, const coord_t thickness, const size_t slice_layer_count, bool use_variable_layer_heights, std::vector<AdaptiveLayer>* adaptive_layers)
    : mesh(i_mesh)
    , application(_application)
{
    const SlicingTolerance slicing_tolerance = mesh->settings.get<SlicingTolerance>("slicing_tolerance");
    const coord_t initial_layer_thickness = application->get_layer_height_0();

    assert(slice_layer_count > 0);

    TimeKeeper slice_timer;

    layers = buildLayersWithHeight(slice_layer_count, slicing_tolerance, initial_layer_thickness, thickness, use_variable_layer_heights, adaptive_layers);


    std::vector<std::pair<int32_t, int32_t>> zbbox = buildZHeightsForFaces(*mesh);
    if(mesh->settings.get<bool>("support_paint_enable"))
        buildSegments_paint_support(application, *mesh, zbbox, slicing_tolerance, layers);
    else
        buildSegments(application, *mesh, zbbox, slicing_tolerance, layers);

    if (mesh->settings.has("zseam_paint_enable") && mesh->settings.get<bool>("zseam_paint_enable"))
        return;


    LOGI("Slice of mesh took { %f } seconds", slice_timer.restart());

    ///makePolygons(application, *i_mesh, slicing_tolerance, layers);
    makePolygons(application, *i_mesh, slicing_tolerance, layers, slicerLayers);
    LOGI("Make polygons took { %f } seconds", slice_timer.restart());

	
    if (mesh->settings.get<bool>("keep_open_polygons"))
        mergeOpenPloygons(mesh, layers);
}

void Slicer::buildSegments(SliceContext* application, const Mesh& mesh, const std::vector<std::pair<int32_t, int32_t>>& zbbox, const SlicingTolerance& slicing_tolerance, std::vector<SlicerLayer>& layers)
{
    cura52::parallel_for(application, layers,
                       [&](auto layer_it)
                       {
                           std::vector<SlicerSegment> segments;
                           SlicerLayer& layer = *layer_it;
                           const int32_t& z = layer.z;
                           int layer_height = mesh.settings.get<coord_t>("layer_height");
                           const int32_t& minz = layer_height;
                           layer.segments.reserve(100);

                           // loop over all mesh faces
                           for (unsigned int mesh_idx = 0; mesh_idx < mesh.faces.size(); mesh_idx++)
                           {
                               const MeshFace& face = mesh.faces[mesh_idx];
                               const MeshVertex& v0 = mesh.vertices[face.vertex_index[0]];
                               const MeshVertex& v1 = mesh.vertices[face.vertex_index[1]];
                               const MeshVertex& v2 = mesh.vertices[face.vertex_index[2]];

                               // get all vertices represented as 3D point
                               Point3 p0 = v0.p;
                               Point3 p1 = v1.p;
                               Point3 p2 = v2.p;
                               int color = face.color;

                               // Compensate for points exactly on the slice-boundary, except for 'inclusive', which already handles this correctly.
                               if (slicing_tolerance != SlicingTolerance::INCLUSIVE)
                               {
                                   p0.z += static_cast<int>(p0.z == z);
                                   p1.z += static_cast<int>(p1.z == z);
                                   p2.z += static_cast<int>(p2.z == z);
                               }
                               if (p0.z == p1.z && p1.z == p2.z)
                               {
                                  
                                //    if ((z + minz >= p0.z&&z<p0.z)|| z - minz <p0.z&&z>=p0.z)
                                    int n = 3;
                                   if ( ((z + minz > p0.z)&&z<=p0.z)||((z - minz*n <= p0.z)&&z>p0.z)||((z + minz*n > p0.z)&&z<=p0.z)||(z>0&&z<minz&&p0.z==0))
                                   {    
                                       SlicerSegment s1;
                                       s1.start.X = p0.x;
                                       s1.start.Y = p0.y;
                                       s1.end.X = p1.x;
                                       s1.end.Y = p1.y;
                                       s1.color = color;
                                       SlicerSegment s2;
                                       s2.start.X = p1.x;
                                       s2.start.Y = p1.y;
                                       s2.end.X = p2.x;
                                       s2.end.Y = p2.y;
                                       s2.color = color;
                                       SlicerSegment s3;
                                       s3.start.X = p2.x;
                                       s3.start.Y = p2.y;
                                       s3.end.X = p0.x;
                                       s3.end.Y = p0.y;
                                       s3.color = color;
                                      ClipperLib::Path path;
                                      path.push_back(s1.end);
                                      path.push_back(s2.end);
                                      path.push_back(s3.end);
                                      if(layer.skincolorpaths.find(color)!=layer.skincolorpaths.end()){
                                        layer.skincolorpaths[color].push_back(path);
                                      } else {
                                         ClipperLib::Paths paths;
                                          paths.push_back(path);
                                        layer.skincolorpaths.insert(std::pair<int,ClipperLib::Paths>(color,paths));
                                      }
                                       layer.skinsegments.push_back(s1);
                                       layer.skinsegments.push_back(s2);
                                       layer.skinsegments.push_back(s3);
                                       layer.isSkin = true;
                                       
                                       for(int inner = 1;inner<=n;inner++){
                                            if((z + minz*inner > p0.z)&&z<=p0.z){
                                                layer.inner = inner-1;
                                                break;
                                            }
                                       }

                                       for(int inner = 1;inner<=n;inner++){
                                            if((z - minz*inner <= p0.z)&&z>p0.z){
                                                layer.inner = inner-1;
                                                // std::cout << "s" << std::endl;
                                                break;
                                            }
                                       }
                                       
                                   }
                                   continue;
                               }

                               SlicerSegment s;
                               s.endVertex = nullptr;
                               int end_edge_idx = -1;

                               /*
                               Now see if the triangle intersects the layer, and if so, where.

                               Edge cases are important here:
                               - If all three vertices of the triangle are exactly on the layer,
                                 don't count the triangle at all, because if the model is
                                 watertight, there will be adjacent triangles on all 3 sides that
                                 are not flat on the layer.
                               - If two of the vertices are exactly on the layer, only count the
                                 triangle if the last vertex is going up. We can't count both
                                 upwards and downwards triangles here, because if the model is
                                 manifold there will always be an adjacent triangle that is going
                                 the other way and you'd get double edges. You would also get one
                                 layer too many if the total model height is an exact multiple of
                                 the layer thickness. Between going up and going down, we need to
                                 choose the triangles going up, because otherwise the first layer
                                 of where the model starts will be empty and the model will float
                                 in mid-air. We'd much rather let the last layer be empty in that
                                 case.
                               - If only one of the vertices is exactly on the layer, the
                                 intersection between the triangle and the plane would be a point.
                                 We can't print points and with a manifold model there would be
                                 line segments adjacent to the point on both sides anyway, so we
                                 need to discard this 0-length line segment then.
                               - Vertices in ccw order if look from outside.
                               */

                               if (p0.z < z && p1.z > z && p2.z > z) //  1_______2
                               { //   \     /
                                   s = project2D(p0, p2, p1, z, color); //------------- z
                                   end_edge_idx = 0; //     \ /
                               } //      0

                               else if (p0.z > z && p1.z <= z && p2.z <= z) //      0
                               { //     / \      .
                                   s = project2D(p0, p1, p2, z, color); //------------- z
                                   end_edge_idx = 2; //   /     \    .
                                   if (p2.z == z) //  1_______2
                                   {
                                       s.endVertex = &v2;
                                   }
                               }

                               else if (p1.z < z && p0.z > z && p2.z > z) //  0_______2
                               { //   \     /
                                   s = project2D(p1, p0, p2, z, color); //------------- z
                                   end_edge_idx = 1; //     \ /
                               } //      1

                               else if (p1.z > z && p0.z <= z && p2.z <= z) //      1
                               { //     / \      .
                                   s = project2D(p1, p2, p0, z, color); //------------- z
                                   end_edge_idx = 0; //   /     \    .
                                   if (p0.z == z) //  0_______2
                                   {
                                       s.endVertex = &v0;
                                   }
                               }

                               else if (p2.z < z && p1.z > z && p0.z > z) //  0_______1
                               { //   \     /
                                   s = project2D(p2, p1, p0, z, color); //------------- z
                                   end_edge_idx = 2; //     \ /
                               } //      2

                               else if (p2.z > z && p1.z <= z && p0.z <= z) //      2
                               { //     / \      .
                                   s = project2D(p2, p0, p1, z, color); //------------- z
                                   end_edge_idx = 1; //   /     \    .
                                   if (p1.z == z) //  0_______1
                                   {
                                       s.endVertex = &v1;
                                   }
                               }
                               else
                               {
                                   // Not all cases create a segment, because a point of a face could create just a dot, and two touching faces
                                   //   on the slice would create two segments
                                   continue;
                               }

                               // store the segments per layer
                               layer.face_idx_to_segment_idx.insert(std::make_pair(mesh_idx, layer.segments.size()));
                               s.faceIndex = mesh_idx;
                               s.endOtherFaceIdx = face.connected_face_index[end_edge_idx];
                               s.addedToPolygon = false;
                               segments.push_back(s);
                               layer.segments.push_back(s);
                           }
		});
}

void Slicer::buildSegments_paint_support(SliceContext* application, const Mesh& mesh, const std::vector<std::pair<int32_t, int32_t>>& zbbox, const SlicingTolerance& slicing_tolerance, std::vector<SlicerLayer>& layers)
{
    int32_t lastZ = 0;
     cura52::parallel_for(application, layers,
                        [&](auto layer_it)
         {
        SlicerLayer& layer = *layer_it;
        const int32_t& z = layer.z;
        layer.segments.reserve(100);

        // loop over all mesh faces
        for (unsigned int mesh_idx = 0; mesh_idx < mesh.faces.size(); mesh_idx++)
        {
            // get all vertices per face
            const MeshFace& face = mesh.faces[mesh_idx];
            const MeshVertex& v0 = mesh.vertices[face.vertex_index[0]];
            const MeshVertex& v1 = mesh.vertices[face.vertex_index[1]];
            const MeshVertex& v2 = mesh.vertices[face.vertex_index[2]];

            // get all vertices represented as 3D point
            Point3 p0 = v0.p;
            Point3 p1 = v1.p;
            Point3 p2 = v2.p;



            if (zbbox[mesh_idx].first == zbbox[mesh_idx].second
                && lastZ <= zbbox[mesh_idx].first
                && z >= zbbox[mesh_idx].first)
            {
                const MeshFace& face = mesh.faces[mesh_idx];
                for (int i = 0; i < 3; i++)
                {
                    const MeshVertex& v0 = mesh.vertices[face.vertex_index[i]];
                    const MeshVertex& v1 = mesh.vertices[face.vertex_index[(i+1)%3]];
                    SlicerSegment s;
                    s.start.X = v0.p.x;
                    s.start.Y = v0.p.y;
                    s.end.X = v1.p.x;
                    s.end.Y = v1.p.y;

                    // store the segments per layer
                    layer.face_idx_to_segment_idx.insert(std::make_pair(mesh_idx, layer.segments.size()));
                    s.faceIndex = mesh_idx;
                    s.addedToPolygon = false;
                    layer.segments.push_back(s);
                }

                continue;
            }

            if ((z < zbbox[mesh_idx].first) || (z > zbbox[mesh_idx].second))
            {
                continue;
            }

            // Compensate for points exactly on the slice-boundary, except for 'inclusive', which already handles this correctly.
            if (slicing_tolerance != SlicingTolerance::INCLUSIVE)
            {
                p0.z += static_cast<int>(p0.z == z);
                p1.z += static_cast<int>(p1.z == z);
                p2.z += static_cast<int>(p2.z == z);
            }

            SlicerSegment s;
            s.endVertex = nullptr;
            int end_edge_idx = -1;

            if (p0.z < z && p1.z > z && p2.z > z) //  1_______2
            { //   \     /
                s = project2D(p0, p2, p1, z); //------------- z
                end_edge_idx = 0; //     \ /
            } //      0

            else if (p0.z > z && p1.z <= z && p2.z <= z) //      0
            { //     / \      .
                s = project2D(p0, p1, p2, z); //------------- z
                end_edge_idx = 2; //   /     \    .
                if (p2.z == z) //  1_______2
                {
                    s.endVertex = &v2;
                }
            }

            else if (p1.z < z && p0.z > z && p2.z > z) //  0_______2
            { //   \     /
                s = project2D(p1, p0, p2, z); //------------- z
                end_edge_idx = 1; //     \ /
            } //      1

            else if (p1.z > z && p0.z <= z && p2.z <= z) //      1
            { //     / \      .
                s = project2D(p1, p2, p0, z); //------------- z
                end_edge_idx = 0; //   /     \    .
                if (p0.z == z) //  0_______2
                {
                    s.endVertex = &v0;
                }
            }

            else if (p2.z < z && p1.z > z && p0.z > z) //  0_______1
            { //   \     /
                s = project2D(p2, p1, p0, z); //------------- z
                end_edge_idx = 2; //     \ /
            } //      2

            else if (p2.z > z && p1.z <= z && p0.z <= z) //      2
            { //     / \      .
                s = project2D(p2, p0, p1, z); //------------- z
                end_edge_idx = 1; //   /     \    .
                if (p1.z == z) //  0_______1
                {
                    s.endVertex = &v1;
                }
            }
            else
            {
                continue;
            }

            layer.face_idx_to_segment_idx.insert(std::make_pair(mesh_idx, layer.segments.size()));
            s.faceIndex = mesh_idx;
            s.endOtherFaceIdx = face.connected_face_index[end_edge_idx];
            s.addedToPolygon = false;
            layer.segments.push_back(s);
        }

        lastZ = z;
    });
}

std::vector<SlicerLayer>
    Slicer::buildLayersWithHeight(size_t slice_layer_count, SlicingTolerance slicing_tolerance, coord_t initial_layer_thickness, coord_t thickness, bool use_variable_layer_heights, const std::vector<AdaptiveLayer>* adaptive_layers)
{
    std::vector<SlicerLayer> layers_res;

    layers_res.resize(slice_layer_count);

    // set (and initialize compensation for) initial layer, depending on slicing mode
    layers_res[0].z = slicing_tolerance == SlicingTolerance::INCLUSIVE ? 0 : std::max(0LL, initial_layer_thickness - thickness);
    coord_t adjusted_layer_offset = initial_layer_thickness;
    if (use_variable_layer_heights)
    {
        layers_res[0].z = (*adaptive_layers)[0].z_position;
    }
    else if (slicing_tolerance == SlicingTolerance::MIDDLE)
    {
        layers_res[0].z = initial_layer_thickness / 2;
        adjusted_layer_offset = initial_layer_thickness + (thickness / 2);
    }

    // define all layer z positions (depending on slicing mode, see above)
    for (unsigned int layer_nr = 1; layer_nr < slice_layer_count; layer_nr++)
    {
        if (use_variable_layer_heights)
        {
            layers_res[layer_nr].z = (*adaptive_layers)[layer_nr].z_position;
        }
        else
        {
            layers_res[layer_nr].z = adjusted_layer_offset + (thickness * (layer_nr - 1));
        }
    }

    return layers_res;
}

void Slicer::makePolygons(SliceContext* application, Mesh& mesh, SlicingTolerance slicing_tolerance, std::vector<SlicerLayer>& layers,std::map<int,std::vector<SlicerLayer>>& slicerLayers)
{
    cura52::parallel_for(application, layers, [&mesh](auto layer_it) { layer_it->makePolygons(&mesh); });

    int size = layers.size();
    for (int i = 0; i < layers.size(); i++)
    {
        std::vector<std::unique_ptr<Polygon>> polygons = std::move(layers[i].polygons.polys);
        SlicerLayer layer_it = layers[i];
        int z = layer_it.z;

        std::map<int, SlicerLayer> layerMap;
        for (int j = 0; j < polygons.size(); j++)
        {
            if (layerMap.find(polygons[j]->color) != layerMap.end())
            {
                layerMap.at(polygons[j]->color).polygons.add(*(polygons[j]));
            }
            else
            {
                SlicerLayer layer;
                layer.z = z;
                layer.polygons.add((*(polygons[j])));
                layerMap.insert(std::pair<int, SlicerLayer>(polygons[j]->color, layer));
            }
        }

        if (layerMap.size() > 1)
        {
            for (auto it : layerMap)
            {
                if (slicerLayers.find(it.first) != slicerLayers.end())
                {
                    slicerLayers.at(it.first).push_back(it.second);
                }
                else
                {
                    std::vector<SlicerLayer> layerlist;
                    layerlist.push_back(it.second);
                    slicerLayers.insert(std::pair<int, std::vector<SlicerLayer>>(it.first, layerlist));
                }
            }
        }
    }

    switch (slicing_tolerance)
    {
    case SlicingTolerance::INCLUSIVE:
        for (unsigned int layer_nr = 0; layer_nr + 1 < layers.size(); layer_nr++)
        {
            layers[layer_nr].polygons = layers[layer_nr].polygons.unionPolygons(layers[layer_nr + 1].polygons);
        }
        break;
    case SlicingTolerance::EXCLUSIVE:
        for (unsigned int layer_nr = 0; layer_nr + 1 < layers.size(); layer_nr++)
        {
            layers[layer_nr].polygons = layers[layer_nr].polygons.intersection(layers[layer_nr + 1].polygons);
        }
        layers.back().polygons.clear();
        break;
    case SlicingTolerance::MIDDLE:
    default:
        // do nothing
        ;
    }

    size_t layer_apply_initial_xy_offset = 0;
    if (layers.size() > 0 && layers[0].polygons.size() == 0 && ! mesh.settings.get<bool>("support_mesh") && ! mesh.settings.get<bool>("anti_overhang_mesh") && ! mesh.settings.get<bool>("cutting_mesh")
        && ! mesh.settings.get<bool>("infill_mesh"))
    {
        layer_apply_initial_xy_offset = 1;
    }

    if (mesh.settings.get<bool>("support_paint_enable"))
        return;

    const coord_t xy_offset = mesh.settings.get<coord_t>("xy_offset");
    const coord_t xy_offset_0 = mesh.settings.get<coord_t>("xy_offset_layer_0");
    const coord_t offset_rectify = 0.5 * mesh.settings.get<coord_t>("layer_height") * float(1. - 0.25 * M_PI) + 0.5;

    cura52::parallel_for<size_t>(application, 0,
                               layers.size(),
                               [&mesh, &layers, layer_apply_initial_xy_offset, xy_offset, xy_offset_0, offset_rectify](size_t layer_nr)
                               {
                                   const coord_t xy_offset_local = (layer_nr <= layer_apply_initial_xy_offset) ? xy_offset_0 - offset_rectify : xy_offset - offset_rectify;
                                   if (xy_offset_local != 0)
                                   {
                                    //    layers[layer_nr].polygons = layers[layer_nr].polygons.offset(xy_offset_local, ClipperLib::JoinType::jtRound);
                                   }
                               });

    mesh.expandXY(xy_offset);
}

std::vector<std::pair<int32_t, int32_t>> Slicer::buildZHeightsForFaces(const Mesh& mesh)
{
    std::vector<std::pair<int32_t, int32_t>> zHeights;
    zHeights.reserve(mesh.faces.size());
    for (const auto& face : mesh.faces)
    {
        // const MeshFace& face = mesh.faces[mesh_idx];
        const MeshVertex& v0 = mesh.vertices[face.vertex_index[0]];
        const MeshVertex& v1 = mesh.vertices[face.vertex_index[1]];
        const MeshVertex& v2 = mesh.vertices[face.vertex_index[2]];

        // get all vertices represented as 3D point
        Point3 p0 = v0.p;
        Point3 p1 = v1.p;
        Point3 p2 = v2.p;

        // find the minimum and maximum z point
        int32_t minZ = p0.z;
        if (p1.z < minZ)
        {
            minZ = p1.z;
        }
        if (p2.z < minZ)
        {
            minZ = p2.z;
        }

        int32_t maxZ = p0.z;
        if (p1.z > maxZ)
        {
            maxZ = p1.z;
        }
        if (p2.z > maxZ)
        {
            maxZ = p2.z;
        }

        zHeights.emplace_back(std::make_pair(minZ, maxZ));
    }

    return zHeights;
}


SlicerSegment Slicer::project2D(const Point3& p0, const Point3& p1, const Point3& p2, const coord_t z, int color)
{
    SlicerSegment seg;

    seg.start.X = interpolate(z, p0.z, p1.z, p0.x, p1.x);
    seg.start.Y = interpolate(z, p0.z, p1.z, p0.y, p1.y);
    seg.end.X = interpolate(z, p0.z, p2.z, p0.x, p2.x);
    seg.end.Y = interpolate(z, p0.z, p2.z, p0.y, p2.y);
    seg.color = color;

    return seg;
}

SlicerSegment Slicer::project2D(const Point3& p0, const Point3& p1, const Point3& p2, const coord_t z)
{
    SlicerSegment seg;

    seg.start.X = interpolate(z, p0.z, p1.z, p0.x, p1.x);
    seg.start.Y = interpolate(z, p0.z, p1.z, p0.y, p1.y);
    seg.end.X = interpolate(z, p0.z, p2.z, p0.x, p2.x);
    seg.end.Y = interpolate(z, p0.z, p2.z, p0.y, p2.y);

    return seg;
}

coord_t Slicer::interpolate(const coord_t x, const coord_t x0, const coord_t x1, const coord_t y0, const coord_t y1)
{
    const coord_t dx_01 = x1 - x0;
    coord_t num = (y1 - y0) * (x - x0);
    num += num > 0 ? dx_01 / 4 : -dx_01 / 4; // add in offset to round result
    return y0 + num / dx_01;
}

} // namespace cura52
