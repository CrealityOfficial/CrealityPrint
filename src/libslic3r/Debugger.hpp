#ifndef slic3r_Debugger_hpp_
#define slic3r_Debugger_hpp_

#include "libslic3r/Print.hpp"
#include "libslic3r/support_new/TreeSupport.hpp"
#include "libslic3r/SurfaceCollection.hpp"
#include "libslic3r/GCode/SeamPlacer.hpp"


namespace Slic3r {

	class Debugger
	{
	public:
		virtual ~Debugger() {}

		virtual void volume_slices(Print* print, PrintObject* object, const std::vector<VolumeSlices>& slices, const std::vector<float>& slice_zs) = 0;
		virtual void raw_lslices(Print* print, PrintObject* object) = 0;

		virtual void begin_concial_overhang(Print* print, PrintObject* object) = 0;
		virtual void end_concial_overhang(Print* print, PrintObject* object) = 0;

		virtual void begin_hole_to_polyhole(Print* print, PrintObject* object) = 0;
		virtual void end_hole_to_polyhole(Print* print, PrintObject* object) = 0;

		virtual void perimeters(Print* print, PrintObject* object) = 0;
		virtual void curled_extrusions(Print* print, PrintObject* object) = 0;

		virtual void surfaces_type(Print* print, PrintObject* object) = 0;
		virtual void infill_surfaces_type(Print* print, PrintObject* object, int step) = 0;

		virtual void infills(Print* print, PrintObject* object) = 0;

		virtual void seamplacer(Print* print, SeamPlacer* placer) = 0;
		virtual void gcode(const std::string& name, int layer, bool by_object, const std::string& gcode) = 0;
	};

    ////////////////////using immediate window for debug////////////////////////////
    void to_svg(const char* path, const ExPolygons& expolygons, bool fill = true);
    void to_svg(const char* path, const ExPolygon& expolygon, bool fill = true);
    void to_svg(const char* path, const ExPolygons& expolys0, const ExPolygons& expolys1, bool fill = true);
    void to_svg(const char* path, const ExPolygon& expoly0, const ExPolygon& expoly1, bool fill = true);
    void to_svg(const char* path, const ExPolygons& expolys0, const ExPolygon& expoly, bool fill = true);
    void to_svg(const char* path, const ExPolygon& expoly, const ExPolygons& expolys1, bool fill = true);

    void to_svg(const char* path, const ExPolygon& expoly, const BoundingBox& bbox, bool fill = true);
    void to_svg(const char* path, const ExPolygons& expolys, const BoundingBox& bbox, bool fill = true);
    
    void to_svg(const char* path, const ExPolygons& expolys, const Polylines& polylines, bool fill = true);

    void to_svg(const char* path, const ExPolygon& expoly, const Point& pt0, bool fill = true);
    void to_svg(const char* path, const ExPolygon& expoly, const Point& pt0, const Point& pt1, bool fill = true);
    void to_svg(const char* path, const ExPolygon& expoly, const Point& pt0, const Point& pt1, const Point& pt2, bool fill = true);

    void to_svg(const char* path, const ExPolygons& expolys, const Point& pt0, bool fill = true);
    void to_svg(const char* path, const ExPolygons& expolys, const Point& pt0, const Point& pt1, bool fill = true);
    void to_svg(const char* path, const ExPolygons& expolys, const Point& pt0, const Point& pt1, const Point& pt2, bool fill = true);
    void to_svg(const char* path, const ExPolygons& expolys, const Point& pt0, const Point& pt1, const Point& pt2, const Point& pt3, bool fill = true);
    void to_svg(const char* path, const ExPolygons& expolys, const Point& pt0, const Point& pt1, const Point& pt2, const Point& pt3, const Point& pt4, bool fill = true);
    
