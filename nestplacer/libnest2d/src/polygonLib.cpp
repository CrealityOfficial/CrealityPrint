#include"polygonLib/polygonLib.h"
#include "polygonLib/delaunator.h"

#include <iostream>
#include <set>

namespace polygonLib
{
	PolygonPro::PolygonPro()
	{

	}
	PolygonPro::~PolygonPro()
	{

	}

	std::vector<double> concavehull(const std::vector<double>& coords, double chi_factor = 0.1) {

		if (chi_factor < 0 || chi_factor > 1) {
			throw std::invalid_argument("Chi factor must be between 0 and 1 inclusive");
		}

		delaunator::Delaunator d(coords);

		// Determine initial path on outside hull
		std::vector<size_t> bpoints = d.get_hull_points();
		std::set<size_t> bset(bpoints.begin(), bpoints.end());

		// Make max heap of boundary edges with lengths
		typedef std::pair<size_t, double> hpair;

		auto cmp = [](hpair left, hpair right) {
			return left.second < right.second;
		};

		std::vector<hpair> bheap(bpoints.size());

		double max_len = std::numeric_limits<double>::min();
		double min_len = std::numeric_limits<double>::max();

		for (auto point : bpoints) {
			size_t e = d.hull_tri[point];
			double len = d.edge_length(e);

			bheap.push_back({ e, len });
			std::push_heap(bheap.begin(), bheap.end(), cmp);

			min_len = std::min(len, min_len);
			max_len = std::max(len, max_len);
		}

		// Determine length parameter
		double length_param = chi_factor * max_len + (1 - chi_factor) * min_len;

		// Iteratively add path to boundary by iterating over the triangles on the hull
		while (!bheap.empty()) {

			// Get edge with the largest length
			std::pop_heap(bheap.begin(), bheap.end(), cmp);
			//const auto [e, len] = bheap.back();
			const auto e = bheap.back().first;
			const auto len = bheap.back().second;

			bheap.pop_back();

			// Length of edge too small for our chi factor
			if (len <= length_param) {
				break;
			}

			// Find interior point given edge e (a -> b)
			//       e
			//  b <----- a
			//     \   /
			//  e_b \ / e_a
			//       c
			size_t c = d.get_interior_point(e);

			// Point already belongs to boundary
			if (bset.count(c)) {
				continue;
			}

			// Get two edges connected to interior point
			//  c -> b
			size_t e_b = d.halfedges[delaunator::next_halfedge(e)];
			//  a -> c
			size_t e_a = d.halfedges[delaunator::next_halfedge(delaunator::next_halfedge(e))];

			// Add edges to heap
			double len_a = d.edge_length(e_a);
			double len_b = d.edge_length(e_b);

			bheap.push_back({ e_a, len_a });
			std::push_heap(bheap.begin(), bheap.end(), cmp);
			bheap.push_back({ e_b, len_b });
			std::push_heap(bheap.begin(), bheap.end(), cmp);

			// Update outer hull and connect new edges
			size_t a = d.triangles[e];
			size_t b = d.triangles[delaunator::next_halfedge(e)];

			d.hull_next[c] = b;
			d.hull_prev[c] = a;
			d.hull_next[a] = d.hull_prev[b] = c;

			bset.insert(c);
		}

		return d.get_hull_coords();
	}


	double PerpendicularDistance(const Clipper3r::IntPoint& pt, const Clipper3r::IntPoint& lineStart, const Clipper3r::IntPoint& lineEnd)
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

