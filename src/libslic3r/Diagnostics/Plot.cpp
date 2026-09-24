#include "Plot.hpp"

#ifdef _WIN32
#include "../GeometrySender.h"
#include "../TriangleMesh.hpp"
#include "../support_new/TreeSupport.hpp"

// Force this static-library object into the DLL; never called at runtime.
extern "C" void slic3r_diagnostics_plot_anchor() {}
#endif

namespace Slic3r::Diagnostics{

#ifdef _WIN32
    void ensure_config()
    {
        static std::string host = "192.168.10.47";
        static std::uint16_t port = 45454;
        static bool init = false;

        if (init)
        {
            return;
        }

        init = GeometryDebugerSender::configure(host, port);
    }

    std::vector<GeometryDebugerSender::Polyline> to_pl(const ExPolygons& expolys, double z = 0.0)
    {
        std::vector<GeometryDebugerSender::Polyline> pls;

        for (const ExPolygon& expolygon : expolys)
        {
            GeometryDebugerSender::Polyline pl;
            for (auto& pt : expolygon.contour.points)
            {
                GeometryDebugerSender::Point3 point;

                point.x = unscaled(pt.x());
                point.y = unscaled(pt.y());
                point.z = z;
                pl.push_back(point);
            }

            pls.push_back(pl);

            for (const Polygon& hole : expolygon.holes)
            {
                GeometryDebugerSender::Polyline pl;
                for (auto& pt : hole.points)
                {
                    GeometryDebugerSender::Point3 point;

                    point.x = unscaled(pt.x());
                    point.y = unscaled(pt.y());
                    point.z = z;
                    pl.push_back(point);
                }

                pls.push_back(pl);
            }
        }

        return pls;
    }


    void plot_mesh(const char* name, const indexed_triangle_set& its)
    {
        ensure_config();

        GeometryDebugerSender::TriangleSoupGroup tsg;
        std::vector<GeometryDebugerSender::Color> colors;

        GeometryDebugerSender::TriangleSoup ts;

        for (int i = 0; i < its.indices.size(); i++)
        {
            stl_triangle_vertex_indices tri = its.indices[i];
            GeometryDebugerSender::TriangleFacet facet;
            facet[0].x = its.vertices[tri[0]][0];
            facet[0].y = its.vertices[tri[0]][1];
            facet[0].z = its.vertices[tri[0]][2];

            facet[1].x = its.vertices[tri[1]][0];
            facet[1].y = its.vertices[tri[1]][1];
            facet[1].z = its.vertices[tri[1]][2];

            facet[2].x = its.vertices[tri[2]][0];
            facet[2].y = its.vertices[tri[2]][1];
            facet[2].z = its.vertices[tri[2]][2];

            ts.push_back(facet);
        }

        tsg.push_back(ts);
        colors.push_back({ 0,0,255 });
        GeometryDebugerSender::sendTriangleSoupGroupObject(name, tsg, colors);

    }

    void plot_pt(const char* name, const std::vector<SupportNode*>& nodes, int color)
    {
        ensure_config();

        GeometryDebugerSender::PointSetGroup point_set_group;
        std::vector<GeometryDebugerSender::Color> colors;

        GeometryDebugerSender::PointSet ps;
        for (auto& node : nodes)
        {
            GeometryDebugerSender::Point3 point;

            point.x = unscaled(node->position.x());
            point.y = unscaled(node->position.y());
            point.z = 0.0;
            ps.push_back(point);
        }
        point_set_group.push_back(ps);

        if (color == 0)
        {
            colors.push_back({ 255, 0, 0 });
        }
        else if (color == 1)
        {
            colors.push_back({ 0, 255, 0 });
        }
        else
        {
            colors.push_back({ 0, 0, 255 });
        }


        GeometryDebugerSender::sendPointSetGroupObject(name, point_set_group, colors);
    }

