#include "MESHTOOL/SimpleMeshFactory.hpp"

namespace MESHTOOL {

	SimpleMeshFactory::SimpleMeshFactory() {
		converter = MESHTOOL::UnitConverter();
	}

	MESH::SimpleMeshPtr SimpleMeshFactory::build(const TriMeshPtr& triMesh) const
	{
		return build(triMesh->faces, triMesh);
	}

	MESH::SimpleMeshPtr SimpleMeshFactory::build(const std::vector<trimesh::TriMesh::Face>& faces, const TriMeshPtr& triMesh) const{
		MESH::SimpleMeshPtr mesh = std::make_shared<MESH::SimpleMesh>();
		for (size_t i = 0; i < faces.size(); i++) {
			auto& vertex0 = triMesh->vertices.at(faces.at(i).x);
			auto& vertex1 = triMesh->vertices.at(faces.at(i).y);
			auto& vertex2 = triMesh->vertices.at(faces.at(i).z);
			GEOM::Point p0 = converter.toInside(vertex0);
			GEOM::Point p1 = converter.toInside(vertex1);
			GEOM::Point p2 = converter.toInside(vertex2);
			mesh->addPointAndFace(p0, p1, p2);
		}
		return mesh;
	}

	MESH::SimpleMeshPtr SimpleMeshFactory::build(const MESH::TopologyMeshPtr& triMesh) const
	{
		MESH::SimpleMeshPtr mesh = std::make_shared<MESH::SimpleMesh>();
		std::vector<MESH::VertexPtr> vertices;
		std::vector<std::array<unsigned int, 3>> faceIndices;
		triMesh->getverticesAndFaceIndex(vertices, faceIndices);
		for (size_t i = 0; i < faceIndices.size(); i++) {
			GEOM::Point p1 = vertices.at(faceIndices.at(i).at(0))->getPoint();
			GEOM::Point p2 = vertices.at(faceIndices.at(i).at(1))->getPoint();
			GEOM::Point p3 = vertices.at(faceIndices.at(i).at(2))->getPoint();
			mesh->addPointAndFace(p1, p2, p3);
		}
		return mesh;
	}

}  // namespace MESHTOOL