// Vertex.cpp
#include "MESH/Vertex.hpp"
#include "MESH/Mesh.hpp"
#include <stdexcept>

namespace MESH
{
	Vertex::Vertex(const GEOM::Point& p, const std::weak_ptr<Mesh>& meshWeak)
		: p(p), refFaceCount(0), meshWeak(meshWeak) {
		id = p.hash();
	}

	/*
	bool Vertex::setZ(const int& z) {
		return set(p.x, p.y, z);
	}

	void Vertex::set(const int& xNew, const int& yNew, const int zNew) {
		VertexHash hashNew = hash(xNew, yNew, zNew);
		if (hashNew == id)
		{
			return;
		}
		else
		{
			p.x = xNew;
			p.y = yNew;
			p.z = zNew;
			meshWeak->set(shared_from_this());
		}
	}
	*/

	void Vertex::refFaceCountPlus()
	{
		refFaceCount++;
	}

	void Vertex::refFaceCountMinus()
	{
		refFaceCount--;
	}

	void Vertex::refEdgeCountPlus()
	{
		refEdgeCount++;
	}

	void Vertex::refEdgeCountMinus()
	{
		refEdgeCount--;
	}

	GEOM::Point Vertex::getPoint() const {
		return p;
	}

	VertexHash Vertex::getId() const
	{
		return id;
	}

	unsigned int Vertex::getRefFaceCount() const
	{
		return refFaceCount;
	}

	unsigned int Vertex::getRefEdgeCount() const
	{
		return refEdgeCount;
	}

	bool Vertex::isBorder() const {
		if (refFaceCount == refEdgeCount)
		{
			return false;
		}
		return true;
	}

	std::vector<VertexPtr> Vertex::neighborVertex() const
	{
		if (MeshPtr mesh = meshWeak.lock())
			return mesh->vertexNeighborVertex(id);
		else
		{
			throwGetMeshPtrException();
			return {};
		}
	}

	std::vector<EdgePtr> Vertex::neighborEdge() const {
		if (MeshPtr mesh = meshWeak.lock())
			return mesh->vertexNeighborEdge(id);
		else
		{
			throwGetMeshPtrException();
			return {};
		}
	}

	std::vector<FacePtr> Vertex::neighborFace() const {
		if (MeshPtr mesh = meshWeak.lock())
			return mesh->vertexNeighborFace(id);
		else
		{
			throwGetMeshPtrException();
			return {};
		}
	}

	std::ostream& operator<<(std::ostream& os, const Vertex& v)
	{
		os << "Vertex(" << v.p.x << ", " << v.p.y << ", " << v.p.z << ")";
		return os;
	}

	void Vertex::throwGetMeshPtrException() const
	{
		throw std::logic_error("Attempted to access Mesh in a deleted or invalid state.");
	}

}