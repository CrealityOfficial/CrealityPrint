#pragma once

#include "MESH/SimpleMesh.hpp"
#include "MESH/TopologyMesh.hpp"
#include "MESHTOOL/UnitConverter.hpp"
#include "trimesh2/TriMesh.h"
#include <memory>
#include <vector>

namespace MESHTOOL {

	using TriMeshPtr = std::shared_ptr<trimesh::TriMesh>;

	class SimpleMeshFactory {
	public:

		SimpleMeshFactory();

		MESH::SimpleMeshPtr build(const TriMeshPtr& triMesh) const;
		MESH::SimpleMeshPtr build(const std::vector<trimesh::TriMesh::Face>& faces, const TriMeshPtr& trimesh) const;

		MESH::SimpleMeshPtr build(const MESH::TopologyMeshPtr& triMesh) const;

	private:
		UnitConverter converter;
	};

}  // namespace MESHTOOL
