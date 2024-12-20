#pragma once

#include "MESH/Mesh.hpp"
#include "MESHTOOL/UnitConverter.hpp"
#include "trimesh2/TriMesh.h"
#include <memory>
#include <vector>

namespace MESHTOOL {

	using TriMeshPtr = std::shared_ptr<trimesh::TriMesh>;

	class MeshFactory {
	public:

		MeshFactory();

		MESH::MeshPtr build(const TriMeshPtr& triMesh) const;
		MESH::MeshPtr build(const std::vector<trimesh::TriMesh::Face>& faces, const TriMeshPtr& trimesh) const;

	protected:
		void addTriMeshFaces(const std::vector<trimesh::TriMesh::Face>& faces, const TriMeshPtr& triMesh, MESH::MeshPtr &mesh) const;

		MESHTOOL::UnitConverter converter;
	};

}  // namespace MESHTOOL
