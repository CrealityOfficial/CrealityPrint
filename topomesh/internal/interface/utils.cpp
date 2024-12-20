#include "topomesh/interface/utils.h"
#include "internal/alg/earclipping.h"
#include "internal/data/CMesh.h"
#include "msbase/mesh/get.h"
#include <queue>
#include "internal/polygon/polygon.h"
#include <clipper/clipper.hpp>
#include "internal/polygon/svg.h"
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <functional>
#include <vector>

namespace topomesh
{
	TriPolygons GetOpenMeshBoundarys(trimesh::TriMesh* triMesh)
	{
		CMesh mesh(triMesh);
		TriPolygons polys;
		std::vector<int> edges;
		mesh.SelectIndividualEdges(edges);
		//CMesh edgeMesh = mesh.SaveEdgesToMesh(edges);
		//edgeMesh.WriteSTLFile("edges.stl");
		//第0步，底面轮廓边界所有边排序
		std::vector<std::vector<int>>sequentials;
        if (!mesh.GetSequentialPoints(edges, sequentials))
            return polys;
		//第1步，构建底面边界轮廓多边形
		polys.reserve(sequentials.size());
		const auto& points = mesh.mpoints;
		for (const auto& seq : sequentials) {
			TriPolygon border;
			border.reserve(seq.size());
			for (const auto& v : seq) {
				const auto& p = points[v];
				border.emplace_back(p);
			}
			polys.emplace_back(border);
		}
		return polys;
	}

	void findNeignborFacesOfSameAsNormal(trimesh::TriMesh* trimesh, int indicate, float angle_threshold, std::vector<int>& faceIndexs)
	{
		if (!trimesh)
			return;

		trimesh->need_across_edge();
		faceIndexs.push_back(indicate);		
		trimesh::point normal = msbase::getFaceNormal(trimesh, indicate);
		std::vector<int> vis(trimesh->faces.size(), 0);
		std::queue<int> facequeue;
		facequeue.push(indicate);
		while (!facequeue.empty())
		{
			vis[facequeue.front()] = 1;
			for (int i = 0; i < trimesh->across_edge[facequeue.front()].size(); i++)
			{
				int face = trimesh->across_edge[facequeue.front()][i];
				if (face==-1||vis[face]) continue;				
				trimesh::point nor = msbase::getFaceNormal(trimesh, face);
				float arc = nor ^ normal;
				arc = arc >= 1.f ? 1.f : arc;
				arc = arc <= -1.f ? -1.f : arc;
				float ang = std::acos(arc) * 180 / M_PI;
				if (ang < angle_threshold)
				{
					vis[face] = 1;
					facequeue.push(face);
					faceIndexs.push_back(face);
				}
			}
			facequeue.pop();
		}
	}

	void findBoundary(trimesh::TriMesh* trimesh)
	{

	}

	void triangulate(trimesh::TriMesh* trimesh, std::vector<int>& sequentials)
	{
		std::vector<std::pair<trimesh::point, int>> lines;
		for (int i = 0; i < sequentials.size(); i++)
		{
			lines.push_back(std::make_pair(trimesh->vertices[sequentials[i]], sequentials[i]));
		}
		topomesh::EarClipping earclip(lines);
		std::vector<trimesh::ivec3> result = earclip.getResult();
		for (int fi = 0; fi < result.size(); fi++)
		{
			trimesh->faces.push_back(result[fi]);
		}
	}


    //////////////////////////////////////////////////////////////////////////
#define scale_(val) ((val) * 1000000.0)
#define unscale_(val) ((val) * 0.000001)

    //construct a path of clipper from contour_2d_t(trimesh::dbox2)
    static ClipperLib::Path contour_to_clipper_path(const contour_2d_t& contour)
    {
        ClipperLib::Path out(contour.size());

        auto point_map = [&](tbb::blocked_range<size_t> r)
        {
            for (size_t i = r.begin(); i < r.end(); ++i)
            {
                out[i].X = scale_(contour[i][0]);
                out[i].Y = scale_(contour[i][1]);
            }
        };
        tbb::parallel_for(tbb::blocked_range<size_t>(0, contour.size()), point_map);

        //compiler will optimize this using return value optimization
        return out;
    }

