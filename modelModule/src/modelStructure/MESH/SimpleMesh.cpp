#include "MESH/SimpleMesh.hpp"
#include <unordered_map>

namespace MESH {

	SimpleMesh::SimpleMesh() {
		// 构造函数
	}

	void SimpleMesh::addPoint(const int& x, const int& y, const int& z) {
		GEOM::Point point(x, y, z);
		points.push_back(point);
	}

	void SimpleMesh::addFace(const unsigned int& x, const unsigned int& y, const unsigned int& z) {
		FaceIndex face = { x, y, z };
		faces.push_back(face);
	}

	void SimpleMesh::addPointAndFace(const GEOM::Point& p0, const GEOM::Point& p1, const GEOM::Point& p2)
	{
		unsigned int index = points.size();
		points.push_back(p0);
		points.push_back(p1);
		points.push_back(p2);
		addFace(index, index + 1, index + 2);
	}

	void SimpleMesh::duplicate() {
		using PointHash = unsigned int;
		using PointRealIndex = unsigned int;
		using PointOriginIndex = unsigned int;

		struct PointAndIndex
		{
			GEOM::Point p;
			PointRealIndex ind;

			PointAndIndex(const GEOM::Point& p, const PointRealIndex& ind) : p(p), ind(ind) {}
		};

		std::unordered_map<PointHash, PointAndIndex> hashToRealMap;
		std::unordered_map<PointOriginIndex, PointHash> originToHashMap;
		unsigned int originIndex = 0;
		unsigned int realIndex = 0;
		for (GEOM::Point& p : points)
		{
			PointHash pointHash = p.hash();
			if (hashToRealMap.find(pointHash) == hashToRealMap.end())
			{
				PointAndIndex pi(p, realIndex);
				hashToRealMap.insert(std::make_pair(pointHash, pi));
			}
			originToHashMap[originIndex] = pointHash;
			realIndex++;
			originIndex++;
		}
		std::vector<GEOM::Point> pointsNew;
		pointsNew.reserve(hashToRealMap.size());
		for (auto& iter : hashToRealMap)
		{
			pointsNew.emplace_back(iter.second.p);
		}
		std::vector<FaceIndex> facesNew;
		for (auto& faceIndex : faces)
		{
			FaceIndex faceIndexNew;
			for (unsigned int i = 0; i < 3; i++)
			{
				faceIndexNew[i] = hashToRealMap.at(originToHashMap.at(faceIndex[i])).ind;
			}
			facesNew.emplace_back(faceIndexNew);
		}
		points.swap(pointsNew);
		faces.swap(facesNew);
	}

	std::vector<GEOM::Point> SimpleMesh::getPoints() const
	{
		return points;
	}

	std::vector<FaceIndex> SimpleMesh::getFaces() const
	{
		return faces;
	}

	std::vector<GEOM::Vector> SimpleMesh::getNormals() const
	{
		std::vector<GEOM::Vector> normals;
		normals.reserve(faces.size());
		for (auto& faceIndex : faces)
		{
			GEOM::Point p0 = points.at(faceIndex.at(0));
			GEOM::Point p1 = points.at(faceIndex.at(1));
			GEOM::Point p2 = points.at(faceIndex.at(2));
			GEOM::Vector v1 = p1 - p0;
			GEOM::Vector v2 = p2 - p0;
			normals.emplace_back(v1.cross(v2));
		}
		return normals;
	}

	unsigned int SimpleMesh::pointCount() const
	{
		return points.size();
	}

	unsigned int SimpleMesh::faceCount() const
	{
		return faces.size();
	}

}  // namespace MESH