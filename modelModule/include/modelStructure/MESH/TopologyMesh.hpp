#pragma once

#include "MESH/Mesh.hpp"
#include <unordered_map>
#include <unordered_set>
#include <queue>

namespace MESH {

	class TopologyMesh : public Mesh {

	public:
		TopologyMesh();

		void addFace(const GEOM::Point& p1, const GEOM::Point& p2, const GEOM::Point& p3) override;  // 添加面，拓扑方法
		void addFace(const FacePtr& face);

	private:
		void deleteFace(const FaceHash& facehash) override;  // 删除面，拓扑方法
		std::vector<FacePtr> edgeNeighborFace(const EdgeHash& edgeHash) override;  // 边的邻居面，拓扑方法
		std::vector<VertexPtr> vertexNeighborVertex(const VertexHash& vertexHash) override;  // 点找邻居点，拓扑方法
		std::vector<EdgePtr> vertexNeighborEdge(const VertexHash& vertexHash) override;  // 点找邻居边，拓扑方法
		std::vector<FacePtr> vertexNeighborFace(const VertexHash& vertexHash) override;  // 点找邻居面，拓扑方法
		void storeNewAddFace(const FacePtr& face) override;  // 存储新增加的面
		void storeNewDeleteFace(const FacePtr& face) override;  // 存储新删除的面

	private:
		std::unordered_map<VertexHash, std::unordered_set<FaceHash>> vertexFaceMap;
		std::unordered_map<VertexHash, std::unordered_set<EdgeHash>> vertexEdgeMap;
		std::unordered_map<EdgeHash, std::unordered_set<FaceHash>> edgeFaceMap;

		std::queue<FacePtr> newFaceAddQueue;
		std::queue<FacePtr> newFaceDeleteQueue;

	};

	using TopologyMeshPtr = std::shared_ptr<TopologyMesh>;

}  // namespace MESH