    void plot_pt(const char* name, const Point& pt, int color)
    {
        ensure_config();

        GeometryDebugerSender::PointSetGroup point_set_group;
        std::vector<GeometryDebugerSender::Color> colors;

        GeometryDebugerSender::PointSet ps;
        GeometryDebugerSender::Point3 point;
        point.x = unscaled(pt.x());
        point.y = unscaled(pt.y());
        point.z = 0.0;
        ps.push_back(point);

        point_set_group.push_back(ps);

        if (color == 0)
        {
            colors.push_back({ 255, 0, 0 });
        }
        else if (color == 1)
        {
            colors.push_back({ 0, 255, 0 });
        }
        else
        {
            colors.push_back({ 0, 0, 255 });
        }


        GeometryDebugerSender::sendPointSetGroupObject(name, point_set_group, colors);
    }

    void plot_pt(const char* name, const std::vector<Point>& pts, int color)
    {
        ensure_config();

        GeometryDebugerSender::PointSetGroup point_set_group;
        std::vector<GeometryDebugerSender::Color> colors;

        GeometryDebugerSender::PointSet ps;
        for (auto& pt : pts)
        {
            GeometryDebugerSender::Point3 point;

            point.x = unscaled(pt.x());
            point.y = unscaled(pt.y());
            point.z = 0.0;
            ps.push_back(point);
        }
        point_set_group.push_back(ps);

        if (color == 0)
        {
            colors.push_back({ 255, 0, 0 });
        }
        else if (color == 1)
        {
            colors.push_back({ 0, 255, 0 });
        }
        else
        {
            colors.push_back({ 0, 0, 255 });
        }


        GeometryDebugerSender::sendPointSetGroupObject(name, point_set_group, colors);
    }

    void plot_pt(const char* name, const ExPolygons& expolys, int color)
    {
        ensure_config();

        Points pts = to_points(expolys);

        GeometryDebugerSender::PointSetGroup point_set_group;
        std::vector<GeometryDebugerSender::Color> colors;

        GeometryDebugerSender::PointSet ps;
        for (auto& pt : pts)
        {
            GeometryDebugerSender::Point3 point;

            point.x = unscaled(pt.x());
            point.y = unscaled(pt.y());
            point.z = 0.0;
            ps.push_back(point);
        }
        point_set_group.push_back(ps);

        if (color == 0)
        {
            colors.push_back({ 255, 0, 0 });
        }
        else if (color == 1)
        {
            colors.push_back({ 0, 255, 0 });
        }
        else
        {
            colors.push_back({ 0, 0, 255 });
        }


        GeometryDebugerSender::sendPointSetGroupObject(name, point_set_group, colors);
    }

    void plot_polyline(const char* name, const Line& line, int color)
    {
        ensure_config();

        GeometryDebugerSender::Color color_;
        if (color == 0)
        {
            color_ = { 255, 0, 0 };
        }
        else if (color == 1)
        {
            color_ = { 0, 255, 0 };
        }
        else
        {
            color_ = { 0, 0, 255 };
        }

        GeometryDebugerSender::PolylineGroup plg;
        std::vector<GeometryDebugerSender::Color> colors;

        {
            GeometryDebugerSender::Polyline pl;

            {
                GeometryDebugerSender::Point3 point;

                point.x = unscaled(line.a.x());
                point.y = unscaled(line.b.y());
                point.z = 0.0;
                pl.push_back(point);
            }

            plg.push_back(pl);
            colors.push_back(color_);
        }

        GeometryDebugerSender::sendPolylineGroupObject(name, plg, colors);
    }

    void plot_polyline(const char* name, const Lines& lines, int color)
    {
        ensure_config();

        GeometryDebugerSender::Color color_;
        if (color == 0)
        {
            color_ = { 255, 0, 0 };
        }
        else if (color == 1)
        {
            color_ = { 0, 255, 0 };
        }
        else
        {
            color_ = { 0, 0, 255 };
        }

        GeometryDebugerSender::PolylineGroup plg;
        std::vector<GeometryDebugerSender::Color> colors;

        for (auto &line : lines)
        {
            GeometryDebugerSender::Polyline pl;

            {
                GeometryDebugerSender::Point3 point;

                point.x = unscaled(line.a.x());
                point.y = unscaled(line.b.y());
                point.z = 0.0;
                pl.push_back(point);
            }

            plg.push_back(pl);
            colors.push_back(color_);
        }

        GeometryDebugerSender::sendPolylineGroupObject(name, plg, colors);
    }

