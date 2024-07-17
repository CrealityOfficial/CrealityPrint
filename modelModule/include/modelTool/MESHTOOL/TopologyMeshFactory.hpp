#pragma once

#include "MESH/TopologyMesh.hpp"
#include "MESHTOOL/MeshFactory.hpp"
#include "trimesh2/TriMesh.h"
#include <memory>
#include <vector>

namespace MESHTOOL {

	using TriMeshPtr = std::shared_ptr<trimesh::TriMesh>;

	class TopologyMeshFactory : public MeshFactory {
	public:

		TopologyMeshFactory();

		MESH::TopologyMeshPtr build(const TriMeshPtr& trimesh) const;
		MESH::TopologyMeshPtr build(const std::vector<trimesh::TriMesh::Face>& faces, const TriMeshPtr& trimesh) const;

	protected:
		void addTriMeshFaces(const std::vector<trimesh::TriMesh::Face>& faces, const TriMeshPtr& triMesh, MESH::TopologyMeshPtr& mesh) const;
	};

}  // namespace MESHTOOL
