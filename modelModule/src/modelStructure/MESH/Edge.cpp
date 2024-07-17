#include "MESH/Edge.hpp"
#include "MESH/Face.hpp"
#include "MESH/Mesh.hpp"
#include <stdexcept>

namespace MESH {

	Edge::Edge(const VertexPtr& p1, const VertexPtr& p2, const std::weak_ptr<Mesh>& meshWeak) : refFaceCount(0), meshWeak(meshWeak) {
		initializeHelper(p1, p2);
	}

	void Edge::refFaceCountPlus()
	{
		refFaceCount++;
	}

	void Edge::refFaceCountMinus()
	{
		refFaceCount--;
	}

	double Edge::length() const {
		GEOM::Point p1 = begin->getPoint();
		GEOM::Point p2 = end->getPoint();
		GEOM::Vector v = p1 - p2;
		return v.length();
	}

	VertexPtr Edge::getBegin() const {
		return begin;
	}

	VertexPtr Edge::getEnd() const {
		return end;
	}

	bool Edge::isBorder() const {
		if (refFaceCount == 1)
		{
			return true;
		}
		return false;
	}

	EdgeHash Edge::getId() const
	{
		return id;
	}

	unsigned int Edge::getRefFaceCount() const
	{
		return refFaceCount;
	}

	std::vector<FacePtr> Edge::neighborFace() const {
		if (MeshPtr mesh = meshWeak.lock())
		{
			return mesh->edgeNeighborFace(id);
		}
		else
		{
			throwGetMeshPtrException();
			return {};
		}

	}

	std::ostream& operator<<(std::ostream& os, const Edge& edge)
	{
		os << "Edge(" << *(edge.begin) << " -> " << *(edge.end) << ")";
		return os;
	}

	void Edge::throwGetMeshPtrException() const
	{
		throw std::logic_error("Attempted to access Mesh in a deleted or invalid state.");
	}

	void Edge::throwGetFacePtrException() const
	{
		throw std::logic_error("Attempted to access Face in a deleted or invalid state.");
	}

	void Edge::initializeHelper(const VertexPtr& p1, const VertexPtr& p2)
	{
		GEOM::Point point1 = p1->getPoint();
		GEOM::Point point2 = p2->getPoint();
		if (point2.x < point1.x ||
			point2.x == point1.x && point2.y < point1.y ||
			point2.x == point1.x && point2.y == point1.y && point2.z == point1.z)
		{
			id = hash(p2, p1);
			begin = p2;
			end = p1;
		}
		else
		{
			id = hash(p1, p2);
			begin = p1;
			end = p2;
		}
	}

	EdgeHash Edge::hash(const VertexPtr& p1, const VertexPtr& p2) const
	{
		return std::hash<GEOM::HashNumType>()(p1->getId()) ^ std::hash<GEOM::HashNumType>()(p2->getId());
	}

}  // namespace MESH