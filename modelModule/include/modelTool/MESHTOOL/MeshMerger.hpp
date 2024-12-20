#pragma once

#include "MESH/Mesh.hpp"
#include "MESH/TopologyMesh.hpp"
#include "MESH/SimpleMesh.hpp"

namespace MESHTOOL {

	class MeshMerger {
	public:

		MeshMerger();

		
		MESH::MeshPtr merge(const std::vector<MESH::MeshPtr>& meshes);  
		MESH::TopologyMeshPtr merge(const std::vector<MESH::TopologyMeshPtr>& meshes);
		MESH::SimpleMeshPtr merge(const std::vector<MESH::SimpleMeshPtr>& meshes);

	};

}  // namespace MESHTOOL