    void plot_polyline(const char* name, const Polygon& polygon, int color)
    {
        ensure_config();

        GeometryDebugerSender::Color color_;
        if (color == 0)
        {
            color_ = { 255, 0, 0 };
        }
        else if (color == 1)
        {
            color_ = { 0, 255, 0 };
        }
        else
        {
            color_ = { 0, 0, 255 };
        }

        GeometryDebugerSender::PolylineGroup plg;
        std::vector<GeometryDebugerSender::Color> colors;

        {
            GeometryDebugerSender::Polyline pl;
            for (auto& pt : polygon.points)
            {
                GeometryDebugerSender::Point3 point;

                point.x = unscaled(pt.x());
                point.y = unscaled(pt.y());
                point.z = 0.0;
                pl.push_back(point);
            }
            //the last segment
            {
                GeometryDebugerSender::Point3 point;
                point.x = unscaled(polygon.points[0].x());
                point.y = unscaled(polygon.points[0].y());
                point.z = 0.0;
                pl.push_back(point);
            }

            plg.push_back(pl);
            colors.push_back(color_);
        }

        GeometryDebugerSender::sendPolylineGroupObject(name, plg, colors);
    }

    void plot_polyline(const char* name, const Polygons& polygons, int color)
    {
        ensure_config();

        GeometryDebugerSender::Color color_;
        if (color == 0)
        {
            color_ = { 255, 0, 0 };
        }
        else if (color == 1)
        {
            color_ = { 0, 255, 0 };
        }
        else
        {
            color_ = { 0, 0, 255 };
        }

        GeometryDebugerSender::PolylineGroup plg;
        std::vector<GeometryDebugerSender::Color> colors;
        for (auto &polygon : polygons)
        {
            GeometryDebugerSender::Polyline pl;
            for (auto& pt : polygon.points)
            {
                GeometryDebugerSender::Point3 point;

                point.x = unscaled(pt.x());
                point.y = unscaled(pt.y());
                point.z = 0.0;
                pl.push_back(point);
            }
            //the last segment
            {
                GeometryDebugerSender::Point3 point;
                point.x = unscaled(polygon.points[0].x());
                point.y = unscaled(polygon.points[0].y());
                point.z = 0.0;
                pl.push_back(point);
            }

            plg.push_back(pl);
            colors.push_back(color_);
        }

        GeometryDebugerSender::sendPolylineGroupObject(name, plg, colors);
    }

    void plot_polyline(const char* name, const ExPolygon& expoly, int color)
    {
        ensure_config();

        GeometryDebugerSender::Color color_;
        if (color == 0)
        {
            color_ = { 255, 0, 0 };
        }
        else if (color == 1)
        {
            color_ = { 0, 255, 0 };
        }
        else
        {
            color_ = { 0, 0, 255 };
        }

        GeometryDebugerSender::PolylineGroup plg;
        std::vector<GeometryDebugerSender::Color> colors;

        {
            GeometryDebugerSender::Polyline pl;
            for (auto& pt : expoly.contour.points)
            {
                GeometryDebugerSender::Point3 point;

                point.x = unscaled(pt.x());
                point.y = unscaled(pt.y());
                point.z = 0.0;
                pl.push_back(point);
            }
            //the last segment
            {
                GeometryDebugerSender::Point3 point;
                point.x = unscaled(expoly.contour.points[0].x());
                point.y = unscaled(expoly.contour.points[0].y());
                point.z = 0.0;
                pl.push_back(point);
            }


            plg.push_back(pl);
            colors.push_back(color_);

            for (const Polygon& hole : expoly.holes)
            {
                GeometryDebugerSender::Polyline pl;
                for (auto& pt : hole.points)
                {
                    GeometryDebugerSender::Point3 point;

                    point.x = unscaled(pt.x());
                    point.y = unscaled(pt.y());
                    point.z = 0.0;
                    pl.push_back(point);
                }
                //the last segment
                {
                    GeometryDebugerSender::Point3 point;
                    point.x = unscaled(hole.points[0].x());
                    point.y = unscaled(hole.points[0].y());
                    point.z = 0.0;
                    pl.push_back(point);
                }

                plg.push_back(pl);
                colors.push_back(color_);
            }
        }

        GeometryDebugerSender::sendPolylineGroupObject(name, plg, colors);
    }

