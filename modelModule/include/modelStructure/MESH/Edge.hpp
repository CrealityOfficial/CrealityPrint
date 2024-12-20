// Face.hpp

#pragma once

#include "MESH/Vertex.hpp"
#include <memory>
#include <vector>
#include <iostream>

namespace MESH {

	class Edge {
	public:
		Edge(const VertexPtr& p1, const VertexPtr& p2, const std::weak_ptr<Mesh>& meshWeak);

		void refFaceCountPlus();  // 引用次数增加1
		void refFaceCountMinus();  // 引用次数减少1

		double length() const;      // 计算长度
		VertexPtr getBegin() const;   // 获取起点
		VertexPtr getEnd() const;     // 获取终点
		bool isBorder() const;   // 判断是否为边界边
		EdgeHash getId() const;      // 获取id
		unsigned int getRefFaceCount() const;

		std::vector<FacePtr> neighborFace() const;  // 获取相邻面，开启拓扑才能使用

		friend std::ostream& operator<<(std::ostream& os, const Edge& edge);  // 友元函数，重载 << 操作符

	private:
		void throwGetMeshPtrException() const;    // 如果获取不到Mesh抛出异常
		void throwGetFacePtrException() const;    // 如果获取不到Face抛出异常
		void initializeHelper(const VertexPtr& p1, const VertexPtr& p2);  // 计算端点的hash值
		EdgeHash hash(const VertexPtr& p1, const VertexPtr& p2) const;  // 计算正确端点的hash值

	private:
		EdgeHash id;     // 边hash值
		VertexPtr begin;  // 起始点
		VertexPtr end;  // 终点
		unsigned int refFaceCount;  // 被面引用的计数
		const std::weak_ptr<Mesh> meshWeak;   // 所属网格
		std::weak_ptr<Face> faceWeak;   // 所属面
	};

}  // namespace MESH

