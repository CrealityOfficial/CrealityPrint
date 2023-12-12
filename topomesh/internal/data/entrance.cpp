#include "entrance.h"
#include "internal/alg/subdivision.h"
#include "internal/alg/segmentation.h"
#include "internal/alg/remesh.h"
#include "unordered_map"


#ifndef EPS
#define EPS 1e-8f
#endif // !EPS

namespace topomesh {

	
	trimesh::TriMesh* InternelData::mmesht2trimesh(bool save)
	{
		trimesh::TriMesh* newmesh = new trimesh::TriMesh();
		if (!save)
			this->_mesh.quickTransform(newmesh);
		else
			this->_mesh.mmesh2trimesh(newmesh);
		return newmesh;
	}

	trimesh::TriMesh* InternelData::chunkmmesht2trimesh(int i)
	{
		/*if (i > this->_ChunkMesh.size() - 1) return nullptr;
		if (this->_ChunkMesh[i].faces.empty()) return nullptr;
		trimesh::TriMesh* chunkmesh = new trimesh::TriMesh();
		this->_ChunkMesh[i].quickTransform(chunkmesh);
		return chunkmesh;*/
		int chunkindex = i + 1;
		std::vector<int> chunk;
		for (MMeshFace& f : this->_mesh.faces)
		{
			if (!f.IsD() && f.GetU() == chunkindex)
				chunk.push_back(f.index);
		}
		trimesh::TriMesh* chunkmesh = new trimesh::TriMesh();
		this->_mesh.set_FFadjacent(false);
		this->_mesh.set_VVadjacent(false);
		this->_mesh.set_VFadjacent(false);
		MMeshT mt1(&this->_mesh, chunk);
		this->_mesh.set_FFadjacent(true);
		this->_mesh.set_VVadjacent(true);
		this->_mesh.set_VFadjacent(true);
		mt1.quickTransform(chunkmesh);
		return chunkmesh;
	}

	void InternelData::loopSubdivsion(std::vector<int>& faceindexs, std::vector<std::tuple<int, trimesh::point>>& vertex,
		std::vector<std::tuple<int, trimesh::ivec3>>& face_vertex, bool is_move, int iteration)
	{
		topomesh::loopSubdivision(&this->_mesh, faceindexs, vertex, face_vertex,is_move, iteration);
		return;
	}

	void InternelData::SimpleSubdivsion(std::vector<int>& faceindexs)
	{
		topomesh::SimpleMidSubdivision( &this->_mesh,faceindexs);
		return;
	}

	void InternelData::chunkedMesh(int n)
	{
		if (!this->_mesh.is_BoundingBox()) this->_mesh.getBoundingBox();
		float x = (this->_mesh.boundingbox.max_x - this->_mesh.boundingbox.min_x) * 1.f / n * 1.f;
		float y = (this->_mesh.boundingbox.max_y - this->_mesh.boundingbox.min_y) * 1.f / n * 1.f;
		float z = (this->_mesh.boundingbox.max_z - this->_mesh.boundingbox.min_z) * 1.f / n * 1.f;
		int Chunknum = std::pow(n, 3);		
		std::vector<std::vector<int>> chunkface(Chunknum);
		this->mapping.resize(Chunknum);
		for (MMeshFace& f : this->_mesh.faces)
		{
			if (f.GetU() > 0) continue;
			trimesh::point c = (f.V0(0)->p + f.V1(0)->p + f.V2(0)->p) / 3.f;
			int xi = (c.x - this->_mesh.boundingbox.min_x) / (1.01f*x);
			int yi = (c.y - this->_mesh.boundingbox.min_y) / (1.01f*y);
			int zi = (c.z - this->_mesh.boundingbox.min_z) / (1.01f*z);
			int chunked = zi * std::pow(n, 2) + yi * n + xi + 1;
			f.SetU(chunked);
			chunkface[chunked-1].push_back(f.index);
		}

		for (int c=0;c<chunkface.size();c++)
		{
			//MMeshT mt(&this->_mesh, vec);
			//this->_ChunkMesh.emplace_back(std::move(mt));
			for (int i = 0; i < chunkface[c].size(); i++)
			{
				this->mapping[c][i] = chunkface[c][i];
			}
		}

	}

	void InternelData::getChunkFaces(int ni ,std::vector<int>& faceindexs)
	{
		int deleteFnum = 0;
		for (MMeshFace& f : this->_mesh.faces)
		{
			if (f.IsD())
			{
				deleteFnum++; continue;
			}
			if (f.GetU() == ni)
				faceindexs.push_back(f.index-deleteFnum);
		}
	}