    static ClipperLib::Path contour_to_clipper_path(const std::vector<trimesh::dvec3>& contour)
    {
        ClipperLib::Path out(contour.size());

        auto point_map = [&](tbb::blocked_range<size_t> r)
        {
            for (size_t i = r.begin(); i < r.end(); ++i)
            {
                out[i].X = scale_(contour[i][0]);
                out[i].Y = scale_(contour[i][1]);
            }
        };
        tbb::parallel_for(tbb::blocked_range<size_t>(0, contour.size()), point_map);

        //compiler will optimize this using return value optimization
        return out;
    }

    //construct a contour_2d_t(trimesh::dbox2) from clipper path
    static contour_2d_t clipper_path_to_contour(const ClipperLib::Path& path)
    {
        contour_2d_t out(path.size());

        auto point_map = [&](tbb::blocked_range<size_t> r)
        {
            for (size_t i = r.begin(); i < r.end(); ++i)
            {
                out[i][0] = unscale_(path[i].X);
                out[i][1] = unscale_(path[i].Y);
            }
        };
        tbb::parallel_for(tbb::blocked_range<size_t>(0, path.size()), point_map);

        //compiler will optimize this using return value optimization
        return out;
    }

    bool contour_is_cw(const contour_2d_t& contour)
    {
        double area2 = 0.0;
        size_t size = contour.size();

        std::vector<double> area_triangle(size);

        for (size_t i = 0; i < size; ++i)
        {
            //the next point
            size_t j = (i + 1) % size;

            const auto& a = contour[i];
            const auto& b = contour[j];
            area2 += (a.x * b.y - a.y * b.x);
        }

        //contour is cw when its area is negative
        return area2 < std::numeric_limits<double>::epsilon();
    }

    static double PerpendicularDistance1(const ClipperLib::IntPoint& pt, const ClipperLib::IntPoint& lineStart, const ClipperLib::IntPoint& lineEnd)
    {
        double dx = lineEnd.X - lineStart.X;
        double dy = lineEnd.Y - lineStart.Y;

        //Normalize
        double mag = pow(pow(dx, 2.0) + pow(dy, 2.0), 0.5);
        if (mag > 0.0)
        {
            dx /= mag; dy /= mag;
        }

        double pvx = pt.X - lineStart.X;
        double pvy = pt.Y - lineStart.Y;

        //Get dot product (project pv onto normalized direction)
        double pvdot = dx * pvx + dy * pvy;

        //Scale line direction vector
        double dsx = pvdot * dx;
        double dsy = pvdot * dy;

        //Subtract this from pv
        double ax = pvx - dsx;
        double ay = pvy - dsy;

        return pow(pow(ax, 2.0) + pow(ay, 2.0), 0.5);
    }

    static void RamerDouglasPeucker1(const ClipperLib::Path& pointList, double epsilon, ClipperLib::Path& out)
    {
        if (pointList.size() < 2)
            return;

        // Find the point with the maximum distance from line between start and end
        double dmax = 0.0;
        size_t index = 0;
        size_t end = pointList.size() - 1;
        for (size_t i = 1; i < end; i++)
        {
            double d = PerpendicularDistance1(pointList[i], pointList[0], pointList[end]);
            if (d > dmax)
            {
                index = i;
                dmax = d;
            }
        }

        // If max distance is greater than epsilon, recursively simplify
        if (dmax > epsilon)
        {
            // Recursive call
            ClipperLib::Path recResults1;
            ClipperLib::Path recResults2;
            ClipperLib::Path firstLine(pointList.begin(), pointList.begin() + index + 1);
            ClipperLib::Path lastLine(pointList.begin() + index, pointList.end());
            RamerDouglasPeucker1(firstLine, epsilon, recResults1);
            RamerDouglasPeucker1(lastLine, epsilon, recResults2);

            // Build the result list
            out.assign(recResults1.begin(), recResults1.end() - 1);
            out.insert(out.end(), recResults2.begin(), recResults2.end());
            if (out.size() < 2)
                return;
        }
        else
        {
            //Just return start and end path
            out.clear();
            out.push_back(pointList[0]);
            out.push_back(pointList[end]);
        }
    }

