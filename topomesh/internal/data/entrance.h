#pragma once
#include "mmesht.h"

namespace topomesh {

	class BaseParameterClass {};

	class InternelData
	{
	public:
		InternelData(){};		
		InternelData(trimesh::TriMesh* trimesh) :_mesh(trimesh){};
		//InternelData(trimesh::TriMesh* trimesh);
		~InternelData() { _mesh.clear(); std::vector<MMeshT>().swap(_ChunkMesh); std::vector<std::map<int, int>>().swap(mapping); };
		
	private:
		MMeshT _mesh;
		std::vector<MMeshT> _ChunkMesh;
		std::vector<std::map<int, int>> mapping;

	public:
		void chunkedMesh(int n); //�ֿ�
		void getChunkFaces(int ni, std::vector<int>& faceindexs);//��ȡ��������Щ��
		int getFaceChunk(int faceindex);//��ȡ�������Ǹ���
		void QuickCombinationMesh();//�ϲ�mesh
		void getVertex(const std::vector<int>& faceindexs,
			std::vector<std::tuple<trimesh::ivec3, trimesh::point, trimesh::point, trimesh::point>>& vertexindexs);//��ȡ���Ӧ�ĵ�����������
		trimesh::TriMesh* mmesht2trimesh(bool save = false); //����trimesh ������ٶ�mesh���� ��saveΪĬ��ֵfalse ��������ں������� save��Ϊtrue
		trimesh::TriMesh* chunkmmesht2trimesh(int i);
		void shrinkChunkMesh(int i);//��Сmesh�����ݶ���
		void loopSubdivsion(std::vector<int>& faceindexs, std::vector<std::tuple<int, trimesh::point>>& vertex,
			std::vector<std::tuple<int, trimesh::ivec3>>& face_vertex, bool is_move=false,int iteration=1);
		void SimpleSubdivsion(std::vector<int>& faceindexs);
		void SimpleRemeshing(const std::vector<int>& faceindexs, float thershold);//������ֵϸ��������		
		void ChunkMeshSimpleRemshing(const std::vector<int>& Chunkid,const std::vector<std::vector<int>>& ChunkMeshfaceindexs,float thershold,std::vector<int>& chunks);
	};
}