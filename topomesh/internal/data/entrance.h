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
		void chunkedMesh(int n); //分块
		void getChunkFaces(int ni, std::vector<int>& faceindexs);//获取块内有哪些面
		int getFaceChunk(int faceindex);//获取面所属那个块
		void QuickCombinationMesh();//合并mesh
		void getVertex(const std::vector<int>& faceindexs,
			std::vector<std::tuple<trimesh::ivec3, trimesh::point, trimesh::point, trimesh::point>>& vertexindexs);//获取面对应的点索引和坐标
		trimesh::TriMesh* mmesht2trimesh(bool save = false); //返回trimesh 如果不再对mesh操作 则save为默认值false 如果还存在后续操作 save设为true
		trimesh::TriMesh* chunkmmesht2trimesh(int i);
		void shrinkChunkMesh(int i);//对小mesh做数据对齐
		void loopSubdivsion(std::vector<int>& faceindexs, std::vector<std::tuple<int, trimesh::point>>& vertex,
			std::vector<std::tuple<int, trimesh::ivec3>>& face_vertex, bool is_move=false,int iteration=1);
		void SimpleSubdivsion(std::vector<int>& faceindexs);
		void SimpleRemeshing(const std::vector<int>& faceindexs, float thershold);//根据阈值细分三角面		
		void ChunkMeshSimpleRemshing(const std::vector<int>& Chunkid,const std::vector<std::vector<int>>& ChunkMeshfaceindexs,float thershold,std::vector<int>& chunks);
	};
}