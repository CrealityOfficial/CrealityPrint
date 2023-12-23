#include "uniformpoints.h"

namespace msbase
{
	UniformPoints::UniformPoints()
	{

	}

	UniformPoints::~UniformPoints()
	{

	}

	int UniformPoints::add(const trimesh::dvec3& point)
	{
		int index = -1;
		point_iterator it = upoints.find(point);
		if (it != upoints.end())
		{
			index = (*it).second;
		}
		else
		{
			index = (int)upoints.size();
			upoints.insert(unique_point::value_type(point, index));
		}

		return index;
	}

	int UniformPoints::uniformSize()
	{
		return (int)upoints.size();
	}

	void UniformPoints::toVector(std::vector<trimesh::dvec3>& points)
	{
		int size = uniformSize();
		if (size == 0)
			return;

		points.resize(size);
		for (auto it = upoints.begin(); it != upoints.end(); ++it)
		{
			points.at((*it).second) = (*it).first;
		}
	}

	void UniformPoints::clear()
	{
		upoints.clear();
	}

	//
	void mergeIndexPolygon(std::vector<IndexPolygon>& indexPolygons)
	{
		size_t indexPolygonSize = indexPolygons.size();
		std::map<int, IndexPolygon*> IndexPolygonMap;
		for (size_t i = 0; i < indexPolygonSize; ++i)
		{
			IndexPolygon& p1 = indexPolygons.at(i);
			if (!p1.closed())
				IndexPolygonMap.insert(std::pair<int, IndexPolygon*>(p1.start, &p1));
		}

		//combime
		for (size_t i = 0; i < indexPolygonSize; ++i)
		{
			IndexPolygon& p1 = indexPolygons.at(i);

			if (p1.polygon.size() == 0 || p1.closed())
				continue;

			auto it = IndexPolygonMap.find(p1.end);
			while (it != IndexPolygonMap.end())
			{

				IndexPolygon& p2 = *(*it).second;
				if (p2.polygon.size() == 0)
					break;

				bool merged = false;
				if (p1.start == p2.end)
				{
					p1.start = p2.start;
					for (auto iter = p2.polygon.rbegin(); iter != p2.polygon.rend(); ++iter)
					{
						if ((*iter) != p1.polygon.front()) p1.polygon.push_front((*iter));
					}
					merged = true;
				}
				else if (p1.end == p2.start)
				{
					p1.end = p2.end;
					for (auto iter = p2.polygon.begin(); iter != p2.polygon.end(); ++iter)
					{
						if ((*iter) != p1.polygon.back()) p1.polygon.push_back((*iter));
					}
					merged = true;
				}

				if (merged)
				{
					p2.polygon.clear();
				}
				else
					break;

				it = IndexPolygonMap.find(p1.end);
			}
		}

		std::vector<IndexPolygon> validIndexPolygons;
		for (size_t i = 0; i < indexPolygons.size(); ++i)
		{
			if (indexPolygons.at(i).polygon.size() > 0)
			{
				validIndexPolygons.push_back(indexPolygons.at(i));
			}
		}

		indexPolygons.swap(validIndexPolygons);
	}

	int findIndex(int n, const std::vector<int>& orderEdgesPoint)
	{
		for (size_t i = 0; i < orderEdgesPoint.size(); i++)
		{
			if (n == orderEdgesPoint[i])
				return i;
		}
		return -1;
	}

	void dealIntersectLine(const std::vector<IndexPolygon>& validIndexPolygons, const std::vector<int>& orderEdgesPoints, std::vector<int>& vertexEdges, std::vector<bool>& isIntersect)
	{
		for (size_t i = 0; i < validIndexPolygons.size(); i++)
		{
			int sIndex = findIndex(validIndexPolygons[i].start, orderEdgesPoints);
			int eIndex = findIndex(validIndexPolygons[i].end, orderEdgesPoints);
			if (sIndex && eIndex)
			{
				if (std::abs(sIndex - eIndex) % 2 == 0)
				{
					isIntersect.at(i) = true;
					vertexEdges.at(validIndexPolygons[i].start) = -2;
					vertexEdges.at(validIndexPolygons[i].end) = -2;
				}
			}
		}
	}

	int PointInPolygon(trimesh::dvec2 pt, const std::vector<int>& polygon, std::vector<trimesh::dvec2>& d2points)
	{
		//returns 0 if false, +1 if true, -1 if pt ON polygon boundary
		int result = 0;
		size_t cnt = polygon.size();
		if (cnt < 3) return 0;
		trimesh::dvec2 ip = d2points[polygon[0]];
		for (size_t i = 1; i <= cnt; ++i)
		{
			trimesh::dvec2 ipNext = (i == cnt ? d2points[polygon[0]] : d2points[polygon[i]]);
			if (ipNext.y == pt.y)
			{
				if ((ipNext.x == pt.x) || (ip.y == pt.y &&
					((ipNext.x > pt.x) == (ip.x < pt.x)))) return -1;
			}
			if ((ip.y < pt.y) != (ipNext.y < pt.y))
			{
				if (ip.x >= pt.x)
				{
					if (ipNext.x > pt.x) result = 1 - result;
					else
					{
						double d = (double)(ip.x - pt.x) * (ipNext.y - pt.y) -
							(double)(ipNext.x - pt.x) * (ip.y - pt.y);
						if (!d) return -1;
						if ((d > 0) == (ipNext.y > ip.y)) result = 1 - result;
					}
				}
				else
				{
					if (ipNext.x > pt.x)
					{
						double d = (double)(ip.x - pt.x) * (ipNext.y - pt.y) -
							(double)(ipNext.x - pt.x) * (ip.y - pt.y);
						if (!d) return -1;
						if ((d > 0) == (ipNext.y > ip.y)) result = 1 - result;
					}
				}
			}
			ip = ipNext;
		}
		return result;
	}

	bool testNeedfitMesh(trimesh::TriMesh* mesh, float& scale)
	{
		if (!mesh)
			return false;

		mesh->need_bbox();
		trimesh::vec3 size = mesh->bbox.size();

		bool needScale = false;
		scale = 1.0f;
		if (size.max() > 1000.0f)
		{
			needScale = true;
			scale = 100.0f / size.max();
		}
		else if (size.min() < 10.0f && size.min() > 0.00001f)
		{
			needScale = true;
			scale = 100.0f / size.min();
		}

		return needScale;
	}

	bool testNeedfitMesh(trimesh::TriMesh* mesh, trimesh::xform& xf)
	{
		if (!mesh)
			return false;

		mesh->need_bbox();
		trimesh::vec3 size = mesh->bbox.size();
		trimesh::vec3 c = mesh->bbox.center();

		bool needScale = false;
		float scale = 1.0f;
		if (size.max() > 1000.0f)
		{
			needScale = true;
			scale = 100.0f / size.max();
		}
		else if (size.min() < 10.0f && size.min() > 0.00001f)
		{
			needScale = true;
			scale = 100.0f / size.min();
		}

		if (needScale)
		{
			xf = trimesh::xform::trans(c) * trimesh::xform::scale(scale) * trimesh::xform::trans(-c);
		}

		return needScale;
	}
}