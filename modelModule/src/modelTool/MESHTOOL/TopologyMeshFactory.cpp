#include "MESHTOOL/TopologyMeshFactory.hpp"
#include "MESHTOOL/UnitConverter.hpp"

namespace MESHTOOL {

	TopologyMeshFactory::TopologyMeshFactory() {
		MeshFactory();
	}

	MESH::TopologyMeshPtr TopologyMeshFactory::build(const TriMeshPtr& trimesh) const
	{
		return build(trimesh->faces, trimesh);
	}

	MESH::TopologyMeshPtr TopologyMeshFactory::build(const std::vector<trimesh::TriMesh::Face>& faces, const TriMeshPtr& triMesh) const{
		MESH::TopologyMeshPtr mesh = std::make_shared<MESH::TopologyMesh>();
		addTriMeshFaces(faces, triMesh, mesh);
		return mesh;
	}

	void TopologyMeshFactory::addTriMeshFaces(const std::vector<trimesh::TriMesh::Face>& faces, const TriMeshPtr& triMesh, MESH::TopologyMeshPtr& mesh) const
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