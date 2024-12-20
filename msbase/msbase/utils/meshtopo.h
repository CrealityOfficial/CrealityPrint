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

		std::vector<std::vector<int>> m_outHalfEdges;//��һ���ʾģ�Ͷ������������СΪģ�Ͷ��������ڶ��������洢ĳ�����������õ����Ͷ�����ɵ�����intֵ������int ���Face<<2|index&3
		std::vector<trimesh::ivec3> m_oppositeHalfEdges;//��һ���ʾ�����������СΪģ���������洢ĳ����������������Ϣ(һ����������������)��������������ߵı�﷽ʽΪFace<<2|index&3
		bool m_topoBuilded;
	};
}
#endif // MSBASE_CREATIVE_KERNEL_MESHTOPO_1596006983404_H