#pragma once

#include "MESH/TopologyMesh.hpp"

namespace MESHTOOL {

	class MeshSpliter {
	public:

		MeshSpliter();

		// 将一个Mesh分解为多个Mesh
		std::vector<MESH::TopologyMeshPtr> split(const MESH::TopologyMeshPtr& mesh);  

	private:
		// 广度优先搜索
		void BFS(const MESH::FacePtr& startFace, std::unordered_set<MESH::FacePtr>& visited, MESH::TopologyMeshPtr& currentMesh);

	};

}  // namespace MESHTOOL
