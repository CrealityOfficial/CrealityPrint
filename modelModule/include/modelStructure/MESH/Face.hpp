// Face.hpp

#pragma once

#include "MESH/Vertex.hpp"
#include "MESH/Edge.hpp"
#include <vector>
#include <array>
#include <memory>
#include <unordered_set>

namespace MESH {

	class Face {
	public:

		Face(const std::array<VertexPtr, 3>& vertices, const std::array<EdgePtr, 3>& edges, const std::weak_ptr<Mesh>& meshWeak);

		void deleteFace();  // 删除面，拓扑方法

		GEOM::Vector normal() const;   // 计算面的法向量
		double area() const;    // 计算面的面积
		FaceHash getId() const;  // 返回id值
		std::array<VertexPtr, 3> getVertices() const;  // 获取顶点
		std::array<EdgePtr, 3> getEdges() const;  // 获取边
		std::unordered_set<FacePtr> neighborFace() const;  // 邻居面
		VertexPtr oppositeVertex(const EdgePtr& edge) const;  // 边的对面点（这个算法使用属于该面的边才成立）
		EdgePtr oppositeEdge(const VertexPtr &vertex) const;  // 点的对面边（这个算法使用属于该面的点才成立）

	private:
		void throwGetMeshPtrException() const;
		void initializeHelper(const std::array<VertexPtr, 3>& vertices) const;
		FaceHash hash(const std::array<VertexPtr, 3>& vertices) const;

	private:
		FaceHash id;
		std::array<VertexPtr, 3> vertices;
		std::array<EdgePtr, 3> edges;
		std::weak_ptr<Mesh> meshWeak;
	};

}  // namespace MESH

