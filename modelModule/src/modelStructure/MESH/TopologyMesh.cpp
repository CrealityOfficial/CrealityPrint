// TopologyMesh.cpp

#include "MESH/TopologyMesh.hpp"

namespace MESH {

	TopologyMesh::TopologyMesh()
		: Mesh() {
	}

	void TopologyMesh::addFace(const GEOM::Point& p1, const GEOM::Point& p2, const GEOM::Point& p3) {
		Mesh::addFace(p1, p2, p3);
		// 以下处理拓扑关系
		while (newFaceAddQueue.size() > 0)
		{
			// 处理点面关系
			FacePtr face = newFaceAddQueue.front();
			FaceHash faceHash = face->getId();
			newFaceAddQueue.pop();
			auto vertices = face->getVertices();
			for (auto& vertex : vertices)
			{
				VertexHash vertexHash = vertex->getId();
				if (vertexFaceMap.find(vertexHash) == vertexFaceMap.end())
				{
					std::unordered_set<FaceHash> faceHashSet;
					vertexFaceMap.insert(std::make_pair(vertexHash, faceHashSet));
				}
				vertexFaceMap.at(vertexHash).insert(faceHash);
			}
			// 处理边面关系、点边关系
			auto edges = face->getEdges();
			for (auto& edge : edges)
			{
				// 处理边面关系
				EdgeHash edgeHash = edge->getId();
				if (edgeFaceMap.find(edgeHash) == edgeFaceMap.end())
				{
					std::unordered_set<FaceHash> faceHashSet;
					edgeFaceMap.insert(std::make_pair(edgeHash, faceHashSet));
				}
				edgeFaceMap.at(edgeHash).insert(faceHash);
				// 处理点边关系
				for (auto& vertex : { edge->getBegin(), edge->getEnd() })
				{
					VertexHash vertexHash = vertex->getId();
					if (vertexEdgeMap.find(vertexHash) == vertexEdgeMap.end())
					{
						std::unordered_set<EdgeHash> edgeHashSet;
						vertexEdgeMap.insert(std::make_pair(vertexHash, edgeHashSet));
					}
					vertexEdgeMap.at(vertexHash).insert(edgeHash);
				}
			}
		}
	}

	void TopologyMesh::addFace(const FacePtr& face)
	{
		Mesh::addFace(face);
	}

	void TopologyMesh::deleteFace(const FaceHash& facehash) {
		Mesh::deleteFace(facehash);
		// 以下处理拓扑关系
		while (newFaceDeleteQueue.size() > 0)
		{
			FacePtr face = newFaceDeleteQueue.front();
			FaceHash faceHash = face->getId();

			newFaceDeleteQueue.pop();
			// 处理点面关系
			auto vertices = face->getVertices();
			for (auto& vertex : vertices)
			{
				VertexHash vertexHash = vertex->getId();
				if (vertexFaceMap.find(vertexHash) != vertexFaceMap.end())
				{
					vertexFaceMap.at(vertexHash).erase(faceHash);
					if (vertexFaceMap.at(vertexHash).size() == 0)
					{
						vertexFaceMap.erase(vertexHash);
					}
				}
			}
			// 处理边面关系、点边关系
			auto edges = face->getEdges();
			for (auto& edge : edges)
			{
				// 处理边面关系
				EdgeHash edgeHash = edge->getId();
				if (edgeFaceMap.find(edgeHash) != edgeFaceMap.end())
				{
					edgeFaceMap.at(edgeHash).erase(faceHash);
					if (edgeFaceMap.at(edgeHash).size() == 0)
					{
						edgeFaceMap.erase(edgeHash);
						// 该边被删除，此时要处理点边关系
						for (auto& vertex : { edge->getBegin(), edge->getEnd() })
						{
							VertexHash vertexHash = vertex->getId();
							if (vertexEdgeMap.find(vertexHash) != vertexEdgeMap.end())
							{
								vertexEdgeMap.at(vertexHash).erase(edgeHash);
								if (vertexEdgeMap.at(vertexHash).size() == 0)
								{
									vertexEdgeMap.erase(vertexHash);
								}
							}
						}
					}
				}
			}
		}
	}


	std::vector<FacePtr> TopologyMesh::edgeNeighborFace(const EdgeHash& edgeHash)
	{
		auto faceHashSet = edgeFaceMap.at(edgeHash);
		std::vector<FacePtr> faces;
		faces.reserve(faceHashSet.size());
		for (FaceHash faceHash : faceHashSet)
		{
			faces.emplace_back(faceMap.at(faceHash));
		}
		return faces;
	}

	std::vector<VertexPtr> TopologyMesh::vertexNeighborVertex(const VertexHash& vertexHash)
	{
		auto edgeHashSet = vertexEdgeMap.at(vertexHash);
		std::vector<VertexPtr> vertices;
		vertices.reserve(edgeHashSet.size());
		for (EdgeHash edgeHash : edgeHashSet)
		{
			EdgePtr& edge = edgeMap.at(edgeHash);
			if (edge->getBegin()->getId() != vertexHash)
			{
				vertices.emplace_back(edge->getBegin());
			}
			else if (edge->getEnd()->getId() != vertexHash)
			{
				vertices.emplace_back(edge->getEnd());
			}
			else
			{
				throw std::logic_error("At edge not found the vertex.");
			}
		}
		return vertices;
	}

	std::vector<EdgePtr> TopologyMesh::vertexNeighborEdge(const VertexHash& vertexHash)
	{
		auto edgeHashSet = vertexEdgeMap.at(vertexHash);
		std::vector<EdgePtr> edges;
		edges.reserve(edgeHashSet.size());
		for (EdgeHash edgeHash : edgeHashSet)
		{
			edges.emplace_back(edgeMap.at(edgeHash));
		}
		return edges;
	}
	std::vector<FacePtr> TopologyMesh::vertexNeighborFace(const VertexHash& vertexHash)
	{
		auto faceHashSet = vertexFaceMap.at(vertexHash);
		std::vector<FacePtr> faces;
		faces.reserve(faceHashSet.size());
		for (FaceHash faceHash : faceHashSet)
		{
			faces.emplace_back(faceMap.at(faceHash));
		}
		return faces;
	}

	void TopologyMesh::storeNewAddFace(const FacePtr& face)
	{
		newFaceAddQueue.push(face);
	}

	void TopologyMesh::storeNewDeleteFace(const FacePtr& face)
	{
		newFaceDeleteQueue.push(face);
	}

}  // namespace MESH