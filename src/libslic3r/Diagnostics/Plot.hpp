#ifndef slic3r_Diagnostics_Plot_hpp_
#define slic3r_Diagnostics_Plot_hpp_

#ifdef _WIN32
#include "../ExPolygon.hpp"
#include "../Line.hpp"

#include <vector>

struct indexed_triangle_set;

namespace Slic3r {
struct SupportNode;
}

namespace Slic3r::Diagnostics {

    // Geometry viewers for manual debugging. color: 0=red, 1=green, otherwise blue.
    __declspec(dllexport) void plot_mesh(const char* name, const indexed_triangle_set& its);

    __declspec(dllexport) void plot_pt(const char* name, const std::vector<SupportNode*>& nodes, int color);
    __declspec(dllexport) void plot_pt(const char* name, const Point& pt, int color);
    __declspec(dllexport) void plot_pt(const char* name, const std::vector<Point>& pts, int color);
    __declspec(dllexport) void plot_pt(const char* name, const ExPolygons& expolys, int color);

    __declspec(dllexport) void plot_polyline(const char* name, const Line& line, int color);
    __declspec(dllexport) void plot_polyline(const char* name, const Lines& lines, int color);
    __declspec(dllexport) void plot_polyline(const char* name, const Polygon& polygon, int color);
    __declspec(dllexport) void plot_polyline(const char* name, const Polygons& polygons, int color);
    __declspec(dllexport) void plot_polyline(const char* name, const ExPolygon& expoly, int color);
    __declspec(dllexport) void plot_polyline(const char* name, const ExPolygons& expolys, int color);
    __declspec(dllexport) void plot_polyline(const char* name, const std::vector<ExPolygons>& expolys, double z_start = 0.1, double layer_height = 0.2, int color = 0);

} // namespace Slic3r::Diagnostics
#endif

#endif
