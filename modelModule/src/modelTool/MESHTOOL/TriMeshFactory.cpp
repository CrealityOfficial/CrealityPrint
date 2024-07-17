#include "MESHTOOL/TriMeshFactory.hpp"
#include "MESHTOOL/UnitConverter.hpp"

namespace MESHTOOL {

	TriMeshFactory::TriMeshFactory() {}

	TriMeshPtr TriMeshFactory::build(const MESH::MeshPtr& mesh)
	{
		TriMeshPtr trimesh = std::make_shared<trimesh::TriMesh>();
		UnitConverter converter;
		// 处理顶点
		std::vector<MESH::VertexPtr> vertices;
		std::vector<std::array<unsigned int, 3>> faceIndices;
		mesh->getverticesAndFaceIndex(vertices, faceIndices);
		std::vector<trimesh::point> triMeshPoints;
		triMeshPoints.reserve(vertices.size());
		for (auto &vertex: vertices)
		{
			GEOM::Point p = vertex->getPoint();
			std::array<float, 3> pFloat = converter.toOutside(p);
			trimesh::point pNew = { pFloat.at(0), pFloat.at(1), pFloat.at(2) };
			triMeshPoints.emplace_back(pNew);
		}
		// 处理面片
		std::vector<trimesh::TriMesh::Face> triMeshFaces;
		triMeshFaces.reserve(faceIndices.size());
		for (auto faceIndex : faceIndices)
		{
			int faceIndex0 = static_cast<int>(faceIndex.at(0));
			int faceIndex1 = static_cast<int>(faceIndex.at(1));
			int faceIndex2 = static_cast<int>(faceIndex.at(2));
			if (faceIndex0 < 0 || faceIndex1 < 0 || faceIndex2 < 0)
			{
				throw std::logic_error("faceIndex static from unsigned int to int error.");
			}
			trimesh::TriMesh::Face faceIndexNew = { faceIndex0, faceIndex1, faceIndex2 };
			triMeshFaces.emplace_back(faceIndexNew);
		}
		trimesh->vertices.swap(triMeshPoints);
		trimesh->faces.swap(triMeshFaces);
		return trimesh;
	}

	TriMeshPtr TriMeshFactory::build(const MESH::SimpleMeshPtr& mesh)
	{
		TriMeshPtr trimesh = std::make_shared<trimesh::TriMesh>();
		// 处理顶点
		auto points = mesh->getPoints();
		std::vector<trimesh::point> triMeshPoints;
		triMeshPoints.reserve(points.size());
		MESHTOOL::UnitConverter converter;
		for (auto &p: points)
		{
			auto coord = converter.toOutside(p);
			trimesh::point pNew = { coord.at(0), coord.at(1), coord.at(2) };
			triMeshPoints.emplace_back(pNew);
		}
		// 处理面片
		auto faces = mesh->getFaces();
		std::vector<trimesh::TriMesh::Face> triMeshFaces;
		triMeshFaces.reserve(faces.size());
		for (auto& faceIndex : faces)
		{
			int faceIndex0 = static_cast<int>(faceIndex.at(0));
			int faceIndex1 = static_cast<int>(faceIndex.at(1));
			int faceIndex2 = static_cast<int>(faceIndex.at(2));
			if (faceIndex0 < 0 || faceIndex1 < 0 || faceIndex2 < 0)
			{
				throw std::logic_error("faceIndex static from unsigned int to int error.");
			}
			trimesh::TriMesh::Face faceIndexNew = { faceIndex0, faceIndex1, faceIndex2 };
			triMeshFaces.emplace_back(faceIndexNew);
		}
		trimesh->vertices.swap(triMeshPoints);
		trimesh->faces.swap(triMeshFaces);
		return trimesh;
	}

}  // namespace MESHTOOL