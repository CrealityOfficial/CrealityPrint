#ifndef MMESH_UNIFORMPOINTS_1631469381297_H
#define MMESH_UNIFORMPOINTS_1631469381297_H
#include "trimesh2/Vec.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/TriMesh_algo.h"

#include <unordered_map>
#include <map>
#include <float.h>
#include <list>
#include <vector>

namespace msbase
{
	class UniformPoints
	{
		class point_cmp
		{
		public:
			point_cmp(float e = FLT_MIN) :eps(e) {}

			bool operator()(const trimesh::dvec3& v0, const trimesh::dvec3& v1) const
			{
				if (fabs(v0.x - v1.x) <= eps)
				{
					if (fabs(v0.y - v1.y) <= eps)
					{
						return (v0.z < v1.z - eps);
					}
					else return (v0.y < v1.y - eps);
				}
				else return (v0.x < v1.x - eps);
			}
		private:
			float eps;
		};

		typedef std::map<trimesh::dvec3, int, point_cmp> unique_point;
		typedef unique_point::iterator point_iterator;

	public:
		UniformPoints();
		~UniformPoints();

		int add(const trimesh::dvec3& point);
		int uniformSize();
		void toVector(std::vector<trimesh::dvec3>& points);
		void clear();
	protected:
		unique_point upoints;
	};

	struct IndexPolygon
	{
		std::list<int> polygon;
		int start;
		int end;

		bool closed()
		{
			return (polygon.size() >= 2) && (polygon.front() == polygon.back());
		}
	};

	struct IndexSegment
	{
		int start;
		int end;

		void swap()
		{
			std::swap(start, end);
		}
	};

	void mergeIndexPolygon(std::vector<IndexPolygon>& polygons);
	int findIndex(int n, const std::vector<int>& orderEdgesPoint);
	int PointInPolygon(trimesh::dvec2 pt, const std::vector<int>& polygon, std::vector<trimesh::dvec2>& d2points);
	void dealIntersectLine(const std::vector<IndexPolygon>&validIndexPolygons,const std::vector<int>& orderEdgesPoints, std::vector<int>& vertexEdges,std::vector<bool>& isIntersect);

	bool testNeedfitMesh(trimesh::TriMesh* mesh, float& scale);
	bool testNeedfitMesh(trimesh::TriMesh* mesh, trimesh::xform& xf);
}

#endif // MMESH_UNIFORMPOINTS_1631469381297_H