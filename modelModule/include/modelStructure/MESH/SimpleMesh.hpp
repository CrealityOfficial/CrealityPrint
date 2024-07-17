#pragma once

#include <vector>
#include "GEOM/Point.hpp"
#include <memory>
#include <array>

namespace MESH {

	using FaceIndex = std::array<unsigned int, 3>;

	class SimpleMesh {
	public:

		SimpleMesh();

		void addPoint(const int& x, const int& y, const int& z);
		void addFace(const unsigned int& x, const unsigned int& y, const unsigned int& z);
		void addPointAndFace(const GEOM::Point &p0, const GEOM::Point &p1, const GEOM::Point &p2);
		void duplicate();  // 去重

		std::vector<GEOM::Point> getPoints() const;
		std::vector<FaceIndex> getFaces() const;
		std::vector<GEOM::Vector> getNormals() const;
		
		unsigned int pointCount() const;
		unsigned int faceCount() const;

	private:
		std::vector<GEOM::Point> points;
		std::vector<FaceIndex> faces;
	};

	using SimpleMeshPtr = std::shared_ptr<SimpleMesh>;
}