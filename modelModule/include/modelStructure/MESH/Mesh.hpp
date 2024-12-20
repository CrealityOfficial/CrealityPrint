// Mesh.hpp

#pragma once

#include "MESH/Vertex.hpp"
#include "MESH/Edge.hpp"
#include "MESH/Face.hpp"
#include <vector>
#include <unordered_map>
#include <memory>

namespace MESH {

	class Mesh : public std::enable_shared_from_this<Mesh> {

	public:
		friend class Vertex;
		friend class Edge;
		friend class Face;

		Mesh();

		virtual void addFace(const GEOM::Point& p1, const GEOM::Point& p2, const GEOM::Point& p3);  // 添加面，拓扑方法
		void addFace(const FacePtr& face);
		void addFaces(const std::vector<FacePtr>& faces);
		void addFaces(const std::vector<GEOM::Point>& points, const std::vector<unsigned int>& faceIndices);  // 添加多个面，拓扑方法
		
		std::vector<VertexPtr> getVertices() const;  
		std::vector<EdgePtr> getEdges() const; 
		std::vector<FacePtr> getFaces() const;
		unsigned int vertexCount() const;
		unsigned int edgeCount() const;
		unsigned int faceCount() const;
		void getverticesAndFaceIndex(std::vector<VertexPtr>& vertices, std::vector<std::array<unsigned int, 3>>& faceIndices) const;  // 获取顶点和面索引

	protected:
		virtual void deleteFace(const FaceHash& facehash);  // 删除面，拓扑方法
		virtual std::vector<FacePtr> edgeNeighborFace(const EdgeHash& edgeHash);  // 边的邻居面，拓扑方法
		virtual std::vector<VertexPtr> vertexNeighborVertex(const VertexHash& vertexHash);  // 点找邻居点，拓扑方法
		virtual std::vector<EdgePtr> vertexNeighborEdge(const VertexHash& vertexHash);  // 点找邻居边，拓扑方法
		virtual std::vector<FacePtr> vertexNeighborFace(const VertexHash& vertexHash);  // 点找邻居面，拓扑方法
		/*
		bool set(const VertexPtr& vertex);
		*/
		void throwNotImplementMethod();
		virtual void storeNewAddFace(const FacePtr& face);  // 存储新增加的面
		virtual void storeNewDeleteFace(const FacePtr& face);  // 存储新删除的面

	protected:
		std::unordered_map<VertexHash, VertexPtr> vertexMap;
		std::unordered_map<EdgeHash, EdgePtr> edgeMap;
		std::unordered_map<FaceHash, FacePtr> faceMap;
	};

}  // namespace MESH