    void plot_polyline(const char* name, const ExPolygons& expolys, int color)
    {
        ensure_config();

        GeometryDebugerSender::Color color_;
        if (color == 0)
        {
            color_ = { 255, 0, 0 };
        }
        else if (color == 1)
        {
            color_ = { 0, 255, 0 };
        }
        else
        {
            color_ = { 0, 0, 255 };
        }

        GeometryDebugerSender::PolylineGroup plg;
        std::vector<GeometryDebugerSender::Color> colors;

        for (const ExPolygon& expolygon : expolys)
        {
            GeometryDebugerSender::Polyline pl;
            for (auto &pt : expolygon.contour.points)
            {
                GeometryDebugerSender::Point3 point;

                point.x = unscaled(pt.x());
                point.y = unscaled(pt.y());
                point.z = 0.0;
                pl.push_back(point);
            }
            //the last segment
            {
                GeometryDebugerSender::Point3 point;
                point.x = unscaled(expolygon.contour.points[0].x());
                point.y = unscaled(expolygon.contour.points[0].y());
                point.z = 0.0;
                pl.push_back(point);
            }


            plg.push_back(pl);
            colors.push_back(color_);

            for (const Polygon& hole : expolygon.holes)
            {
                GeometryDebugerSender::Polyline pl;
                for (auto& pt : hole.points)
                {
                    GeometryDebugerSender::Point3 point;

                    point.x = unscaled(pt.x());
                    point.y = unscaled(pt.y());
                    point.z = 0.0;
                    pl.push_back(point);
                }
                //the last segment
                {
                    GeometryDebugerSender::Point3 point;
                    point.x = unscaled(hole.points[0].x());
                    point.y = unscaled(hole.points[0].y());
                    point.z = 0.0;
                    pl.push_back(point);
                }

                plg.push_back(pl);
                colors.push_back(color_);
            }
        }

        GeometryDebugerSender::sendPolylineGroupObject(name, plg, colors);
    }

