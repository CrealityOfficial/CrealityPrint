#include "MESHTOOL/MeshFactory.hpp"

namespace MESHTOOL {

	MeshFactory::MeshFactory() {
		converter = MESHTOOL::UnitConverter();
	}

	MESH::MeshPtr MeshFactory::build(const TriMeshPtr& triMesh) const
	{
		return build(triMesh->faces, triMesh);
	}

	MESH::MeshPtr MeshFactory::build(const std::vector<trimesh::TriMesh::Face>& faces, const TriMeshPtr& triMesh) const{
		MESH::MeshPtr mesh = std::make_shared<MESH::Mesh>();
		addTriMeshFaces(faces, triMesh, mesh);
		return mesh;
	}

	void MeshFactory::addTriMeshFaces(const std::vector<trimesh::TriMesh::Face>& faces, const TriMeshPtr& triMesh, MESH::MeshPtr& mesh) const
	{
		for (size_t i = 0; i < faces.size(); i++) {
			auto& vertex1 = triMesh->vertices.at(faces.at(i).x);
			GEOM::Point p1 = converter.toInside(vertex1);
			auto& vertex2 = triMesh->vertices.at(faces.at(i).y);
			GEOM::Point p2 = converter.toInside(vertex2);
			auto& vertex3 = triMesh->vertices.at(faces.at(i).z);
			GEOM::Point p3 = converter.toInside(vertex3);
			mesh->addFace(p1, p2, p3);
		}
	}

}  // namespace MESHTOOL