	void InternelData::getVertex(const std::vector<int>& faceindexs, std::vector<std::tuple<trimesh::ivec3, trimesh::point, trimesh::point, trimesh::point>>& vertexindexs)
	{
		for (int i : faceindexs)
		{
			MMeshFace& f = this->_mesh.faces[i];
			vertexindexs.push_back(std::make_tuple(trimesh::ivec3(f.V0(0)->index, f.V1(0)->index, f.V2(0)->index),
				f.V0(0)->p, f.V1(0)->p, f.V2(0)->p));
		}
	}

	void InternelData::SimpleRemeshing(const std::vector<int>& faceindexs, float thershold)
	{
		//topomesh::SimpleRemeshing(&this->_mesh, faceindexs, thershold);
	}


	int InternelData::getFaceChunk(int faceindex)
	{
		return this->_mesh.faces[faceindex].GetU();
	}

	void InternelData::ChunkMeshSimpleRemshing(const std::vector<int>& Chunkid, const std::vector<std::vector<int>>& ChunkMeshfaceindexs, float thershold, std::vector<int>& chunks)
	{
		/*for (int i=0 ; i<Chunkid.size();i++)
		{
			topomesh::SimpleRemeshing(&this->_ChunkMesh[Chunkid[i]],ChunkMeshfaceindexs[i],thershold);
		}*/
		chunks.clear();
		std::vector<int> faceindexs;
		for (int i = 0; i < Chunkid.size(); i++)
		{
			for (int j = 0; j < ChunkMeshfaceindexs[i].size(); j++)
			{
				faceindexs.push_back(this->mapping[Chunkid[i]][ChunkMeshfaceindexs[i][j]]);
			}
		}		
		topomesh::SimpleRemeshing(&this->_mesh, faceindexs, thershold,chunks);
		for (int i = 0; i < chunks.size(); i++)
		{
			this->mapping[chunks[i]].clear();
		}
		std::map<int, std::vector<int>> mmap;
		for (MMeshFace& f : this->_mesh.faces)
		{
			if (!f.IsD() && std::find(chunks.begin(), chunks.end(), f.GetU()-1) != chunks.end())
			{
				mmap[f.GetU()].push_back(f.index);
			}
		}
		for (std::map<int, std::vector<int>>::iterator it = mmap.begin(); it != mmap.end(); it++)
		{
			for (int i = 0; i < it->second.size(); i++)
			{
				this->mapping[it->first - 1][i] = it->second[i];
			}
		}
	}

	void InternelData::shrinkChunkMesh(int i)
	{
		//this->_ChunkMesh[i].shrinkMesh();
	}

	void InternelData::QuickCombinationMesh()
	{
		struct hash_func {
			size_t operator()(const trimesh::point& v)const
			{
				return (int(v.x * 99971)) ^ (int(v.y * 99989)) ^ (int(v.z * 99991));
			}
		};
		this->_mesh.clear();
		this->_mesh.set_FFadjacent(false);
		this->_mesh.set_VVadjacent(false);
		//this->_mesh.set_VFadjacent(false);
		int Accface = 0;
		std::unordered_map<trimesh::point, std::vector<int>, hash_func> map;
		for (MMeshT& m : this->_ChunkMesh)
		{
			int deleteVnum = 0;		
			for (MMeshVertex& v : m.vertices)
			{
				if (v.IsD())
				{
					deleteVnum++; continue;
				}
				v.index -= deleteVnum;
				this->_mesh.appendVertex(v.p);
				//unsigned long long hash_n = (int(v.p.x * 99971)) ^ (int(v.p.y * 99989)) ^ (int(v.p.z * 99991));
				map[v.p].push_back(this->_mesh.vertices.size() - 1);
			}
			for (MMeshFace& f : m.faces)
			{
				if (!f.IsD())
				{					
					this->_mesh.appendFace(f.connect_vertex[0]->index+ Accface,f.connect_vertex[1]->index+ Accface,f.connect_vertex[2]->index+ Accface);
				}
			}
			Accface = this->_mesh.vertices.size();
		}
		for (auto& value : map)
		{
			if (value.second.size() > 1)
			{
				for (int i = 1; i < value.second.size(); i++)
				{
					for (MMeshFace* vf : this->_mesh.vertices[value.second[i]].connected_face)
					{
						int vi_index = vf->getVFindex(value.second[i]);
						vf->connect_vertex[vi_index] = &this->_mesh.vertices[value.second[0]];
						this->_mesh.deleteVertex(value.second[i]);
					}
				}
			}
		}
		this->_mesh.set_VFadjacent(false);
	}
}