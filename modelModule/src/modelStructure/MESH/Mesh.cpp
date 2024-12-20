// Mesh.cpp

#include "MESH/Mesh.hpp"

namespace MESH {

	Mesh::Mesh(){}

	void Mesh::addFace(const GEOM::Point& p1, const GEOM::Point& p2, const GEOM::Point& p3) {
		// 判断进入的面是退化面
		GEOM::HashNumType hash1 = p1.hash();
		GEOM::HashNumType hash2 = p2.hash();
		GEOM::HashNumType hash3 = p3.hash();
		if (hash1 == hash2 || hash2 == hash3 || hash3 == hash1)
		{
			return;
		}
		// 加面流程
		std::weak_ptr<Mesh> meshWeak = std::weak_ptr<Mesh>(shared_from_this());
		std::array<VertexPtr, 3> vertices;
		unsigned int i = 0;
		for (auto& p : { p1, p2, p3 })
		{
			VertexPtr vertex = std::make_shared<Vertex>(p, meshWeak);
			vertices[i] = vertex;
			if (vertexMap.find(vertex->getId()) == vertexMap.end())
			{
				vertexMap.insert(std::make_pair(vertex->getId(), vertex));
			}
			vertexMap.at(vertex->getId())->refFaceCountPlus();
			i++;
		}
		std::array<EdgePtr, 3> edges;
		std::pair<VertexPtr, VertexPtr> pair1 = { vertices[0], vertices[1] };
		std::pair<VertexPtr, VertexPtr> pair2 = { vertices[1], vertices[2] };
		std::pair<VertexPtr, VertexPtr> pair3 = { vertices[2], vertices[0] };
		std::array<std::pair<VertexPtr, VertexPtr>, 3> vertexPairs = { pair1, pair2, pair3 };
		for (unsigned int i = 0; i < vertexPairs.size(); i++)
		{
			VertexPtr v1 = vertexPairs[i].first;
			VertexPtr v2 = vertexPairs[i].second;
			EdgePtr edge = std::make_shared<Edge>(v1, v2, meshWeak);
			if (edgeMap.find(edge->getId()) == edgeMap.end())
			{
				edgeMap.insert(std::make_pair(edge->getId(), edge));
			}
			edgeMap.at(edge->getId())->refFaceCountPlus();
			vertexMap.at(v1->getId())->refEdgeCountPlus();
			vertexMap.at(v2->getId())->refEdgeCountPlus();
			edges[i] = edge;
		}

		FacePtr face = std::make_shared<Face>(vertices, edges, meshWeak);
		faceMap.insert(std::make_pair(face->getId(), face));
		storeNewAddFace(face);
	}

	void Mesh::addFace(const FacePtr& face)
	{
		std::array<VertexPtr, 3> vertices = face->getVertices();
		addFace(vertices.at(0)->getPoint(), vertices.at(1)->getPoint(), vertices.at(2)->getPoint());
	}

	void Mesh::addFaces(const std::vector<FacePtr>& faces)
	{
		for (auto &face: faces)
		{
			addFace(face);
		}
	}

	void Mesh::addFaces(const std::vector<GEOM::Point>& points, const std::vector<unsigned int>& faceIndices)
	{
		// 遍历每个面的顶点索引
		for (size_t i = 0; i < faceIndices.size(); i += 3) {
			// 获取当前面的三个顶点
			const GEOM::Point& p1 = points[faceIndices[i]];
			const GEOM::Point& p2 = points[faceIndices[i + 1]];
			const GEOM::Point& p3 = points[faceIndices[i + 2]];

			addFace(p1, p2, p3);
		}
	}

	std::vector<VertexPtr> Mesh::getVertices() const
	{
		std::vector<VertexPtr> vertexVec;
		vertexVec.reserve(vertexMap.size());
		for (auto& iter : vertexMap)
		{
			vertexVec.emplace_back(iter.second);
		}
		return vertexVec;
	}

	std::vector<EdgePtr> Mesh::getEdges() const
	{
		std::vector<EdgePtr> edgeVec;
		edgeVec.reserve(edgeMap.size());
		for (auto& iter : edgeMap)
		{
			edgeVec.emplace_back(iter.second);
		}
		return edgeVec;

	}

	std::vector<FacePtr> Mesh::getFaces() const
	{
		std::vector<FacePtr> faceVec;
		faceVec.reserve(faceMap.size());
		for (auto& iter : faceMap)
		{
			faceVec.emplace_back(iter.second);
		}
		return faceVec;
	}