    bool offset_contour(double offset_distance, const contour_2d_t& in, contour_2d_t& out)
    {
        if (in.empty() || std::abs(offset_distance) < std::numeric_limits<double>::epsilon())
        {
            return true;
        }

        //do the offsetting
        ClipperLib::ClipperOffset clip_offset;
        clip_offset.ArcTolerance = scale_(0.1);
        clip_offset.AddPath(contour_to_clipper_path(in), ClipperLib::jtRound, ClipperLib::etClosedPolygon);
        ClipperLib::Paths solution;
        clip_offset.Execute(solution, scale_(offset_distance));

        ClipperLib::Path temp;
        RamerDouglasPeucker1(solution[0], scale_(0.4), temp);

        //take the first path as the offset result
        out = clipper_path_to_contour(temp);

#ifdef DEBUG
        if (0)
        {
            ClipperLib::Path path0 = contour_to_clipper_path(in);
            ClipperLib::Path path1 = contour_to_clipper_path(out);
            PolygonRef poly0(path0);
            PolygonRef poly1(path1);

            AABB aabb(poly0);
            aabb.include(poly1);

            SVG svg("D://test//offsetting.svg", aabb, 0.00001);
            svg.writePolygon(poly0, SVG::Color::RED, 1.0);
            svg.writePolygon(poly1, SVG::Color::GREEN, 1.0);
         }
#endif  DEBUG

        return true;
    }

    bool contour_is_intersect(const contour_2d_t& contour0, const contour_2d_t& contour1)
    {
        ClipperLib::Clipper clipper;
        clipper.AddPath(contour_to_clipper_path(contour0), ClipperLib::ptSubject, true);
        clipper.AddPath(contour_to_clipper_path(contour1), ClipperLib::ptClip, true);
        ClipperLib::Paths retval;
        clipper.Execute(ClipperLib::ctIntersection, retval, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

#ifdef DEBUG
        if (0)
        {
            ClipperLib::Path path0 = contour_to_clipper_path(contour0);
            ClipperLib::Path path1 = contour_to_clipper_path(contour1);
            PolygonRef poly0(path0);
            PolygonRef poly1(path1);
            PolygonRef poly2(retval[0]);

            AABB aabb(poly0);
            aabb.include(poly1);
            aabb.include(poly2);

            SVG svg("D://test//intersection.svg", aabb, 0.00001);
            svg.writePolygon(poly0, SVG::Color::RED, 1.0);
            svg.writePolygon(poly1, SVG::Color::RED, 1.0);
            svg.writePolygon(poly2, SVG::Color::GREEN, 2.0);
        }
#endif // DEBUG

        return !retval.empty();
    }

    bool contour_to_svg(const std::vector<collision_check_info_t>& contours,const std::string &filename)
    {
        AABB aabb(contour_to_clipper_path(contours[0].contour));
        for (size_t i = 1; i < contours.size(); i++)
        {
            aabb.include(AABB(contour_to_clipper_path(contours[i].contour)));
        }

        SVG svg(filename, aabb, 0.00001);
        for (auto &contour: contours)
        {
            ClipperLib::Path path = contour_to_clipper_path(contour.contour);
            svg.writePolygon(path, SVG::Color::RED, 1.0);
        }

        return true;
    }


//     Convex_Contour(const std::vector<trimesh::dvec2>& extranl_contour)
//         :PolygonRef(contour_to_clipper_path(extranl_contour))
//     {
// 
//     }

//     Convex_Contour::Convex_Contour(const std::vector<trimesh::dvec3>& external_contour)
//         :PolygonRef(contour_to_clipper_path(external_contour))
//     {
// 
//     }



#undef scale_
#undef unscale_
    //////////////////////////////////////////////////////////////////////////   
}