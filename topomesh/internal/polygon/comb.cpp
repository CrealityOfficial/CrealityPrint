#include "comb.h"

namespace topomesh
{
    Polygons SaveTriPolygonToPolygons(const TriPolygon& poly, double resolution)
    {
        Polygons polygons;
        ClipperLib::Path path;
        path.reserve(poly.size());
        for (const auto& p : poly) {
            const auto& x = std::round(p.x / resolution);
            const auto& y = std::round(p.y / resolution);
            path.emplace_back(ClipperLib::IntPoint(x, y));
        }
        polygons.paths.emplace_back(path);
        return polygons;
    }

    Polygons SaveTriPolygonsToPolygons(const TriPolygons & polys, double resolution)
    {
        Polygons polygons;
        polygons.paths.reserve(polys.size());
        for (const auto& poly : polys) {
            ClipperLib::Path path;
            path.reserve(poly.size());
            for (const auto& p : poly) {
                const auto& x = std::round(p.x / resolution);
                const auto& y = std::round(p.y / resolution);
                path.emplace_back(ClipperLib::IntPoint(x, y));
            }
            polygons.paths.emplace_back(path);
        }
        return polygons;
    }

    std::vector<std::map<int, int>> GetHexagonEdgeMap(const Polygons& polygons, const ClipperLib::Path& path, double radius, double resolution)
    {
        ClipperLib::IntPoint center;
        for (const auto& p : path) {
            center.X += p.X;
            center.Y += p.Y;
        }
        const int side = std::round(radius / resolution);
        center.X = std::round((double)center.X / (double)path.size());
        center.Y = std::round((double)center.Y / (double)path.size());
        Polygons hexaPoly;
        hexaPoly.paths.emplace_back(path);
        std::vector<std::map<int, int>> edgemaps;
        std::map<int, int> p2hpmap, h2ppmap;
        std::map<int, int> p2hemap, h2pemap;
        for (const auto& tempPath : polygons.paths) {
            const int len = tempPath.size();
            std::vector<bool> insides(len);
            std::vector<int> dists(len);

            for (int i = 0; i < len; ++i) {
                const auto& a = tempPath[i];
                insides[i] = hexaPoly.inside(a);
                dists[i] = std::round(std::sqrt((a.X - center.X) * (a.X - center.X) + (a.Y - center.Y) * (a.Y - center.Y)));
            }
            for (int i = 0; i < len; ++i) {
                if (insides[i]) p2hpmap.emplace(i, -1);
                else {
                    const int d1 = dists[i];
                    if (std::fabs(d1 - side) <= 1) {
                        const auto& a = tempPath[i];
                        const double theta = std::atan2(a.Y - center.Y, a.X - center.X);
                        const int inx = std::round((theta > 0 ? theta / M_PI * 3.0 : (theta + M_PI * 2.0) / M_PI * 3.0));
                        p2hpmap.emplace(i, inx % 6);
                        h2ppmap.emplace(inx % 6, i);
                    } else {
                        p2hpmap.emplace(i, -1);
                    }
                }
            }
            for (int i = 0; i < len; ++i) {
                if (insides[i] || insides[(i + 1) % len]) {
                    p2hemap.emplace(i, -1);
                } else {
                    const int d1 = dists[i];
                    if (std::fabs(d1 - side) <= 1) {
                        const int d2 = dists[(i + 1) % len];
                        if (std::fabs(d2 - side) <= 1) {
                            const auto& a = tempPath[i];
                            const double theta = std::atan2(a.Y - center.Y, a.X - center.X);
                            const int inx = std::round((theta > 0 ? theta / M_PI * 3.0 : (theta + M_PI * 2.0) / M_PI * 3.0));
                            p2hemap.emplace(i, inx % 6);
                            h2pemap.emplace(inx % 6, i);
                        } else {
                            p2hemap.emplace(i, -1);
                        }
                    } else {
                        p2hemap.emplace(i, -1);
                    }
                }
            }
        }
        edgemaps.reserve(4);
        edgemaps.emplace_back(p2hpmap);
        edgemaps.emplace_back(h2ppmap);
        edgemaps.emplace_back(p2hemap);
        edgemaps.emplace_back(h2pemap);
        return edgemaps;
    }
}