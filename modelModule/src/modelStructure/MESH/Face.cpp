// Face.cpp

#include "MESH/Face.hpp"
#include "MESH/Mesh.hpp"

namespace MESH {

	Face::Face(const std::array<VertexPtr, 3>& vertices, const std::array<EdgePtr, 3>& edges, const std::weak_ptr<Mesh>& meshWeak)
		: vertices(vertices), edges(edges), meshWeak(meshWeak) {
		id = hash(vertices);
	}

	void Face::deleteFace() {
		if (MeshPtr mesh = meshWeak.lock())
		{
			return mesh->deleteFace(id);
		}
		throwGetMeshPtrException();
	}

	GEOM::Vector Face::normal() const {
		// 计算两个边的向量
		GEOM::Vector v1 = vertices.at(1)->getPoint() - vertices.at(0)->getPoint();
		GEOM::Vector v2 = vertices.at(2)->getPoint() - vertices.at(0)->getPoint();

		// 计算法向量（两个边的叉积）
		return v1.cross(v2);
	}

	double Face::area() const {
		return normal().length()* 0.5;
	}

	FaceHash Face::getId() const
	{
		return id;
	}

	std::array<VertexPtr, 3> Face::getVertices() const {
		return vertices;
	}

	std::array<EdgePtr, 3> Face::getEdges() const {
		return edges;
	}

	std::unordered_set<FacePtr> Face::neighborFace() const
	{
		std::unordered_set<FacePtr> faceSet;
		for (auto &edge: edges)
		{
			auto facesFound = edge->neighborFace();
			for (auto& face : facesFound)
			{
				if (face->getId() != id)
				{
					faceSet.insert(face);
				}
			}
		}
		return faceSet;
	}

	VertexPtr Face::oppositeVertex(const EdgePtr &edge) const {
		for (auto& vertex : vertices)
		{
			if (vertex != edge->getBegin() && vertex != edge->getEnd())
			{
				return vertex;
			}
		}
		throw std::logic_error("May be this edge not in this face.");
	}

	EdgePtr Face::oppositeEdge(const VertexPtr& vertex) const
	{
		std::vector<VertexPtr> vertexCollector;
		vertexCollector.reserve(3);
		for (auto& vertexNow : vertices)
		{
			if (vertexNow != vertex)
			{
				vertexCollector.emplace_back(vertexNow);
			}
		}
		if (vertexCollector.size() != 2)
		{
			throw std::logic_error("May be this vertex not in this face.");
		}
		EdgePtr edgeNow = std::make_shared<Edge>(vertexCollector.at(0), vertexCollector.at(1), meshWeak);
		for (auto &edge: edges)
		{
			if (edgeNow->getId() == edge->getId())
			{
				return edge;
			}
		}
		throw std::logic_error("opposite edge not found.");
	}

	void Face::throwGetMeshPtrException() const
	{
		throw std::logic_error("Attempted to access Mesh in a deleted or invalid state.");
	}

	FaceHash Face::hash(const std::array<VertexPtr, 3>& vertices) const
	{
		return std::hash<GEOM::HashNumType>()(vertices.at(0)->getId()) ^ std::hash<GEOM::HashNumType>()(vertices.at(1)->getId()) ^ std::hash<GEOM::HashNumType>()(vertices.at(2)->getId());
	}


}  // namespace MESH