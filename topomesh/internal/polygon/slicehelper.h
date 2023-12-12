#ifndef SLICE_SLICEHELPER_1598712992630_H
#define SLICE_SLICEHELPER_1598712992630_H
#include "trimesh2/TriMesh.h"
#include "trimesh2/quaternion.h"
#include "point2.h"
#include "vertex.h"
#include "meshobject.h"
#include <unordered_map>

namespace topomesh
{
	class SliceHelper
	{
	public:
		SliceHelper();
		~SliceHelper();

		void prepare(MeshObject* mesh);
		void prepare(trimesh::TriMesh* _mesh);
		void getMeshFace();
		std::vector<std::vector<std::pair<uint32_t, int>>> generateVertexConnectVertexData();
		void generateConcave(std::vector<trimesh::vec3>& concave, const trimesh::quaternion* rotation, const trimesh::vec3 scale);
		void sliceOneLayer(int z,
			std::vector<SlicerSegment>& segments, std::unordered_map<int, int>& face_idx_to_segment_idx);
		
		static SlicerSegment project2D(const Point3& p0, const Point3& p1, const Point3& p2, const coord_t z);
		static void buildMeshFaceHeightsRange(const MeshObject* mesh, std::vector<Point2>& heightRanges);
		static void buildMeshFaceHeightsRange(const trimesh::TriMesh* meshSrc, std::vector<Point2>& heightRanges);
	public:
		std::vector<MeshFace> faces;
		std::vector<std::vector<uint32_t>> vertexConnectFaceData;

		MeshObject* mesh;
		trimesh::TriMesh* meshSrc;
		std::vector<Point2> faceRanges;
	};
}

#endif // SLICE_SLICEHELPER_1598712992630_H