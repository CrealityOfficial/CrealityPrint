#ifndef CXKERNEL_MESHDATA_1681019989200_H
#define CXKERNEL_MESHDATA_1681019989200_H
#include "cxkernel/cxkernelinterface.h"
#include "cxkernel/data/attribute.h"
#include "cxkernel/data/header.h"
#include "modelndata.h"

namespace cxkernel
{
	class CXKERNEL_API MeshData 
	{
	public:
		MeshData(TriMeshPtr mesh, bool toCenter = true);
		MeshData();
		
		void setMesh(TriMeshPtr mesh, bool toCenter = true);

		trimesh::dbox3 calculateBox(const trimesh::xform& matrix = trimesh::xform::identity());
		trimesh::dbox3 localBox();

		double localZ();
		void calculateFaces();
		void resetHull();

		void adaptSmallBox(const trimesh::dbox3& box);
		void adaptBigBox(const trimesh::dbox3& box);

		void convex(const trimesh::xform& matrix, std::vector<trimesh::dvec3>& datas);
		TriMeshPtr createGlobalMesh(const trimesh::xform& matrix);

		//for render
		bool traitTriangle(int faceID, std::vector<trimesh::vec3>& position, const trimesh::fxform& matrix, bool _offset);
		bool traitTriangleEx(int faceID, std::vector<trimesh::vec3>& position, trimesh::vec3& normal, const trimesh::fxform& matrix, float offsetValue, bool _offset);
	public:
		TriMeshPtr mesh;
		TriMeshPtr hull;
		trimesh::dvec3 offset;
		std::vector<cxkernel::KernelHullFace> faces;

	public:
		static std::vector<cxkernel::KernelHullFace> calculateFaces(TriMeshPtr mesh, TriMeshPtr hull);
		static TriMeshPtr calculateHull(TriMeshPtr mesh);

	};

	typedef std::shared_ptr<cxkernel::MeshData> MeshDataPtr;
}

#endif // CXKERNEL_MESHDATA_1681019989200_H