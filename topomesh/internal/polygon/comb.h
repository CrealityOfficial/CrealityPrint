#ifndef TOPOMESH_COMB_1694153038685_H
#define TOPOMESH_COMB_1694153038685_H
#include <vector>
#include <map>
#include <clipper/clipper.hpp>

#include "internal/polygon/polygon.h"
#include "internal/polygon/aabb.h"
#include "internal/polygon/svg.h"
#include "topomesh/interface/idata.h"

namespace topomesh
{
    Polygons SaveTriPolygonToPolygons(const TriPolygon& poly, double resolution = 1E-4);
    Polygons SaveTriPolygonsToPolygons(const TriPolygons& polys, double resolution = 1E-4);
	std::vector<std::map<int, int>> GetHexagonEdgeMap(const Polygons& polygons, const ClipperLib::Path& path, double radius, double resolution = 1E-4);
}

#endif // TOPOMESH_COMB_1694153038685_H