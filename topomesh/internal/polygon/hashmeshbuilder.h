#ifndef CX_HASHMESHBUILDER_1600071100235_H
#define CX_HASHMESHBUILDER_1600071100235_H
#include "meshobject.h"
#include "mesh.h"
#include <unordered_map>

namespace topomesh
{
	class HashMeshBuilder
	{
	public:
		HashMeshBuilder();
		~HashMeshBuilder();

		void addFace(const Point3& v0, const Point3& v1, const Point3& v2);
		MeshObject* build(bool swap = true);
		Mesh* buildMesh(bool swap = true);
	protected:
		int findIndexOfVertex(const Point3& v);
		int getFaceIdxWithPoints(int idx0, int idx1, int notFaceIdx, int notFaceVertexIdx) const;
	protected:
		std::unordered_map<uint32_t, std::vector<uint32_t> > vertex_hash_map;
		std::vector<MeshVertex> vertices;
		std::vector<MeshFace> faces;
		mutable bool has_disconnected_faces;
		mutable bool has_overlapping_faces;
	};
}

#endif // CX_HASHMESHBUILDER_1600071100235_H