	unsigned int Mesh::vertexCount() const
	{
		return vertexMap.size();
	}

	unsigned int Mesh::edgeCount() const
	{
		return edgeMap.size();
	}

	unsigned int Mesh::faceCount() const
	{
		return faceMap.size();
	}

	void Mesh::getverticesAndFaceIndex(std::vector<VertexPtr>& vertices, std::vector<std::array<unsigned int, 3>>& faceIndices) const
	{
		vertices.reserve(vertexMap.size());
		std::unordered_map<VertexHash, unsigned int> hashToIndexMap;
		unsigned int index = 0;
		for (auto& iter : vertexMap)
		{
			hashToIndexMap.insert(std::make_pair(iter.first, index++));
			vertices.emplace_back(iter.second);
		}
		faceIndices.reserve(faceMap.size());
		for (auto& iter : faceMap)
		{
			std::array<VertexPtr, 3> faceVertices = iter.second->getVertices();
			std::array<unsigned int, 3> faceIndexArray;
			for (unsigned int i = 0; i < 3; i++)
			{
				faceIndexArray.at(i) = hashToIndexMap.at(faceVertices.at(i)->getId());
			}
			faceIndices.emplace_back(faceIndexArray);
		}
	}

	void Mesh::deleteFace(const FaceHash& facehash) {
		if (faceMap.find(facehash) != faceMap.end())
		{
			FacePtr face = faceMap.at(facehash);
			// 如果面还在面表内，就进行处理，否则不处理
			storeNewDeleteFace(face);
			faceMap.erase(face->getId());
			std::array<EdgePtr, 3> edges = face->getEdges();
			for (auto& edge : edges)
			{
				edge->refFaceCountMinus();
				edge->getBegin()->refEdgeCountMinus();
				edge->getEnd()->refEdgeCountMinus();
				if (edge->getRefFaceCount() == 0)
				{
					if (edgeMap.find(edge->getId()) != edgeMap.end())
					{
						edgeMap.erase(edge->getId());
					}
				}
			}
			std::array<VertexPtr, 3> vertices = face->getVertices();
			for (auto& vertex : vertices)
			{
				vertex->refFaceCountMinus();
				if (vertex->getRefFaceCount() == 0)
				{
					if (vertexMap.find(vertex->getId()) != vertexMap.end())
					{
						vertexMap.erase(vertex->getId());
					}
				}
			}
		}
	}

	std::vector<VertexPtr> Mesh::vertexNeighborVertex(const VertexHash& vertexHash)
	{
		throwNotImplementMethod();
		return {};
	}

	std::vector<FacePtr> Mesh::edgeNeighborFace(const EdgeHash& edgeHash) {
		throwNotImplementMethod();
		return {};
	}

	std::vector<EdgePtr> Mesh::vertexNeighborEdge(const VertexHash& vertexHash) {
		throwNotImplementMethod();
		return {};
	}

	std::vector<FacePtr> Mesh::vertexNeighborFace(const VertexHash& vertexHash) {
		throwNotImplementMethod();
		return {};
	}


	/*
	bool set(const VertexPtr &vertex)
	{
		// 逻辑比较复杂，而且目前不需要，后续有时间处理
		if (vertexMap.find(vertex->id()) == vertexMap.end())
		{
			auto& faceWeak = vertex->getFaceWeak();
			FaceHash faceId = faceWeak->id();
			if (faceMap.find(faceId) == faceMap.end())
			{
				throw std::logic_error("faceMap not found FaceId");
			}
			faceWeak->vertexChange(shared_from_this());
			faceMap.insert(std::make_pair(faceWeak->id, faceMap.at(faceId)));
			faceMap.erase(faceId);
			for (auto& e : faceWeak->getEdges())
			{
				EdgeHash edgeId = e->id();
				if (edgeMap.find(edgeId) == edgeMap.end())
				{
					throw std::logic_error("edgeMap not found EdgeId");
				}
				e->vertexChange(shared_from_this());
			}
		}
	}
	*/

	void Mesh::throwNotImplementMethod()
	{
		throw std::runtime_error("Mesh::neighborEdge method not implemented");
	}

	void Mesh::storeNewAddFace(const FacePtr& face)
	{

	}

	void Mesh::storeNewDeleteFace(const FacePtr& face)
	{

	}

}  // namespace MESH