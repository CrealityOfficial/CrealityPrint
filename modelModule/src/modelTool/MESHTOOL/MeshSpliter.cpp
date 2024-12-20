#include "MESHTOOL/MeshSpliter.hpp"

namespace MESHTOOL {

	MeshSpliter::MeshSpliter(){}

	std::vector<MESH::TopologyMeshPtr> MeshSpliter::split(const MESH::TopologyMeshPtr& mesh)
	{
		// 广度优先搜索
		std::vector<MESH::TopologyMeshPtr> result;
		std::unordered_set<MESH::FacePtr> visited;

		for (const auto& face : mesh->getFaces()) {
			if (visited.find(face) == visited.end()) {
				MESH::TopologyMeshPtr currentMesh = std::make_shared<MESH::TopologyMesh>();
				BFS(face, visited, currentMesh);

				if (currentMesh->faceCount() > 0) {
					result.push_back(currentMesh);
				}
			}
		}

		return result;
	}

	void MeshSpliter::BFS(const MESH::FacePtr& startFace, std::unordered_set<MESH::FacePtr>& visited, MESH::TopologyMeshPtr& currentMesh) {
		std::queue<MESH::FacePtr> faceQueue;
		faceQueue.push(startFace);
		visited.insert(startFace);

		while (!faceQueue.empty()) {
			MESH::FacePtr currentFace = faceQueue.front();
			faceQueue.pop();

			currentMesh->addFace(currentFace);

			for (const auto& neighbor : currentFace->neighborFace()) {
				if (visited.find(neighbor) == visited.end()) {
					visited.insert(neighbor);
					faceQueue.push(neighbor);
				}
			}
		}
	}
}