    void plot_polyline(const char* name, const std::vector<ExPolygons>& expolys, double z_start/* = 0.1*/, double layer_height/* = 0.2*/, int color/* = 0*/)
    {
        ensure_config();

        GeometryDebugerSender::Color color_;
        if (color == 0)
        {
            color_ = { 255, 0, 0 };
        }
        else if (color == 1)
        {
            color_ = { 0, 255, 0 };
        }
        else
        {
            color_ = { 0, 0, 255 };
        }

        GeometryDebugerSender::PolylineGroup plg;
        std::vector<GeometryDebugerSender::Color> colors;

        for (int i = 0; i < expolys.size(); ++i)
        {
            if (expolys[i].empty())
            {
                continue;
            }

            std::vector<GeometryDebugerSender::Polyline> ps = to_pl(expolys[i], i * layer_height + z_start);

            for (int j = 0; j < ps.size(); ++j)
            {
                plg.push_back(ps[j]);
                colors.push_back(color_);
            }
        }

        GeometryDebugerSender::sendPolylineGroupObject(name, plg, colors);
    }


#endif

//     void debug_view_pl(const char* name, const IntersectionLine& il, double z, int color)
//     {
//         ensure_config();
//
//         GeometryDebugerSender::Color color_;
//         if (color == 0)
//         {
//             color_ = { 255, 0, 0 };
//         }
//         else if (color == 1)
//         {
//             color_ = { 0, 255, 0 };
//         }
//         else
//         {
//             color_ = { 0, 0, 255 };
//         }
//
//         GeometryDebugerSender::PolylineGroup plg;
//         std::vector<GeometryDebugerSender::Color> colors;
//
//         GeometryDebugerSender::Polyline pl;
//
//         GeometryDebugerSender::Point3 point0;
//         point0.x = unscaled(il.b[0]);
//         point0.y = unscaled(il.b[1]);
//         point0.z = z;
//         pl.push_back(point0);
//
//         GeometryDebugerSender::Point3 point1;
//         point1.x = unscaled(il.a[0]);
//         point1.y = unscaled(il.a[1]);
//         point1.z = z;
//         pl.push_back(point1);
//
//         plg.push_back(pl);
//         colors.push_back(color_);
//
//         GeometryDebugerSender::sendPolylineGroupObject(name, plg, colors);
//     }
//
//     void debug_view_pl(const char* name, const IntersectionLines& ils, double z, int color)
//     {
//         ensure_config();
//
//         GeometryDebugerSender::Color color_;
//         if (color == 0)
//         {
//             color_ = { 255, 0, 0 };
//         }
//         else if (color == 1)
//         {
//             color_ = { 0, 255, 0 };
//         }
//         else
//         {
//             color_ = { 0, 0, 255 };
//         }
//
//         GeometryDebugerSender::PolylineGroup plg;
//         std::vector<GeometryDebugerSender::Color> colors;
//
//         for (int i = 0; i < ils.size(); i++)
//         {
//             GeometryDebugerSender::Polyline pl;
//
//             GeometryDebugerSender::Point3 point0;
//             point0.x = unscaled(ils[i].b[0]);
//             point0.y = unscaled(ils[i].b[1]);
//             point0.z = z;
//             pl.push_back(point0);
//
//             GeometryDebugerSender::Point3 point1;
//             point1.x = unscaled(ils[i].a[0]);
//             point1.y = unscaled(ils[i].a[1]);
//             point1.z = z;
//             pl.push_back(point1);
//
//             plg.push_back(pl);
//             colors.push_back(color_);
//         }
//
//         GeometryDebugerSender::sendPolylineGroupObject(name, plg, colors);
//     }
//
//     void debug_view_pl(const char* name, const std::vector<IntersectionLines>& ilss, const std::vector<float> &zs, int color)
//     {
//         ensure_config();
//
//         GeometryDebugerSender::Color color_;
//         if (color == 0)
//         {
//             color_ = { 255, 0, 0 };
//         }
//         else if (color == 1)
//         {
//             color_ = { 0, 255, 0 };
//         }
//         else
//         {
//             color_ = { 0, 0, 255 };
//         }
//
//         GeometryDebugerSender::PolylineGroup plg;
//         std::vector<GeometryDebugerSender::Color> colors;
//
//         for (int i = 0; i < ilss.size(); i++)
//         {
//             for (int j = 0; j < ilss[i].size(); j++)
//             {
//                 GeometryDebugerSender::Polyline pl;
//
//                 GeometryDebugerSender::Point3 point0;
//                 point0.x = unscaled(ilss[i][j].b[0]);
//                 point0.y = unscaled(ilss[i][j].b[1]);
//                 point0.z = zs[i];
//                 pl.push_back(point0);
//
//                 GeometryDebugerSender::Point3 point1;
//                 point1.x = unscaled(ilss[i][j].a[0]);
//                 point1.y = unscaled(ilss[i][j].a[1]);
//                 point1.z = zs[i];
//                 pl.push_back(point1);
//
//                 plg.push_back(pl);
//                 colors.push_back(color_);
//             }
//         }
//
//         GeometryDebugerSender::sendPolylineGroupObject(name, plg, colors);
//     }

} // namespace Slic3r::Diagnostics
