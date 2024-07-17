#pragma once

#include "MESH/Mesh.hpp"
#include "MESH/SimpleMesh.hpp"
#include <memory>
#include <vector>
#include <string>

namespace MESHTOOL {

	class MeshOutputer {
	public:

		MeshOutputer();

		bool writeSTLPlain(const MESH::MeshPtr& mesh, const std::string& filename);
		bool writeSTLBinary(const MESH::MeshPtr& mesh, const std::string& filename);
		bool writeSTLBinary(const MESH::SimpleMeshPtr& mesh, const std::string& filename);
		bool writeOFF(const MESH::MeshPtr& mesh, const std::string& filename);
		bool writeOFF(const MESH::SimpleMeshPtr& mesh, const std::string& filename);

	private:
		bool writeSTLBinaryTool(const std::vector<GEOM::Vector> &normals, const std::vector< std::array<std::array<float, 3>, 3>> &faceCoords, const std::string& filename) const;
	};

}  // namespace MESHTOOL
