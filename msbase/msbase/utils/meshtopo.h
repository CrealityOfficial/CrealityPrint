#ifndef MSBASE_CREATIVE_KERNEL_MESHTOPO_1596006983404_H
#define MSBASE_CREATIVE_KERNEL_MESHTOPO_1596006983404_H
#include "msbase/interface.h"
#include "trimesh2/Vec.h"
#include "trimesh2/TriMesh.h"

namespace msbase
{
	class MSBASE_API MeshTopo 
	{
	public:
		MeshTopo();
		~MeshTopo();

		inline int halfcode(int face, int index)
		{
			return (face << 2) + index;
		}
		inline void halfdecode(int half, int& face, int& index)
		{
			index = half & 3;
			face = half >> 2;
		}

		inline int faceid(int half)
		{
			return half >> 2;
		}

		inline int startvertexid(int half)
		{
			int face = half >> 2;
			int idx = half & 3;
			return m_mesh->faces.at(face)[idx];
		}

		inline int endvertexid(int half)
		{
			int face = half >> 2;
			int idx = ((half & 3) + 1) % 3;
			return m_mesh->faces.at(face)[idx];
		}

		bool builded();
		void build(trimesh::TriMesh* mesh);

		void lowestVertex(std::vector<trimesh::vec3>& vertexes, std::vector<int>& indices);
		void hangEdge(std::vector<trimesh::vec3>& vertexes, std::vector<trimesh::vec3>& normals, std::vector<float>& dotValues, float faceCosValue, std::vector<trimesh::ivec2>& edges);
		void hangEdgeCloud(std::vector<trimesh::vec3>& vertexes, std::vector<trimesh::vec3>& normals, std::vector<float>& dotValues, float faceCosValue, std::vector<trimesh::ivec2>& edges);
		void chunkFace(std::vector<float>& dotValues, std::vector<std::vector<int>>& faces, float faceCosValue);
		void flipFace(std::vector<int>& facesChunk);

		trimesh::TriMesh* m_mesh;

		std::vector<std::vector<int>> m_outHalfEdges;//第一层表示模型顶点索引，其大小为模型顶点数，第二层索引存储某点出发，共享该点的面和顶点组成的三个int值，其中int 组合Face<<2|index&3
		std::vector<trimesh::ivec3> m_oppositeHalfEdges;//第一层表示面索引，其大小为模型面数，存储某面相连的三个面信息(一个三角形有三个面)，其中相连面其边的表达方式为Face<<2|index&3
		bool m_topoBuilded;
	};
}
#endif // MSBASE_CREATIVE_KERNEL_MESHTOPO_1596006983404_H