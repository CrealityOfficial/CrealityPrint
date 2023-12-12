#ifndef CREATIVE_KERNEL_POLYGONSTACK_1592554738233_H
#define CREATIVE_KERNEL_POLYGONSTACK_1592554738233_H
#include "trimesh2/Vec.h"
#include "trimesh2/TriMesh.h"
#include <stack>

struct MergeInfo
{
	int innerIndex;
	int matual;
	int start;
	int end;
};

namespace msbase
{
	class Polygon2;
	class PolygonStack
	{
	public:
		PolygonStack();
		~PolygonStack();

		void clear();
		void generates(std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points, std::vector<trimesh::TriMesh::Face>& triangles, int layer);
		void generatesWithoutTree(std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points, std::vector<trimesh::TriMesh::Face>& triangles);

		void prepareWithoutTree(std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points);
		void prepare(std::vector<std::vector<int>>& polygons, std::vector<trimesh::dvec2>& points);
		void generate(std::vector<trimesh::TriMesh::Face>& triangles);

		bool earClipping(trimesh::TriMesh::Face& face, std::vector<int>* earIndices = nullptr);
		void getEars(std::vector<int>* earIndices = nullptr);

		Polygon2* validIndexPolygon(int index);
		int validPolygon();

		std::vector<MergeInfo> mergeInfo();
		void setMergeCount(int count);
	protected:
		std::vector<Polygon2*> m_polygon2s;

		int m_currentPolygon;
		std::vector<MergeInfo> m_mergeInfo;

		int m_mregeCount;
	};

	void transform3to2(std::vector<trimesh::dvec3>& d3points, const trimesh::dvec3& normal, std::vector<trimesh::dvec2>& d2points);
	void generateTriangleSoup(std::vector<trimesh::dvec3>& points, const trimesh::dvec3& normal, std::vector<std::vector<int>>& polygons,
		std::vector<trimesh::vec3>& newTriangles);
	void generateTriangleSoup(std::vector<trimesh::dvec3>& points, std::vector<trimesh::dvec2>& d2points, std::vector<std::vector<int>>& polygons,
		std::vector<trimesh::vec3>& newTriangles);
}
#endif // CREATIVE_KERNEL_POLYGONSTACK_1592554738233_H