    void to_svg(const char* path, const ExPolygons& expolys0, const ExPolygons& expolys1, const Point& pt0, const Point& pt1, bool fill = true);
    void to_svg(const char* path, const ExPolygons& expolys0, const ExPolygons& expolys1, const Point& pt0, const Point& pt1, const Point& pt2, bool fill = true);
    void to_svg(const char* path, const ExPolygons& expolys0, const ExPolygons& expolys1, const Point& pt0, const Point& pt1, const Point& pt2, const Point& pt3, bool fill = true);
    void to_svg(const char* path, const ExPolygons& expolys0, const ExPolygons& expolys1, const Point& pt0, const Point& pt1, const Point& pt2, const Point& pt3, const Point& pt4, bool fill = true);

    void to_svg(const char* path, const ExPolygons& overhangs, const std::vector<SupportNode*>& layer_nodes, bool fill = true);
    void to_svg(const char* path, const ExPolygons& overhangs, const ExPolygons& outline, const std::vector<SupportNode*>& layer_nodes, bool fill = true);
    void to_svg(const char* path, const ExPolygons& overhangs,const ExPolygons& avoidance,const ExPolygons& collision, const std::vector<SupportNode*>& origin_nodes,const std::vector<SupportNode*>& move_nodes, bool fill = true);



    void to_svg(const char* dir, const std::vector<ExPolygons>& expolyss, bool fill = true);

    //------------------------------------------------------------------------------
    void to_svg(const char* path, const Polygon& polygon, bool fill = true);
    void to_svg(const char* path, const Polygon& poly0, const Polygon& poly1, bool fill = true);
    void to_svg(const char* path, const Polygon& poly0, const ExPolygon& expoly1, bool fill = true);
    void to_svg(const char* path, const Polygon& poly, const Polylines& polylines, bool fill = true);
    void to_svg(const char* path, const Polygon& poly, const BoundingBox& bbox, bool fill = true);
    void to_svg(const char* path, const Polygons& polygons, bool fill = true);
    void to_svg(const char* path, const Polygons& polys0, const Polygons& polys1, bool fill = true);
    void to_svg(const char* path, const Polygons& polys, const Polylines& polylines, bool fill = true);
    void to_svg(const char* path, const Polygons& polys, const BoundingBox& bbox, bool fill = true);
    void to_svg(const char* path, const Polygons& polys, const ExPolygon& expoly, bool fill = true);
    void to_svg(const char* path, const Polygons& polys, const ExPolygons& expoly, bool fill = true);
    void to_svg(const char* path, const Polygons& polys, const Point& pt, bool fill = true);
    void to_svg(const char* path, const Polygons& polys, const Point& pt0, const Point& pt1, bool fill = true);
    void to_svg(const char* path, const Polygons& polys, const Point& pt0, const Point& pt1, const Point& pt2, bool fill = true);

    void to_svg(const char* path, const Polygons& polys0, const Polygons& polys1, const Polygons& polys2, bool fill = true);
    void to_svg(const char* dir, const std::vector<Polygons>& polyss, bool fill = true);

    //------------------------------------------------------------------------------
    void to_svg(const char* path, const Polyline& polyline);
    void to_svg(const char* path, const Polyline& polyline0, const Polyline& polyline1);
    void to_svg(const char* path, const Polylines& polylines);
    void to_svg(const char* dir, const std::vector<Polylines>& polyliness);

    //------------------------------------------------------------------------------
    void to_svg(const char* path, const SurfaceCollection& surfacs);

    //------------------------------------------------------------------------------
    void to_obj(const char* path, const indexed_triangle_set& its);
    void to_obj(const char* path, const std::vector<indexed_triangle_set>& itss);
    void to_obj(const char* dir, const std::vector<std::vector<indexed_triangle_set>>& itsss);

    void to_obj(const char* path, const TriangleMesh& tm);
    void to_obj(const char* path, const std::vector<TriangleMesh>& tms);
    void to_obj(const char* dir, const std::vector<std::vector<TriangleMesh>>& tmss, bool sperate_file = false);

    ///////////////////////////////////////////////////////////////////////////////

}

#endif
