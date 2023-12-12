#ifndef CX_MESHSLICE_1599726705111_H
#define CX_MESHSLICE_1599726705111_H
#include <vector>
#include "polygon.h"
#include "trimesh2/TriMesh.h"
#include "dlpinput.h"

namespace topomesh
{
	class SlicedMeshLayer
	{
	public:
		SlicedMeshLayer();
		~SlicedMeshLayer();

		Polygons polygons;
		Polygons openPolylines;
		ClipperLib::cInt z;
	};

	class SlicedMesh
	{
	public:
		SlicedMesh();
		~SlicedMesh();

		std::vector<SlicedMeshLayer> m_layers;
	};

	class SlicedResult
	{
	public:
		SlicedResult();
		~SlicedResult();

		void save(const std::string& prefix);
		void connect();
		void simplify(coord_t resolution, coord_t deviation,
			float xy_offset, bool enable_xy_offset);
		int meshCount();
		int layerCount();

		std::vector<SlicedMesh> slicedMeshes;
	};

	class SliceHelper;
	void sliceMeshes(std::vector<MeshObject*>& meshes, std::vector<SlicedMesh>& slicedMeshes, std::vector<int>& z);
	void sliceMesh(MeshObject* mesh, SlicedMesh& slicedMesh, std::vector<int>& z);
	void sliceMesh(MeshObject* mesh, SlicedMeshLayer& slicedMeshLayer, int z, SliceHelper* helper);

	void sliceMeshes_src(const std::vector<trimesh::TriMesh*>& meshes, std::vector<SlicedMesh>& slicedMeshes, std::vector<int>& z);
	void sliceMesh_src(trimesh::TriMesh* mesh, SlicedMesh& slicedMesh, std::vector<int>& z);
	void sliceMesh_src(SlicedMeshLayer& slicedMeshLayer, int z, SliceHelper* helper);

	void buildSliceInfos(const DLPInput& input, std::vector<int>& z);
	bool sliceInput(const DLPInput& input, SlicedResult& result, ccglobal::Tracer* tracer = nullptr);
}

#endif // CX_MESHSLICE_1599726705111_H