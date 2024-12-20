#ifndef CRSLICE_TEST_SKELETAL_1698397403190_H
#define CRSLICE_TEST_SKELETAL_1698397403190_H
#include "crslice/load.h"

namespace crslice
{
	struct SkeletalNode
	{
		trimesh::vec3 p;
		float r;
	};

	struct SkeletalEdge
	{
		trimesh::vec3 from;
		trimesh::vec3 to;
	};

	struct SkeletalGraph
	{
		std::vector<SkeletalEdge> edges;
		std::vector<SkeletalNode> nodes;
	};

	struct SkeletalDetail
	{
		SkeletalGraph graph;
	};

	struct ParabolaDetal
	{
		std::vector<trimesh::vec3> points;
	};

	struct CrDiscretizeEdges
	{
		CrPolygon edges;
		CrPolygon parabola;
		CrPolygon straightedges;
	};

	struct CrDiscretizeCell
	{
		trimesh::vec3 start;
		trimesh::vec3 end;
		std::vector<trimesh::vec3> edges;
	};

	struct CrMissingVertex
	{
		std::vector<trimesh::vec3> edges1;
		std::vector<trimesh::vec3> edges2;
	};

	CRSLICE_API void testDiscretizeParabola(CrPolygon& points, ParabolaDetal& detail);

	class SkeletalCheckImpl;
	class CRSLICE_API SkeletalCheck
	{
	public:
		SkeletalCheck();
		virtual ~SkeletalCheck();

		void setInput(const SerailCrSkeletal& skeletal);
		virtual bool isValid();

		void detectMissingVoronoiVertex(CrMissingVertex& vertex);
		const CrPolygons& outline();
		void skeletalTrapezoidation(CrPolygons& innerPoly, std::vector<CrVariableLines>& out,
			SkeletalDetail* detail = nullptr);

		void transferEdges(CrDiscretizeEdges& discretizeEdges);
		bool transferCell(int index, CrDiscretizeCell& discretizeCell);
		bool transferInvalidCell(int index, CrDiscretizeCell& discretizeCell);
		void transferGraph();

		void generateBoostVoronoiTxt(const std::string& fileName);
		void generateNoPlanarVertexSVG(const std::string& fileName);

		void generateTransferEdgeSVG(const std::string& fileName);
	protected:
		SkeletalCheckImpl* impl;
		CrPolygons outPoly;
	};
}

#endif // CRSLICE_TEST_SKELETAL_1698397403190_H