	//polugon simplify
	void RamerDouglasPeucker(const Clipper3r::Path& pointList, double epsilon, Clipper3r::Path& out)
	{
		if (pointList.size() < 2)
			return;

		// Find the point with the maximum distance from line between start and end
		double dmax = 0.0;
		size_t index = 0;
		size_t end = pointList.size() - 1;
        out.reserve(end + 1);
		for (size_t i = 1; i < end; i++)
		{
			double d = PerpendicularDistance(pointList[i], pointList[0], pointList[end]);
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
			Clipper3r::Path recResults1;
			Clipper3r::Path recResults2;
			Clipper3r::Path firstLine(pointList.begin(), pointList.begin() + index + 1);
			Clipper3r::Path lastLine(pointList.begin() + index, pointList.end());
			RamerDouglasPeucker(firstLine, epsilon, recResults1);
			RamerDouglasPeucker(lastLine, epsilon, recResults2);

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

	int get_miny_point_id(Clipper3r::Path& path) { //get the point with min_y
		int i, min_id = 0;
		double miny = 10000;
		int size = path.size();
		for (i = 0; i < size; i++) {
			if (path[i].Y < miny) {
				miny = path[i].X;
				min_id = i;
			}
		}
		return min_id;
	}

	void get_cos(Clipper3r::Path& path, double* mcos, int id) {  //get point's cos
		int i;
		double coss;
		int size = path.size();
		for (i = 0; i < size; i++) {
			if (i == id) {
				mcos[i] = 2;
			}
			else {
				coss = (path[i].X - path[id].X) / sqrt((path[i].X - path[id].X) * (path[i].X - path[id].X) + 
					(path[i].Y - path[id].Y) * (path[i].Y - path[id].Y));
				mcos[i] = coss;
			}
		}
	}

	void sort_points(Clipper3r::Path& path, double* mcos) {   //sort the path
		int i, j;
		double temp_cos;
		int size = path.size();
		Clipper3r::IntPoint temp_point;
		for (i = 0; i < size; i++) {
			for (j = 0; j < size - i - 1; j++) {      //bubble sorting
				if (mcos[j] < mcos[j + 1]) {
					temp_cos = mcos[j];
					mcos[j] = mcos[j + 1];
					mcos[j + 1] = temp_cos;

					temp_point = path[j];
					path[j] = path[j + 1];
					path[j + 1] = temp_point;
				}
			}
		}
	}

	int ccw(Clipper3r::IntPoint a, Clipper3r::IntPoint b, Clipper3r::IntPoint c) {          //judge if it is couter-colockwise
		double area2 = (b.X - a.X) * (c.Y - a.Y) - (b.Y - a.Y) * (c.X - a.X);
		if (area2 < 0) {
			return -1;          // clockwise
		}
		else {
			if (area2 > 0) return 1;    // counter-clockwise
			else return 0;              // collinear
		}

	}

	Clipper3r::Path get_outpoint(Clipper3r::Path path) {    //get path in stack
		int i, k;
		int size = path.size();
		Clipper3r::Path outpoint;
		outpoint.push_back(path[0]);
		outpoint.push_back(path[1]);
		i = 2;
		while (true) {
			if (i == size) {
				break;
			}
			if (ccw(outpoint[outpoint.size() - 2], outpoint[outpoint.size() - 1], path[i]) > 0) {
				outpoint.push_back(path[i]);
				i = i + 1;
			}
			else {
				outpoint.pop_back();
			}
		}
		return outpoint;
	}

	Clipper3r::Path PolygonPro::polygonSimplyfy(const Clipper3r::Path& input, double epsilon)
	{
        bool closed = false;
		Clipper3r::Path result;
        if (input.empty()) return result;
        Clipper3r::Path path = input;
        if (path.front() == path.back()) {
            closed = true;
            path.pop_back();
        }
		RamerDouglasPeucker(path, epsilon, result);
        if (closed) result.emplace_back(result.front());
		return result;
	}

	Clipper3r::Paths PolygonPro::polygonSimplyfy(const Clipper3r::Paths& input, double epsilon )
	{
		Clipper3r::Paths result(input.size());
		for (int i = 0; i < input.size(); i++)
		{
			Clipper3r::Path path = input[i];
			RamerDouglasPeucker(path, epsilon, result[i]);
		}
		return result;
	}

    bool PolygonPro::PointInPolygon(const Clipper3r::IntPoint& pt, const Clipper3r::Path& input)
    {
        bool inside = false;
        const size_t len = input.size();
        for (size_t i = 0, j = len - 1; i < len; j = i++) {
            const auto& a = input[i];
            const auto& b = input[j];
            if (((a.Y > pt.Y) != (b.Y > pt.Y)) &&
                ((pt.X < (b.X - a.X) * (pt.Y - a.Y) / (b.Y - a.Y) + a.X))) inside = !inside;
        }
        return inside;
    }

    Clipper3r::Path PolygonPro::polygonConvexHull2(const Clipper3r::Path& input, bool clockwise)
    {
        typedef Clipper3r::IntPoint Point;
        typedef Clipper3r::cInt Unit;
        size_t edges = input.size();
        if (edges < 3) return {};
        else if (edges == 3) return input;

        bool closed = false;
        std::vector<Point> U, L;
        U.reserve(1 + edges / 2); L.reserve(1 + edges / 2);

        std::vector<Point> pts; pts.reserve(edges);
        std::copy(input.cbegin(), input.cend(), std::back_inserter(pts));

        auto fpt = pts.front(), lpt = pts.back();
        if (fpt.X == lpt.X && fpt.Y == lpt.Y) {
            closed = true; pts.pop_back();
        }

        std::sort(pts.begin(), pts.end(),[](const Point & v1, const Point & v2) {
            auto x1 = v1.X, x2 = v2.X, y1 = v1.Y, y2 = v2.Y;
            return x1 == x2 ? y1 < y2 : x1 < x2;
        });

        auto dir = [](const Point & p, const Point & q, const Point & r) {
            return (q.Y - p.Y) * (r.X - p.X) - (q.X - p.X) * (r.Y - p.Y);
        };

        auto ik = pts.begin();
        while (ik != pts.end()) {
            while (U.size() > 1 && dir(U[U.size() - 2], U.back(), *ik) <= 0)
                U.pop_back();
            while (L.size() > 1 && dir(L[L.size() - 2], L.back(), *ik) >= 0)
                L.pop_back();

            U.emplace_back(*ik);
            L.emplace_back(*ik);
            ++ik;
        }

        Clipper3r::Path ret; 
        ret.reserve(U.size() + L.size());
        if (clockwise) {
            for (auto it = U.begin(); it != std::prev(U.end()); ++it)
                ret.emplace_back(*it);
            for (auto it = L.rbegin(); it != std::prev(L.rend()); ++it)
                ret.emplace_back(*it);
            if (closed) ret.emplace_back(*std::prev(L.rend()));
        } else {
            for (auto it = L.begin(); it != std::prev(L.end()); ++it)
                ret.emplace_back(*it);
            for (auto it = U.rbegin(); it != std::prev(U.rend()); ++it)
                ret.emplace_back(*it);
            if (closed) ret.emplace_back(*std::prev(U.rend()));
        }

        return ret;
    }

	Clipper3r::Path PolygonPro::polygonConcaveHull(const Clipper3r::Path& input, double chi_factor)
	{
		std::vector<double> vertices_data, vertices_data_dst;
		vertices_data.reserve(input.size() * 2);
		for (Clipper3r::IntPoint pt : input)
		{
			vertices_data.push_back(pt.X);
			vertices_data.push_back(pt.Y);
		}
		vertices_data_dst = concavehull(vertices_data, chi_factor);

		Clipper3r::Path result(vertices_data_dst.size() / 2);
		for (int i = 0; i < vertices_data_dst.size(); i += 2)
			result[i / 2] = Clipper3r::IntPoint(Clipper3r::cInt(vertices_data_dst[i]), Clipper3r::cInt(vertices_data_dst[i + 1]));

		return result;
	}

	Clipper3r::Paths PolygonPro::polygonConcaveHull(const Clipper3r::Paths& input, double chi_factor)
	{
		Clipper3r::Paths result(input.size());
		for (int i = 0; i < input.size(); i ++)
		{
			result[i] = polygonConcaveHull(input[i], chi_factor);
		}

		return result;
	}

	Clipper3r::Path PolygonPro::polygonsConcaveHull(const Clipper3r::Paths& input, double chi_factor)
	{
		Clipper3r::Path result, allPt;
		for (int i = 0; i < input.size(); i++)
		{
			allPt.insert(allPt.end(), input[i].begin(), input[i].end());
		}
		result = polygonConcaveHull(allPt, chi_factor);
		return result;
	}

	std::vector<PointF> PolygonPro::polygonConcaveHull(const std::vector<PointF> input, double chi_factor)
	{
		std::vector<double> vertices_data, vertices_data_dst;
		vertices_data.reserve(input.size() * 2);
		for (PointF pt : input)
		{
			vertices_data.push_back(pt.x);
			vertices_data.push_back(pt.y);
		}
		vertices_data_dst = concavehull(vertices_data, chi_factor);

		std::vector<PointF> result(vertices_data_dst.size() / 2);
		for (int i = 0; i < vertices_data_dst.size(); i += 2)
			result[i / 2] = PointF(vertices_data_dst[i], vertices_data_dst[i + 1]);

		return result;
	}

	Clipper3r::Path convexHull(Clipper3r::Path sh)
	{
		double* mcos = new double[sh.size()];
		int miny_point_id = get_miny_point_id(sh);
		get_cos(sh, mcos, miny_point_id);
		sort_points(sh, mcos);	
		return get_outpoint(sh);
	}

	std::vector<PointF> PolygonPro::polygonConvexHull(const std::vector<PointF> input)
	{
		double _scale = 10000;
		Clipper3r::Path ItemPath;
		for (int i = 0; i < input.size(); i ++)
		{
			ItemPath.push_back(Clipper3r::IntPoint(input[i].x * _scale, input[i].y * _scale));
		}
		Clipper3r::Path newItemPath = convexHull(ItemPath);

		std::vector<PointF> result(newItemPath.size());
		for (int i = 0; i < newItemPath.size(); i++)
			result.push_back(PointF(newItemPath[i].X / _scale, newItemPath[i].Y / _scale));
		return result;
	}

	Clipper3r::Path PolygonPro::polygonConvexHull(const Clipper3r::Path& input)
	{
		return convexHull(input);
	}

	Clipper3r::Path PolygonPro::polygonsConvexHull(const Clipper3r::Paths& input)
	{
		Clipper3r::Path result, allPt;
		for (int i = 0; i < input.size(); i++)
		{
			allPt.insert(allPt.end(), input[i].begin(), input[i].end());
		}
		result = polygonConvexHull(allPt);
		return result;
	}

}