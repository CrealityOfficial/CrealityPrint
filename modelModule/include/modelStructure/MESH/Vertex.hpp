// Vertex.hpp

#pragma once

#include "GEOM/Point.hpp"  // 包含Point类的适当头文件
#include <memory>
#include <vector>
#include <iostream>

namespace MESH
{
	class Vertex; // Vertex类的前向声明
	class Edge;   // Edge类的前向声明
	class Face;   // Face类的前向声明
	class Mesh;   // Mesh类的前向声明

	using VertexPtr = std::shared_ptr<Vertex>;
	using EdgePtr = std::shared_ptr<Edge>;
	using FacePtr = std::shared_ptr<Face>;
	using MeshPtr = std::shared_ptr<Mesh>;
	using VertexHash = GEOM::HashNumType;
	using EdgeHash = GEOM::HashNumType;
	using FaceHash = GEOM::HashNumType;

	class Vertex : public std::enable_shared_from_this<Vertex> {
	public:

		Vertex(const GEOM::Point& p, const std::weak_ptr<Mesh>& meshWeak);

		bool setZ(const GEOM::GeomNumType& z);  // 设置点的z坐标，拓扑方法
		void set(const GEOM::GeomNumType& x, const GEOM::GeomNumType& y, const GEOM::GeomNumType z);  // 设置点的坐标，拓扑方法
		void refFaceCountPlus();  // 面引用次数增加1
		void refFaceCountMinus();  // 面引用次数减少1
		void refEdgeCountPlus();  // 边引用次数增加1
		void refEdgeCountMinus();  // 边引用次数减少1

		GEOM::Point getPoint() const;
		VertexHash getId() const;
		unsigned int getRefFaceCount() const;
		unsigned int getRefEdgeCount() const;
		bool isBorder() const;  // 判断点是否为边界

		std::vector<VertexPtr> neighborVertex() const;  // 点找邻居边，开启拓扑才能使用
		std::vector<EdgePtr> neighborEdge() const;  // 点找邻居边，开启拓扑才能使用
		std::vector<FacePtr> neighborFace() const;  // 点找邻居面，开启拓扑才能使用

		friend std::ostream& operator<<(std::ostream& os, const Vertex& v);  // 友元函数，重载 << 操作符

	private:
		void throwGetMeshPtrException() const;  // 抛出获取不到Mesh指针的异常

	private:
		VertexHash id;   // 顶点hash值
		GEOM::Point p;  // 顶点几何数据
		unsigned int refFaceCount;  // 被面引用的计数
		unsigned int refEdgeCount;  // 被边引用的计数
		const std::weak_ptr<Mesh> meshWeak;  // 所属Mesh
		std::weak_ptr<Face> faceWeak;  // 所属Face
	};
}

