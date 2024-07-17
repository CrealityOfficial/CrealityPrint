#pragma once

#include "MESH/Mesh.hpp"
#include <unordered_set>

namespace MESHTOOL {

	class MeshExplorer {
	public:

		MeshExplorer();

		std::unordered_set<MESH::VertexPtr> borderVertices(const MESH::MeshPtr& mesh);

	};

}  // namespace MESHTOOL
