#pragma once

#include "MESH/Mesh.hpp"
#include "MESH/SimpleMesh.hpp"
#include "trimesh2/TriMesh.h"
#include <memory>
#include <vector>

namespace MESHTOOL {

	using TriMeshPtr = std::shared_ptr<trimesh::TriMesh>;

	class TriMeshFactory {
	public:

		TriMeshFactory();

		TriMeshPtr build(const MESH::MeshPtr& mesh);
		TriMeshPtr build(const MESH::SimpleMeshPtr& mesh);

	};

}  // namespace MESHTOOL
