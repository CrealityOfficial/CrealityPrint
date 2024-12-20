#include"polygonLib.h"
#include "delaunator.h"

#include <iostream>
#include <set>

namespace topomesh
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

		delaunatorex::Delaunator d(coords);

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
			size_t e_b = d.halfedges[delaunatorex::next_halfedge(e)];
			//  a -> c
			size_t e_a = d.halfedges[delaunatorex::next_halfedge(delaunatorex::next_halfedge(e))];

			// Add edges to heap
			double len_a = d.edge_length(e_a);
			double len_b = d.edge_length(e_b);

			bheap.push_back({ e_a, len_a });
			std::push_heap(bheap.begin(), bheap.end(), cmp);
			bheap.push_back({ e_b, len_b });
			std::push_heap(bheap.begin(), bheap.end(), cmp);

			// Update outer hull and connect new edges
			size_t a = d.triangles[e];
			size_t b = d.triangles[delaunatorex::next_halfedge(e)];

			d.hull_next[c] = b;
			d.hull_prev[c] = a;
			d.hull_next[a] = d.hull_prev[b] = c;

			bset.insert(c);
		}

		return d.get_hull_coords();
	}


	double PerpendicularDistance(const ClipperLib::IntPoint& pt, const ClipperLib::IntPoint& lineStart, const ClipperLib::IntPoint& lineEnd)
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
	void RamerDouglasPeucker(const ClipperLib::Path& pointList, double epsilon, ClipperLib::Path& out)
	{
		if (pointList.size() < 2)
			return;

		// Find the point with the maximum distance from line between start and end
		double dmax = 0.0;
		size_t index = 0;
		size_t end = pointList.size() - 1;
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
			ClipperLib::Path recResults1;
			ClipperLib::Path recResults2;
			ClipperLib::Path firstLine(pointList.begin(), pointList.begin() + index + 1);
			ClipperLib::Path lastLine(pointList.begin() + index, pointList.end());
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

	int get_miny_point_id(ClipperLib::Path& path) { //get the point with min_y
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

	void get_cos(ClipperLib::Path& path, double* mcos, int id) {  //get point's cos
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

	void sort_points(ClipperLib::Path& path, double* mcos) {   //sort the path
		int i, j;
		double temp_cos;
		int size = path.size();
		ClipperLib::IntPoint temp_point;
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

	int ccw(ClipperLib::IntPoint a, ClipperLib::IntPoint b, ClipperLib::IntPoint c) {          //judge if it is couter-colockwise
		double area2 = (b.X - a.X) * (c.Y - a.Y) - (b.Y - a.Y) * (c.X - a.X);
		if (area2 < 0) {
			return -1;          // clockwise
		}
		else {
			if (area2 > 0) return 1;    // counter-clockwise
			else return 0;              // collinear
		}

	}

	ClipperLib::Path get_outpoint(ClipperLib::Path path) {    //get path in stack
		int i, k;
		int size = path.size();
		ClipperLib::Path outpoint;
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

	ClipperLib::Path PolygonPro::polygonSimplyfy(const ClipperLib::Path& input, double epsilon)
	{
		ClipperLib::Path result;
		RamerDouglasPeucker(input, epsilon, result);
		return result;
	}

	ClipperLib::Paths PolygonPro::polygonSimplyfy(const ClipperLib::Paths& input, double epsilon )
	{
		ClipperLib::Paths result(input.size());
		for (int i = 0; i < input.size(); i++)
		{
			ClipperLib::Path path = input[i];
			RamerDouglasPeucker(path, epsilon, result[i]);
		}
		return result;
	}

	ClipperLib::Path PolygonPro::polygonConcaveHull(const ClipperLib::Path& input, double chi_factor)
	{
		std::vector<double> vertices_data, vertices_data_dst;
		vertices_data.reserve(input.size() * 2);
		for (ClipperLib::IntPoint pt : input)
		{
			vertices_data.push_back(pt.X);
			vertices_data.push_back(pt.Y);
		}
		vertices_data_dst = concavehull(vertices_data, chi_factor);

		ClipperLib::Path result(vertices_data_dst.size() / 2);
		for (int i = 0; i < vertices_data_dst.size(); i += 2)
			result[i / 2] = ClipperLib::IntPoint(ClipperLib::cInt(vertices_data_dst[i]), ClipperLib::cInt(vertices_data_dst[i + 1]));

		return result;
	}

	ClipperLib::Paths PolygonPro::polygonConcaveHull(const ClipperLib::Paths& input, double chi_factor)
	{
		ClipperLib::Paths result(input.size());
		for (int i = 0; i < input.size(); i ++)
		{
			result[i] = polygonConcaveHull(input[i], chi_factor);
		}

		return result;
	}

	ClipperLib::Path PolygonPro::polygonsConcaveHull(const ClipperLib::Paths& input, double chi_factor)
	{
		ClipperLib::Path result, allPt;
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

	ClipperLib::Path convexHull(ClipperLib::Path sh)
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
		ClipperLib::Path ItemPath;
		for (int i = 0; i < input.size(); i ++)
		{
			ItemPath.push_back(ClipperLib::IntPoint(input[i].x * _scale, input[i].y * _scale));
		}
		ClipperLib::Path newItemPath = convexHull(ItemPath);

		std::vector<PointF> result(newItemPath.size());
		for (int i = 0; i < newItemPath.size(); i++)
			result.push_back(PointF(newItemPath[i].X / _scale, newItemPath[i].Y / _scale));
		return result;
	}

	ClipperLib::Path PolygonPro::polygonConvexHull(const ClipperLib::Path& input)
	{
		return convexHull(input);
	}

	ClipperLib::Path PolygonPro::polygonsConvexHull(const ClipperLib::Paths& input)
	{
		ClipperLib::Path result, allPt;
		for (int i = 0; i < input.size(); i++)
		{
			allPt.insert(allPt.end(), input[i].begin(), input[i].end());
		}
		result = polygonConvexHull(allPt);
		return result;
	}

}